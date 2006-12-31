#include <cmath>
#include "map.h"
#include "../freecnc.h"
#include "path.h"
#include "projectileanim.h"
#include "talkback.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "unitorstructure.h"
#include "unitanimations.h"
#include "weaponspool.h"

using std::runtime_error;

UnitAnimEvent::UnitAnimEvent(unsigned int p, Unit* un) : ActionEvent(p)
{
    //logger->debug("UAE cons: this:%p un:%p\n",this,un);
    this->un = un;
    un->referTo();
    scheduled = NULL;
}

UnitAnimEvent::~UnitAnimEvent()
{
    //logger->debug("UAE dest: this:%p un:%p sch:%p\n",this,un,scheduled);
    if (scheduled != NULL) {
        p::aequeue->scheduleEvent(scheduled);
    }
    un->unrefer();
}

void UnitAnimEvent::setSchedule(UnitAnimEvent* e)
{
    //logger->debug("Scheduling an event. (this: %p, e: %p)\n",this,e);
    if (scheduled != NULL) {
        scheduled->setSchedule(NULL);
        scheduled->stop();
    }
    scheduled = e;
}

void UnitAnimEvent::stopScheduled()
{
    if (scheduled != NULL) {
        scheduled->stop();
    }
}

MoveAnimEvent::MoveAnimEvent(unsigned int p, Unit* un) : UnitAnimEvent(p,un)
{
    //logger->debug("MoveAnim cons (this %p un %p)\n",this,un);
    stopping = false;
    blocked = false;
    range = 0;
    this->dest = un->getTargetCell();
    this->un = un;
    path = NULL;
    newpos = 0xffff;
    istep = 0;
    moved_half = true;
    waiting = false;
    pathinvalid = true;
}

MoveAnimEvent::~MoveAnimEvent()
{
    if (un->moveanim == this)
        un->moveanim = NULL;
    //logger->debug("MoveAnim dest (this %p un %p dest %u)\n",this,un,dest);
    if( !moved_half && newpos != 0xffff) {
        p::uspool->abortMove(un,newpos);
    }
    delete path;
    if (un->walkanim != NULL) {
        un->walkanim->stop();
    }
    if (un->turnanim1 != NULL) {
        un->turnanim1->stop();

    }
    if (un->turnanim2 != NULL) {
        un->turnanim2->stop();
    }
}

void MoveAnimEvent::run()
{
    char uxoff, uyoff;
    unsigned char oldsubpos;

    waiting = false;
    if( !un->isAlive() ) {
        delete this;
        return;
    }

    if( path == NULL ) {
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range);
        if( !path->empty() ) {
            startMoveOne(false);
        } else {
            if (un->attackanim != NULL) {
                un->attackanim->stop();
            }
            delete this;
        }
        return;
    }

    if( blocked ) {
        blocked = false;
        startMoveOne(true);
        return;
    }
    /* if distance left is smaller than xmod we're ready */

    un->xoffset += xmod;
    un->yoffset += ymod;

    if( !moved_half && (abs(un->xoffset) >= 12 || abs(un->yoffset) >= 12) ) {
        oldsubpos = un->subpos;
        un->subpos = p::uspool->postMove(un, newpos);
        un->cellpos = newpos;
        un->xoffset = -un->xoffset;
        un->yoffset = -un->yoffset;
        if( un->type->isInfantry() ) {
            un->infgrp->GetSubposOffsets(oldsubpos, un->subpos, &uxoff, &uyoff);
            un->xoffset += uxoff;
            un->yoffset += uyoff;
            xmod = 0;
            ymod = 0;
            if( un->xoffset < 0 )
                xmod = 1;
            else if( un->xoffset > 0 )
                xmod = -1;
            if( un->yoffset < 0 )
                ymod = 1;
            else if( un->yoffset > 0 )
                ymod = -1;
        }
        moved_half = true;
    }

    if( abs(un->xoffset) < abs(xmod) )
        xmod = 0;
    if( abs(un->yoffset) < abs(ymod) )
        ymod = 0;

    if( xmod == 0 && ymod == 0 ) {
        un->xoffset = 0;
        un->yoffset = 0;
        moveDone();
        return;
    }

    p::aequeue->scheduleEvent(this);
}

void MoveAnimEvent::startMoveOne(bool wasblocked)
{
#ifdef LOOPEND_TURN
    //TODO: transport boat is jerky (?)
    unsigned char loopend=((UnitType*)un->type)->getAnimInfo().loopend;
#endif
    unsigned char face;

    newpos = p::uspool->preMove(un, path->top(), &xmod, &ymod);
    if( newpos == 0xffff ) {
        delete path;
        path = NULL;
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range);
        pathinvalid = false;
        if( path->empty() ) {
            xmod = 0;
            ymod = 0;
            p::aequeue->scheduleEvent(this);
            return;
        }
        newpos = p::uspool->preMove(un, path->top(), &xmod, &ymod);
        if( newpos == 0xffff ) {
            if (wasblocked) {
                xmod = 0;
                ymod = 0;
                if (un->attackanim != NULL) {
                    un->attackanim->stop();
                }
                this->stop();
                p::aequeue->scheduleEvent(this);
                return;
            } else {
                /* TODO: tell the blocking unit to move here */

                blocked = true;

                if (un->walkanim != NULL) {
                    un->walkanim->stop();
                }

                p::aequeue->scheduleEvent(this);
                return;
            }
        }
    }

#ifdef LOOPEND_TURN 
    face = ((char)((loopend+1)*(8-path->top())/8))&loopend;
#else
    face = (32-(path->top() << 2))&0x1f;
#endif
    path->pop();

    moved_half = false;

    if (((UnitType *)un->getType())->isInfantry()) {
        if (un->walkanim != NULL) {
            un->walkanim->changedir(face);
        } else {
            un->walkanim = new WalkAnimEvent(((UnitType *)un->getType())->getSpeed(), un, face, 0);
            p::aequeue->scheduleEvent(un->walkanim);
        }
        p::aequeue->scheduleEvent(this);

    } else {
#ifdef LOOPEND_TURN 
        unsigned char curface = (un->getImageNum(0)&loopend);
        unsigned char delta = (abs(curface-face))&loopend;
        if( curface != face ) {
            if( (delta <= (char)((loopend+1)/8)) || (delta >= (char)(loopend*7/8))) {
#else
        unsigned char curface = (un->getImageNum(0)&0x1f);
        unsigned char delta = (abs(curface-face))&0x1f;
        if( (un->getImageNum(0)&0x1f) != face ) {
            if( (delta < 5) || (delta > 27) ) {
#endif
                un->turn(face,0);
                p::aequeue->scheduleEvent(this);
            } else {
                waiting = true;
                un->turn(face,0);
                un->turnanim1->setSchedule(this);
            }
            if (un->getType()->getNumLayers() > 1) {
                un->turn(face,1);
            }
        } else {
            p::aequeue->scheduleEvent(this);
        }
    }
}

void MoveAnimEvent::moveDone()
{
    un->xoffset = 0;
    un->yoffset = 0;

    if (pathinvalid) {
        delete path;
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range);
        pathinvalid = false;
    }
    if( !path->empty() && !stopping ) {
        startMoveOne(false);
    } else {
        if( dest != un->getPos() && !stopping ) {
            delete path;
            p::uspool->setCostCalcOwnerAndType(un->owner, 0);
            path = new Path(un->getPos(), dest, range);
            pathinvalid = false;
        }
        if( path->empty() || stopping ) {
            delete this;
        } else {
            startMoveOne(false);
        }
    }
}

void MoveAnimEvent::stop()
{
    stopping = true;
    //stopScheduled();
}

void MoveAnimEvent::update()
{
    //logger->debug("Move updating\n");
    dest = un->getTargetCell();
    pathinvalid = true;
    stopping = false;
    range = 0;
}

WalkAnimEvent::WalkAnimEvent(unsigned int p, Unit *un, unsigned char dir, unsigned char layer) : UnitAnimEvent(p,un)
{
    //fprintf(stderr,"debug: WalkAnim constructor\n");
    this->un = un;
    this->dir = dir;
    this->layer = layer;
    stopping = false;
    istep = 0;
    calcbaseimage();
}

WalkAnimEvent::~WalkAnimEvent()
{
#ifdef LOOPEND_TURN
    un->setImageNum((((UnitType*)un->type)->getAnimInfo().loopend+1)*dir/8,layer);
#else
    un->setImageNum(dir>>2,layer);
#endif
    if (un->walkanim == this)
        un->walkanim = NULL;
}

void WalkAnimEvent::run()
{
    unsigned char layerface;
    if (!stopping) {
        layerface = baseimage + istep;
        // XXX: Assumes 6 frames to loop over
        istep = (istep + 1)%6;
        un->setImageNum(layerface,layer);
        p::aequeue->scheduleEvent(this);
    } else {
        delete this;
        return;
    }
}

void WalkAnimEvent::calcbaseimage()
{
    // XXX: this is really nasty, will be taken care of after the rewrite
    baseimage = 16 + 3*(dir/2);
}

TurnAnimEvent::TurnAnimEvent(unsigned int p, Unit *un, unsigned char dir, unsigned char layer) : UnitAnimEvent(p,un)
{
#ifdef LOOPEND_TURN   
    unsigned char loopend=((UnitType*)un->type)->getAnimInfo().loopend;
#endif
    //logger->debug("Turn cons (t%p u%p d%i l%i)\n",this,un,dir,layer);
    unsigned char layerface;
    this->un = un;
    this->dir = dir;
    this->layer = layer;
    stopping = false;
#ifdef LOOPEND_TURN 
    layerface = un->getImageNum(layer)&loopend;
    if( ((layerface-dir)&loopend) < ((dir-layerface)&loopend) ) {
#else
    layerface = un->getImageNum(layer)&0x1f;
    if( ((layerface-dir)&0x1f) < ((dir-layerface)&0x1f) ) {
#endif
        turnmod = -(((UnitType *)un->getType())->getTurnMod());
    } else {
        turnmod = (((UnitType *)un->getType())->getTurnMod());
    }
}

TurnAnimEvent::~TurnAnimEvent()
{
    //logger->debug("TurnAnim dest\n");
    if (layer == 0) {
        if (un->turnanim1 == this)
            un->turnanim1 = NULL;
    } else {
        if (un->turnanim2 == this)
            un->turnanim2 = NULL;
    }
}

void TurnAnimEvent::run()
{
#ifdef LOOPEND_TURN   
    unsigned char loopend=((UnitType*)un->type)->getAnimInfo().loopend;
#endif
    unsigned char layerface;

    //logger->debug("TurnAnim run (s%i)\n",stopping);
    if (stopping) {
        delete this;
        return;
    }

#ifdef LOOPEND_TURN   
    layerface = un->getImageNum(layer)&loopend;
    if( abs((layerface-dir)&loopend) > abs(turnmod) ) {
        layerface += turnmod;
        layerface &= loopend;
#else
    layerface = un->getImageNum(layer)&0x1f;
    if( abs((layerface-dir)&0x1f) > abs(turnmod) ) {
        layerface += turnmod;
        layerface &= 0x1f;
#endif
    } else
        layerface = dir;

    un->setImageNum(layerface,layer);
    if( layerface == dir || !un->isAlive()) {
        delete this;
        return;
    }
    p::aequeue->scheduleEvent(this);
}

UAttackAnimEvent::UAttackAnimEvent(unsigned int p, Unit *un) : UnitAnimEvent(p,un)
{
    //logger->debug("UAttack cons\n");
    this->un = un;
    this->target = un->getTarget();
    stopping = false;
    waiting = 0;
    target->referTo();
}

UAttackAnimEvent::~UAttackAnimEvent()
{
    //logger->debug("UAttack dest\n");
    target->unrefer();
    if (un->attackanim == this)
        un->attackanim = NULL;
}

void UAttackAnimEvent::update()
{
    //logger->debug("UAtk updating\n");
    target->unrefer();
    target = un->getTarget();
    target->referTo();
    stopping = false;
}

void UAttackAnimEvent::stop()
{
    if (un == NULL) {
        throw runtime_error("UAttackAnimEvent::stop Unit pointer is NULL");
    }
    stopping = true;
}

void UAttackAnimEvent::run()
{
    unsigned int distance;
    int xtiles, ytiles;
    unsigned short atkpos;
    float alpha;
    unsigned char facing;
#ifdef LOOPEND_TURN
    unsigned char loopend2=((UnitType*)un->type)->getAnimInfo().loopend2;
#endif

    //logger->debug("attack run t%p u%p\n",this,un);
    waiting = 0;
    if( !un->isAlive() || stopping ) {
        delete this;
        return;
    }

    if( !target->isAlive() || stopping) {
        if ( !target->isAlive() ) {
            un->doRandTalk(TB_postkill);
        }
        delete this;
        return;
    }
    atkpos = un->getTargetCell();

    xtiles = un->cellpos % p::ccmap->getWidth() - atkpos % p::ccmap->getWidth();
    ytiles = un->cellpos / p::ccmap->getWidth() - atkpos / p::ccmap->getWidth();
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);

    if( distance > un->type->getWeapon()->getRange() /* weapons range */ ) {
        setDelay(0);
        waiting = 3;
        un->move(atkpos,false);
        un->moveanim->setRange(un->type->getWeapon()->getRange());
        un->moveanim->setSchedule(this);
        return;
    }
    //Make sure we're facing the right way
    if( xtiles == 0 ) {
        if( ytiles < 0 ) {
            alpha = -M_PI_2;
        } else {
            alpha = M_PI_2;
        }
    } else {
        alpha = atan((float)ytiles/(float)xtiles);
        if( xtiles < 0 ) {
            alpha = M_PI+alpha;
        }
    }
#ifdef LOOPEND_TURN
    facing = ((char)((loopend2+1)*(1-alpha/2/M_PI)+8))&loopend2;
    if (un->type->isInfantry()) {
        if (facing != (un->getImageNum(0)&loopend2)) {
            un->setImageNum((char)((loopend2+1)*facing/8),0);
        }
    } else if (un->type->getNumLayers() > 1 ) {
        if (abs((int)(facing - (un->getImageNum(1)&loopend2))) > un->type->getROT()) {
#else
    facing = (40-(char)(alpha*16/M_PI))&0x1f;
    if (un->type->isInfantry()) {
        if (facing != (un->getImageNum(0)&0x1f)) {
            un->setImageNum(facing>>2,0);
        }
    } else if (un->type->getNumLayers() > 1 ) {
        if (abs((int)(facing - (un->getImageNum(1)&0x1f))) > un->type->getROT()) {
#endif
            setDelay(0);
            waiting = 2;
            un->turn(facing,1);
            un->turnanim2->setSchedule(this);
            return;
        }
    } else {
#ifdef LOOPEND_TURN
        if (abs((int)(facing - un->getImageNum(0)&loopend2)) > un->type->getROT()) {
#else
        if (abs((int)(facing - un->getImageNum(0)&0x1f)) > un->type->getROT()) {
#endif
            setDelay(0);
            waiting = 1;
            un->turn(facing,0);
            un->turnanim1->setSchedule(this);
            return;
        }
    }
    // We can shoot

    un->type->getWeapon()->fire(un, target->getBPos(un->getPos()), target->getSubpos());
    // set delay to reloadtime
    setDelay(un->type->getWeapon()->getReloadTime());
    waiting = 4;
    p::aequeue->scheduleEvent(this);
}


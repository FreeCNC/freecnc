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
}

void UnitAnimEvent::finish()
{
    if (scheduled) {
        p::aequeue->scheduleEvent(scheduled);
    }
}

UnitAnimEvent::~UnitAnimEvent()
{
    //logger->debug("UAE dest: this:%p un:%p sch:%p\n",this,un,scheduled);
    un->unrefer();
}

void UnitAnimEvent::setSchedule(shared_ptr<UnitAnimEvent> e)
{
    //logger->debug("Scheduling an event. (this: %p, e: %p)\n",this,e);
    if (scheduled) {
        scheduled->setSchedule();
        scheduled->stop();
    }
    scheduled = e;
}

void UnitAnimEvent::setSchedule()
{
    scheduled.reset();
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
    delete path;
}

void MoveAnimEvent::finish()
{
    if (waiting) {
        return;
    }

    if (un->moveanim.get() == this)
        un->moveanim.reset();
    //logger->debug("MoveAnim dest (this %p un %p dest %u)\n",this,un,dest);
    if( !moved_half && newpos != 0xffff) {
        p::uspool->abortMove(un,newpos);
    }
    if (un->walkanim) {
        un->walkanim->stop();
    }
    if (un->turnanim1) {
        un->turnanim1->stop();

    }
    if (un->turnanim2) {
        un->turnanim2->stop();
    }

    UnitAnimEvent::finish();
}

bool MoveAnimEvent::run()
{
    char uxoff, uyoff;
    unsigned char oldsubpos;

    waiting = false;
    if( !un->isAlive() ) {
        return false;
    }

    if( path == NULL ) {
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range);
        if( !path->empty() ) {
            return startMoveOne(false);
        } else {
            if (un->attackanim) {
                un->attackanim->stop();
            }
        }
        return false;
    }

    if( blocked ) {
        blocked = false;
        return startMoveOne(true);
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
        return moveDone();
    }

    return true;
}

bool MoveAnimEvent::startMoveOne(bool wasblocked)
{
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
            return true;
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
                return true;
            } else {
                /* TODO: tell the blocking unit to move here */

                blocked = true;

                if (un->walkanim != NULL) {
                    un->walkanim->stop();
                }

                return true;
            }
        }
    }

    face = (32-(path->top() << 2))&0x1f;
    path->pop();

    moved_half = false;

    if (((UnitType *)un->getType())->isInfantry()) {
        if (un->walkanim != NULL) {
            un->walkanim->changedir(face);
        } else {
            un->walkanim.reset(new WalkAnimEvent(((UnitType *)un->getType())->getSpeed(), un, face, 0));
            p::aequeue->scheduleEvent(un->walkanim);
        }
        return true;
    }
    unsigned char curface = (un->getImageNum(0)&0x1f);
    unsigned char delta = (abs(curface-face))&0x1f;
    if( (un->getImageNum(0)&0x1f) != face ) {
        if( (delta < 5) || (delta > 27) ) {
            un->turn(face,0);
            return true;
        } else {
            waiting = true;
            un->turn(face,0);
            un->turnanim1->setSchedule(un->moveanim);
        }
        if (un->getType()->getNumLayers() > 1) {
            un->turn(face,1);
        }
    } else {
        return true;
    }
    return false;
}

bool MoveAnimEvent::moveDone()
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
        return startMoveOne(false);
    }
    if( dest != un->getPos() && !stopping ) {
        delete path;
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range);
        pathinvalid = false;
    }
    if( path->empty() || stopping ) {
        return false;
    }
    return startMoveOne(false);
}

void MoveAnimEvent::stop()
{
    stopping = true;
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

void WalkAnimEvent::finish()
{
    un->setImageNum(dir>>2,layer);
    if (un->walkanim.get() == this)
        un->walkanim.reset();
    UnitAnimEvent::finish();
}

bool WalkAnimEvent::run()
{
    unsigned char layerface;
    if (!stopping) {
        layerface = baseimage + istep;
        // XXX: Assumes 6 frames to loop over
        istep = (istep + 1)%6;
        un->setImageNum(layerface,layer);
        return true;
    }
    return false;
}

void WalkAnimEvent::calcbaseimage()
{
    // XXX: this is really nasty, will be taken care of after the rewrite
    baseimage = 16 + 3*(dir/2);
}

TurnAnimEvent::TurnAnimEvent(unsigned int p, Unit *un, unsigned char dir, unsigned char layer) : UnitAnimEvent(p,un)
{
    //logger->debug("Turn cons (t%p u%p d%i l%i)\n",this,un,dir,layer);
    unsigned char layerface;
    this->un = un;
    this->dir = dir;
    this->layer = layer;
    stopping = false;
    layerface = un->getImageNum(layer)&0x1f;
    if( ((layerface-dir)&0x1f) < ((dir-layerface)&0x1f) ) {
        turnmod = -(((UnitType *)un->getType())->getTurnMod());
    } else {
        turnmod = (((UnitType *)un->getType())->getTurnMod());
    }
}

void TurnAnimEvent::finish()
{
    //logger->debug("TurnAnim dest\n");
    if (layer == 0) {
        if (un->turnanim1.get() == this)
            un->turnanim1.reset();
    } else {
        if (un->turnanim2.get() == this)
            un->turnanim2.reset();
    }
    UnitAnimEvent::finish();
}

bool TurnAnimEvent::run()
{
    unsigned char layerface;

    //logger->debug("TurnAnim run (s%i)\n",stopping);
    if (stopping) {
        return false;
    }

    layerface = un->getImageNum(layer)&0x1f;
    if( abs((layerface-dir)&0x1f) > abs(turnmod) ) {
        layerface += turnmod;
        layerface &= 0x1f;
    } else
        layerface = dir;

    un->setImageNum(layerface,layer);
    if( layerface == dir || !un->isAlive()) {
        return false;
    }
    return true;
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
}

void UAttackAnimEvent::finish()
{
    if (waiting != 0) {
        return;
    }

    if (un->attackanim.get() == this)
        un->attackanim.reset();
    UnitAnimEvent::finish();
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
    assert(un);
    stopping = true;
    if (un->attackanim.get() == this)
        un->attackanim.reset();
}

bool UAttackAnimEvent::run()
{
    unsigned int distance;
    int xtiles, ytiles;
    unsigned short atkpos;
    float alpha;
    unsigned char facing;

    //logger->debug("attack run t%p u%p\n",this,un);
    waiting = 0;
    if( !un->isAlive() || stopping ) {
        return false;
    }

    if( !target->isAlive() || stopping) {
        if ( !target->isAlive() ) {
            un->doRandTalk(TB_postkill);
        }
        return false;
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
        un->moveanim->setSchedule(un->attackanim);
        return false;
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
    facing = (40-(char)(alpha*16/M_PI))&0x1f;
    if (un->type->isInfantry()) {
        if (facing != (un->getImageNum(0)&0x1f)) {
            un->setImageNum(facing>>2,0);
        }
    } else if (un->type->getNumLayers() > 1 ) {
        if (abs((int)(facing - (un->getImageNum(1)&0x1f))) > un->type->getROT()) {
            setDelay(0);
            waiting = 2;
            un->turn(facing,1);
            un->turnanim2->setSchedule(un->attackanim);
            return false;
        }
    } else {
        if (abs((int)(facing - un->getImageNum(0)&0x1f)) > un->type->getROT()) {
            setDelay(0);
            waiting = 1;
            un->turn(facing,0);
            un->turnanim1->setSchedule(un->attackanim);
            return false;
        }
    }
    // We can shoot

    un->type->getWeapon()->fire(un, target->getBPos(un->getPos()), target->getSubpos());
    // set delay to reloadtime
    setDelay(un->type->getWeapon()->getReloadTime());
    waiting = 4;
    return true;
}


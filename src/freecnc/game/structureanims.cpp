#include <cmath>
#include "../freecnc.h"
#include "../sound/sound_public.h"
#include "map.h"
#include "playerpool.h"
#include "projectileanim.h"
#include "unitorstructure.h"
#include "unitandstructurepool.h"
#include "structure.h"
#include "structureanims.h"
#include "weaponspool.h"

BuildingAnimEvent::BuildingAnimEvent(unsigned int p, Structure* str, unsigned char mode) : ActionEvent(p)
{
    strct = str;
    strct->referTo();
    this->strct = strct;
    anim_data.done = false;
    anim_data.mode = mode;
    anim_data.frame0 = 0;
    anim_data.frame1 = 0;
    anim_data.damagedelta = 0;
    anim_data.damaged = false;
    toAttack = false;
    if (getaniminfo().animdelay != 0 && mode != 0) // no delay for building anim
        setDelay(getaniminfo().animdelay);
    e = NULL;
    ea = NULL;
    layer2      = ((strct->type->getNumLayers()==2)?true:false);
}

BuildingAnimEvent::~BuildingAnimEvent()
{
    if ((e != NULL)||(ea!=NULL)) {
        if (toAttack) {
            strct->attackAnim = ea;
            p::aequeue->scheduleEvent(ea);
        } else {
            strct->buildAnim = e;
            p::aequeue->scheduleEvent(e);
        }
    }
    strct->unrefer();
}

void BuildingAnimEvent::run()
{
    BuildingAnimEvent* tmp_ev;

    if( !strct->isAlive() ) {
        delete this;
        return;
    }
    anim_func(&anim_data);

    strct->setImageNum(anim_data.frame0,0);
    if (layer2) {
        strct->setImageNum(anim_data.frame1,1);
    }
    if (anim_data.done) {
        if (anim_data.mode != 6 && anim_data.mode != 5) {
            strct->setImageNum(anim_data.damagedelta,0);
            if (layer2) {
                strct->setImageNum(anim_data.damagedelta2,1);
            }
        }
        if (anim_data.mode == 0) {
            strct->setImageNum(anim_data.damagedelta + anim_data.frame0,0);
        }
        strct->usemakeimgs = false;
        if ((anim_data.mode == 0) || (anim_data.mode == 7)) {
            switch (getaniminfo().animtype) {
            case 1:
                tmp_ev = new LoopAnimEvent(getaniminfo().animspeed,strct);
                setSchedule(tmp_ev);
                break;
            case 4:
                tmp_ev = new ProcAnimEvent(getaniminfo().animspeed,strct);
                setSchedule(tmp_ev);
                break;
            default:
                strct->animating = false;
                break;
            }
        } else if (e == NULL) {
            strct->animating = false;
        }
        delete this;
        return;
    }
    p::aequeue->scheduleEvent(this);
}

void BuildingAnimEvent::updateDamaged()
{
    bool odam = anim_data.damaged;
    anim_data.damaged = (strct->checkdamage() > 0);
    if (anim_data.damaged) {
        if (getaniminfo().dmgoff != 0 || getaniminfo().dmgoff2 != 0) {
            anim_data.damagedelta = getaniminfo().dmgoff;
            anim_data.damagedelta2 = getaniminfo().dmgoff2;
        } else {
            anim_data.damagedelta = getaniminfo().loopend+1;
            if (layer2) {
                anim_data.damagedelta2 = getaniminfo().loopend2+1;
            }
        }
        if (!odam && pc::sfxeng != NULL && !p::ccmap->isLoading()) {
            pc::sfxeng->PlaySound("xplobig4.aud");
        }
    } else {
        anim_data.damagedelta = 0;
        anim_data.damagedelta2 = 0;
    }
}

BuildAnimEvent::BuildAnimEvent(unsigned int p, Structure* str, bool sell) : BuildingAnimEvent(p,str,0)
{
    updateDamaged();
    this->sell = sell;
    framend = getaniminfo().makenum;
    frame = (sell?framend:0);
}

void BuildAnimEvent::anim_func(anim_nfo* data)
{
    if (!sell) {
        if (frame < framend) {
            data->frame0 = frame;
            ++frame;
        } else {
            data->done = true;
            data->frame0 = getType()->getDefaultFace();
        }
    } else {
        if (frame > 0) {
            data->frame0 = frame;
            --frame;
        } else {
            data->done = true;
        }
    }
}
BuildAnimEvent::~BuildAnimEvent()
{}


LoopAnimEvent::LoopAnimEvent(unsigned int p, Structure* str) : BuildingAnimEvent(p,str,1)
{
    updateDamaged();
    framend = getaniminfo().loopend;
    frame = 0;
}

void LoopAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    data->frame0 = frame;
    if ((frame-data->damagedelta) < framend) {
        ++frame;
    } else {
        frame = data->damagedelta;
    }
}

ProcAnimEvent::ProcAnimEvent(unsigned int p, Structure* str) : BuildingAnimEvent(p,str,4)
{
    updateDamaged();
    framend = getaniminfo().loopend;
    frame = 0;
}

void ProcAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    data->frame0 = frame;
    ++frame;
    if ((frame-data->damagedelta) > framend) {
        frame = data->damagedelta;
    }
}

void ProcAnimEvent::updateDamaged()
{
    BuildingAnimEvent::updateDamaged();
    if (anim_data.damaged) {
        anim_data.damagedelta = 30; // fixme: remove magic numbers
        if (frame < 30) {
            frame += 30;
        }
    }
}

BTurnAnimEvent::BTurnAnimEvent(unsigned int p, Structure* str, unsigned char face) : BuildingAnimEvent(p,str,6)
{
    unsigned char layerface;
    updateDamaged();
    targetface = face;
    layerface = (str->getImageNums()[0]&0x1f);
    if (layerface == face) {
        delete this;
        return;
    }
    if( ((layerface-face)&0x1f) < ((face-layerface)&0x1f) ) {
        turnmod = -1;
    } else {
        turnmod = 1;
    }
    this->str = str;
}

void BTurnAnimEvent::anim_func(anim_nfo* data)
{
    unsigned char layerface;
    layerface = (str->getImageNums()[0]&0x1f);
    if( abs((layerface-targetface)&0x1f) > abs(turnmod) ) {
        layerface += turnmod;
        layerface &= 0x1f;
    } else {
        layerface = targetface;
    }
    data->frame0 = layerface+data->damagedelta;
    if( layerface == targetface) {
        data->done = true;
    }
}

DoorAnimEvent::DoorAnimEvent(unsigned int p, Structure* str, bool opening) : BuildingAnimEvent(p,str,5)
{
    updateDamaged();
    if (opening) {
        frame = framestart;
    } else {
        frame = framend;
    }
    this->opening = opening;
}

void DoorAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    if (opening) {
        if (frame < framend) {
            ++frame;
        } else {
            data->done = true;
        }
    } else {
        if (frame > framestart) {
            --frame;
        } else {
            frame = framestart;
            data->done = true;
        }
    }
    data->frame1 = frame;
    data->frame0 = frame0;
}

void DoorAnimEvent::updateDamaged()
{
    BuildingAnimEvent::updateDamaged();
    if (anim_data.damaged) {
        framestart = getaniminfo().loopend2+1;
        framend = framestart+getaniminfo().loopend2;
        if (frame < framestart) {
            frame += framestart;
        }
    } else {
        framestart = 0;
        framend = getaniminfo().loopend2;
    }
    frame0 = anim_data.damagedelta;
}

RefineAnimEvent::RefineAnimEvent(unsigned int p, Structure* str, unsigned char bails) : BuildingAnimEvent(p,str,7)
{
    updateDamaged();
    this->bails = bails;
    this->str = str;
    frame = framestart;
}

void RefineAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    if(bails>0) {
        if (frame < framend) {
            ++frame;
        } else {
            frame = framestart;
            --bails;
            p::ppool->getPlayer(str->getOwner())->changeMoney(100);
        }
    } else {
        data->done = true;
    }
    data->frame0 = frame;
}

void RefineAnimEvent::updateDamaged()
{
    BuildingAnimEvent::updateDamaged();
    if (anim_data.damaged) {
        if (frame < getaniminfo().dmgoff) {
            frame += getaniminfo().dmgoff;
        }
    }
    framestart = getaniminfo().loopend+1+anim_data.damagedelta;
    framend = framestart + 17; // fixme: avoid hardcoded values
}

BAttackAnimEvent::BAttackAnimEvent(unsigned int p, Structure *str) : BuildingAnimEvent(p,str,8)
{
    this->strct = str;
    strct->referTo();
    this->target = str->getTarget();
    target->referTo();
    done = false;
}

BAttackAnimEvent::~BAttackAnimEvent()
{
    target->unrefer();
    strct->unrefer();
    strct->attackAnim = NULL;
}

void BAttackAnimEvent::update()
{
    target->unrefer();
    target = strct->getTarget();
    target->referTo();
}

void BAttackAnimEvent::run()
{
    unsigned int distance;
    int xtiles, ytiles;
    unsigned short atkpos,mwid;
    float alpha;
    unsigned char facing;
    mwid = p::ccmap->getWidth();
    if( !strct->isAlive() || done ) {
        delete this;
        return;
    }

    if( !target->isAlive() || done) {
        if (!target->isAlive()) {}
        delete this;
        return;
    }
    atkpos = target->getPos();

    xtiles = strct->cellpos % mwid - atkpos % mwid;
    ytiles = strct->cellpos / mwid - atkpos / mwid;
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);

    if( distance > strct->type->getWeapon()->getRange() /* weapons range */ ) {
        /* Since buildings can not move, give up for now.
         * Alternatively, we could just wait to see if the target ever 
         * enters range (highly unlikely when the target is a structure)
         */
        delete this;
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
    facing = (40-(char)(alpha*16/M_PI))&0x1f;

    if ((strct->type->hasTurret())&&((strct->getImageNums()[0]&0x1f)!=facing)) { // turn to face target first
        setDelay(0);
        strct->buildAnim = new BTurnAnimEvent(strct->type->getTurnspeed(), strct, facing);
        strct->buildAnim->setSchedule(this,true);
        p::aequeue->scheduleEvent(strct->buildAnim);
        return;
    }


    // We can shoot
    strct->type->getWeapon()->fire(strct, target->getBPos(strct->getPos()), target->getSubpos());
    setDelay(strct->type->getWeapon()->getReloadTime());
    p::aequeue->scheduleEvent(this);
}

void BAttackAnimEvent::stop()
{
    done = true;
}

BExplodeAnimEvent::BExplodeAnimEvent(unsigned int p, Structure* str) : BuildingAnimEvent(p,str,9)
{
    this->strct = str;
    if (getType()->isWall()) {
        lastframe = strct->getImageNums()[0];
    } else {
        lastframe = getType()->getSHPTNum()[0]-1;
    }
    counter = 0;
    setDelay(1);
}

BExplodeAnimEvent::~BExplodeAnimEvent()
{
    // spawn survivors and other goodies
    p::uspool->removeStructure(strct);
}

void BExplodeAnimEvent::run()
{
    if ((counter == 0) && !(getType()->isWall()) && (pc::sfxeng != NULL) && !p::ccmap->isLoading()) {
        pc::sfxeng->PlaySound("crumble.aud");
        // add code to draw flames
    }
    BuildingAnimEvent::run();
}

void BExplodeAnimEvent::anim_func(anim_nfo* data)
{
    ++counter;
    data->frame0 = lastframe;
    if (counter < 10) {
        data->done = false;
    } else {
        data->done = true;
    }
}

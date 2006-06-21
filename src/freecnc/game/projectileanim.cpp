#include <cmath>

#include "../sound/sound_public.h"
#include "map.h"
#include "projectileanim.h"
#include "structure.h"
#include "unit.h"
#include "unitorstructure.h"
#include "unitandstructurepool.h"
#include "weaponspool.h"

ExplosionAnim::ExplosionAnim(unsigned int p, unsigned short pos, unsigned int startimage,
        unsigned char animsteps, char xoff, char yoff) : ActionEvent(p)
{
    l2o = new L2Overlay(1);
    l2o->imagenums[0] = startimage;
    l2o->xoffsets[0]  = xoff;
    l2o->yoffsets[0] = yoff;
    l2o->cellpos = pos;
    l2entry = p::uspool->addL2overlay(pos, l2o);
    this->animsteps = animsteps;
    p::aequeue->scheduleEvent(this);
    this->pos = pos;
}

ExplosionAnim::~ExplosionAnim()
{
    p::uspool->removeL2overlay(l2entry);
    delete l2o;
}

void ExplosionAnim::run()
{
    animsteps--;
    if( animsteps == 0 ) {
        delete this;
        return;
    }
    ++l2o->imagenums[0];
    p::aequeue->scheduleEvent(this);
}

ProjectileAnim::ProjectileAnim(unsigned int p, Weapon *weap, UnitOrStructure* owner,
        unsigned short dest, unsigned char subdest) : ActionEvent(p)
{
    double pixelspertick;
    double totlen;
    float alpha;

    this->weap = weap;
    this->owner = owner;
    this->dest = dest;
    this->subdest = subdest;
    owner->referTo();

    heatseek = weap->isHeatseek();
    inaccurate = weap->isInaccurate();
    facing = 0;
    fuel = weap->getFuel();
    seekfuel = weap->getSeekFuel();
    // zero fuel means not checking projectile lifetime.
    if (fuel == 0) {
        fuelled = false;
    } else {
        fuelled = true;
    }
    if (seekfuel == 0) {
        // Specifying seekfuel is mandatory if you want a projectile to heatseek.
        logger->warning("Zero fuel specified for seekfuel for \"%s\", disabling heatseeking.\n",weap->getName());
        heatseek = false;
    }

    if (heatseek) {
        target = p::uspool->getUnitOrStructureAt(dest,subdest);
        if (target == NULL) {
            seekfuel = 0;
            heatseek = false;
        } else {
            target->referTo();
        }
    } else {
        target = NULL;
    }
    // speed == 100 -> instant hit.
    if( (weap->getSpeed() < 100 ) && (dest != owner->getPos()) ) {
        pixelspertick = (double)weap->getSpeed()/4.0;
        xdiff = owner->getPos()%p::ccmap->getWidth() - dest%p::ccmap->getWidth();
        ydiff = owner->getPos()/p::ccmap->getWidth() - dest/p::ccmap->getWidth();
        xdiff *= 24;
        ydiff *= 24;
        totlen = sqrt(xdiff*xdiff+ydiff*ydiff);
        xmod = -(xdiff*pixelspertick)/totlen;
        ymod = -(ydiff*pixelspertick)/totlen;
        rxoffs = owner->getXoffset();
        ryoffs = owner->getYoffset();

        if (weap->getProjectile()->doesRotate()) {
            if( xdiff == 0 ) {
                if( ydiff < 0 ) {
                    alpha = -M_PI_2;
                } else {
                    alpha = M_PI_2;
                }
            } else {
                alpha = atan((float)ydiff/(float)xdiff);
                if( xdiff < 0 ) {
                    alpha = M_PI+alpha;
                }
            }
            facing = (40-(char)(alpha*16/M_PI))&0x1f;
        } else {
            facing = 0;
        }

        l2o = new L2Overlay(1);
        l2o->cellpos = owner->getPos();
        l2o->imagenums[0] =  weap->getProjectile()->getImageNum()+facing;
        l2o->xoffsets[0]  = owner->getXoffset();
        l2o->yoffsets[0]  = owner->getYoffset();
        l2entry = p::uspool->addL2overlay(owner->getPos(), l2o);
    } else {
        xmod = ymod = xdiff = ydiff = 0;
        target = NULL;
        l2o = NULL;
        heatseek = false;
    }
}

ProjectileAnim::~ProjectileAnim()
{
    if( l2o != NULL ) {
        p::uspool->removeL2overlay(l2entry);
        delete l2o;
    }
    if (heatseek) {
        if (target->isAlive()) {
            target->unrefer();
        }
    }
    owner->unrefer();
}

void ProjectileAnim::run()
{
    unsigned int oldpos;
    Unit *utarget;
    Structure *starget;
    if (fuelled) {
        --fuel;
    }
    if (heatseek) {
        double pixelspertick;
        double totlen;
        float alpha;
        --seekfuel;
        dest = target->getBPos(l2o->cellpos);
        subdest = target->getSubpos();
        pixelspertick = (double)weap->getSpeed()/4.0;
        xdiff = (l2o->cellpos)%p::ccmap->getWidth() - dest%p::ccmap->getWidth();
        ydiff = (l2o->cellpos)/p::ccmap->getWidth() - dest/p::ccmap->getWidth();
        if ((xdiff == 0) && (ydiff == 0)) {
            xmod = 0;
            ymod = 0;
        } else {
            xdiff *= 24;
            ydiff *= 24;
            totlen = sqrt(xdiff*xdiff+ydiff*ydiff);
            xmod = -(xdiff*pixelspertick)/totlen;
            ymod = -(ydiff*pixelspertick)/totlen;
            if (weap->getProjectile()->doesRotate()) {
                if( xdiff == 0 ) {
                    if( ydiff < 0 ) {
                        alpha = -M_PI_2;
                    } else {
                        alpha = M_PI_2;
                    }
                } else {
                    alpha = atan((float)ydiff/(float)xdiff);
                    if( xdiff < 0 ) {
                        alpha = M_PI+alpha;
                    }
                }
                facing = (40-(char)(alpha*16/M_PI))&0x1f;
                l2o->imagenums[0] = weap->getProjectile()->getImageNum()+facing;

            }
        }
        if (seekfuel == 0) {
            heatseek = false;
            target->unrefer();
        }
    }
    //check if we are close enough to target to stop modifying
    if( fabs(xdiff) < fabs(xmod) ) {
        xmod = 0;
    }
    if( fabs(ydiff) < fabs(ymod) ) {
        ymod = 0;
    }
    // if we are so close both
    if( xmod == 0 && ymod == 0 ) {
        //projectile hit..
        if( weap->getWarhead()->getExplosionsound() != NULL ) {
            pc::sfxeng->PlaySound(weap->getWarhead()->getExplosionsound());
        }
        new ExplosionAnim(1, dest, weap->getWarhead()->getEImage(),
                          weap->getWarhead()->getESteps(), 0, 0);

        starget = p::uspool->getStructureAt(dest,weap->getWall());
        if( starget != NULL ) {
            starget->applyDamage(weap->getDamage(),weap,owner);
            delete this;
            return;
        }

        utarget = p::uspool->getUnitAt(dest, subdest);
        if( (utarget != NULL) || inaccurate ) {
            if (inaccurate) {
                for (int sud=0;sud<5;++sud) {
                    utarget = p::uspool->getUnitAt(dest, sud);
                    if (utarget != NULL) {
                        // each soldier in that cell gets one third of
                        // normal damage
                        utarget->applyDamage((short)((double)weap->getDamage()/3.0),weap,owner);
                    }
                } // targeted soldier gets full normal damage
                utarget = p::uspool->getUnitAt(dest, subdest);
                if (utarget != NULL) { // soldier might have already been killed
                    utarget->applyDamage((short)(2.0*(double)weap->getDamage()/3.0),weap,owner);
                }
                delete this;
                return;
            } else {
                utarget->applyDamage(weap->getDamage(),weap,owner);
                delete this;
                return;
            }
        }
        delete this;
        return;
    }
    if (!heatseek) {
        // decrease xdiff by xmod and ydiff by ymod
        xdiff += xmod;
        ydiff += ymod;
    }
    // move the actual projectile
    rxoffs += xmod;
    ryoffs += ymod;

    oldpos = l2o->cellpos;
    while( rxoffs < 0 ) {
        rxoffs += 24;
        --l2o->cellpos;
    }
    while(rxoffs >= 24) {
        rxoffs -= 24;
        ++l2o->cellpos;
    }

    while( ryoffs < 0 ) {
        ryoffs += 24;
        l2o->cellpos -= p::ccmap->getWidth();
    }
    while( ryoffs >= 24 ) {
        ryoffs -= 24;
        l2o->cellpos += p::ccmap->getWidth();
    }
    l2o->xoffsets[0] = (char)rxoffs;
    l2o->yoffsets[0] = (char)ryoffs;

    if( oldpos != l2o->cellpos) {
        p::uspool->removeL2overlay(l2entry);
        l2entry = p::uspool->addL2overlay(l2o->cellpos, l2o);
    }
    setDelay(1);
    if (fuelled && fuel == 0) {
        if( weap->getWarhead()->getExplosionsound() != NULL ) {
            pc::sfxeng->PlaySound(weap->getWarhead()->getExplosionsound());
        }
        new ExplosionAnim(1, l2o->cellpos, weap->getWarhead()->getEImage(),
                          weap->getWarhead()->getESteps(), 0, 0);

        delete this;
        return;
    }
    p::aequeue->scheduleEvent(this);
}

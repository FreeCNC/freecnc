#include <cstdlib>
#include <cstring>

#include "../renderer/renderer_public.h"
#include "../sound/sound_public.h"
#include "map.h"
#include "playerpool.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "unitanimations.h"
#include "unitorstructure.h"
#include "structure.h"
#include "talkback.h"
#include "weaponspool.h"

const char InfantryGroup::unitoffsets[10] = {
    /* Theses values have been heavily tested, do NOT change them unless you're
     *        _really_ sure of what you are doing */
    /* X value */
    -13, -19, -7, -19, -7,
    /* Y value */
    -3, -7, -7, 1, 1
};

UnitType::UnitType(const char *typeName, shared_ptr<INIFile> unitini)
    : UnitOrStructureType(), shpnums(0), name(0), deploytarget(0)
{
    SHPImage* shpimage;
    unsigned int i;
    string shpname(typeName);
#ifdef LOOPEND_TURN
    char* imagename;
#endif
    unsigned int shpnum;
    unsigned int tmpspeed;

    deploytarget = NULL;
    // Ensure that there is a section in the ini file
    try {
        unitini->readKeyValue(typeName,0);
    } catch (int) {
        logger->error("Unknown type: %s\n",typeName);
        name = NULL;
        shpnums = NULL;
        return;
    }

    memset(this->tname,0x0,8);
    strncpy(this->tname,typeName,8);
    name = unitini->readString(tname,"name");
    char* tmp = unitini->readString(tname,"prerequisites");
    if (0 != tmp) {
        prereqs = splitList(tmp,',');
        delete[] tmp;
    }

    unittype = unitini->readInt(tname, "unittype",0);
    if (0 == unittype) {
        logger->warning("No unit type specified for \"%s\"\n", tname);
    }

    numlayers = unitini->readInt(tname, "layers", 1);

    shpnums = new unsigned int[numlayers];

    shpname += ".SHP";
    try {
        shpimage = new SHPImage(shpname.c_str(), mapscaleq);
    } catch (ImageNotFound&) {
        logger->error("Image not found: \"%s\"\n", shpname.c_str());
        numlayers = 0;
        return;
    }
    shpnum = static_cast<unsigned int>(pc::imagepool->size());
    pc::imagepool->push_back(shpimage);
    shpnum <<= 16;
    for( i = 0; i < numlayers; i++ ) {
        /* get layer offsets from inifile */
        shpnums[i] = shpnum;
        shpnum += 32;
    }
    is_infantry = false;

    buildlevel = unitini->readInt(tname,"buildlevel",99);
    techlevel = unitini->readInt(tname,"techlevel",99);

    tmp = unitini->readString(tname, "owners");
    if( tmp != NULL ) {
        owners = splitList(tmp,',');
        delete[] tmp;
    }

    if (unittype == 1)
        is_infantry = true;

    tmpspeed = unitini->readInt(tname, "speed");
    if (is_infantry) {
        if (tmpspeed == INIERROR) {
            speed = 4; // default for infantry is slower
            movemod = 1;
        } else {
            speed = (tmpspeed>4)?2:(7-tmpspeed);
            movemod = (tmpspeed>4)?(tmpspeed-4):1;
        }
    } else {
        if (tmpspeed == INIERROR) {
            speed = 2;
            movemod = 1;
        } else {
            speed = (tmpspeed>4)?2:(7-tmpspeed);
            movemod = (tmpspeed>4)?(tmpspeed-4):1;
        }
    }
    char* talkmode = 0;
    if (is_infantry) {
        talkmode = unitini->readString(tname, "talkback","Generic");
        sight = unitini->readInt(tname, "sight", 3);
    } else {
        talkmode = unitini->readString(tname, "talkback","Generic-Vehicle");
        sight = unitini->readInt(tname, "sight", 5);
    }
    talkback = p::uspool->getTalkback(talkmode);
    delete[] talkmode;
    maxhealth = unitini->readInt(tname, "health", 50);
    cost = unitini->readInt(tname, "cost", 0);
    if (0 == cost) {
        logger->error("\"%s\" has no cost, setting to 1\n", tname);
        cost = 1;
    }
    tmpspeed = unitini->readInt(tname, "turnspeed");
    if (tmpspeed == INIERROR) { // no default for infantry as do not turn
        turnspeed = 2;
        turnmod = 1;
    } else {
        turnspeed = (tmpspeed>4)?2:(7-tmpspeed);
        turnmod = (tmpspeed>4)?(tmpspeed-4):1;
    }
    if( is_infantry ) {
        //      size = 1;
        offset = 0;
    } else {
        //size = shpimage->getWidth();
        offset = (shpimage->getWidth()-24)>>1;
    }
    char* miscnames = unitini->readString(tname, "primary_weapon");
    if( miscnames == NULL ) {
        primary_weapon = NULL;
    } else {
        primary_weapon = p::weappool->getWeapon(miscnames);
        delete[] miscnames;
    }
    miscnames = unitini->readString(tname, "secondary_weapon");
    if( miscnames == NULL ) {
        secondary_weapon = NULL;
    } else {
        secondary_weapon = p::weappool->getWeapon(miscnames);
        delete[] miscnames;
    }
    deploytarget = unitini->readString(tname, "deploysto");
    if (deploytarget != NULL) {
        deployable = true;
        deploytype = p::uspool->getStructureTypeByName(deploytarget);
    }
    pipcolour = unitini->readInt(tname,"pipcolour",0);
    miscnames = unitini->readString(tname,"armour");
    if (miscnames == NULL)
        armour = AC_none;
    else {
        if (strncasecmp(miscnames,"none",4) == 0)
            armour = AC_none;
        else if (strncasecmp(miscnames,"wood",4) == 0)
            armour = AC_wood;
        else if (strncasecmp(miscnames,"light",5) == 0)
            armour = AC_light;
        else if (strncasecmp(miscnames,"heavy",5) == 0)
            armour = AC_heavy;
        else if (strncasecmp(miscnames,"concrete",8) == 0)
            armour = AC_concrete;

        delete[] miscnames;
    }
    valid = true;
}


//////
const char* UnitType::getRandTalk(TalkbackType type) const
{
    if (talkback) {
        return talkback->getRandTalk(type);
    }
    return NULL;
}

UnitType::~UnitType()
{
    unsigned short i;
    delete[] name;
    delete[] shpnums;
    delete[] deploytarget;
    for (i=0;i<owners.size();++i)
        delete[] owners[i];
    for (i=0;i<prereqs.size();++i)
        delete[] prereqs[i];
}

/* note to self, pass owner, cellpos, facing and health to this
 (maybe subcellpos)*/
Unit::Unit(UnitType *type, unsigned short cellpos, unsigned char subpos, InfantryGroupPtr group,
        unsigned char owner, unsigned short rhealth, unsigned char facing) : UnitOrStructure()
{
    targetcell = cellpos;
    unsigned int i;
    this->type = type;
    imagenumbers = new unsigned short[type->getNumLayers()];
    for( i = 0; i < type->getNumLayers(); i++ ) {
        imagenumbers[i] = facing;
        if( owner != 0xff ) {
            if (type->getDeployTarget() != NULL) {
                palettenum = (p::ppool->getStructpalNum(owner)<<11);
            } else {
                palettenum = (p::ppool->getUnitpalNum(owner)<<11);
            }
            imagenumbers[i] |= palettenum;
        }
    }
    this->owner = owner;
    this->cellpos = cellpos;
    this->subpos = subpos;
    l2o = NULL;
    xoffset = 0;
    yoffset = 0;
    ratio = (double)rhealth/256.0f;
    health = (unsigned short)(ratio * type->getMaxHealth());
    infgrp = group;

    if (infgrp) {
        if (infgrp->IsClear(subpos)) { /* else select another subpos */
            infgrp->AddInfantry(this, subpos);
        }
    }
    moveanim = NULL;
    attackanim = NULL;
    walkanim = NULL;
    turnanim1 = turnanim2 = NULL;
    deployed = false;
    p::ppool->getPlayer(owner)->builtUnit(this);
}

Unit::~Unit()
{
    delete[] imagenumbers;
    if (attackanim != NULL && target != NULL) {
        target->unrefer();
    }
    if( l2o != NULL ) {
        p::uspool->removeL2overlay(l2entry);
        delete l2o;
    }
    if (type->isInfantry() && infgrp) {
        infgrp->RemoveInfantry(subpos);
        if (infgrp->GetNumInfantry() == 0) {
            //delete infgrp;
        }
    }
    if (deployed) {
        /** @todo This is a client thing. Will dispatch a "play these sounds"
         * event when the time comes.
         */
        pc::sfxeng->PlaySound("constru2.aud");
        pc::sfxeng->PlaySound("hvydoor1.aud");
        p::uspool->createStructure(type->getDeployTarget(),calcDeployPos(),owner,(unsigned short)(ratio*256.0f),0,true);
    }
}

void Unit::remove() {
    p::ppool->getPlayer(owner)->lostUnit(this,deployed);
    UnitOrStructure::remove();
}


unsigned char Unit::getImageNums(unsigned int **inums, char **xoffsets, char **yoffsets)
{
    int i;
    unsigned int *shpnums;

    shpnums = type->getSHPNums();

    *inums = new unsigned int[type->getNumLayers()];
    *xoffsets = new char[type->getNumLayers()];
    *yoffsets = new char[type->getNumLayers()];
    for(i = 0; i < type->getNumLayers(); i++ ) {
        (*inums)[i] = shpnums[i]+imagenumbers[i];
        (*xoffsets)[i] = xoffset-type->getOffset();
        (*yoffsets)[i] = yoffset-type->getOffset();
    }
    return type->getNumLayers();
}

void Unit::move(unsigned short dest)
{
    move(dest,true);
}

void Unit::move(unsigned short dest, bool stop)
{
    targetcell = dest;
    if (stop && (attackanim != NULL)) {
        attackanim->stop();
        if (target != NULL) {
            target->unrefer();
            target = NULL;
        }
    }
    if (moveanim == NULL) {
        moveanim = new MoveAnimEvent(type->getSpeed(), this);
        p::aequeue->scheduleEvent(moveanim);
    } else {
        moveanim->update();
    }
}

void Unit::attack(UnitOrStructure* target)
{
    attack(target, true);
}

void Unit::attack(UnitOrStructure* target, bool stop)
{
    if (stop && (moveanim != NULL)) {
        moveanim->stop();
    }
    if (this->target != NULL) {
        this->target->unrefer();
    }
    this->target = target;
    target->referTo();
    targetcell = target->getBPos(cellpos);
    if (attackanim == NULL) {
        attackanim = new UAttackAnimEvent(0, this);
        p::aequeue->scheduleEvent(attackanim);
    } else {
        attackanim->update();
    }
}

void Unit::turn(unsigned char facing, unsigned char layer)
{
    TurnAnimEvent** t;
    switch (layer) {
    case 0:
        t = &turnanim1;
        break;
    case 1:
        t = &turnanim2;
        break;
    default:
        logger->error("invalid arg of %i to Unit::turn\n",layer);
        return;
        break;
    }
    if (*t == NULL) {
        *t = new TurnAnimEvent(type->getROT(), this, facing, layer);
        p::aequeue->scheduleEvent(*t);
    } else {
        (*t)->changedir(facing);
    }

}

void Unit::stop()
{
    if (moveanim != NULL) {
        moveanim->stop();
    }
    if (attackanim != NULL) {
        attackanim->stop();
    }
}

void Unit::applyDamage(short amount, Weapon* weap, UnitOrStructure* attacker)
{
    //fprintf(stderr,"%i * %f = ",amount,weap->getVersus(type->getArmour()));
    amount = (short)((double)amount * weap->getVersus(type->getArmour()));
    //fprintf(stderr,"%i (a == %i)\n",amount,type->getArmour());
    if ((health-amount) <= 0) {
        doRandTalk(TB_die);
        p::ppool->getPlayer(attacker->getOwner())->addUnitKill();
        // todo: add infantry death animation
        p::uspool->removeUnit(this);
        return;
    } else if ((health-amount) > type->getMaxHealth()) {
        health = type->getMaxHealth();
    } else {
        health -= amount;
    }
    ratio = (double)health / (double)type->getMaxHealth();
}

void Unit::doRandTalk(TalkbackType ttype)
{
    const char* sname;
    sname = type->getRandTalk(ttype);
    if (sname != NULL) {
        pc::sfxeng->PlaySound(sname);
    }
}

bool Unit::canDeploy()
{
    if (type->canDeploy()) {
        if (type->getDeployTarget() != NULL) {
            if (!deployed)
                return checkDeployTarget(calcDeployPos());
            return false;
        }
   }
    return false;
}

void Unit::deploy()
{
    if (canDeploy()) { // error catching
        if (type->getDeployTarget() != NULL) {
            deployed = true;
            p::uspool->removeUnit(this);
        }
    }
}

bool Unit::checkDeployTarget(unsigned int pos)
{
    static unsigned int mapwidth = p::ccmap->getWidth();
    static unsigned int mapheight = p::ccmap->getHeight();
    unsigned char placexpos, placeypos;
    unsigned int curpos;
    unsigned char typewidth, typeheight;
    if (pos == (unsigned int)(-1)) {
        return false;
    }
    if (type->getDeployType() == NULL) {
        //return (p::ccmap->getCost(pos,this)<=1);
        return false;
    }
    typewidth = type->getDeployType()->getXsize();
    typeheight = type->getDeployType()->getYsize();
    if ((pos%mapwidth)+typewidth > mapwidth) {
        return false;
    }
    if ((pos/mapwidth)+typeheight > mapheight) {
        return false;
    }
    for( placeypos = 0; placeypos < typeheight; ++placeypos) {
        for( placexpos = 0; placexpos < typewidth; ++placexpos) {
            curpos = pos+placeypos*mapwidth+placexpos;
            if( type->getDeployType()->isBlocked(placeypos*typewidth+placexpos) ) {
                if (!p::ccmap->isBuildableAt(curpos,this)) {
                    return false;
                }
            }
        }
    }
    return true;
}

unsigned int Unit::calcDeployPos() const
{
    unsigned int deploypos;
    unsigned int mapwidth = p::ccmap->getWidth();
    unsigned char w,h;

    if (type->getDeployType() == NULL) {
        if (cellpos%mapwidth == mapwidth) {
            return (unsigned int)-1;
        }
        deploypos = cellpos+1;
    } else {
        w = type->getDeployType()->getXsize();
        h = type->getDeployType()->getYsize();

        deploypos = cellpos;
        if ((unsigned int)(w >> 1) > deploypos)
            return (unsigned int)-1; // large number
        else
            deploypos -= w >> 1;
        if ((mapwidth*(h >> 1)) > deploypos)
            return (unsigned int)-1;
        else
            deploypos -= mapwidth*(h >> 1);
    }
    return deploypos;
}

void Unit::setImageNum(unsigned int num, unsigned char layer)
{
    imagenumbers[layer] = num | palettenum;
}

char Unit::getXoffset() const
{
    if (l2o != NULL) {
        return l2o->xoffsets[0];
    } else {
        return xoffset-type->getOffset();
    }
}

char Unit::getYoffset() const
{
    if (l2o != NULL) {
        return l2o->yoffsets[0];
    } else {
        return yoffset-type->getOffset();
    }
}

void Unit::setXoffset(char xo)
{
    if (l2o != NULL) {
        l2o->xoffsets[0] = xo;
    } else {
        xoffset = xo;
    }
}

void Unit::setYoffset(char yo)
{
    if (l2o != NULL) {
        l2o->yoffsets[0] = yo;
    } else {
        yoffset = yo;
    }
}

unsigned short Unit::getDist(unsigned short pos)
{
    unsigned short x, y, nx, ny, xdiff, ydiff;
    x = cellpos%p::ccmap->getWidth();
    y = cellpos/p::ccmap->getWidth();
    nx = pos%p::ccmap->getWidth();
    ny = pos/p::ccmap->getWidth();

    xdiff = abs(x-nx);
    ydiff = abs(y-ny);
    return min(xdiff,ydiff)+abs(xdiff-ydiff);
}

unsigned short Unit::getTargetCell()
{
    if (attackanim != NULL && target != NULL) {
        return target->getBPos(cellpos);
    }
    return targetcell;
}

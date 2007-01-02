#include <cstdlib>
#include <cstring>
#include "../lib/inifile.h"
#include "../renderer/renderer_public.h"
#include "../sound/sound_public.h"
#include "map.h"
#include "playerpool.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "structure.h"
#include "weaponspool.h"

StructureType::StructureType(const char* typeName, shared_ptr<INIFile> structini,
        shared_ptr<INIFile> artini, const char* thext) : UnitOrStructureType()
{
    SHPImage *shpimage, *makeimage;
    unsigned int i;
    char shpname[13], imagename[8];
    char* tmp;
    char blocktest[128];
    unsigned int size;
    char* miscnames;

    // Ensure that there is a section in the ini file
    try {
        structini->readKeyValue(typeName,0);
    } catch (int) {
        shpnums = NULL;
        blocked = NULL;
        shptnum = NULL;
        name    = NULL;
        return;
    }

    is_wall = false;

    memset(this->tname,0x0,8);
    strncpy(this->tname,typeName,8);
    name = structini->readString(tname,"name");
    //prereqs = structini->splitList(tname,"prerequisites",',');
    tmp = structini->readString(tname, "prerequisites");
    if( tmp != NULL ) {
        prereqs = splitList(tmp, ',');
        delete[] tmp;
    }
    //owners = structini->splitList(tname,"owners",',');
    tmp = structini->readString(tname, "owners");
    if(  tmp != NULL ) {
        owners = splitList(tmp, ',');
        delete[] tmp;
    }
    if (owners.empty()) {
        game.log << "StructureType: " << tname << " has no owners" << endl;
    }

    if( !strcasecmp(tname, "BRIK") || !strcasecmp(tname, "SBAG") ||
            !strcasecmp(tname, "BARB") || !strcasecmp(tname, "WOOD") ||
            !strcasecmp(tname, "CYCL") || !strcasecmp(tname, "FENC") )
        is_wall = true;

    /* the size of the structure in tiles */
    xsize = artini->readInt(tname, "xsize",1);
    ysize = artini->readInt(tname, "ysize",1);

    blckoff = 0;
    blocked = new unsigned char[xsize*ysize];
    for( i = 0; i < (unsigned int)xsize*ysize; i++ ) {
        sprintf(blocktest, "notblocked%d", i);
        size = artini->readInt(tname, blocktest);
        if(size == INIERROR) {
            blocked[i] = 1;
            xoffset = -(i%xsize)*24;
            yoffset = -(i/xsize)*24;
            if (blocked[blckoff] == 0) {
                blckoff = i;
            }
        } else {
            blocked[i] = 0;
        }
    }

    memset(shpname,0,13);
    numshps = structini->readInt(tname, "layers",1);

    shpnums = new unsigned short[(is_wall?numshps:numshps+1)];
    shptnum = new unsigned short[numshps];

    buildlevel = structini->readInt(tname,"buildlevel",100);
    techlevel = structini->readInt(tname,"techlevel",99);
    if (buildlevel == 100) {
        game.log << "StructureType: " << tname << " does not have a buildlevel" << endl;
    }

    powerinfo.power = structini->readInt(tname,"power",0);
    powerinfo.drain = structini->readInt(tname,"drain",0);
    powerinfo.powered = structini->readInt(tname,"powered",0) != 0;
    maxhealth = structini->readInt(tname,"health",100);

    for( i = 0; i < numshps; i++ ) {
        sprintf(imagename, "image%d", i+1);
        tmp = structini->readString(tname, imagename);
        if( tmp == NULL ) {
            strncpy(shpname, tname, 13);
            strncat(shpname, ".SHP", 13);
        } else {
            strncpy(shpname, tmp, 13);
            delete[] tmp;
        }
        try {
            shpimage = new SHPImage(shpname, mapscaleq);
        } catch (ImageNotFound&) {
            strncpy(shpname, tname, 13);
            strncat(shpname, thext, 13);
            try {
                shpimage = new SHPImage(shpname, mapscaleq);
            } catch (ImageNotFound&) {
                game.log << "Image not found: \"" << shpname << "\""  << endl;
                numshps = 0;
                return;
            }
        }
        shpnums[i] = pc::imagepool->size();
        shptnum[i] = shpimage->getNumImg();
        pc::imagepool->push_back(shpimage);
    }
    if (!is_wall) {
        numwalllevels = 0;
        animinfo.loopend = structini->readInt(tname,"loopend",0);
        animinfo.loopend2 = structini->readInt(tname,"loopend2",0);

        animinfo.animspeed = structini->readInt(tname,"animspeed", 3);
        animinfo.animspeed = abs(animinfo.animspeed);
        animinfo.animspeed = (animinfo.animspeed>1?animinfo.animspeed:2);
        animinfo.animdelay = structini->readInt(tname,"delay",0);

        animinfo.animtype = structini->readInt(tname, "animtype", 0);
        animinfo.sectype  = structini->readInt(tname, "sectype", 0);

        animinfo.dmgoff   = structini->readInt(tname, "dmgoff", ((shptnum[0]-1)>>1));
        if (numshps == 2)
            animinfo.dmgoff2 = structini->readInt(tname, "dmgoff2", (shptnum[1]>>1));
        else
            animinfo.dmgoff2 = 0;

        defaultface       = structini->readInt(tname, "defaultface", 0);

        if (strlen(tname) <= 4) {
            strncpy(shpname, tname, 13);
            strncat(shpname, "make.shp", 13);
        } else
            game.log << "StructureType: \"" << tname << "\" is a non standard typename" << endl;
        try {
            makeimage = new SHPImage(shpname, mapscaleq);
            makeimg = pc::imagepool->size();
            animinfo.makenum = makeimage->getNumImg();
            pc::imagepool->push_back(makeimage);
        } catch (ImageNotFound&) {
            makeimg = 0;
            animinfo.makenum = 0;
        }

        miscnames = structini->readString(tname, "primary_weapon");
        if( miscnames == NULL ) {
            primary_weapon = NULL;
        } else {
            primary_weapon = p::weappool->getWeapon(miscnames);
            delete[] miscnames;
        }
        miscnames = structini->readString(tname, "secondary_weapon");
        if( miscnames == NULL ) {
            secondary_weapon = NULL;
        } else {
            secondary_weapon = p::weappool->getWeapon(miscnames);
            delete[] miscnames;
        }
        turret = (structini->readInt(tname,"turret",0) != 0);
        if (turret) {
            turnspeed = structini->readInt(tname,"rot",3);
        }
    } else {
        numwalllevels = structini->readInt(tname,"levels",1);
        turret = 0;
        primary_weapon = NULL;
        secondary_weapon = NULL;
    }
    cost = structini->readInt(tname, "cost", 0);
    if (0 == cost) {
        game.log << "StructureType: \"" << tname << "\" has no cost, resetting to 1"  << endl;
        cost = 1;
    }
    miscnames = structini->readString(tname,"armour","none");
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

    primarysettable = (structini->readInt(tname,"primary",0) != 0);
    valid = true;
}


StructureType::~StructureType()
{
    unsigned short i;
    for (i=0;i<owners.size();++i)
        delete[] owners[i];
    for (i=0;i<prereqs.size();++i)
        delete[] prereqs[i];
    delete[] shpnums;
    delete[] blocked;
    delete[] shptnum;
    delete[] name;
}

Structure::Structure(StructureType *type, unsigned short cellpos, unsigned char owner,
        unsigned short rhealth, unsigned char facing) : UnitOrStructure()
{
    unsigned int i;
    targetcell = cellpos;
    this->type = type;
    imagenumbers = new unsigned short[type->getNumLayers()];
    if (!type->hasTurret()) {
        facing = 0;
    }
    for(i=0;i<type->getNumLayers();i++) {
        imagenumbers[i] = facing;
        if( owner != 0xff && !type->isWall() ) {
            imagenumbers[i] |= (p::ppool->getStructpalNum(owner)<<11);
        }
    }
    exploding = false;
    this->owner = owner;
    this->cellpos = cellpos;
    bcellpos = cellpos+(type->getBlckOff() % type->getXsize()) +
               ((type->getBlckOff()/type->getXsize())*p::ccmap->getWidth());
    animating = false;
    usemakeimgs = false;
    primary = false;
    health = (unsigned short)((double)rhealth/256.0f * (double)type->getMaxHealth());
    update_damaged_look(true);
    if( !type->isWall() ) {
        p::ppool->getPlayer(owner)->builtStruct(this);
    }
}

Structure::~Structure()
{
    delete[] imagenumbers;
}

unsigned short Structure::getBPos(unsigned short pos) const
{
    unsigned short x,y,dx,dy,t,retpos,bpos,sc;
    short dw;
    unsigned int mwid = p::ccmap->getWidth();
    x = cellpos%mwid;
    dx = 0;
    if ((pos%mwid) > x) {
        dx = min((unsigned)(type->getXsize()-1),(unsigned)((pos%mwid)-x)-1);
    }
    y = cellpos/mwid;
    dy = 0;
    if ((pos/mwid) > y) {
        dy = min((unsigned)(type->getYsize()-1),(unsigned)((pos/mwid)-y)-1);
    }
    retpos = (x+dx)+(y+dy)*mwid;
    // just makes the bpos calculation cleaner
    sc = x+y*mwid;
    dw = type->getXsize() - mwid;
    bpos   = retpos - sc + dy*dw;
    while (!type->isBlocked(dx+dy*type->getXsize())) {
        /* This happens in this situation (P is position of attacker,
         * X is a blocked cell and _ is an unblocked cell)
         * P   P
         *  _X_
         *  XXX
         *  _X_
         * P   P
         */
        if (dx == type->getXsize()-1) {
            for (t=dx;t>0;--t) {
                retpos = (x+t)+(y+dy)*mwid;
                bpos   = retpos - sc + dy*dw;
                if (type->isBlocked(bpos)) {
                    return retpos;
                }
            }
        } else {
            for (t=dx;t<type->getXsize();++t) {
                retpos = (x+t)+(y+dy)*mwid;
                bpos   = retpos - sc + dy*dw;
                if (type->isBlocked(bpos)) {
                    return retpos;
                }
            }
        }
        ++dy;
        if (dy >= type->getYsize()) {
            game.log << "Structure: ERROR: could not find anywhere to shoot at " << type->getTName() << endl;
        }
        retpos = (x+dx)+(y+dy)*mwid;
    }
    return retpos;
}

/// Helper function for getFreePos
/// @BUG Doesn't check that the terrain is passable (only buildable).
static bool valid_pos(unsigned short pos, unsigned char*) {
    return p::ccmap->isBuildableAt(pos);
}

/// Helper function for getFreePos
/// @BUG Doesn't check that the terrain is passable (only buildable).
static bool valid_possubpos(unsigned short pos, unsigned char* subpos) {
    InfantryGroupPtr ig = p::uspool->getInfantryGroupAt(pos);
    if (!ig) {
        return p::ccmap->isBuildableAt(pos);
    }
    if (ig->IsAvailable()) {
        *subpos = ig->GetFreePos();
        return true;
    }
    return false;
}

/// @BUG This literally doesn't handle edge cases.
unsigned short Structure::getFreePos(unsigned char* subpos, bool findsubpos) {
    bool (*checker)(unsigned short, unsigned char*);
    unsigned char i, xsize, ysize;
    unsigned short x, y, curpos;

    xsize = type->getXsize();
    ysize = type->getYsize();

    curpos = cellpos;
    p::ccmap->translateFromPos(curpos, &x, &y);
    y += ysize; // bottom left of building
    curpos = p::ccmap->translateToPos(x,y);

    if (findsubpos) {
        checker = valid_possubpos;
    } else {
        checker = valid_pos;
    }
    if (!checker(curpos, subpos))
        curpos = 0xffff;

    for (i = 0; (i < xsize && 0xffff == curpos); ++i) {
        ++x;
        curpos = p::ccmap->translateToPos(x, y);
        if (!checker(curpos, subpos))
            curpos = 0xffff;
    }
    // ugly: I assume that the first blocks are noblocked - g0th
    for (i = 0;(i < ysize && 0xffff == curpos); ++i) {
        --y;
        curpos = p::ccmap->translateToPos(x, y);
        if (!checker(curpos, subpos))
            curpos = 0xffff;
    }
    for (i = 0; (i < (xsize+1) && 0xffff == curpos); ++i) {
        --x;
        curpos = p::ccmap->translateToPos(x, y);
        if (!checker(curpos, subpos))
            curpos=0xffff;
    }
    for (i = 0; (i < ysize && 0xffff == curpos); ++i) {
        ++y;
        curpos = p::ccmap->translateToPos(x, y);
        if (!checker(curpos, subpos))
            curpos=0xffff;
    }
    return curpos;
}

void Structure::remove() {
    if (!type->isWall()) {
        p::ppool->getPlayer(owner)->lostStruct(this);
    }
    UnitOrStructure::remove();
}

/** Method to get a list of imagenumbers which the renderer will draw. */
unsigned char Structure::getImageNums(unsigned int **inums, char **xoffsets, char **yoffsets) {
    unsigned short *shps;
    int i;

    shps = type->getSHPNums();

    if (usemakeimgs && (!type->isWall()) && (type->getMakeImg() != 0)) {
        *inums = new unsigned int[1];
        *xoffsets = new char[1];
        *yoffsets = new char[1];
        (*inums)[0] = (type->getMakeImg()<<16)|imagenumbers[0];
        (*xoffsets)[0] = type->getXoffset();
        (*yoffsets)[0] = type->getYoffset();
        return 1;
    } else {
        *inums = new unsigned int[type->getNumLayers()];
        *xoffsets = new char[type->getNumLayers()];
        *yoffsets = new char[type->getNumLayers()];
        for(i = 0; i < type->getNumLayers(); i++ ) {
            (*inums)[i] = (shps[i]<<16)|imagenumbers[i];
            (*xoffsets)[i] = type->getXoffset();
            (*yoffsets)[i] = type->getYoffset();
        }
        return type->getNumLayers();
    }
}

void Structure::runAnim(unsigned int mode)
{
    unsigned int speed;
    if (!animating) {
        animating = true;
        if (mode == 0) { // run build anim at const speed
            usemakeimgs = true;
            buildAnim.reset(new BuildAnimEvent(3,this,false));
        } else {
            speed = type->getAnimInfo().animspeed;
            switch (mode&0xf) {
            case 1:
                if (type->getAnimInfo().animtype == 4) {
                    buildAnim.reset(new ProcAnimEvent(speed,this));
                } else {
                    buildAnim.reset(new LoopAnimEvent(speed,this));
                }
                break;
            case 2:
                buildAnim.reset(new BTurnAnimEvent(speed,this,(mode>>4)));
                break;
            case 7:
                buildAnim.reset(new RefineAnimEvent(speed,this,5));
                break;
            default:
                buildAnim.reset();
                animating = false;
                break;
            }
        }
        if (buildAnim) {
            p::aequeue->scheduleEvent(buildAnim);
        }
    }
}

void Structure::runSecAnim(unsigned int param)
{
    shared_ptr<BuildingAnimEvent> sec_anim;
    unsigned char secmode = type->getAnimInfo().sectype;
    if (secmode == 0) {
        return;
    }

    switch (secmode) { /// @TODO Do something with these arbitrary numbers here
    case 7:
        sec_anim.reset(new RefineAnimEvent(2,this,param));
        break;
    case 5:
        sec_anim.reset(new DoorAnimEvent(2,this,(param!=0)));
        break;
    case 8:
        //sec_anim.reset(new RepairAnimEvent(3,this));
        break;
    }
    if (animating) {
        buildAnim->setSchedule(sec_anim);
        stopAnim();
    } else {
        buildAnim = sec_anim;
        p::aequeue->scheduleEvent(buildAnim);
        animating = true;
    }
}

void Structure::stopAnim()
{
    buildAnim->stop();
}

void Structure::stop()
{
    if (attackAnim) {
        attackAnim->stop();
    }
}

void Structure::applyDamage(short amount, Weapon* weap, UnitOrStructure* attacker)
{
    if (exploding)
        return;
    amount = (short)((double)amount * weap->getVersus(type->getArmour()));
    if ((health-amount) <= 0) {
        exploding = true;
        if (type->isWall()) {
            p::uspool->removeStructure(this);
        } else {
            p::ppool->getPlayer(attacker->getOwner())->addStructureKill();
            shared_ptr<BuildingAnimEvent> boom(new BExplodeAnimEvent(1, this));
            if (animating) {
                buildAnim->setSchedule(boom);
                buildAnim->stop();
            } else {
                buildAnim = boom;
                p::aequeue->scheduleEvent(boom);
            }
        }
        return;
    } else if ((health-amount)>type->getMaxHealth()) {
        health = type->getMaxHealth();
    } else {
        health -= amount;
    }
    if (animating) {
        buildAnim->updateDamaged();
        return;
    }

    update_damaged_look(false);
}

void Structure::update_damaged_look(bool first_time)
{
    unsigned char previously_damaged;
    if (!first_time) {
        previously_damaged = damaged;
    } else {
        previously_damaged = false;
    }
    damaged = checkdamage();

    int new_image[2];
    new_image[0] = imagenumbers[0]&~0x800;
    if (type->getNumLayers() == 2) {
        new_image[1] = imagenumbers[1]&~0x800;
    }

    if (damaged && !previously_damaged) {
        if (type->isWall()) {
            changeImage(0,16);
            return;
        }
        // only play critical damage sound once
        if (!first_time)
            pc::sfxeng->PlaySound("xplobig4.aud");

        new_image[0] += type->getAnimInfo().dmgoff;
        if (type->getNumLayers() == 2) {
            new_image[1] += type->getAnimInfo().dmgoff2;
        }
    } else if (!damaged && previously_damaged) {
        new_image[0] -= type->getAnimInfo().dmgoff;
        if (type->getNumLayers() == 2) {
            new_image[1] -= type->getAnimInfo().dmgoff2;
        }
    }

    setImageNum(new_image[0],0);
    if (type->getNumLayers() == 2) {
        setImageNum(new_image[1],1);
    }

}

void Structure::attack(UnitOrStructure* target)
{
    this->target = target;
    targetcell = target->getBPos(cellpos);
    if (attackAnim) {
        attackAnim->update();
    } else {
        attackAnim.reset(new BAttackAnimEvent(0, this));
        p::aequeue->scheduleEvent(attackAnim);
    }
}

unsigned char Structure::checkdamage()
{
    ratio = ((double)health)/((double)type->getMaxHealth());
    if (type->isWall()) {
        if ((ratio <= 0.33)&&(type->getNumWallLevels() == 3))
            return 2;
        else
            return ((ratio <= 0.66)&&(type->getNumWallLevels() > 1));
    } else {
        return (ratio <= 0.5);
    }
}

void Structure::setImageNum(unsigned int num, unsigned char layer)
{
    imagenumbers[layer]=(num)|(p::ppool->getStructpalNum(owner)<<11);
}

// This should be customisable somehow
unsigned int Structure::getExitCell() const
{
    return cellpos+(type->getYsize()*p::ccmap->getWidth());
}

unsigned short Structure::getTargetCell() const
{
    if (attackAnim && target != NULL) {
        return target->getBPos(cellpos);
    }
    return targetcell;
}


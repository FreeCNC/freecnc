#ifndef _GAME_STRUCTURE_H
#define _GAME_STRUCTURE_H

#include "../freecnc.h"
#include "unitorstructure.h"

class INIFile;
class Weapon;

class StructureType : public UnitOrStructureType {
public:
    StructureType(const char* typeName, shared_ptr<INIFile> structini, shared_ptr<INIFile> artini, 
                  const char* thext);
    ~StructureType();
    Uint16 *getSHPNums() {
        return shpnums;
    }
    Uint16 *getSHPTNum() {
        return shptnum;
    }
    const char* getTName() const {
        return tname;
    }
    const char* getName() const {
        return name;
    }
    std::vector<char*> getPrereqs() const {
        return prereqs;
    }
    std::vector<char*> getOwners() const {
        return owners;
    }
    Uint8 getNumLayers() const {
        return numshps;
    }

    Uint16 getMakeImg() const {
        return makeimg;
    }
    bool isWall() const {
        return is_wall;
    }

    Uint8 getXsize() const {
        return xsize;
    }
    Uint8 getYsize() const {
        return ysize;
    }

    Uint8 isBlocked(Uint16 tile) const {
        return blocked[tile];
    }

    Sint8 getXoffset() const {
        return xoffset;
    }
    Sint8 getYoffset() const {
        return yoffset;
    }
    Uint8 getOffset() const {
        return 0;
    }
    Uint16 getCost() const {
        return cost;
    }
    Uint8 getTurnspeed() const {
        return turnspeed;
    }
    Uint8 getSight() const {
        return sight;
    }
    Uint8 getSpeed() const {
        return 0;
    }
    armour_t getArmour() const {
        return armour;
    }
    animinfo_t getAnimInfo() const {
        return animinfo;
    }

    powerinfo_t getPowerInfo() const {
        return powerinfo;
    }
    Uint16 getMaxHealth() const {
        return maxhealth;
    }
    Weapon *getWeapon(bool primary = true) const {
        return (primary?primary_weapon:secondary_weapon);
    }
    bool hasTurret() const {
        return turret;
    }
    Uint16 getBlckOff() const {
        return blckoff;
    }
    bool isInfantry() const {
        return false;
    }
    Uint8 getNumWallLevels() const {
        return numwalllevels;
    }
    Uint8 getDefaultFace() const {
        return defaultface;
    }
    Uint8 getBuildlevel() const {
        return buildlevel;
    }
    Uint8 getTechlevel() const {
        return techlevel;
    }
    bool primarySettable() const {
        return primarysettable;
    }
    Uint8 getPQueue() const {return 0;}
    bool isStructure() const {return true;}
private:
    Uint16* shpnums, *shptnum;
    Uint16 cost,maxhealth,makeimg,blckoff;
    Sint8 xoffset,yoffset;
    armour_t armour;
    Uint8 turnspeed,sight,xsize,ysize,numshps,numwalllevels,defaultface;
    Uint8 techlevel,buildlevel;
    Uint8 *blocked;
    char tname[8];
    char* name;
    std::vector<char*> owners;
    std::vector<char*> prereqs;

    Weapon* primary_weapon;
    Weapon* secondary_weapon;

    animinfo_t animinfo;
    powerinfo_t powerinfo;

    bool is_wall,turret, primarysettable;
};

class BuildingAnimEvent;
class BAttackAnimEvent;

class Structure : public UnitOrStructure
{
public:
    friend class BuildingAnimEvent;
    friend class BAttackAnimEvent;

    Structure(StructureType *type, Uint16 cellpos, Uint8 owner,
            Uint16 rhealth, Uint8 facing);
    ~Structure();
    Uint8 getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets);
    Uint16* getImageNums() const {
        return imagenumbers;
    }
    void changeImage(Uint8 layer, Sint16 imagechange) {
        imagenumbers[layer]+=imagechange;
    }
    Uint32 getImageNum(Uint8 layer) const {
        return type->getSHPNums()[layer]+imagenumbers[layer];
    }
    void setImageNum(Uint32 num, Uint8 layer);
    UnitOrStructureType* getType() {
        return type;
    }
    void setStructnum(Uint32 stn) {
        structnum = stn;
    }
    Uint32 getNum() const {
        return structnum;
    }
    Uint16 getPos() const {
        return cellpos;
    }
    Uint16 getBPos(Uint16 curpos) const;
    Uint16 getFreePos(Uint8* subpos, bool findsubpos);
    void remove();
    Uint16 getSubpos() const {
        return 0;
    }
    void applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker);
    void runAnim(Uint32 mode);
    void runSecAnim(Uint32 param);
    void stopAnim();
    void stop();
    Uint8 getOwner() const {
        return owner;
    }
    void setOwner(Uint8 newowner) {
        owner = newowner;
    }
    bool canAttack() const {
        return type->getWeapon()!=NULL;
    }
    void attack(UnitOrStructure* target);
    Uint16 getHealth() const {
        return health;
    }
    Sint8 getXoffset() const {
        return type->getXoffset();
    }
    Sint8 getYoffset() const {
        return type->getYoffset();
    }
    bool isWall() const {
        return type->isWall();
    }
    double getRatio() const {
        return ratio;
    }
    bool isPrimary() const {
        return primary;
    }
    void setPrimary(bool pri) {
        primary = pri;
    }
    Uint32 getExitCell() const;
    void resetLoadState(bool runsec, Uint32 param);
    Uint8 checkdamage();
    Uint16 getTargetCell() const;
private:
    StructureType *type;
    Uint32 structnum;
    Uint16 *imagenumbers;
    Uint16 cellpos,bcellpos,health;
    Uint8 owner,references,damaged;
    bool animating,usemakeimgs,exploding,primary;
    double ratio; // health/maxhealth

    BuildingAnimEvent* buildAnim;
    BAttackAnimEvent* attackAnim;
};

#include "structureanims.h"

#endif

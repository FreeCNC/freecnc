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
    unsigned short *getSHPNums() {
        return shpnums;
    }
    unsigned short *getSHPTNum() {
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
    unsigned char getNumLayers() const {
        return numshps;
    }

    unsigned short getMakeImg() const {
        return makeimg;
    }
    bool isWall() const {
        return is_wall;
    }

    unsigned char getXsize() const {
        return xsize;
    }
    unsigned char getYsize() const {
        return ysize;
    }

    unsigned char isBlocked(unsigned short tile) const {
        return blocked[tile];
    }

    char getXoffset() const {
        return xoffset;
    }
    char getYoffset() const {
        return yoffset;
    }
    unsigned char getOffset() const {
        return 0;
    }
    unsigned short getCost() const {
        return cost;
    }
    unsigned char getTurnspeed() const {
        return turnspeed;
    }
    unsigned char getSight() const {
        return sight;
    }
    unsigned char getSpeed() const {
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
    unsigned short getMaxHealth() const {
        return maxhealth;
    }
    Weapon *getWeapon(bool primary = true) const {
        return (primary?primary_weapon:secondary_weapon);
    }
    bool hasTurret() const {
        return turret;
    }
    unsigned short getBlckOff() const {
        return blckoff;
    }
    bool isInfantry() const {
        return false;
    }
    unsigned char getNumWallLevels() const {
        return numwalllevels;
    }
    unsigned char getDefaultFace() const {
        return defaultface;
    }
    unsigned char getBuildlevel() const {
        return buildlevel;
    }
    unsigned char getTechlevel() const {
        return techlevel;
    }
    bool primarySettable() const {
        return primarysettable;
    }
    unsigned char getPQueue() const {return 0;}
    bool isStructure() const {return true;}
private:
    unsigned short* shpnums, *shptnum;
    unsigned short cost,maxhealth,makeimg,blckoff;
    char xoffset,yoffset;
    armour_t armour;
    unsigned char turnspeed,sight,xsize,ysize,numshps,numwalllevels,defaultface;
    unsigned char techlevel,buildlevel;
    unsigned char *blocked;
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

    Structure(StructureType *type, unsigned short cellpos, unsigned char owner,
            unsigned short rhealth, unsigned char facing);
    ~Structure();
    unsigned char getImageNums(unsigned int **inums, char **xoffsets, char **yoffsets);
    unsigned short* getImageNums() const {
        return imagenumbers;
    }
    void changeImage(unsigned char layer, short imagechange) {
        imagenumbers[layer]+=imagechange;
    }
    unsigned int getImageNum(unsigned char layer) const {
        return type->getSHPNums()[layer]+imagenumbers[layer];
    }
    void setImageNum(unsigned int num, unsigned char layer);
    UnitOrStructureType* getType() {
        return type;
    }
    void setStructnum(unsigned int stn) {
        structnum = stn;
    }
    unsigned int getNum() const {
        return structnum;
    }
    unsigned short getPos() const {
        return cellpos;
    }
    unsigned short getBPos(unsigned short curpos) const;
    unsigned short getFreePos(unsigned char* subpos, bool findsubpos);
    void remove();
    unsigned short getSubpos() const {
        return 0;
    }
    void applyDamage(short amount, Weapon* weap, UnitOrStructure* attacker);
    void runAnim(unsigned int mode);
    void runSecAnim(unsigned int param);
    void stopAnim();
    void stop();
    unsigned char getOwner() const {
        return owner;
    }
    void setOwner(unsigned char newowner) {
        owner = newowner;
    }
    bool canAttack() const {
        return type->getWeapon()!=NULL;
    }
    void attack(UnitOrStructure* target);
    unsigned short getHealth() const {
        return health;
    }
    char getXoffset() const {
        return type->getXoffset();
    }
    char getYoffset() const {
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
    unsigned int getExitCell() const;
    void resetLoadState(bool runsec, unsigned int param);
    unsigned char checkdamage();
    unsigned short getTargetCell() const;
private:
    StructureType *type;
    unsigned int structnum;
    unsigned short *imagenumbers;
    unsigned short cellpos,bcellpos,health;
    unsigned char owner,references,damaged;
    bool animating,usemakeimgs,exploding,primary;
    double ratio; // health/maxhealth

    BuildingAnimEvent* buildAnim;
    BAttackAnimEvent* attackAnim;
};

#include "structureanims.h"

#endif

#ifndef _GAME_UNIT_H
#define _GAME_UNIT_H

#include <cassert>
#include <stack>

#include "../freecnc.h"
#include "../lib/inifile.h"
#include "unitorstructure.h"
#include "talkback.h"

struct L2Overlay;
class MoveAnimEvent;
class Structure;
class StructureType;
class Talkback;
class TurnAnimEvent;
class UAttackAnimEvent;
class WalkAnimEvent;
class Weapon;

class UnitType : public UnitOrStructureType {
public:
    UnitType(const char *typeName, shared_ptr<INIFile> unitini);
    ~UnitType();
    unsigned int *getSHPNums() {
        return shpnums;
    }
    unsigned char getNumLayers() const {
        return numlayers;
    }
    bool isInfantry() const {
        return is_infantry;
    }
    unsigned char getType() const {
        return unittype;
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
    unsigned char getOffset() const {
        return offset;
    }
    unsigned char getSpeed() const {
        return speed;
    }
    unsigned char getROT() const {
        return turnspeed;
    }
    char getMoveMod() const {
        return movemod;
    }
    unsigned char getTurnMod() const {
        return turnmod;
    }
    unsigned short getMaxHealth() const {
        return maxhealth;
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
    armour_t getArmour() const {
        return armour;
    }
    const char* getRandTalk(TalkbackType type) const;
    Weapon *getWeapon(bool primary = true) const {
        return (primary?primary_weapon:secondary_weapon);
    }
    bool isWall() const {
        return false;
    }
    bool canDeploy() const {
        return deployable;
    }
    const char* getDeployTarget() const {
        return deploytarget;
    }
    StructureType* getDeployType() const {
        return deploytype;
    }
    unsigned char getBuildlevel() const {
        return buildlevel;
    }
    unsigned char getTechlevel() const {
        return techlevel;
    }
    // what colour pip should be displayed for this unit when being carried
    unsigned char getPipColour() const {
        return pipcolour;
    }
    unsigned char getMaxPassengers() const {
        return maxpassengers;
    }
    std::vector<unsigned char> getPassengerAllow() const {
        return passengerAllow;
    }
    std::vector<UnitType*> getSpecificTypeAllow() const {
        return specificTypeAllow;
    }
    unsigned char getPQueue() const {return ptype;}
    bool isStructure() const {return false;}
private:
    unsigned int *shpnums;
    unsigned short cost,maxhealth;
    unsigned char numlayers,speed,turnspeed,turnmod,sight,offset,pipcolour;
    armour_t armour;
    unsigned char techlevel,buildlevel,unittype;
    char movemod;

    char tname[8];
    char* name;
    std::vector<char*> prereqs;
    std::vector<char*> owners;

    // Talkback related members
    shared_ptr<Talkback> talkback;

    bool is_infantry, deployable;
    char* deploytarget;
    // this is used to check the unit can deploy
    StructureType* deploytype;
    unsigned char maxpassengers;
    // matches the unit's type value specified in units.ini
    std::vector<unsigned char> passengerAllow;
    // matches the unit's type name.
    std::vector<UnitType*> specificTypeAllow;

    Weapon* primary_weapon;
    Weapon* secondary_weapon;
};

class Unit : public UnitOrStructure {
public:
    friend class UnitAnimEvent;
    friend class MoveAnimEvent;
    friend class UAttackAnimEvent;
    friend class TurnAnimEvent;
    friend class WalkAnimEvent;
    Unit(UnitType *type, unsigned short cellpos, unsigned char subpos, InfantryGroupPtr group,
            unsigned char owner, unsigned short rhealth, unsigned char facing);
    ~Unit();
    unsigned char getImageNums(unsigned int **inums, char **xoffsets, char **yoffsets);
    InfantryGroupPtr getInfantryGroup() {
        return infgrp;
    }
    void setInfantryGroup(InfantryGroupPtr ig) {
        infgrp = ig;
    }
    unsigned int getImageNum(unsigned char layer) const {
        return type->getSHPNums()[layer]+imagenumbers[layer];
    }
    void setImageNum(unsigned int num, unsigned char layer);
    char getXoffset() const; // return xoffset-type->getOffset();
    char getYoffset() const; // return yoffset-type->getOffset();
    void setXoffset(char xo);
    void setYoffset(char yo);
    UnitOrStructureType* getType() {
        return type;
    }
    unsigned short getPos() const {
        return cellpos;
    }
    unsigned short getBPos(unsigned short pos) const {
        return cellpos;
    }
    unsigned short getSubpos() const {
        return subpos;
    }
    unsigned int getNum() const {
        return unitnum;
    }
    void setUnitnum(unsigned int unum) {
        unitnum = unum;
    }
    unsigned short getHealth() const {
        return health;
    }
    void move(unsigned short dest);
    void move(unsigned short dest, bool stop);
    void attack(UnitOrStructure* target);
    void attack(UnitOrStructure* target, bool stop);
    void turn(unsigned char facing, unsigned char layer);
    void stop();

    unsigned char getOwner() const {
        return owner;
    }
    void setOwner(unsigned char newowner) {
        owner = newowner;
    }
    void remove();
    void applyDamage(short amount, Weapon* weap, UnitOrStructure* attacker);
    bool canAttack() {
        return type->getWeapon()!=NULL;
    }
    void doRandTalk(TalkbackType ttype);
    void deploy(void);
    bool canDeploy();
    bool checkDeployTarget(unsigned int pos);
    unsigned int calcDeployPos() const;
    unsigned int getExitCell() const {
        return calcDeployPos();
    }
    double getRatio() const {
        return ratio;
    }
    unsigned short getDist(unsigned short pos);
    unsigned short getTargetCell();
    enum movement {m_up = 0, m_upright = 1, m_right = 2, m_downright = 3,
        m_down = 4, m_downleft = 5, m_left = 6, m_upleft = 7};
private:
    UnitType *type;
    unsigned int unitnum;
    unsigned short *imagenumbers;
    unsigned short cellpos,health,palettenum;
    unsigned char owner,references,subpos;
    char xoffset,yoffset;
    bool deployed;
    double ratio;

    L2Overlay* l2o;
    std::multimap<unsigned short, L2Overlay*>::iterator l2entry;

    InfantryGroupPtr infgrp;

    shared_ptr<MoveAnimEvent> moveanim;
    shared_ptr<UAttackAnimEvent> attackanim;
    shared_ptr<WalkAnimEvent> walkanim;
    shared_ptr<TurnAnimEvent> turnanim1;
    shared_ptr<TurnAnimEvent> turnanim2;
};

/*
 * This should be a member of unit for infantry. When an infantry unit walks
 * into a previously empty cell a new group is created, otherwise the existing
 * group is used.
 * We need one more bit in the unit/structure matrix to tell if infantry is in
 * that cell.
 * TODO: Implement group reuse, or just scrap this in favour of something that
 * won't cause lots of allocations and deallocations whilst moving infantry.
 */
class InfantryGroup{
public:
    InfantryGroup() : numinfantry(0)
    {
        //logger->debug("Setting up infgroup %p\n", this);
        for (int i=0;i<5;i++) positions[i] = 0;
    }
    ~InfantryGroup()
    {
        //logger->debug("Destructing infgroup %p\n", this);
    }
    bool AddInfantry(Unit* inf, unsigned char subpos)
    {
        assert(subpos < 5);
        assert(numinfantry < 5);
        positions[subpos] = inf;
        ++numinfantry;
        return true;
    }
    bool RemoveInfantry(unsigned char subpos)
    {
        assert(subpos < 5);
        assert(numinfantry > 0);
        positions[subpos] = 0;
        --numinfantry;
        return true;
    }
    bool IsClear(unsigned char subpos)
    {
        assert(subpos < 5);
        return (positions[subpos] == 0);
    }
    unsigned char GetNumInfantry() const
    {
        return numinfantry;
    }
    bool IsAvailable() const
    {
        return (numinfantry < 5);
    }
    unsigned char GetFreePos() const
    {
        for (int i = 0; i < 5; ++i) {
            if (0 == positions[i]) {
                return i;
            }
        }
        return (unsigned char)-1;
    }
    Unit* UnitAt(unsigned char subpos)
    {
        assert(subpos < 5);
        return positions[subpos];
    }

    unsigned char GetImageNums(unsigned int** inums, char** xoffsets, char** yoffsets)
    {
        (*inums) = new unsigned int[numinfantry];
        (*xoffsets) = new char[numinfantry];
        (*yoffsets) = new char[numinfantry];
        int j = 0;
        for (int i = 0; i < 5; ++i) {
            if (0 != positions[i]) {
                (*inums)[j]=positions[i]->getImageNum(0);
                (*xoffsets)[j]=positions[i]->getXoffset()+unitoffsets[i];
                (*yoffsets)[j]=positions[i]->getYoffset()+unitoffsets[i+5];
                j++;
            }
        }
        return numinfantry;
    }
    void GetSubposOffsets(unsigned char oldsp, unsigned char newsp, char* xoffs, char* yoffs)
    {
        *xoffs = unitoffsets[oldsp]-unitoffsets[newsp];
        *yoffs = unitoffsets[oldsp+5]-unitoffsets[newsp+5];
    }
    static const char* GetUnitOffsets()
    {
        return unitoffsets;
    }
    Unit* GetNearest(unsigned char subpos);
private:
    Unit* positions[5];
    unsigned char numinfantry;
    static const char unitoffsets[];
};

inline Unit* InfantryGroup::GetNearest(unsigned char subpos)
{
    static const unsigned char lut[20] = {
        1,2,3,4,
        3,0,2,4,
        1,0,4,3,
        1,0,4,2,
        3,0,2,1
    };
    unsigned char x;
    /* The compiler will optimise this nicely with -funroll-loops,
     * leaving it like this to keep it readable.
     */
    for (x=0;x<4;++x)
        if (0 != positions[lut[x+subpos*4]])
            return positions[lut[x+subpos*4]];
    return 0;
}

#endif

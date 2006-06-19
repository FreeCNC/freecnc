#ifndef _GAME_UNIT_H
#define _GAME_UNIT_H

#include <cassert>
#include <stack>

#include "../freecnc.h"
#include "../lib/inifile.h"
#include "unitorstructure.h"

struct L2Overlay;
class MoveAnimEvent;
class Structure;
class StructureType;
class Talkback;
enum TalkbackType;
class TurnAnimEvent;
class UAttackAnimEvent;
class WalkAnimEvent;
class Weapon;

class UnitType : public UnitOrStructureType {
public:
    UnitType(const char *typeName, shared_ptr<INIFile> unitini);
    ~UnitType();
    Uint32 *getSHPNums() {
        return shpnums;
    }
    Uint8 getNumLayers() const {
        return numlayers;
    }
    bool isInfantry() const {
        return is_infantry;
    }
    Uint8 getType() const {
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
    Uint8 getOffset() const {
        return offset;
    }
    Uint8 getSpeed() const {
        return speed;
    }
    Uint8 getROT() const {
        return turnspeed;
    }
    Sint8 getMoveMod() const {
        return movemod;
    }
    Uint8 getTurnMod() const {
        return turnmod;
    }
    Uint16 getMaxHealth() const {
        return maxhealth;
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
    armour_t getArmour() const {
        return armour;
    }
#ifdef LOOPEND_TURN
    animinfo_t getAnimInfo() const {
        return animinfo;
    }
#endif
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
    Uint8 getBuildlevel() const {
        return buildlevel;
    }
    Uint8 getTechlevel() const {
        return techlevel;
    }
    // what colour pip should be displayed for this unit when being carried
    Uint8 getPipColour() const {
        return pipcolour;
    }
    Uint8 getMaxPassengers() const {
        return maxpassengers;
    }
    std::vector<Uint8> getPassengerAllow() const {
        return passengerAllow;
    }
    std::vector<UnitType*> getSpecificTypeAllow() const {
        return specificTypeAllow;
    }
    Uint8 getPQueue() const {return ptype;}
    bool isStructure() const {return false;}
private:
    Uint32 *shpnums;
    Uint16 cost,maxhealth;
    Uint8 numlayers,speed,turnspeed,turnmod,sight,offset,pipcolour;
    armour_t armour;
#ifdef LOOPEND_TURN
    animinfo_t animinfo;
#endif
    Uint8 techlevel,buildlevel,unittype;
    Sint8 movemod;

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
    Uint8 maxpassengers;
    // matches the unit's type value specified in units.ini
    std::vector<Uint8> passengerAllow;
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
    Unit(UnitType *type, Uint16 cellpos, Uint8 subpos, InfantryGroupPtr group,
            Uint8 owner, Uint16 rhealth, Uint8 facing);
    ~Unit();
    Uint8 getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets);
    InfantryGroupPtr getInfantryGroup() {
        return infgrp;
    }
    void setInfantryGroup(InfantryGroupPtr ig) {
        infgrp = ig;
    }
    Uint32 getImageNum(Uint8 layer) const {
        return type->getSHPNums()[layer]+imagenumbers[layer];
    }
    void setImageNum(Uint32 num, Uint8 layer);
    Sint8 getXoffset() const; // return xoffset-type->getOffset();
    Sint8 getYoffset() const; // return yoffset-type->getOffset();
    void setXoffset(Sint8 xo);
    void setYoffset(Sint8 yo);
    UnitOrStructureType* getType() {
        return type;
    }
    Uint16 getPos() const {
        return cellpos;
    }
    Uint16 getBPos(Uint16 pos) const {
        return cellpos;
    }
    Uint16 getSubpos() const {
        return subpos;
    }
    Uint32 getNum() const {
        return unitnum;
    }
    void setUnitnum(Uint32 unum) {
        unitnum = unum;
    }
    Uint16 getHealth() const {
        return health;
    }
    void move(Uint16 dest);
    void move(Uint16 dest, bool stop);
    void attack(UnitOrStructure* target);
    void attack(UnitOrStructure* target, bool stop);
    void turn(Uint8 facing, Uint8 layer);
    void stop();

    Uint8 getOwner() const {
        return owner;
    }
    void setOwner(Uint8 newowner) {
        owner = newowner;
    }
    void remove();
    void applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker);
    bool canAttack() {
        return type->getWeapon()!=NULL;
    }
    void doRandTalk(TalkbackType ttype);
    void deploy(void);
    bool canDeploy();
    bool checkDeployTarget(Uint32 pos);
    Uint32 calcDeployPos() const;
    Uint32 getExitCell() const {
        return calcDeployPos();
    }
    double getRatio() const {
        return ratio;
    }
    Uint16 getDist(Uint16 pos);
    Uint16 getTargetCell();
    enum movement {m_up = 0, m_upright = 1, m_right = 2, m_downright = 3,
        m_down = 4, m_downleft = 5, m_left = 6, m_upleft = 7};
private:
    UnitType *type;
    Uint32 unitnum;
    Uint16 *imagenumbers;
    Uint16 cellpos,health,palettenum;
    Uint8 owner,references,subpos;
    Sint8 xoffset,yoffset;
    bool deployed;
    double ratio;

    L2Overlay* l2o;
    std::multimap<Uint16, L2Overlay*>::iterator l2entry;

    InfantryGroupPtr infgrp;

    MoveAnimEvent *moveanim;
    UAttackAnimEvent *attackanim;
    WalkAnimEvent *walkanim;
    TurnAnimEvent *turnanim1;
    TurnAnimEvent *turnanim2;
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
    bool AddInfantry(Unit* inf, Uint8 subpos)
    {
        assert(subpos < 5);
        assert(numinfantry < 5);
        positions[subpos] = inf;
        ++numinfantry;
        return true;
    }
    bool RemoveInfantry(Uint8 subpos)
    {
        assert(subpos < 5);
        assert(numinfantry > 0);
        positions[subpos] = 0;
        --numinfantry;
        return true;
    }
    bool IsClear(Uint8 subpos)
    {
        assert(subpos < 5);
        return (positions[subpos] == 0);
    }
    Uint8 GetNumInfantry() const
    {
        return numinfantry;
    }
    bool IsAvailable() const
    {
        return (numinfantry < 5);
    }
    Uint8 GetFreePos() const
    {
        for (int i = 0; i < 5; ++i) {
            if (0 == positions[i]) {
                return i;
            }
        }
        return (Uint8)-1;
    }
    Unit* UnitAt(Uint8 subpos)
    {
        assert(subpos < 5);
        return positions[subpos];
    }

    Uint8 GetImageNums(Uint32** inums, Sint8** xoffsets, Sint8** yoffsets)
    {
        (*inums) = new Uint32[numinfantry];
        (*xoffsets) = new Sint8[numinfantry];
        (*yoffsets) = new Sint8[numinfantry];
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
    void GetSubposOffsets(Uint8 oldsp, Uint8 newsp, Sint8* xoffs, Sint8* yoffs)
    {
        *xoffs = unitoffsets[oldsp]-unitoffsets[newsp];
        *yoffs = unitoffsets[oldsp+5]-unitoffsets[newsp+5];
    }
    static const Sint8* GetUnitOffsets()
    {
        return unitoffsets;
    }
    Unit* GetNearest(Uint8 subpos);
private:
    Unit* positions[5];
    Uint8 numinfantry;
    static const Sint8 unitoffsets[];
};

inline Unit* InfantryGroup::GetNearest(Uint8 subpos)
{
    static const Uint8 lut[20] = {
        1,2,3,4,
        3,0,2,4,
        1,0,4,3,
        1,0,4,2,
        3,0,2,1
    };
    Uint8 x;
    /* The compiler will optimise this nicely with -funroll-loops,
     * leaving it like this to keep it readable.
     */
    for (x=0;x<4;++x)
        if (0 != positions[lut[x+subpos*4]])
            return positions[lut[x+subpos*4]];
    return 0;
}

#endif

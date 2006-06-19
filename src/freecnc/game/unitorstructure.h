#ifndef _GAME_UNITORSTRUCTURE_H
#define _GAME_UNITORSTRUCTURE_H

#include <cassert>
#include <stack>
#include "../freecnc.h"

class Weapon;

/**
 * UnitOrStructureType is used when you can't or don't need to know
 * whether you are dealing with a unit type or a structure type.  Note
 * that this class is abstract, it is only used for performing casts.
 */
class UnitOrStructureType
{
public:
    UnitOrStructureType() : ptype(0), valid(false) {}

    virtual ~UnitOrStructureType() {};

    /// Returns the cost in credits.
    virtual Uint16 getCost() const = 0;

    /// Speed is measured in artitrary units
    virtual Uint8 getSpeed() const = 0;

    /// Turn speed is measured in arbitrary units
    virtual Uint8 getTurnspeed() const = 0;

    /// Sight is measured in cells
    virtual Uint8 getSight() const = 0;

    /// Returns the maximum health for this type
    virtual Uint16 getMaxHealth() const = 0;

    /** @brief Returns a number corresponding to the type's armour
     * class. See freecnc.h for the enum definition
     */
    virtual armour_t getArmour() const = 0;

    /** @brief Returns number of layers to render, 1 or 2.  The only TD
     * structure that returns 2 is the weapons factory.  Units with
     * turrets (tanks, humvee, buggy, missile launchers) return 2.
     */
    virtual Uint8 getNumLayers() const = 0;

    /**
     * Units and structures can have at most two weapons.
     * Currently any secondary weapons are ignored.
     *
     * @todo Write a version that accepts an armour type and returns
     * the weapon that'll cause the most damage.
     */
    virtual Weapon* getWeapon(bool primary) const = 0;

    /// Only applicable to units.  StructureType always returns false.
    virtual bool isInfantry() const = 0;

    /// Only applicable to structures.  UnitType always returns false.
    virtual bool isWall() const = 0;

    /// Only applicable to units.  StructureType always returns zero.
    virtual Uint8 getOffset() const = 0;

    /// @returns the internal name, e.g. E1
    virtual const char* getTName() const = 0;

    /// @returns the external name, e.g. Minigunner
    virtual const char* getName() const = 0;

    /// @returns the prerequisites
    virtual std::vector<char*> getPrereqs() const = 0;

    /// @returns the level the player has to have reached before can be built
    virtual Uint8 getBuildlevel() const = 0;

    /// Earliest level this unit or structure appears
    virtual Uint8 getTechlevel() const = 0;

    /// @returns the names of the sides that can build this
    virtual std::vector<char*> getOwners() const = 0;

    /// @returns whether the type is valid or not (loaded fully)
    virtual bool isValid() const {return valid;}

    /// @returns which production queue the type is for
    virtual Uint8 getPQueue() const = 0;

    /// @returns the production type of this type.
    Uint8 getPType() const {return ptype;}
    void setPType(Uint8 p) {ptype = p;}

    // Calling a virtual function is much faster than a dynamic_cast
    virtual bool isStructure() const = 0;

protected:
    Uint8 ptype;
    bool valid;
};

/**
 * Like UnitOrStructureType, UnitOrStructure is a way of referring to
 * either units or structures (but only when it is either not possible
 * or not necessary to know what it actually is).  This class is also
 * abstract.  Since both classes are abstract, care must be taken to
 * ensure one does not call a pure virtual method by mistake.
 */

class UnitOrStructure
{
public:
    UnitOrStructure();

    virtual ~UnitOrStructure();

    virtual Uint32 getNum() const = 0;

    virtual Sint8 getXoffset() const = 0;

    virtual Sint8 getYoffset() const = 0;

    virtual void setXoffset(Sint8 xo) {};

    virtual void setYoffset(Sint8 yo) {};

    virtual Uint16 getHealth() const = 0;

    virtual Uint8 getOwner() const = 0;

    virtual void setOwner(Uint8) = 0;

    virtual Uint16 getPos() const = 0;

    virtual Uint16 getSubpos() const = 0;

    /// get the first blocked cell in structure.
    /**
     * Normally the first blocked cell is the top-left most cell.
     * However, certain structures, such as the repair bay, do not have
     * this cell blocked, so the original targetting code could not hit
     * that structure.  There is no overhead in calculating the first
     * blocked cell as it is done at the same time as the blocked matrix
     * is first created.
     */
    virtual Uint16 getBPos(Uint16 pos) const = 0;

    /**
     * When calling this function, one must be explicit in your casting
     * for MSVC's sake (it isn't an issue with gcc).
     */
    virtual UnitOrStructureType* getType() = 0;

    virtual Uint32 getImageNum(Uint8 layer) const = 0;

    virtual void setImageNum(Uint32 num, Uint8 layer) = 0;

    void referTo() {++references;}

    void unrefer();

    void remove();

    bool isAlive() {return !deleted;}

    void select() {selected = true;  showorder_timer = 0;}

    void unSelect() {selected = false; showorder_timer = 0;}

    bool isSelected() {return selected;}

    virtual void attack(UnitOrStructure* target) = 0;

    virtual void applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker) = 0;

    /// @returns ratio of actual health over maximum health for type.
    virtual double getRatio() const = 0;

    virtual Uint32 getExitCell() const {return 0;}

    virtual Uint16 getTargetCell() const {return targetcell;}

    virtual UnitOrStructure* getTarget() {return target;}

protected:
    /* using protected data members to make the code cleaner:
     * referTo, unrefer, select and unselect are common to both Units
     * and structures, so rather than duplicate code, we handle them
     * here.
     */
    Uint8 references;
    bool deleted, selected;
    Uint16 targetcell;
    UnitOrStructure* target;
    Uint8 showorder_timer;
};

inline UnitOrStructure::UnitOrStructure() :
    references(0), deleted(false), selected(false), targetcell(0),
    target(NULL), showorder_timer(0)
{}

inline UnitOrStructure::~UnitOrStructure()
{
    assert(references == 0);
}

inline void UnitOrStructure::unrefer()
{
    assert(references > 0);
    --references;
    if (deleted && references == 0) {
        delete this;
    }
}

inline void UnitOrStructure::remove()
{
    deleted = true;
    if (references == 0) {
        delete this;
    }
}


#endif /* UNITORSTRUCTURE_H */

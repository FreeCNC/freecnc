#ifndef _GAME_STRUCTUREANIMS_H
#define _GAME_STRUCTUREANIMS_H

#include "../freecnc.h"
#include "actioneventqueue.h"

class Structure;
class StructureType;
class UnitOrStructure;

/** \author Euan MacGregor, 2001
 *
 * Like the rest of the project, this code is licensed under the GPL.
 *
 * Please direct your bug reports to the Sourceforge bug tracker.
 * Set the category to "structures"
 */

/**
 * Structure animation introduction
 */

/**
 * Rather than have each derived class be a friend of the Structure
 * class, all changes to the structure are made in the BuildingAnimEvent::run
 * function, based on information stored in this struct (passed to the derived
 * class's anim_func function)
 */
struct anim_nfo {
    /// layer zero of the structure will be set to this value
    Uint16 frame0;
    /// layer one (if it exists) will be set to this value
    Uint16 frame1;
    /// offset in the loop animation for layer zero if structure is damaged
    Uint16 damagedelta;
    /// offset in the loop animation for layer one if structure is damaged
    Uint16 damagedelta2;
    /// has the animation finished
    bool done;
    /// is the structure damaged
    bool damaged;
    /// identifying constant for the animation type
    Uint8 mode;
};

class BAttackAnimEvent;

/// This is the base class for all structure animations
class BuildingAnimEvent : public ActionEvent {
public:
    /**
     * @param p the priority of the event
     * @param str the structure to which the animation is to be applied
     * @param mode a numberic constant that is unique for each 
     *        structure animation event.  Used for chaining events
     */
    BuildingAnimEvent(Uint32 p, Structure* str, Uint8 mode);
    /// cleans up, checks for a scheduled event and runs it if there is
    virtual ~BuildingAnimEvent();
    /// Passes control over to the anim_func (defined in derived classes)
    virtual void run();
    /// Cleanly terminates the animation
    /**
     * Structure animations should always be stopped using this function
     * as it ensures that any scheduled animations are run.
     * (Although you may have reasons not to, such as to avoid this side-effect)
     */
    virtual void stop()
    {
        anim_data.done = true;
    }
    /// checks whether the structure has been critically damaged
    virtual void updateDamaged();
    /** \note
     * Brief note to explain my thinking behind this:
     * Originally, (until I knew better) the code used protected members
     * to pass data between the generic building animation class and the
     * actual animation classes.  This was changed in favour of passing
     * a pointer to a struct from the run function to be modified by the
     * derived class.
     *
     * \par
     * Some classes needed their own updateDamaged function, which either
     * needs to update the anim_data structure or read from it.
     *
     * \par
     * Looking through those functions, at a later date from when I first
     * wrote this code, I think I can resolve this issue nicely.
     */

    //@{
    /// if a class overrides the updateDamaged method it should be a friend
    friend class RefineAnimEvent;
    friend class ProcAnimEvent;
    friend class DoorAnimEvent;
    friend class BAttackAnimEvent;
    //@}

    //@{
    /// Sets the next animation to run when the current animation finishes
    /**
     * Since the code for the AttackAnimEvent is different to the others
     * there needs to be seperate code to handle scheduling Attack events
     */
    void setSchedule(BAttackAnimEvent* ea,bool attack) {
        if (attack) {
            this->ea = ea;
        } else {
            this->e = (BuildingAnimEvent*)ea;
        }
        toAttack = attack;
    }
    void setSchedule(BuildingAnimEvent* e) {
        this->e = e;
    }
    //@}
    virtual void update() {}
private:
    Structure* strct;
    anim_nfo anim_data;
    bool layer2,toAttack;
    BuildingAnimEvent* e;
    BAttackAnimEvent* ea;
protected:
    /// all derived classes must define this function
    virtual void anim_func(anim_nfo* data) = 0;
    /// retrieve some constant data from the structure
    animinfo_t getaniminfo() {
        return ((StructureType *)strct->getType())->getAnimInfo();
    }
    /// this is needed as strct->getType() returns UnitOrStructureType*
    StructureType* getType() {
        return strct->type;
    }
};


/// The animation that is shown when a structure is either built or sold
class BuildAnimEvent : public BuildingAnimEvent {
public:
    /**
     * @param p the priority of this event
     * @param str pointer to the structure being built/sold
     * @param sell whether the structure is being built or sold (true if sold)
     */
    BuildAnimEvent(Uint32 p, Structure* str, bool sell);
    ~BuildAnimEvent();
    void anim_func(anim_nfo* data);
private:
    Uint8 frame,framend;
    bool sell;
};

/// The animation that turns structures to face a given direction.
/// This is only used when attacking.
class BTurnAnimEvent : public BuildingAnimEvent {
public:
    /**
     * @param p the priority of this event
     * @param str the structure to which this animation is to be applied
     * @param face the direction the structure is to face
     */
    BTurnAnimEvent(Uint32 p, Structure* str, Uint8 face);
    void anim_func(anim_nfo* data);
private:
    Uint8 frame,targetface;
    Sint8 turnmod;
    Structure* str;
};

/// Simple animation that linearly increases the frame number
/// between two given limits
class LoopAnimEvent : public BuildingAnimEvent {
public:
    /**
     * @param p the priority of this event
     * @param str the structure to which this animation is to be applied
     */
    LoopAnimEvent(Uint32 p, Structure* str);
    void anim_func(anim_nfo* data);
private:
    Uint8 frame, framend;
};

/// Animation used by the Weapons Factory to animate the door opening
/// and closing
class DoorAnimEvent : public BuildingAnimEvent {
public:
    /**
     * @param p the priority of this event
     * @param str the structure to which this animation is to be applied
     * @param opening whether the door is opening or closing
     */
    DoorAnimEvent(Uint32 p, Structure* str, bool opening);
    void anim_func(anim_nfo* data);
    void updateDamaged();
private:
    Uint8 frame,framend,framestart,frame0;
    bool opening;
};

/// Modifed LoopAnimEvent to account for when the damaged frames
/// do not concurrently follow the normal frames
class ProcAnimEvent : public BuildingAnimEvent {
public:
    ProcAnimEvent(Uint32 p, Structure* str);
    void anim_func(anim_nfo* data);
    void updateDamaged();
private:
    Uint8 frame,framend;
};

/// Animation depicting the refinery processing tiberium
class RefineAnimEvent : public BuildingAnimEvent {
public:
    RefineAnimEvent(Uint32 p, Structure* str, Uint8 bails);
    void anim_func(anim_nfo* data);
    void updateDamaged();
private:
    Structure* str;
    Uint8 framestart,frame,framend;
    Uint8 bails;
};

/// Defines the attack logic
/**
 * This animation is different to the others as it overrides
 * the run function rather than the anim_func function.
 */
class BAttackAnimEvent : public BuildingAnimEvent {
public:
    /**
     * @param p the priority of this event
     * @param str the attacking structure
     * @param target the unit or structure to be attacked
     */
    BAttackAnimEvent(Uint32 p, Structure* str);
    ~BAttackAnimEvent();
    void run();
    void stop();
    void anim_func(anim_nfo* data) {}
    void update();
private:
    Uint8 frame;
    Structure* strct;
    UnitOrStructure* target;
    bool done;
};

/// Animation used when a building explodes
/**
 * This animation updates the UnitAndStructurePool to remove
 * the destroyed structure upon finishing.  This class also
 * overrides the run function (but calls the BuildingAnimEvent::run 
 * function)
 * \see BuildingAnimEvent
 */
class BExplodeAnimEvent : public BuildingAnimEvent {
public:
    /**
     * @param p the priority of this event
     * @param str the structure that has just been destroyed
     */
    BExplodeAnimEvent(Uint32 p, Structure* str);

    /// Updates the UnitAndStructurePool
    /// @todo spawn survivors
    /// @todo spawn flame objects
    ~BExplodeAnimEvent();
    virtual void run();
private:
    Structure* strct;
    Uint16 lastframe, counter;
    virtual void anim_func(anim_nfo* data);
};

#endif /* STRUCTUREANIMS_H */

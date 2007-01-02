#ifndef _GAME_STRUCTUREANIMS_H
#define _GAME_STRUCTUREANIMS_H

#include "actioneventqueue.h"

class Structure;
class StructureType;
class UnitOrStructure;

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
    unsigned short frame0;
    /// layer one (if it exists) will be set to this value
    unsigned short frame1;
    /// offset in the loop animation for layer zero if structure is damaged
    unsigned short damagedelta;
    /// offset in the loop animation for layer one if structure is damaged
    unsigned short damagedelta2;
    /// has the animation finished
    bool done;
    /// is the structure damaged
    bool damaged;
    /// identifying constant for the animation type
    unsigned char mode;
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
    BuildingAnimEvent(unsigned int p, Structure* str, unsigned char mode);


    virtual ~BuildingAnimEvent();

    /// Passes control over to the anim_func (defined in derived classes)
    virtual bool run();

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

    /// if a class overrides the updateDamaged method it should be a friend
    friend class RefineAnimEvent;
    friend class ProcAnimEvent;
    friend class DoorAnimEvent;
    friend class BAttackAnimEvent;

    void setSchedule(shared_ptr<BAttackAnimEvent> new_attack) {
        next_attack_event = new_attack;
    }

    void setSchedule(shared_ptr<BuildingAnimEvent> event) {
        next_anim = event;
    }

    void finish();

    virtual void update() {}
private:
    Structure* strct;
    anim_nfo anim_data;
    bool layer2;
    shared_ptr<BuildingAnimEvent> next_anim;
    shared_ptr<BAttackAnimEvent> next_attack_event;
protected:
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
    BuildAnimEvent(unsigned int p, Structure* str, bool sell);
    void anim_func(anim_nfo* data);
private:
    unsigned char frame,framend;
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
    BTurnAnimEvent(unsigned int p, Structure* str, unsigned char face);
    void anim_func(anim_nfo* data);
private:
    unsigned char frame,targetface;
    char turnmod;
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
    LoopAnimEvent(unsigned int p, Structure* str);
    void anim_func(anim_nfo* data);
private:
    unsigned char frame, framend;
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
    DoorAnimEvent(unsigned int p, Structure* str, bool opening);
    void anim_func(anim_nfo* data);
    void updateDamaged();
private:
    unsigned char frame,framend,framestart,frame0;
    bool opening;
};

/// Modifed LoopAnimEvent to account for when the damaged frames
/// do not concurrently follow the normal frames
class ProcAnimEvent : public BuildingAnimEvent {
public:
    ProcAnimEvent(unsigned int p, Structure* str);
    void anim_func(anim_nfo* data);
    void updateDamaged();
private:
    unsigned char frame,framend;
};

/// Animation depicting the refinery processing tiberium
class RefineAnimEvent : public BuildingAnimEvent {
public:
    RefineAnimEvent(unsigned int p, Structure* str, unsigned char bails);
    void anim_func(anim_nfo* data);
    void updateDamaged();
private:
    Structure* str;
    unsigned char framestart,frame,framend;
    unsigned char bails;
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
    BAttackAnimEvent(unsigned int p, Structure* str);
    bool run();
    void stop();
    void anim_func(anim_nfo* data) {}
    void update();
    void finish();
private:
    unsigned char frame;
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
    BExplodeAnimEvent(unsigned int p, Structure* str);

    /// Updates the UnitAndStructurePool
    /// @todo spawn survivors
    /// @todo spawn flame objects
    ~BExplodeAnimEvent();
    bool run();
private:
    Structure* strct;
    unsigned short lastframe, counter;
    void anim_func(anim_nfo* data);
};

#endif /* STRUCTUREANIMS_H */

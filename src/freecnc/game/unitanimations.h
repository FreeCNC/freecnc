#ifndef _GAME_UNITANIMATIONS_H
#define _GAME_UNITANIMATIONS_H

#include "../freecnc.h"
#include "actioneventqueue.h"

class Path;
class Unit;
class UnitOrStructure;

class UnitAnimEvent : public ActionEvent
{
public:
    UnitAnimEvent(Uint32 p, Unit* un);
    virtual ~UnitAnimEvent();
    void setSchedule(UnitAnimEvent* e);
    void stopScheduled();
    virtual void stop() = 0;
    virtual void update() {}
    virtual void run() = 0;

private:
    Unit* un;
    UnitAnimEvent* scheduled;
};

class MoveAnimEvent : public UnitAnimEvent
{
public:
    MoveAnimEvent(Uint32 p, Unit* un);
    virtual ~MoveAnimEvent();
    virtual void stop();
    virtual void run();
    virtual void update();
    virtual void setRange(Uint32 nr) {range = nr;}

private:
    bool stopping;
    void startMoveOne(bool wasblocked);
    void moveDone();
    Uint16 dest,newpos;
    bool blocked, moved_half, pathinvalid, waiting;
    Sint8 xmod, ymod;
    Unit* un;
    Path* path;
    Uint8 istep,dir;
    Uint32 range;
};

class WalkAnimEvent : public UnitAnimEvent {
public:
    WalkAnimEvent(Uint32 p, Unit* un, Uint8 dir, Uint8 layer);
    virtual ~WalkAnimEvent();
    virtual void stop() {stopping = true;}
    virtual void run();
    virtual void changedir(Uint8 ndir) {
        stopping = false;
        dir = ndir;
        calcbaseimage();
    }
    void update() {}

private:
    bool stopping;
    void calcbaseimage(void);
    Unit* un;
    Uint8 dir, istep, layer, baseimage;
};

class TurnAnimEvent; // Cyclic dependency

class TurnAnimEvent : public UnitAnimEvent
{
public:
    TurnAnimEvent(Uint32 p, Unit *un, Uint8 dir, Uint8 layer);
    virtual ~TurnAnimEvent();
    virtual void run();
    virtual void stop() {stopping = true;}
    void update() {}
    virtual void changedir(Uint8 ndir) {
        stopping = false;
        dir = ndir;
    }

private:
    bool stopping,runonce;
    Sint8 turnmod;
    Unit *un;
    Uint8 dir;
    Uint8 layer;
};

class UAttackAnimEvent : public UnitAnimEvent
{
public:
    UAttackAnimEvent(Uint32 p, Unit *un);
    virtual ~UAttackAnimEvent();
    void stop();
    virtual void update();
    virtual void run();

private:
    Unit *un;
    bool stopping;
    Uint8 waiting;
    UnitOrStructure* target;
};

#endif

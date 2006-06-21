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
    UnitAnimEvent(unsigned int p, Unit* un);
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
    MoveAnimEvent(unsigned int p, Unit* un);
    virtual ~MoveAnimEvent();
    virtual void stop();
    virtual void run();
    virtual void update();
    virtual void setRange(unsigned int nr) {range = nr;}

private:
    bool stopping;
    void startMoveOne(bool wasblocked);
    void moveDone();
    unsigned short dest,newpos;
    bool blocked, moved_half, pathinvalid, waiting;
    char xmod, ymod;
    Unit* un;
    Path* path;
    unsigned char istep,dir;
    unsigned int range;
};

class WalkAnimEvent : public UnitAnimEvent {
public:
    WalkAnimEvent(unsigned int p, Unit* un, unsigned char dir, unsigned char layer);
    virtual ~WalkAnimEvent();
    virtual void stop() {stopping = true;}
    virtual void run();
    virtual void changedir(unsigned char ndir) {
        stopping = false;
        dir = ndir;
        calcbaseimage();
    }
    void update() {}

private:
    bool stopping;
    void calcbaseimage(void);
    Unit* un;
    unsigned char dir, istep, layer, baseimage;
};

class TurnAnimEvent; // Cyclic dependency

class TurnAnimEvent : public UnitAnimEvent
{
public:
    TurnAnimEvent(unsigned int p, Unit *un, unsigned char dir, unsigned char layer);
    virtual ~TurnAnimEvent();
    virtual void run();
    virtual void stop() {stopping = true;}
    void update() {}
    virtual void changedir(unsigned char ndir) {
        stopping = false;
        dir = ndir;
    }

private:
    bool stopping,runonce;
    char turnmod;
    Unit *un;
    unsigned char dir;
    unsigned char layer;
};

class UAttackAnimEvent : public UnitAnimEvent
{
public:
    UAttackAnimEvent(unsigned int p, Unit *un);
    virtual ~UAttackAnimEvent();
    void stop();
    virtual void update();
    virtual void run();

private:
    Unit *un;
    bool stopping;
    unsigned char waiting;
    UnitOrStructure* target;
};

#endif

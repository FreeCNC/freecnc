#ifndef _GAME_UNITANIMATIONS_H
#define _GAME_UNITANIMATIONS_H

#include "actioneventqueue.h"

class Path;
class Unit;
class UnitOrStructure;

class UnitAnimEvent : public ActionEvent
{
public:
    UnitAnimEvent(unsigned int p, Unit* un);
    virtual ~UnitAnimEvent();
    void setSchedule();
    void setSchedule(shared_ptr<UnitAnimEvent> e);
    virtual void finish();
    virtual void stop() = 0;
    virtual void update() {}
    virtual bool run() = 0;

private:
    Unit* un;
    shared_ptr<UnitAnimEvent> scheduled;
};

class MoveAnimEvent : public UnitAnimEvent
{
public:
    MoveAnimEvent(unsigned int p, Unit* un);
    ~MoveAnimEvent();
    void finish();
    void stop();
    bool run();
    void update();
    void setRange(unsigned int nr) {range = nr;}

private:
    bool stopping;
    bool startMoveOne(bool wasblocked);
    bool moveDone();
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
    void stop() {stopping = true;}
    bool run();
    void finish();
    void changedir(unsigned char ndir) {
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
    bool run();
    void finish();
    void stop() {stopping = true;}
    void update() {}
    void changedir(unsigned char ndir) {
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
    ~UAttackAnimEvent();
    void finish();
    void stop();
    void update();
    bool run();

private:
    Unit *un;
    bool stopping;
    unsigned char waiting;
    UnitOrStructure* target;
};

#endif

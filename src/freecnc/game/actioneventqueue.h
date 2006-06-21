#ifndef _GAME_ACTIONEVENTQUEUE_H
#define _GAME_ACTIONEVENTQUEUE_H

#include <queue>
#include "../freecnc.h"

/** An abstract class which all actionevents must extend. the run must
 * be implemented. */
class ActionEvent
{
public:
    friend class Comp;
    ActionEvent( unsigned int p )
    {
        delay = p;
    }
    void addCurtick( unsigned int curtick )
    {
        prio=delay+curtick;
    }
    virtual void run()
    {}

    void setDelay(unsigned int p)
    {
        delay = p;
    }
    unsigned int getPrio()
    {
        return prio;
    }
    virtual ~ActionEvent()
    {}
    virtual void stop()
    {}
private:
    unsigned int prio, delay;
};

// Friend class which compares ActionEvents priority
class Comp
{
public:
    bool operator()(ActionEvent *x, ActionEvent *y)
    {
        return x->prio > y->prio;
    }
};

class ActionEventQueue
{
public:
    ActionEventQueue();
    ~ActionEventQueue();
    void scheduleEvent(ActionEvent *ev);
    void runEvents();
    unsigned int getElapsedTime();
    unsigned int getCurtick();
private:
    unsigned int starttick;
    std::priority_queue<ActionEvent*, std::vector<ActionEvent*>, Comp> eventqueue;
};

#endif

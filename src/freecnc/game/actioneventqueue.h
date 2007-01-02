#ifndef _GAME_ACTIONEVENTQUEUE_H
#define _GAME_ACTIONEVENTQUEUE_H

#include <queue>
#include "../basictypes.h"

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
        prio = delay + curtick;
    }
    virtual bool run() { return false; }
    virtual void finish () {}

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
    bool operator()(shared_ptr<ActionEvent> x, shared_ptr<ActionEvent> y)
    {
        // The lower the prio, the sooner it runs:
        // priority_queue comp wants to know if x is "less" than y, and puts
        // the highest priorities at the top, so we inverse the comp.
        return y->prio < x->prio;
    }
};

class ActionEventQueue
{
public:
    ActionEventQueue();
    void scheduleEvent(shared_ptr<ActionEvent> ev);
    void runEvents();
    unsigned int getElapsedTime();
    unsigned int getCurtick();
private:
    unsigned int starttick;
    std::priority_queue<shared_ptr<ActionEvent>,
        vector<shared_ptr<ActionEvent> >, Comp> eventqueue;
};

#endif

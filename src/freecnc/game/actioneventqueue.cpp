#include "SDL.h"
#include "actioneventqueue.h"

/** Constructor, starts the timer */
ActionEventQueue::ActionEventQueue()
{
    starttick = SDL_GetTicks();
}

/** scedules  event for later ececution.
 * @param a class containing the action to run.
 */
void ActionEventQueue::scheduleEvent(shared_ptr<ActionEvent> ev)
{
    ev->addCurtick(getCurtick());
    eventqueue.push(ev);
}

/** Run all events in the actionqueue. */
void ActionEventQueue::runEvents()
{
    unsigned int curtick = getCurtick();

    // run all events in the queue with a prio lower than curtick
    while (!eventqueue.empty() && eventqueue.top()->getPrio() <= curtick) {
        shared_ptr<ActionEvent> ev = eventqueue.top();
        eventqueue.pop();
        if (ev->run()) {
            scheduleEvent(ev);
        } else {
            ev->finish();
        }
    }
}

unsigned int ActionEventQueue::getElapsedTime()
{
    return SDL_GetTicks()-starttick;
}

unsigned int ActionEventQueue::getCurtick()
{
    return (SDL_GetTicks()-starttick) >> 5;
}


//
// BuildQueue - Manages the players unit & structure build queue
//
#ifndef _GAME_BUILDQUEUE_H
#define _GAME_BUILDQUEUE_H

#include <algorithm>
#include <list>
#include "../freecnc.h"

class Player;
class UnitOrStructureType;

namespace BuildQueue
{
    class BQTimer;

    typedef std::map<const UnitOrStructureType*, Uint8> Production;
    typedef std::list<const UnitOrStructureType*> Queue;

    class BQueue
    {
    public:
        BQueue(Player* p);
        ~BQueue();

        bool Add(const UnitOrStructureType* type);
        ConStatus PauseCancel(const UnitOrStructureType* type);

        ConStatus getStatus(const UnitOrStructureType* type, Uint8* quantity, Uint8* progress) const;

        void Placed();

        const UnitOrStructureType* getCurrentType() const {return *queue.begin();}

    private:
        void next();
        enum RQstate {RQ_DONE, RQ_NEW, RQ_MAXED};
        RQstate requeue(const UnitOrStructureType* type);

        Player *player;
        Uint32 last, left;
        ConStatus status;

        BQTimer* timer;
        bool tick();
        void rescheduled();

        Production production;
        Queue queue;
    };
}

#endif

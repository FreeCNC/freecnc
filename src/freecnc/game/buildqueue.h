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

    typedef std::map<const UnitOrStructureType*, unsigned char> Production;
    typedef std::list<const UnitOrStructureType*> Queue;

    class BQueue
    {
    public:
        BQueue(Player* p);

        bool Add(const UnitOrStructureType* type);
        ConStatus PauseCancel(const UnitOrStructureType* type);

        ConStatus getStatus(const UnitOrStructureType* type, unsigned char* quantity, unsigned char* progress) const;

        void Placed();

        const UnitOrStructureType* getCurrentType() const {return *queue.begin();}

    private:
        bool next();
        enum RQstate {RQ_DONE, RQ_NEW, RQ_MAXED};
        RQstate requeue(const UnitOrStructureType* type);

        Player *player;
        unsigned int last, left;
        ConStatus status;

        shared_ptr<BQTimer> timer;
        bool tick();
        void rescheduled();

        Production production;
        Queue queue;
    };
}

#endif

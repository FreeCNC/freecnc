#include <cassert>
#include <stdexcept>

#include "actioneventqueue.h"
#include "buildqueue.h"
#include "dispatcher.h"
#include "player.h"
#include "playerpool.h"
#include "unitorstructure.h"
#include "unitandstructurepool.h"

namespace BuildQueue
{
    class BQTimer : public ActionEvent
    {
    public:
        BQTimer(BQueue* queue, bool (BQueue::*func)(), shared_ptr<BQTimer>* backref)
            : ActionEvent(1), queue(queue), func(func), scheduled(false),
            backref(backref) {}

        bool run()
        {
            scheduled = (queue->*func)();
            return scheduled;
        }

        void Reschedule()
        {
            if (!scheduled) {
                scheduled = true;
                p::aequeue->scheduleEvent(*backref);
            }
        }

    private:
        BQueue* queue;
        bool (BQueue::*func)();
        bool scheduled;
        shared_ptr<BQTimer>* backref;
    };


    /// @TODO These values should be moved to a configuration file
    // I don't think buildspeed needs to be changed once this code is stable.
    const unsigned char buildspeed = 1;
    //const unsigned char buildspeed = 5;
    const unsigned char maxbuild = 99;

    BQueue::BQueue(Player *p) : player(p), status(BQ_EMPTY)
    {
        timer.reset(new BQTimer(this, &BQueue::tick, &timer));
    }

    bool BQueue::Add(const UnitOrStructureType *type)
    {
        switch (status) {
        case BQ_INVALID:
            game.log << "BQueue: Queue " << this << " in invalid state" << endl;
            break;
        case BQ_EMPTY:
            if (0 == (left = type->getCost())) {
                game.log << "BQueue: Type \"" << type->getTName() << "\" has no cost" << endl;
            }
            last = p::aequeue->getCurtick();
            production.insert(Production::value_type(type, 1));
            queue.push_back(type);
            status = BQ_RUNNING;
            timer->Reschedule();
            return true;
            break;
        case BQ_PAUSED:
            if (getCurrentType() == type) {
                status = BQ_RUNNING;
                last = p::aequeue->getCurtick();
                timer->Reschedule();
            }
            return true;
            break;
        case BQ_READY:
            // Can't enqueue, waiting for placement
            if (!type->isStructure()) {
                if (p::dispatcher->unitSpawn((UnitType*)type, player->getPlayerNum())) {
                    Placed();
                } else {
                    game.log << "BQueue: Did not spawn \"" << type->getTName() << endl;
                }
            }
            return false;
            break;
        case BQ_RUNNING:
            // First try to requeue another of the type
            switch (requeue(type)) {
            case RQ_DONE:
                return true;
            case RQ_MAXED:
                return false;
            case RQ_NEW:
                // This type is new to the queue
                if (0 == type->getCost()) {
                    // We divide by cost, so must not be zero.
                    game.log << "BQueue: Type \"" << type->getTName() << "\" has no cost" << endl;
                    return false;
                }
                production.insert(Production::value_type(type,1));
                queue.push_back(type);
                timer->Reschedule();
                return true;
            }
            return false;
            break;
        default:
            game.log << "BQueue: Queue " << this << " in invalid state " << status << endl;
            break;
        }
        return false;
    }

    ConStatus BQueue::PauseCancel(const UnitOrStructureType* type)
    {
        if (BQ_EMPTY == status) {
            return BQ_EMPTY;
        }
        // We search the map first because it should be faster.
        Production::iterator mit = production.find(type);
        if (production.end() == mit) {
            // Not queued this type at all
            // UI decision: do we behave as if we had clicked on the type currently
            // being built?
            return BQ_EMPTY;
        }
        Queue::iterator lit = find(queue.begin(), queue.end(), type);

        if (queue.begin() == lit) {
            switch (status) {
            case BQ_RUNNING:
                status = BQ_PAUSED;
                p::ppool->updateSidebar();
                return status;
                break;
            case BQ_READY:
                // Fallthrough
            case BQ_PAUSED:
                if (mit->second > 1) {
                    --mit->second;
                } else {
                    // Refund what we've spent so far
                    player->changeMoney(mit->first->getCost()-left);
                    // Remain in paused state
                    next();
                }
                break;
            default:
                std::string msg("Unhandled state in pausecancel: "); msg += status;
                throw std::runtime_error(msg);
                break;
            }
        } else {
            if (mit->second > 1) {
                --mit->second;
            } else {
                queue.erase(lit);
                production.erase(mit);
            }
        }

        p::ppool->updateSidebar();
        return BQ_CANCELLED;
    }

    bool BQueue::tick() {
        if (BQ_RUNNING != status) {
            return false;
        }
        unsigned char delta = min((p::aequeue->getCurtick()-last)/buildspeed, left);

        if (delta == 0) {
            return false;
        }
        last += delta*buildspeed;
        /// @TODO Play "tink" sound
        if (!player->changeMoney(-delta)) {
            /// @TODO Play "insufficient funds" sound
            // Note: C&C didn't put build on hold when you initially run out, so you
            // could leave something partially built whilst waiting for the
            // harvester to return

            // Reschedule to keep checking for money
            return true;
        }
        left -= delta;

        if (0 != left) {
            p::ppool->updateSidebar();
            return true;
        }
        const UnitOrStructureType* type = getCurrentType();
        // For structures and blocked unit production, we wait for user
        // interaction.
        status = BQ_READY;
        if (!type->isStructure()) {
            UnitType* utype = (UnitType*)type;
            if (p::dispatcher->unitSpawn(utype, player->getPlayerNum())) {
                /// @TODO Play "unit ready" sound
                // If we were able to spawn the unit, move onto the next
                // item
                status = BQ_RUNNING;
                next();
            }
        } else {
            /// @TODO Play "construction complete" sound
        }
        p::ppool->updateSidebar();
        return false;
    }

    ConStatus BQueue::getStatus(const UnitOrStructureType* type, unsigned char* quantity, unsigned char* progress) const
    {
        *quantity = 0;
        *progress = 100; // Default to not grey'd out.
        if (BQ_EMPTY == status || BQ_INVALID == status) {
            // Fast exit for states with nothing to report
            return status;
        }
        Production::const_iterator it = production.find(type);
        *progress = 0;
        if (it == production.end()) {
            return status;
        }
        *quantity = it->second;
        if (getCurrentType() == type) {
            *progress = 100*(it->first->getCost()-left)/getCurrentType()->getCost();
        }
        return status;
    }

    void BQueue::next()
    {
        assert(!queue.empty());
        Production::iterator it = production.find(getCurrentType());
        assert(it != production.end());
        if (it->second <= 1) {
            production.erase(it);
            queue.pop_front();
            if (queue.empty()) {
                status = BQ_EMPTY;
                return;
            } else {
                // None left of the current type of thing being built, so move onto
                // the next item in the queue and start building
                status = BQ_RUNNING;
                it = production.find(getCurrentType());
                assert(it != production.end());
            }
        } else {
            --it->second;
        }
        left = it->first->getCost();
        last = p::aequeue->getCurtick();
        timer->Reschedule();
        p::ppool->updateSidebar();
    }

    BQueue::RQstate BQueue::requeue(const UnitOrStructureType* type)
    {
        Production::iterator it = production.find(type);
        if (it == production.end()) {
            return RQ_NEW;
        }
        if (it->second > maxbuild) {
            return RQ_MAXED;
        }
        it->second++;
        return RQ_DONE;
    }

    /// Used for other bits of code to notify the buildqueue that they can continue
    // building having placed what was waiting for a position
    void BQueue::Placed()
    {
        p::ppool->updateSidebar();
        status = BQ_RUNNING;
        next();
    }
}

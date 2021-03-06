#include "actioneventqueue.h"
#include "dispatcher.h"
#include "map.h"
#include "playerpool.h"
#include "unitandstructurepool.h"
#include "unit.h"
#include "unitorstructure.h"
#include "structure.h"

namespace Dispatcher
{
    /** NOTE: I've stripped out the sections related to logging and playback as that
     * part isn't as stable as the rest (basically need to fix a horrible synch
     * issue with the playback)
     */

    Dispatcher::Dispatcher()
        : logstate(NORMAL), localPlayer(p::ppool->getLPlayerNum())
    {
    }

    Dispatcher::~Dispatcher()
    {
        switch (logstate) {
        case RECORDING:
            break;
        case PLAYING:
            break;
        case NORMAL:
        default:
            break;
        }
    }

    void Dispatcher::unitMove(Unit* un, unsigned int dest)
    {
        if (un == 0) {
            return;
        }
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                un->move(dest);
                break;
            case PLAYING:
            default:
                break;
        }
    }

    void Dispatcher::unitAttack(Unit* un, UnitOrStructure* target, bool tisunit)
    {
        if (un == 0) {
            return;
        }
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                un->attack(target);
                break;
            case PLAYING:
            default:
                break;
        }
    }

    void Dispatcher::unitDeploy(Unit* un)
    {
        if (un == 0) {
            return;
        }
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                un->deploy();
                break;
            case PLAYING:
            default:
                break;
        }
    }

    void Dispatcher::structureAttack(Structure* st, UnitOrStructure* target, bool tisunit)
    {
        if (st == 0) {
            return;
        }
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                st->attack(target);
                break;
            case PLAYING:
            default:
                break;
        }
    }

    bool Dispatcher::structurePlace(const StructureType* type, unsigned int pos, unsigned char owner) {
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                /// XXX TEMP HACK!
                return p::uspool->createStructure(const_cast<StructureType*>(type),pos,owner,FULLHEALTH,0,true);
                break;
            case PLAYING:
            default:
                break;
        };
        /// XXX This won't always be true.
        return true;
    }

    bool Dispatcher::structurePlace(const char* tname, unsigned int pos, unsigned char owner) {
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                return p::uspool->createStructure(tname,pos,owner,FULLHEALTH,0,true);
                break;
            case PLAYING:
            default:
                break;
        };
        /// XXX This won't always be true.
        return true;
    }

    bool Dispatcher::unitSpawn(UnitType* type, unsigned char owner) {
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                return p::uspool->spawnUnit(type,owner);
                break;
            case PLAYING:
            default:
                break;
        };
        /// XXX This won't always be true.
        return true;
    }

    bool Dispatcher::unitSpawn(const char* tname, unsigned char owner) {
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                return p::uspool->spawnUnit(tname,owner);
                break;
            case PLAYING:
            default:
                break;
        };
        /// XXX This won't always be true.
        return true;
    }

    bool Dispatcher::unitCreate(const char* tname, unsigned int pos, unsigned char subpos, unsigned char owner) {
        switch (logstate) {
            case RECORDING:
                // deliberate fallthrough
            case NORMAL:
                return p::uspool->createUnit(tname,pos,subpos,owner,FULLHEALTH,0);
                break;
            case PLAYING:
            default:
                break;
        };
        /// XXX This won't always be true.
        return true;
    }
}

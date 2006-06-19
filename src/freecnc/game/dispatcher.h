// 
// Dispatcher - stores and plays back game sessions
//
#ifndef _GAME_DISPATCHER_H
#define _GAME_DISPATCHER_H

#include <cstdio>
#include <queue>

#include "../freecnc.h"

class Unit;
class UnitType;
class UnitOrStructure;
class UnitOrStructureType;
class Structure;
class StructureType;

namespace Dispatcher
{
    enum DispatchLogState {NORMAL, RECORDING, PLAYING};

    class Dispatcher {
    public:
        Dispatcher();
        ~Dispatcher();
        void unitMove(Unit* un, Uint32 dest);
        void unitAttack(Unit* un, UnitOrStructure* target, bool tisunit);
        void unitDeploy(Unit* un);

        void structureAttack(Structure* st, UnitOrStructure* target, bool tisunit);

        /// @TODO Implement these
        bool constructionStart(const UnitOrStructureType* type);
        void constructionPause(const UnitOrStructureType* type);
        void constructionPause(Uint8 ptype);
        void constructionResume(const UnitOrStructureType* type);
        void constructionResume(Uint8 ptype);
        void constructionCancel(const UnitOrStructureType* type);
        void constructionCancel(Uint8 ptype);

        ConStatus constructionQuery(const UnitOrStructureType* type);
        ConStatus constructionQuery(Uint8 ptype);

        /// @returns true if structure was placed at given location.
        bool structurePlace(const StructureType* type, Uint32 pos, Uint8 owner);
        bool structurePlace(const char* typen, Uint32 pos, Uint8 owner);
        /// Spawns a unit at the player's appropriate primary building
        bool unitSpawn(UnitType* type, Uint8 owner);
        bool unitSpawn(const char* tname, Uint8 owner);
        /// Temporary function to place a unit directly on the map
        bool unitCreate(const char* tname, Uint32 pos, Uint8 subpos, Uint8 owner);

        Uint16 getExitCell(const UnitOrStructureType* type);
        Uint16 getExitCell(Uint8 ptype);
    private:
        DispatchLogState logstate;
        Uint8 localPlayer;
    };
}

#endif

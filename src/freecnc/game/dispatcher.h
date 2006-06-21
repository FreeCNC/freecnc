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
        void unitMove(Unit* un, unsigned int dest);
        void unitAttack(Unit* un, UnitOrStructure* target, bool tisunit);
        void unitDeploy(Unit* un);

        void structureAttack(Structure* st, UnitOrStructure* target, bool tisunit);

        /// @TODO Implement these
        bool constructionStart(const UnitOrStructureType* type);
        void constructionPause(const UnitOrStructureType* type);
        void constructionPause(unsigned char ptype);
        void constructionResume(const UnitOrStructureType* type);
        void constructionResume(unsigned char ptype);
        void constructionCancel(const UnitOrStructureType* type);
        void constructionCancel(unsigned char ptype);

        ConStatus constructionQuery(const UnitOrStructureType* type);
        ConStatus constructionQuery(unsigned char ptype);

        /// @returns true if structure was placed at given location.
        bool structurePlace(const StructureType* type, unsigned int pos, unsigned char owner);
        bool structurePlace(const char* typen, unsigned int pos, unsigned char owner);
        /// Spawns a unit at the player's appropriate primary building
        bool unitSpawn(UnitType* type, unsigned char owner);
        bool unitSpawn(const char* tname, unsigned char owner);
        /// Temporary function to place a unit directly on the map
        bool unitCreate(const char* tname, unsigned int pos, unsigned char subpos, unsigned char owner);

        unsigned short getExitCell(const UnitOrStructureType* type);
        unsigned short getExitCell(unsigned char ptype);
    private:
        DispatchLogState logstate;
        unsigned char localPlayer;
    };
}

#endif

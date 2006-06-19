#ifndef _GAME_PLAYER_H
#define _GAME_PLAYER_H

#include <list>
#include "../freecnc.h"
#include "unitorstructure.h"

class Unit;
class Structure;
class StructureType;
class MoneyCounter;

namespace BuildQueue
{
    class BQueue;
}

namespace AI
{
    struct AIPluginData;
}

class Player;

class Player
{
public:
    explicit Player(const char *pname, shared_ptr<INIFile> mapini);
    ~Player();
    void setPlayerNum(Uint8 num) {playernum = num;}
    void setMultiColour(const char* colour);
    void setSettings(const char* nick, const char* colour, const char* mside);

    Uint8 getPlayerNum() const {return playernum;}
    const char* getName() const {return playername;}
    Uint8 getSide() const {return playerside;}
    Uint8 getMSide() const {return multiside;}

    bool changeMoney(Sint32 change);
    Sint32 getMoney() const {return money;}

    bool startBuilding(UnitOrStructureType* type);
    ConStatus stopBuilding(UnitOrStructureType* type);
    void placed(UnitOrStructureType* type);
    ConStatus getStatus(UnitOrStructureType* type, Uint8* quantity, Uint8* progress);
    BuildQueue::BQueue* getQueue(Uint8 ptype);

    void builtUnit(Unit* un);
    void lostUnit(Unit* un, bool wasDeployed);
    void movedUnit(Uint32 oldpos, Uint32 newpos, Uint8 sight);
    void builtStruct(Structure* str);
    void lostStruct(Structure* str);

    size_t getNumUnits() {return unitpool.size();}
    size_t getNumStructs() const {return structurepool.size();}
    const std::vector<Unit*>& getUnits() const {return unitpool;}
    const std::vector<Structure*>& getStructures() const {return structurepool;}

    Uint8 getStructpalNum() const {return structpalnum;}
    Uint8 getUnitpalNum() const {return unitpalnum;}

    Uint32 getPower() const {return powerGenerated;}
    Uint32 getPowerUsed() const {return powerUsed;}

    Uint16 getPlayerStart() const {return playerstart;}
    void placeMultiUnits();

    void updateOwner(Uint8 newnum);

    bool isDefeated() const{return defeated;}

    bool isAllied(Player* pl) const;
    size_t getNumAllies() const {return allies.size() - unallycalls;}
    bool allyWithPlayer(Player* pl);
    void didAlly(Player* pl);
    bool unallyWithPlayer(Player* pl);
    void didUnally(Player* pl);
    void setAlliances();
    void clearAlliances();

    void addUnitKill() {++unitkills;}
    void addStructureKill() {++structurekills;}
    Uint32 getUnitKills() const {return unitkills;}
    Uint32 getUnitLosses() const {return unitlosses;}
    Uint32 getStructureKills() const {return structurekills;}
    Uint16 getStructureLosses() const {return structurelosses;}

    size_t ownsStructure(StructureType* stype) {return structures_owned[stype].size();}
    Structure*& getPrimary(const UnitOrStructureType* uostype) {
        return primary_structure[uostype->getPType()];
    }
    Structure*& getPrimary(Uint32 ptype) {
        return primary_structure[ptype];
    }
    void setPrimary(Structure* str);

    // SOB == Sight or Build.
    enum SOB_update { SOB_SIGHT = 1, SOB_BUILD = 2 };
    void setVisBuild(SOB_update mode, bool val);
    std::vector<bool>& getMapVis() {return mapVisible;}
    std::vector<bool>& getMapBuildable() {return mapBuildable;}

    /// Turns on a block of cells in either the sight or buildable matrix
    void addSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode);
    /// Turns off a block of cells in either the sight or buildable matrix
    void removeSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode);

    bool canBuildAll() const {return buildall;}
    bool canBuildAny() const {return buildany;}
    bool canSeeAll() const {return allmap; }
    bool hasInfMoney() const {return infmoney;}
    void enableBuildAll() {buildall = true;}
    void enableInfMoney() {infmoney = true;}
private:
    // Do not want player being constructed using default constructor
    Player() {};
    Player(const Player&) {};

    // This instead of a vector as we don't have to check ranges before
    // operations
    std::map<Uint8, BuildQueue::BQueue*> queues;

    bool defeated;
    char *playername, *nickname;
    Uint8 playerside, multiside, playernum, radarstat, unitpalnum, structpalnum;

    // See the alliance code in the .cpp file
    Uint8 unallycalls;

    Sint32 money;
    Uint32 powerGenerated, powerUsed;

    Uint32 unitkills, unitlosses, structurekills, structurelosses;

    Uint16 playerstart;

    // All of these pointers are owned elsewhere.
    std::vector<Unit*> unitpool;
    std::vector<Structure*> structurepool;
    std::map<StructureType*, std::list<Structure*> > structures_owned;
    std::map<Uint32, std::list<Structure*> > production_groups;
    std::map<Uint32, Structure*> primary_structure;

    std::vector<Player*> allies;
    // players that have allied with this player, but this player
    // has not allied in return.  Used to force an unally when player
    // is defeated.
    std::vector<Player*> non_reciproc_allies;

    std::vector<Uint8> sightMatrix, buildMatrix;

    std::vector<bool> mapVisible, mapBuildable;
    // cheat/debug flags: allmap (reveal all map), buildany (remove
    // proximity check), buildall (disable prerequisites) infmoney (doesn't
    // care if money goes negative).
    bool allmap, buildany, buildall, infmoney;

    MoneyCounter* counter;
};


#endif

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
    void setPlayerNum(unsigned char num) {playernum = num;}
    void setMultiColour(const char* colour);
    void setSettings(const char* nick, const char* colour, const char* mside);

    unsigned char getPlayerNum() const {return playernum;}
    const char* getName() const {return playername;}
    unsigned char getSide() const {return playerside;}
    unsigned char getMSide() const {return multiside;}

    bool changeMoney(int change);
    int getMoney() const {return money;}

    bool startBuilding(UnitOrStructureType* type);
    ConStatus stopBuilding(UnitOrStructureType* type);
    void placed(UnitOrStructureType* type);
    ConStatus getStatus(UnitOrStructureType* type, unsigned char* quantity, unsigned char* progress);
    BuildQueue::BQueue* getQueue(unsigned char ptype);

    void builtUnit(Unit* un);
    void lostUnit(Unit* un, bool wasDeployed);
    void movedUnit(unsigned int oldpos, unsigned int newpos, unsigned char sight);
    void builtStruct(Structure* str);
    void lostStruct(Structure* str);

    size_t getNumUnits() {return unitpool.size();}
    size_t getNumStructs() const {return structurepool.size();}
    const std::vector<Unit*>& getUnits() const {return unitpool;}
    const std::vector<Structure*>& getStructures() const {return structurepool;}

    unsigned char getStructpalNum() const {return structpalnum;}
    unsigned char getUnitpalNum() const {return unitpalnum;}

    unsigned int getPower() const {return powerGenerated;}
    unsigned int getPowerUsed() const {return powerUsed;}

    unsigned short getPlayerStart() const {return playerstart;}
    void placeMultiUnits();

    void updateOwner(unsigned char newnum);

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
    unsigned int getUnitKills() const {return unitkills;}
    unsigned int getUnitLosses() const {return unitlosses;}
    unsigned int getStructureKills() const {return structurekills;}
    unsigned short getStructureLosses() const {return structurelosses;}

    size_t ownsStructure(StructureType* stype) {return structures_owned[stype].size();}
    Structure*& getPrimary(const UnitOrStructureType* uostype) {
        return primary_structure[uostype->getPType()];
    }
    Structure*& getPrimary(unsigned int ptype) {
        return primary_structure[ptype];
    }
    void setPrimary(Structure* str);

    // SOB == Sight or Build.
    enum SOB_update { SOB_SIGHT = 1, SOB_BUILD = 2 };
    void setVisBuild(SOB_update mode, bool val);
    std::vector<bool>& getMapVis() {return mapVisible;}
    std::vector<bool>& getMapBuildable() {return mapBuildable;}

    /// Turns on a block of cells in either the sight or buildable matrix
    void addSoB(unsigned int pos, unsigned char width, unsigned char height, unsigned char sight, SOB_update mode);
    /// Turns off a block of cells in either the sight or buildable matrix
    void removeSoB(unsigned int pos, unsigned char width, unsigned char height, unsigned char sight, SOB_update mode);

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
    std::map<unsigned char, BuildQueue::BQueue*> queues;

    bool defeated;
    char *playername, *nickname;
    unsigned char playerside, multiside, playernum, radarstat, unitpalnum, structpalnum;

    // See the alliance code in the .cpp file
    unsigned char unallycalls;

    int money;
    unsigned int powerGenerated, powerUsed;

    unsigned int unitkills, unitlosses, structurekills, structurelosses;

    unsigned short playerstart;

    // All of these pointers are owned elsewhere.
    std::vector<Unit*> unitpool;
    std::vector<Structure*> structurepool;
    std::map<StructureType*, std::list<Structure*> > structures_owned;
    std::map<unsigned int, std::list<Structure*> > production_groups;
    std::map<unsigned int, Structure*> primary_structure;

    std::vector<Player*> allies;
    // players that have allied with this player, but this player
    // has not allied in return.  Used to force an unally when player
    // is defeated.
    std::vector<Player*> non_reciproc_allies;

    std::vector<unsigned char> sightMatrix, buildMatrix;

    std::vector<bool> mapVisible, mapBuildable;
    // cheat/debug flags: allmap (reveal all map), buildany (remove
    // proximity check), buildall (disable prerequisites) infmoney (doesn't
    // care if money goes negative).
    bool allmap, buildany, buildall, infmoney;

    MoneyCounter* counter;
};


#endif

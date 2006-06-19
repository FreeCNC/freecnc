#ifndef _GAME_PLAYERPOOL_H
#define _GAME_PLAYERPOOL_H

#include "../freecnc.h"
#include "buildqueue.h"
#include "player.h"

namespace AI {
    struct AIPluginData;
}
namespace BuildQueue {
    class BQEvent;
}

class INIFile;
class PlayerPool;

// TODO: Currently the player starts are shuffled randomly without any way of accepting a preshuffled list.
class PlayerPool
{
public:
    explicit PlayerPool(shared_ptr<INIFile> inifile, Uint8 gamemode);
    ~PlayerPool();
    Uint8 getNumPlayers() const {
        return static_cast<Uint8>(playerpool.size());
    }
    Uint8 getLPlayerNum() const {
        return localPlayer;
    }
    Player *getLPlayer()
    {
        return playerpool[localPlayer];
    }
    void setLPlayer(const char *pname);
    void setLPlayer(Uint8 number, const char* nick, const char* colour, const char* mside);
    Player *getPlayer(Uint8 player)
    {
        return playerpool[player];
    }
    Uint8 getPlayerNum(const char *pname);
    Player* getPlayerByName(const char* pname);

    Uint8 getUnitpalNum(Uint8 player) const {
        return playerpool[player]->getUnitpalNum();
    }
    Uint8 getStructpalNum(Uint8 player) const {
        return playerpool[player]->getStructpalNum();
    }
    std::vector<Player*> getOpponents(Player* pl);
    void playerDefeated(Player *pl);
    void playerUndefeated(Player *pl);
    bool hasWon() const {
        return won;
    }
    bool hasLost() const {
        return lost;
    }
    void setAlliances();
    void placeMultiUnits();
    shared_ptr<INIFile> getMapINI();
    Uint16 getAStart();
    void setWaypoints(std::vector<Uint16> wps);

    /// Called by input to see if sidebar needs updating
    bool pollSidebar();

    /// Called by the local player when sidebar is to be updated
    void updateSidebar();

    /// Called by input to see if radar status has changed.
    Uint8 statRadar();

    /// Called by the local player to update the radar status
    void updateRadar(Uint8 status);

private:
    PlayerPool();
    PlayerPool(const PlayerPool&);

    std::vector<Player *> playerpool;
    std::vector<Uint16> player_starts;
    Uint8 localPlayer, gamemode, radarstatus;
    bool won, lost, updatesidebar;
    shared_ptr<INIFile> mapini;
};

#endif /* PLAYERPOOL_H */

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
    explicit PlayerPool(shared_ptr<INIFile> inifile, unsigned char gamemode);
    ~PlayerPool();
    unsigned char getNumPlayers() const {
        return static_cast<unsigned char>(playerpool.size());
    }
    unsigned char getLPlayerNum() const {
        return localPlayer;
    }
    Player *getLPlayer()
    {
        return playerpool[localPlayer];
    }
    void setLPlayer(const char *pname);
    void setLPlayer(unsigned char number, const char* nick, const char* colour, const char* mside);
    Player *getPlayer(unsigned char player)
    {
        return playerpool[player];
    }
    unsigned char getPlayerNum(const char *pname);
    Player* getPlayerByName(const char* pname);

    unsigned char getUnitpalNum(unsigned char player) const {
        return playerpool[player]->getUnitpalNum();
    }
    unsigned char getStructpalNum(unsigned char player) const {
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
    unsigned short getAStart();
    void setWaypoints(std::vector<unsigned short> wps);

    /// Called by input to see if sidebar needs updating
    bool pollSidebar();

    /// Called by the local player when sidebar is to be updated
    void updateSidebar();

    /// Called by input to see if radar status has changed.
    unsigned char statRadar();

    /// Called by the local player to update the radar status
    void updateRadar(unsigned char status);

private:
    PlayerPool();
    PlayerPool(const PlayerPool&);

    std::vector<Player *> playerpool;
    std::vector<unsigned short> player_starts;
    unsigned char localPlayer, gamemode, radarstatus;
    bool won, lost, updatesidebar;
    shared_ptr<INIFile> mapini;
};

#endif /* PLAYERPOOL_H */

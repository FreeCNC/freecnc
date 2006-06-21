#include <cstring>
#include "../freecnc.h"
#include "../lib/inifile.h"
#include "playerpool.h"

PlayerPool::PlayerPool(shared_ptr<INIFile> inifile, unsigned char gamemode)
{
    lost = false;
    won = false;
    mapini = inifile;
    updatesidebar = false;
    radarstatus = 0;
    this->gamemode = gamemode;
}

PlayerPool::~PlayerPool()
{
    unsigned char i;
    for( i = 0; i < playerpool.size(); i++ ) {
        delete playerpool[i];
    }
}

void PlayerPool::setLPlayer(const char* pname)
{
    unsigned char i;
    for( i = 0; i < playerpool.size(); i++ ) {
        if( !strcasecmp(playerpool[i]->getName(), pname) ) {
            localPlayer = i;
            return;
        }
    }
    //logger->warning("Tried to set local player to non-existing player \"%s\"\n", pname);
    playerpool.push_back(new Player(pname, mapini));
    localPlayer = static_cast<unsigned char>(playerpool.size()-1);
    playerpool[localPlayer]->setPlayerNum(localPlayer);
}

void PlayerPool::setLPlayer(unsigned char number, const char* nick, const char* colour, const char* mside)
{
    unsigned char i;
    for( i = 0; i < playerpool.size(); i++ ) {
        if( playerpool[i]->getMSide() == number ) {
            localPlayer = i;
            playerpool[i]->setSettings(nick,colour,mside);
            return;
        }
    }
    //logger->warning("Tried to set local player to non-existing player number %i\n", number);
    /*  playerpool.push_back(new Player("multi", mapini, this));
        localPlayer = playerpool.size()-1;
        playerpool[localPlayer]->setSettings(nick,colour,mside);
    */
}

unsigned char PlayerPool::getPlayerNum(const char *pname)
{
    unsigned char i;
    for( i = 0; i < playerpool.size(); i++ ) {
        if( !strcasecmp(playerpool[i]->getName(), pname) ) {
            return i;
        }
    }
    playerpool.push_back(new Player(pname, mapini));
    playerpool[playerpool.size()-1]->setPlayerNum(static_cast<unsigned char>(playerpool.size()-1));
    return static_cast<unsigned char>(playerpool.size()-1);
}

Player* PlayerPool::getPlayerByName(const char* pname)
{
    return playerpool[getPlayerNum(pname)];
}

std::vector<Player*> PlayerPool::getOpponents(Player* pl)
{
    std::vector<Player*> opps;
    for(unsigned char i = 0; i < playerpool.size(); i++ ) {
        if (!playerpool[i]->isDefeated() && !pl->isAllied(playerpool[i])) {
            opps.push_back(playerpool[i]);
        }
    }
    return opps;
}

void PlayerPool::playerDefeated(Player *pl)
{
    unsigned char i;

    pl->clearAlliances();
    for( i = 0; i < playerpool.size(); i++ ) {
        if( playerpool[i] == pl ) {
            break;
        }
    }
    if( i == localPlayer ) {
        lost = true;
    }
    if (!lost) {
        unsigned char defeated = 0;
        for (i = 0; i < playerpool.size(); ++i) {
            if (playerpool[i]->isDefeated()) {
                ++defeated;
            }
        }
        if (playerpool.size() - defeated == 1) {
            won = true;
        } else if (playerpool.size() - defeated == playerpool[localPlayer]->getNumAllies() ) {
            won = true;
        }
    }
}

void PlayerPool::playerUndefeated(Player* pl)
{
    unsigned char i;

    pl->setAlliances();
    for( i = 0; i < playerpool.size(); i++ ) {
        if( playerpool[i] == pl ) {
            break;
        }
    }
    if( i == localPlayer ) {
        lost = false;
    }
    if (gamemode == 0) {
        if (!lost) {
            won = false;
        }
    }
}

shared_ptr<INIFile> PlayerPool::getMapINI()
{
    return mapini;
}

void PlayerPool::setAlliances()
{
    for (unsigned short i=0; i < playerpool.size() ; ++i) {
        playerpool[i]->setAlliances();
    }
}

void PlayerPool::placeMultiUnits()
{
    for (unsigned short i=0; i < playerpool.size() ; ++i) {
        if (playerpool[i]->getPlayerStart() != 0) {
            playerpool[i]->placeMultiUnits();
        }
    }
}

unsigned short PlayerPool::getAStart()
{
    unsigned char rnd,sze;
    unsigned short rv;
    sze = static_cast<unsigned char>(player_starts.size());
    if (sze == 0) {
        return 0;
    }
    for (rv = 0; rv < sze ; ++rv) {
        if (player_starts[rv] != 0) {
            break;
        }
    }
    if (rv == sze) {
        player_starts.resize(0);
        return 0;
    }

    // pick a starting location at random
    rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
    while (player_starts[rnd] == 0) {
        rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
    }
    rv  = player_starts[rnd];
    // ensure this starting location is not reused.
    player_starts[rnd] = 0;
    return rv;
}

void PlayerPool::setWaypoints(std::vector<unsigned short> wps)
{
    player_starts = wps;
}

bool PlayerPool::pollSidebar()
{
    if (updatesidebar) {
        updatesidebar = false;
        return true;
    }
    return false;
}

void PlayerPool::updateSidebar()
{
    updatesidebar = true;
}

unsigned char PlayerPool::statRadar()
{
    unsigned char tmp = radarstatus;
    radarstatus = 0;
    return tmp;
}

void PlayerPool::updateRadar(unsigned char status)
{
    radarstatus = status;
}

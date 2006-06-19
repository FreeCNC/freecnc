#include <algorithm>
#include <cassert>
#include <cstring>

#include "../lib/inifile.h"
#include "../ui/ui_public.h"
#include "map.h"
#include "moneycounter.h"
#include "dispatcher.h"
#include "player.h"
#include "playerpool.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "unitorstructure.h"
#include "structure.h"
#include "buildqueue.h"

using BuildQueue::BQueue;

/**
 * @TODO Make hardcoded side names customisable (Needed to make RA support
 * cleaner)
 *
 * @TODO The colour lookup code needs to ensure duplicates don't happen.
 *
 * @TODO Do something about the player start points in multiplayer: server
 * should take points from map, shuffle them, and then hand out only the start
 * point for each player (possibly include start points for allies if we have
 * map sharing between allies).  This requires some work on the USPool too (see
 * that file for details).
 */
Player::Player(const char *pname, shared_ptr<INIFile> mapini) {
    Uint32 mapsize;
    playername = cppstrdup(pname);
    multiside = 0;
    unallycalls = 0;
    playerstart = 0;
    defeated = false;
    if( !strcasecmp(playername, "goodguy") ) {
        playerside = PS_GOOD;
        unitpalnum = 0;
        structpalnum = 0;
    } else if( !strcasecmp(playername, "badguy") ) {
        playerside = PS_BAD;
        unitpalnum = 2;
        structpalnum = 1;
    } else if( !strcasecmp(playername, "neutral") ) {
        playerside = PS_NEUTRAL;
        unitpalnum = 0;
        structpalnum = 0;
    } else if( !strcasecmp(playername, "special") ) {
        playerside = PS_SPECIAL;
        unitpalnum = 0;
        structpalnum = 0;
    } else if( !strncasecmp(playername, "multi", 5) ) {
        playerside = PS_MULTI;
        if (playername[5] < 49 || playername[5] > 57) {
            logger->error("Invalid builtin multi name: %s\n",playername);
            /// @TODO Nicer error handling here
            multiside = 9;
        } else {
            multiside = playername[5] - 48;
        }
        if (multiside > 0) {
            unitpalnum = multiside - 1;
            structpalnum = multiside - 1;
        } else {
            unitpalnum = structpalnum = 0;
        }
        playerstart = p::ppool->getAStart();
        if (playerstart != 0) {
            // conversion algorithm from loadmap.cpp
            playerstart = p::ccmap->normaliseCoord(playerstart);
        }
    } else {
        logger->warning("Player Side \"%s\" not recognised, using gdi instead\n",pname);
        playerside = PS_GOOD;
        unitpalnum = 0;
        structpalnum = 0;
    }
    money = mapini->readInt(playername, "Credits", 0) * 100;
    powerGenerated = powerUsed = radarstat = 0;
    unitkills = unitlosses = structurekills = structurelosses = 0;

    mapsize = p::ccmap->getWidth()*p::ccmap->getHeight();
    sightMatrix.resize(mapsize);
    buildMatrix.resize(mapsize);
    mapVisible.resize(mapsize);
    mapBuildable.resize(mapsize);

    allmap = buildall = buildany = infmoney = false;

    queues[0] = new BQueue(this);
    /// @TODO Only play sound if this is the local player
    counter = new MoneyCounter(&money, this, &counter);
}

Player::~Player()
{
    delete[] playername;
    map<Uint8, BQueue*>::iterator i, end;
    end = queues.end();
    for (i = queues.begin(); i != end; ++i) {
        delete i->second;
    }
    delete counter;
}

bool Player::changeMoney(Sint32 change) {
    if (0 == change) {
        return true;
    } else if (change > 0) {
        counter->addCredit(change);
        return true;
    } else {
        Sint32 after = money + change - counter->getDebt();
        if (!infmoney && after < 0) {
            return false;
        }
        counter->addDebt(-change);
        return true;
    }
}

void Player::setVisBuild(SOB_update mode, bool val)
{
    if (mode == SOB_SIGHT) {
        std::fill(mapVisible.begin(), mapVisible.end(), val);
    } else {
        std::fill(mapBuildable.begin(), mapBuildable.end(), val);
    }
}

void Player::setSettings(const char* nick, const char* colour, const char* mside)
{
    if (mside == NULL || strlen(mside) == 0) {
        playerside |= PS_GOOD;
    } else {
        if (strcasecmp(mside,"gdi") == 0) {
            playerside |= PS_GOOD;
        } else if (strcasecmp(mside,"nod") == 0) {
            playerside |= PS_BAD;
        }
    }
    if (colour != NULL && strlen(colour) != 0) {
        setMultiColour(colour);
    }
    if (nick != NULL && strlen(nick) != 0) {
        if (strlen(nick) < strlen(playername)) {
            strncpy(playername, nick, strlen(playername));
        } else {
            delete[] playername;
            playername = cppstrdup(nick);
        }
    }
}

void Player::setAlliances()
{
    shared_ptr<INIFile> mapini = p::ppool->getMapINI();
    std::vector<char*> allies_n;
    char *tmp;

    tmp = mapini->readString(playername, "Allies");
    if( tmp != NULL ) {
        allies_n = splitList(tmp,',');
        delete[] tmp;
    }

    // always allied to self
    allyWithPlayer(this);
    // no initial allies for multiplayer
    if (multiside == 0) {
        for (Uint16 i=0;i<allies_n.size();++i) {
            if (strcasecmp(allies_n[i],playername)) {
                allyWithPlayer(p::ppool->getPlayerByName(allies_n[i]));
            }
            delete[] allies_n[i];
        }
    } else {
        for (Uint16 i=0;i<allies_n.size();++i) {
            delete[] allies_n[i];
        }
    }
    // since this part of the initialisation is deferred until after map
    // is loaded check for no units or structures
    if (unitpool.empty() && structurepool.empty()) {
        defeated = true;
        clearAlliances();
        return;
    }
}

void Player::clearAlliances()
{
    Uint32 i;
    Player* tmp;
    for (i = 0; i < allies.size() ; ++i) {
        if (allies[i] != NULL) {
            tmp = allies[i];
            unallyWithPlayer(tmp);
        }
    }
    for (i = 0; i < non_reciproc_allies.size() ; ++i) {
        if (non_reciproc_allies[i] != NULL) {
            non_reciproc_allies[i]->unallyWithPlayer(this);
        }
    }
}

// todo: check that we aren't duplicating colours
// ideally, the player information should be fed in from the server.
void Player::setMultiColour(const char* colour)
{
    if (strcasecmp(colour,"yellow") == 0) {
        unitpalnum = structpalnum = 0;
        return;
    } else if (strcasecmp(colour,"red") == 0) {
        unitpalnum = structpalnum = 1;
        return;
    } else if (strcasecmp(colour,"blue") == 0) {
        unitpalnum = structpalnum = 2;
        return;
    } else if (strcasecmp(colour,"orange") == 0) {
        unitpalnum = structpalnum = 3;
        return;
    } else if (strcasecmp(colour,"green") == 0) {
        unitpalnum = structpalnum = 4;
        return;
    } else if (strcasecmp(colour,"turquoise") == 0) {
        unitpalnum = structpalnum = 5;
        return;
    }
}

BQueue* Player::getQueue(Uint8 queuenum)
{
    map<Uint8, BQueue*>::iterator it = queues.find(queuenum);
    if (queues.end() == it) {
        return 0;
    }
    return it->second;
}

ConStatus Player::getStatus(UnitOrStructureType* type, Uint8* quantity, Uint8* progress)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        logger->error("No possible status for type \"%s\" as we have no primary building\n",
                type->getTName());
        return BQ_INVALID;
    }
    return queue->getStatus(type, quantity, progress);
}

bool Player::startBuilding(UnitOrStructureType *type)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        logger->error("Didn't find build queue for \"%s\" (pqueue: %i)\n",
                type->getTName(), type->getPQueue());
        return false;
    }
    return queue->Add(type);
}

ConStatus Player::stopBuilding(UnitOrStructureType *type)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        logger->error("Didn't find build queue for \"%s\" (pqueue: %i)\n",
                type->getTName(), type->getPQueue());
        return BQ_EMPTY;
    }
    return queue->PauseCancel(type);
}

void Player::placed(UnitOrStructureType *type)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        logger->error("Didn't find build queue for \"%s\" (pqueue: %i) - especially weird as we've just built it\n",
                type->getTName(), type->getPQueue());
        return;
    }
    queue->Placed();
}

void Player::builtUnit(Unit* un)
{
    unitpool.push_back(un);

    addSoB(un->getPos(), 1, 1, un->getType()->getSight(),SOB_SIGHT);

    if (defeated) {
        defeated = false;
        p::ppool->playerUndefeated(this);
    }
}

void Player::lostUnit(Unit* un, bool wasDeployed)
{
    Uint32 i;

    removeSoB(un->getPos(), 1, 1, un->getType()->getSight(), SOB_SIGHT);

    if (!wasDeployed) {
        logger->gameMsg("%s has %d structs and %d units", playername, (Uint32)structurepool.size(), (Uint32)unitpool.size()-1);
        ++unitlosses;
    }
    if( unitpool.size() <= 1 && structurepool.empty() && !wasDeployed) {
        logger->gameMsg("Player \"%s\" defeated", playername);
        defeated = true;
        p::ppool->playerDefeated(this);
    } else {
        for (i=0;i<unitpool.size();++i) {
            if (unitpool[i] == un) {
                break;
            }
        }
        for (i=i+1;i<unitpool.size();++i) {
            unitpool[i-1] = unitpool[i];
        }
        unitpool.resize(unitpool.size()-1);
    }
}

void Player::movedUnit(Uint32 oldpos, Uint32 newpos, Uint8 sight)
{
    removeSoB(oldpos, 1, 1, sight, SOB_SIGHT);
    addSoB(newpos, 1, 1, sight, SOB_SIGHT);
}

void Player::builtStruct(Structure* str)
{
    StructureType* st = (StructureType*)str->getType();
    structurepool.push_back(str);
    addSoB(str->getPos(), st->getXsize(), st->getYsize(), 2, SOB_SIGHT);//str->getType()->getSight());
    addSoB(str->getPos(), st->getXsize(), st->getYsize(), 1, SOB_BUILD);
    powerinfo_t newpower = st->getPowerInfo();
    powerGenerated += newpower.power;
    powerUsed += newpower.drain;
    structures_owned[st].push_back(str);
    if (st->primarySettable() && (0 != st->getPType())) {
        production_groups[st->getPType()].push_back(str);
        if ((production_groups[st->getPType()].size() == 1) ||
                (0 == getPrimary(str->getType()))) {
            setPrimary(str);
            queues[st->getPType()] = new BQueue(this);
        }
    }
    if (defeated) {
        defeated = false;
        p::ppool->playerUndefeated(this);
    }
    if (playernum == p::ppool->getLPlayerNum()) {
        p::ppool->updateSidebar();
        if (radarstat == 0) {
            if ((strcasecmp(((StructureType*)str->getType())->getTName(),"eye") == 0) ||
                (strcasecmp(((StructureType*)str->getType())->getTName(),"hq") == 0)  ||
                (strcasecmp(((StructureType*)str->getType())->getTName(),"dome") == 0)) {
                p::ppool->updateRadar(1);
                radarstat = 1;
            }
        }
    }
}

void Player::setPrimary(Structure* str)
{
    StructureType* st = (StructureType*)str->getType();
    if (st->primarySettable()) {
        Structure*& os = getPrimary(st);
        if (0 != os) {
            os->setPrimary(false);
        }
        os = str; // This works because os is a reference.
        str->setPrimary(true);
    }
}

void Player::lostStruct(Structure* str)
{
    StructureType* st = (StructureType*)str->getType();
    std::list<Structure*>& sto = structures_owned[st];
    Uint32 i;
    removeSoB(str->getPos(), ((StructureType*)str->getType())->getXsize(), ((StructureType*)str->getType())->getYsize(), 1, SOB_BUILD);
    powerinfo_t newpower = ((StructureType*)str->getType())->getPowerInfo();
    powerGenerated -= newpower.power;
    powerUsed -= newpower.drain;
    logger->gameMsg("%s has %d structs and %d units", playername, (Uint32)structurepool.size()-1, (Uint32)unitpool.size());
    std::list<Structure*>::iterator it = std::find(sto.begin(), sto.end(), str);
    assert(it != sto.end());
    sto.erase(it);
    if (st->primarySettable() && (st->getPType() != 0)) {
        std::list<Structure*>& prg = production_groups[st->getPType()];
        it = std::find(prg.begin(), prg.end(), str);
        assert(it != prg.end());
        prg.erase(it);
        if (str == getPrimary(str->getType())) {
            if (prg.empty()) {
                getPrimary(str->getType()) = 0;
                map<Uint8, BQueue*>::iterator it = queues.find(st->getPType());
                assert(it != queues.end());
                delete (*it).second;
                queues.erase(it);
            } else {
                setPrimary(prg.front());
            }
        }
    }
    if (playernum == p::ppool->getLPlayerNum()) {
        p::ppool->updateSidebar();
        if (radarstat == 1) {
            if ((structures_owned[p::uspool->getStructureTypeByName("eye")].empty()) &&
                (structures_owned[p::uspool->getStructureTypeByName("hq")].empty())  &&
                (structures_owned[p::uspool->getStructureTypeByName("dome")].empty())) {
                p::ppool->updateRadar(2);
                radarstat = 0;
            }
        }
    }
    ++structurelosses;

    if( unitpool.empty() && structurepool.size() <= 1 ) {
        logger->gameMsg("Player \"%s\" defeated", playername);
        defeated = true;
        p::ppool->playerDefeated(this);
    } else {
        for (i=0;i<structurepool.size();++i) {
            if (structurepool[i] == str) {
                break;
            }
        }
        for (i=i+1;i<structurepool.size();++i) {
            structurepool[i-1] = structurepool[i];
        }
        structurepool.resize(structurepool.size()-1);
    }
}

bool Player::isAllied(Player* pl) const {
    for (Uint16 i = 0; i < allies.size() ; ++i) {
        if (allies[i] == pl)
            return true;
    }
    return false;
}

void Player::updateOwner(Uint8 newnum)
{
    Uint32 i;
    for (i=0;i<unitpool.size();++i)
        unitpool[i]->setOwner(newnum);
    for (i=0;i<structurepool.size();++i)
        structurepool[i]->setOwner(newnum);
}

bool Player::allyWithPlayer(Player* pl)
{
    Uint16 i;
    if (isAllied(pl)) {
        return false;
    }
    if (unallycalls == 0) {
        allies.push_back(pl);
        pl->didAlly(this);
        return true;
    } else {
        for (i=0;i<allies.size();++i) {
            if (allies[i] == NULL) {
                allies[i] = pl;
                pl->didAlly(this);
                --unallycalls;
                return true;
            }
        }
    }
    // shouldn't get here, but in case unallycalls becomes invalid
    allies.push_back(pl);
    pl->didAlly(this);
    unallycalls = 0;
    return true;
}

bool Player::unallyWithPlayer(Player* pl)
{
    Uint16 i;
    if (pl == this) {
        return false;
    }
    for (i=0;i<allies.size();++i) {
        if (allies[i] == pl) {
            allies[i] = NULL;
            pl->didUnally(this);
            ++unallycalls;
            return true;
        }
    }
    return true;
}

void Player::didAlly(Player* pl)
{
    Uint32 i;
    if (isAllied(pl)) {
        return;
    }
    for (i=0;i<non_reciproc_allies.size();++i) {
        if (non_reciproc_allies[i] == pl) {
            // player has reciprocated alliance, remove from one-sided ally list
            non_reciproc_allies[i] = NULL;
            return;
        }
    }
    non_reciproc_allies.push_back(pl);
}

void Player::didUnally(Player* pl)
{
    if (isAllied(pl)) {
        unallyWithPlayer(pl);
    }
}

void Player::placeMultiUnits()
{
    p::uspool->createUnit("MCV",playerstart,0,playernum, 256, 0);
}

void Player::addSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode)
{
    Uint32 curpos, xsize, ysize, cpos;
    Sint32 xstart, ystart;
    std::vector<bool>* mapVoB = NULL;
    static Uint8 brad = getConfig().buildable_radius;

    if (mode == SOB_SIGHT) {
        mapVoB = &mapVisible;
    } else if (mode == SOB_BUILD) {
        mapVoB = &mapBuildable;
        sight  = brad;
    } else {
        logger->error("addSoB was given an invalid mode: %i\n", mode);
        return;
    }
    xstart = pos%p::ccmap->getWidth() - sight;
    xsize = 2*sight+width;
    if( xstart < 0 ) {
        xsize += xstart;
        xstart = 0;
    }
    if( xstart+xsize > p::ccmap->getWidth() ) {
        xsize = p::ccmap->getWidth()-xstart;
    }
    ystart = pos/p::ccmap->getWidth() - sight;
    ysize = 2*sight+height;
    if( ystart < 0 ) {
        ysize += ystart;
        ystart = 0;
    }
    if( ystart+ysize > p::ccmap->getHeight() ) {
        ysize = p::ccmap->getHeight()-ystart;
    }
    curpos = ystart*p::ccmap->getWidth()+xstart;
    for( cpos = 0; cpos < xsize*ysize; cpos++ ) {
        sightMatrix[curpos] += (mode == SOB_SIGHT);
        buildMatrix[curpos] += (mode == SOB_BUILD);
        (*mapVoB)[curpos] = true;
        curpos++;
        if (cpos%xsize == xsize-1)
            curpos += p::ccmap->getWidth()-xsize;
    }
}

void Player::removeSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode)
{
    Uint32 curpos, xsize, ysize, cpos;
    Sint32 xstart, ystart;
    static Uint16 mwid = p::ccmap->getWidth();
    static Uint8 brad = getConfig().buildable_radius;

    if (mode == SOB_BUILD) {
        sight = brad;
    }

    xstart = pos%mwid - sight;
    xsize = 2*sight+width;
    if( xstart < 0 ) {
        xsize += xstart;
        xstart = 0;
    }
    if( xstart+xsize > mwid ) {
        xsize = mwid-xstart;
    }
    ystart = pos/mwid - sight;
    ysize = 2*sight+height;
    if( ystart < 0 ) {
        ysize += ystart;
        ystart = 0;
    }
    if( ystart+ysize > p::ccmap->getHeight() ) {
        ysize = p::ccmap->getHeight()-ystart;
    }
    curpos = ystart*mwid+xstart;
    // I've done it this way to make each loop more efficient. (zx64)
    if (mode == SOB_SIGHT) {
        for( cpos = 0; cpos < xsize*ysize; cpos++ ) {
            // sightMatrix[curpos] will never be < 1 here
            sightMatrix[curpos]--;
            curpos++;
            if (cpos%xsize == xsize-1)
                curpos += mwid-xsize;
        }
    } else if (mode == SOB_BUILD && !buildany) {
        for( cpos = 0; cpos < xsize*ysize; cpos++ ) {
            if (buildMatrix[curpos] <= 1) {
                mapBuildable[curpos] = false;
                buildMatrix[curpos] = 0;
            } else {
                --buildMatrix[curpos];
            }
            curpos++;
            if (cpos%xsize == xsize-1)
                curpos += mwid-xsize;
        }
    }
}



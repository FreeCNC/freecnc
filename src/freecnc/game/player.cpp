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
    unsigned int mapsize;
    playername = cppstrdup(pname);
    multiside = 0;
    unallycalls = 0;
    playerstart = 0;
    defeated = false;

    money = mapini->readInt(playername, "Credits", 0) * 100;
    queues[0] = new BQueue(this);
    /// @TODO Only play sound if this is the local player
    counter.reset(new MoneyCounter(&money, this, &counter));

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
        // Workaround for inability to specify a side for multi
        // playerside = PS_MULTI; // actual value set later
        playerside = PS_GOOD;
        if (playername[5] < 49 || playername[5] > 57) {
            game.log << "Invalid builtin multi name: " << playername << "" << endl;
            /// @TODO Nicer error handling here
            multiside = 9;
        } else {
            multiside = playername[5] - 48;
        }
        /*
        if (multiside > 0) {
            unitpalnum = multiside - 1;
            structpalnum = multiside - 1;
        } else {
            unitpalnum = structpalnum = 0;
        }*/
        unitpalnum = 3;
        structpalnum = 3;
        playerstart = p::ppool->getAStart();
        if (playerstart != 0) {
            // conversion algorithm from loadmap.cpp
            playerstart = p::ccmap->normaliseCoord(playerstart);
        }
        if (money == 0) {
            money = 10000;
        }
    } else {
        game.log << "Player Side \"" << pname << "\" not recognised, using gdi instead" << endl;
        playerside = PS_GOOD;
        unitpalnum = 0;
        structpalnum = 0;
    }
    powerGenerated = powerUsed = radarstat = 0;
    unitkills = unitlosses = structurekills = structurelosses = 0;

    mapsize = p::ccmap->getWidth()*p::ccmap->getHeight();
    sightMatrix.resize(mapsize);
    buildMatrix.resize(mapsize);
    mapVisible.resize(mapsize);
    mapBuildable.resize(mapsize);

    allmap = buildall = buildany = infmoney = false;
}

Player::~Player()
{
    delete[] playername;
    map<unsigned char, BQueue*>::iterator i, end;
    end = queues.end();
    for (i = queues.begin(); i != end; ++i) {
        delete i->second;
    }
}

bool Player::changeMoney(int change) {
    if (0 == change) {
        return true;
    } else if (change > 0) {
        counter->addCredit(change);
        return true;
    } else {
        int after = money + change - counter->getDebt();
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
        for (unsigned short i=0;i<allies_n.size();++i) {
            if (strcasecmp(allies_n[i],playername)) {
                allyWithPlayer(p::ppool->getPlayerByName(allies_n[i]));
            }
            delete[] allies_n[i];
        }
    } else {
        for (unsigned short i=0;i<allies_n.size();++i) {
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
    unsigned int i;
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

BQueue* Player::getQueue(unsigned char queuenum)
{
    map<unsigned char, BQueue*>::iterator it = queues.find(queuenum);
    if (queues.end() == it) {
        return 0;
    }
    return it->second;
}

ConStatus Player::getStatus(UnitOrStructureType* type, unsigned char* quantity, unsigned char* progress)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        game.log << "No possible status for type \"" << type->getTName() << "\" as we have no primary building"  << endl;
        return BQ_INVALID;
    }
    return queue->getStatus(type, quantity, progress);
}

bool Player::startBuilding(UnitOrStructureType *type)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        game.log << "Didn't find build queue for \"" << type->getTName() << "\" (pqueue: " << type->getPQueue() << ")" << endl;
        return false;
    }
    return queue->Add(type);
}

ConStatus Player::stopBuilding(UnitOrStructureType *type)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        game.log << "Didn't find build queue for \"" << type->getTName() << "\" (pqueue: " << type->getPQueue() << ")" << endl;
        return BQ_EMPTY;
    }
    return queue->PauseCancel(type);
}

void Player::placed(UnitOrStructureType *type)
{
    BQueue* queue = getQueue(type->getPQueue());
    if (0 == queue) {
        game.log << "Didn't find build queue for \"" << type->getTName() << "\" (pqueue: " << type->getPQueue() << ")" << endl;
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
    unsigned int i;

    removeSoB(un->getPos(), 1, 1, un->getType()->getSight(), SOB_SIGHT);

    if (!wasDeployed) {
        logger->gameMsg("%s has %d structs and %d units", playername, (unsigned int)structurepool.size(), (unsigned int)unitpool.size()-1);
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

void Player::movedUnit(unsigned int oldpos, unsigned int newpos, unsigned char sight)
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
    unsigned int i;
    removeSoB(str->getPos(), ((StructureType*)str->getType())->getXsize(), ((StructureType*)str->getType())->getYsize(), 1, SOB_BUILD);
    powerinfo_t newpower = ((StructureType*)str->getType())->getPowerInfo();
    powerGenerated -= newpower.power;
    powerUsed -= newpower.drain;
    logger->gameMsg("%s has %d structs and %d units", playername, (unsigned int)structurepool.size()-1, (unsigned int)unitpool.size());
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
                map<unsigned char, BQueue*>::iterator it = queues.find(st->getPType());
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
    for (unsigned short i = 0; i < allies.size() ; ++i) {
        if (allies[i] == pl)
            return true;
    }
    return false;
}

void Player::updateOwner(unsigned char newnum)
{
    unsigned int i;
    for (i=0;i<unitpool.size();++i)
        unitpool[i]->setOwner(newnum);
    for (i=0;i<structurepool.size();++i)
        structurepool[i]->setOwner(newnum);
}

bool Player::allyWithPlayer(Player* pl)
{
    unsigned short i;
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
    unsigned short i;
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
    unsigned int i;
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

void Player::addSoB(unsigned int pos, unsigned char width, unsigned char height, unsigned char sight, SOB_update mode)
{
    unsigned int curpos, xsize, ysize, cpos;
    int xstart, ystart;
    std::vector<bool>* mapVoB = NULL;
    static unsigned char buildable_radius = game.config.buildable_radius;

    if (mode == SOB_SIGHT) {
        mapVoB = &mapVisible;
    } else if (mode == SOB_BUILD) {
        mapVoB = &mapBuildable;
        sight  = buildable_radius;
    } else {
        game.log << "addSoB was given an invalid mode: " << mode << endl;
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

void Player::removeSoB(unsigned int pos, unsigned char width, unsigned char height, unsigned char sight, SOB_update mode)
{
    unsigned int curpos, xsize, ysize, cpos;
    int xstart, ystart;
    static unsigned short mwid = p::ccmap->getWidth();
    static unsigned char buildable_radius = game.config.buildable_radius;

    if (mode == SOB_BUILD) {
        sight = buildable_radius;
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



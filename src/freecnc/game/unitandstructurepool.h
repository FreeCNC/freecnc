#ifndef _GAME_UNITANDSTRUCTUREPOOL_H
#define _GAME_UNITANDSTRUCTUREPOOL_H

#include <set>
#include "../freecnc.h"

class INIFile;
class Player;
class Structure;
class StructureType;
class Talkback;
class UnitOrStructure;
class UnitOrStructureType;
class Unit;
class UnitType;

#define US_LOWER_RIGHT  0x80000000
#define US_IS_UNIT      0x40000000
#define US_IS_STRUCTURE 0x20000000
#define US_IS_WALL      0x10000000

#define US_MOVING_HERE  0x07000000

#define US_HAS_L2OVERLAY 0x08000000
//#define US_HAS_PROJECTILE 0x00800000
//#define US_HAS_HIGHPROJ   0x00400000
//#define US_HAS_EXPLOTION  0x00200000

//#define US_HAS_AIRUNIT

struct L2Overlay
{
    L2Overlay(Uint8 numimages);
    Uint8 getImages(Uint32** images, Sint8** xoffs, Sint8** yoffs);
    std::vector<Uint32> imagenums;
    Uint16 cellpos;
    std::vector<Sint8> xoffsets;
    std::vector<Sint8> yoffsets;
    Uint8 numimages;
};

/// Stores all units and structures.
/// Handles level two overlays.
class UnitAndStructurePool {
public:
    UnitAndStructurePool();
    ~UnitAndStructurePool();

    Uint8 getUnitOrStructureNum(Uint16 cellpos, Uint32 **inumbers,
                                Sint8 **xoffsets, Sint8 **yoffsets);
    // Retrieve a limited amount of information from the cell.
    bool getUnitOrStructureLimAt(Uint32 curpos, float* width, float* height,
                                 Uint32* cellpos, Uint8* igroup, Uint8* owner,
                                 Uint8* pcol, bool* blocked);
    bool hasL2overlay(Uint16 cellpos) const {
        return (unitandstructmat[cellpos]&US_HAS_L2OVERLAY)!=0;
    }
    Uint8 getL2overlays(Uint16 cellpos, Uint32 **inumbers, Sint8 **xoddset, Sint8 **yoffset);
    std::multimap<Uint16, L2Overlay*>::iterator addL2overlay(Uint16 cellpos, L2Overlay *ov);
    void removeL2overlay(std::multimap<Uint16, L2Overlay*>::iterator entry);

    bool createStructure(const char* typen, Uint16 cellpos, Uint8 owner,
            Uint16 health, Uint8 facing, bool makeanim );
    bool createStructure(StructureType* type, Uint16 cellpos, Uint8 owner,
            Uint16 health, Uint8 facing, bool makeanim );
    bool createUnit(const char* typen, Uint16 cellpos, Uint8 subpos,
            Uint8 owner, Uint16 health, Uint8 facing);
    bool createUnit(UnitType* type, Uint16 cellpos, Uint8 subpos,
            Uint8 owner, Uint16 health, Uint8 facing);

    bool spawnUnit(const char* typen, Uint8 owner);
    bool spawnUnit(UnitType* type, Uint8 owner);

    Unit* getUnitAt(Uint32 cell, Uint8 subcell);
    Unit* getUnit(Uint32 num);
    Structure* getStructureAt(Uint32 cell);
    Structure* getStructureAt(Uint32 cell, bool wall);
    Structure* getStructure(Uint32 num);
    UnitOrStructure* getUnitOrStructureAt(Uint32 cell, Uint8 subcell, bool wall = false);
    InfantryGroupPtr getInfantryGroupAt(Uint32 cell);
    Uint16 getSelected(Uint32 pos);
    Uint16 preMove(Unit *un, Uint8 dir, Sint8 *xmod, Sint8 *ymod);
    Uint8 postMove(Unit *un, Uint16 newpos);
    void abortMove(Unit* un, Uint32 pos);
    UnitType* getUnitTypeByName(const char* unitname);
    StructureType *getStructureTypeByName(const char *structname);
    UnitOrStructureType* getTypeByName(const char* typen);
    bool freeTile(Uint16 pos) const {
        return (unitandstructmat[pos]&0x70000000)==0;
    }
    Uint16 getTileCost(Uint16 pos, Unit* excpUn) const;
    bool tileAboutToBeUsed(Uint16 pos) const;
    void setCostCalcOwnerAndType(Uint8 owner, Uint8 type)
    {
        costcalcowner = owner;
        costcalctype = type;
    }
    void removeUnit(Unit *un);
    void removeStructure(Structure *st);
    bool hasDeleted()
    {
        bool retval = deleted_unitorstruct;
        deleted_unitorstruct = false;
        return retval;
    }
    void showMoves();

    // techtree code
    void addPrerequisites(UnitType* unittype);
    void addPrerequisites(StructureType* structtype);

    /// scans both inifiles for things with techlevel <= that of the parameter
    /// then retrives those types.
    void preloadUnitAndStructures(Uint8 techlevel);

    /// Generate reverse dependency information from what units we have loaded.
    void generateProductionGroups();

    // these get passed to the sidebar
    std::vector<const char*> getBuildableUnits(Player* pl);
    std::vector<const char*> getBuildableStructures(Player* pl);

    // unit is removed from map (to be stored in transport)
    void hideUnit(Unit* un);
    Uint8 unhideUnit(Unit* un, Uint16 newpos, bool unload);

    shared_ptr<Talkback> getTalkback(const char* talkback);
private:
    char theaterext[5];

    std::vector<Uint32> unitandstructmat;

    std::vector<Structure *> structurepool;
    std::vector<StructureType *> structuretypepool;
    std::map<std::string, Uint16> structname2typenum;

    std::vector<Unit *> unitpool;
    std::vector<UnitType *> unittypepool;
    std::map<std::string, Uint16> unitname2typenum;

    std::multimap<Uint16, L2Overlay*> l2pool;
    std::map<Uint16, Uint16> numl2images;

    std::multimap<StructureType*, std::vector<StructureType*>* > struct_prereqs;
    std::multimap<UnitType*, std::vector<StructureType*>* > unit_prereqs;
    void splitORPreReqs(const char* prereqs, std::vector<StructureType*>* type_prereqs);

    std::map<std::string, shared_ptr<Talkback> > talkbackpool;

    shared_ptr<INIFile> structini, unitini, tbackini, artini;

    Uint8 costcalcowner;
    Uint8 costcalctype;

    bool deleted_unitorstruct;
    Uint16 numdeletedunit;
    Uint16 numdeletedstruct;
    void updateWalls(Structure* st, bool add);
};

#endif

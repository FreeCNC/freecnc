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
    L2Overlay(unsigned char numimages);
    unsigned char getImages(unsigned int** images, char** xoffs, char** yoffs);
    std::vector<unsigned int> imagenums;
    unsigned short cellpos;
    std::vector<char> xoffsets;
    std::vector<char> yoffsets;
    unsigned char numimages;
};

/// Stores all units and structures.
/// Handles level two overlays.
class UnitAndStructurePool {
public:
    UnitAndStructurePool();
    ~UnitAndStructurePool();

    unsigned char getUnitOrStructureNum(unsigned short cellpos, unsigned int **inumbers,
                                char **xoffsets, char **yoffsets);
    // Retrieve a limited amount of information from the cell.
    bool getUnitOrStructureLimAt(unsigned int curpos, float* width, float* height,
                                 unsigned int* cellpos, unsigned char* igroup, unsigned char* owner,
                                 unsigned char* pcol, bool* blocked);
    bool hasL2overlay(unsigned short cellpos) const {
        return (unitandstructmat[cellpos]&US_HAS_L2OVERLAY)!=0;
    }
    unsigned char getL2overlays(unsigned short cellpos, unsigned int **inumbers, char **xoddset, char **yoffset);
    std::multimap<unsigned short, L2Overlay*>::iterator addL2overlay(unsigned short cellpos, L2Overlay *ov);
    void removeL2overlay(std::multimap<unsigned short, L2Overlay*>::iterator entry);

    bool createStructure(const char* typen, unsigned short cellpos, unsigned char owner,
            unsigned short health, unsigned char facing, bool makeanim );
    bool createStructure(StructureType* type, unsigned short cellpos, unsigned char owner,
            unsigned short health, unsigned char facing, bool makeanim );
    bool createUnit(const char* typen, unsigned short cellpos, unsigned char subpos,
            unsigned char owner, unsigned short health, unsigned char facing);
    bool createUnit(UnitType* type, unsigned short cellpos, unsigned char subpos,
            unsigned char owner, unsigned short health, unsigned char facing);

    bool spawnUnit(const char* typen, unsigned char owner);
    bool spawnUnit(UnitType* type, unsigned char owner);

    Unit* getUnitAt(unsigned int cell, unsigned char subcell);
    Unit* getUnit(unsigned int num);
    Structure* getStructureAt(unsigned int cell);
    Structure* getStructureAt(unsigned int cell, bool wall);
    Structure* getStructure(unsigned int num);
    UnitOrStructure* getUnitOrStructureAt(unsigned int cell, unsigned char subcell, bool wall = false);
    InfantryGroupPtr getInfantryGroupAt(unsigned int cell);
    unsigned short getSelected(unsigned int pos);
    unsigned short preMove(Unit *un, unsigned char dir, char *xmod, char *ymod);
    unsigned char postMove(Unit *un, unsigned short newpos);
    void abortMove(Unit* un, unsigned int pos);
    UnitType* getUnitTypeByName(const char* unitname);
    StructureType *getStructureTypeByName(const char *structname);
    UnitOrStructureType* getTypeByName(const char* typen);
    bool freeTile(unsigned short pos) const {
        return (unitandstructmat[pos]&0x70000000)==0;
    }
    unsigned short getTileCost(unsigned short pos, Unit* excpUn) const;
    bool tileAboutToBeUsed(unsigned short pos) const;
    void setCostCalcOwnerAndType(unsigned char owner, unsigned char type)
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
    void preloadUnitAndStructures(unsigned char techlevel);

    /// Generate reverse dependency information from what units we have loaded.
    void generateProductionGroups();

    // these get passed to the sidebar
    std::vector<const char*> getBuildableUnits(Player* pl);
    std::vector<const char*> getBuildableStructures(Player* pl);

    // unit is removed from map (to be stored in transport)
    void hideUnit(Unit* un);
    unsigned char unhideUnit(Unit* un, unsigned short newpos, bool unload);

    shared_ptr<Talkback> getTalkback(const char* talkback);
private:
    char theaterext[5];

    std::vector<unsigned int> unitandstructmat;

    std::vector<Structure *> structurepool;
    std::vector<StructureType *> structuretypepool;
    std::map<std::string, unsigned short> structname2typenum;

    std::vector<Unit *> unitpool;
    std::vector<UnitType *> unittypepool;
    std::map<std::string, unsigned short> unitname2typenum;

    std::multimap<unsigned short, L2Overlay*> l2pool;
    std::map<unsigned short, unsigned short> numl2images;

    std::multimap<StructureType*, std::vector<StructureType*>* > struct_prereqs;
    std::multimap<UnitType*, std::vector<StructureType*>* > unit_prereqs;
    void splitORPreReqs(const char* prereqs, std::vector<StructureType*>* type_prereqs);

    std::map<std::string, shared_ptr<Talkback> > talkbackpool;

    shared_ptr<INIFile> structini, unitini, tbackini, artini;

    unsigned char costcalcowner;
    unsigned char costcalctype;

    bool deleted_unitorstruct;
    unsigned short numdeletedunit;
    unsigned short numdeletedstruct;
    void updateWalls(Structure* st, bool add);
};

#endif

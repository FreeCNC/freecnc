#ifndef _GAME_WEAPONSPOOL_H
#define _GAME_WEAPONSPOOL_H

#include "../freecnc.h"

class INIFile;
class UnitOrStructure;

class Warhead
{
public:
    Warhead(const char *whname, shared_ptr<INIFile> weapini);
    ~Warhead();
    //void getExplosion(Uint32 &image, Uint8 &steps){image = explosionimage; steps = explosionanimsteps;}
    Uint32 getEImage()
    {
        return explosionimage;
    }
    Uint8 getESteps()
    {
        return explosionanimsteps;
    }
    const char *getExplosionsound()
    {
        return explosionsound;
    }
    bool getWall()
    {
        return walls;
    }
    Uint8 getVersus(armour_t armour)
    {
        return versus[(Uint8)armour];
    }

private:
    //Uint8 explosiontype;
    Uint32 explosionimage;
    Uint8 explosionanimsteps;
    char *explosionsound;
    Uint8 infantrydeath;
    Uint8 blastradius;
    unsigned int versus[5];
    bool walls;
    bool trees;
    //Uint16 damage;
};

class Projectile
{
public:
    Projectile(const char *pname, shared_ptr<INIFile> weapini);
    ~Projectile();
    Uint32 getImageNum()
    {
        return imagenum;
    }
    //Uint8 getSpeed(){return speed;}
    bool doesRotate()
    {
        return rotates;
    }

private:
    Uint32 imagenum;
    Uint8 rotationimgs;
    bool AA;
    bool AG;
    //Uint8 speed;
    bool high, inacurate, rotates;
};

class Weapon
{
public:
    Weapon(const char* wname);
    ~Weapon();
    Uint8 getReloadTime() const
    {
        return reloadtime;
    }
    Uint8 getRange() const
    {
        return range;
    }
    Uint8 getSpeed() const
    {
        return speed;
    }
    Sint16 getDamage() const
    {
        return damage;
    }
    bool getWall() const
    {
        return whead->getWall();
    }
    Projectile *getProjectile()
    {
        return projectile;
    }
    Warhead *getWarhead()
    {
        return whead;
    }
    void fire(UnitOrStructure* owner, Uint16 target, Uint8 subtarget);
    //Uint32 tmppif;
    bool isHeatseek() const
    {
        return heatseek;
    }
    bool isInaccurate() const
    {
        return inaccurate;
    }
    double getVersus(armour_t armour) const
    {
        return (whead->getVersus(armour))/(double)100.0;
    }
    Uint8 getFuel() const
    {
        return fuel;
    }
    Uint8 getSeekFuel() const
    {
        return seekfuel;
    }
    const char* getName() const
    {
        return name.c_str();
    }

private:
    Weapon() {};
    Projectile *projectile;
    Warhead *whead;
    Uint8 speed;
    Uint8 range;
    Uint8 reloadtime;
    Sint16 damage;
    Uint8 burst;
    // Fuel - how many ticks this projectile can move for until being removed.
    // Seekfuel - how many ticks can this projectile change course to track its
    // target before falling back to flying in a straight line.
    Uint8 fuel, seekfuel;
    bool heatseek,inaccurate;
    Uint32 fireimage;
    Uint32* fireimages;
    Uint8 numfireimages,numfiredirections;
    char *firesound;
    std::string name;
};

class WeaponsPool
{
public:
    friend class Weapon;
    friend class Projectile;
    friend class Warhead;
    WeaponsPool();
    ~WeaponsPool();
    Weapon *getWeapon(const char *wname);
    shared_ptr<INIFile> getWeaponsINI()
    {
        return weapini;
    }

private:
    std::map<std::string, Weapon*> weaponspool;
    std::map<std::string, Projectile*> projectilepool;
    std::map<std::string, Warhead*> warheadpool;
    shared_ptr<INIFile> weapini;
};

#endif

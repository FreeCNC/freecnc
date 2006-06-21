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
    //void getExplosion(unsigned int &image, unsigned char &steps){image = explosionimage; steps = explosionanimsteps;}
    unsigned int getEImage()
    {
        return explosionimage;
    }
    unsigned char getESteps()
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
    unsigned char getVersus(armour_t armour)
    {
        return versus[(unsigned char)armour];
    }

private:
    //unsigned char explosiontype;
    unsigned int explosionimage;
    unsigned char explosionanimsteps;
    char *explosionsound;
    unsigned char infantrydeath;
    unsigned char blastradius;
    unsigned int versus[5];
    bool walls;
    bool trees;
    //unsigned short damage;
};

class Projectile
{
public:
    Projectile(const char *pname, shared_ptr<INIFile> weapini);
    ~Projectile();
    unsigned int getImageNum()
    {
        return imagenum;
    }
    //unsigned char getSpeed(){return speed;}
    bool doesRotate()
    {
        return rotates;
    }

private:
    unsigned int imagenum;
    unsigned char rotationimgs;
    bool AA;
    bool AG;
    //unsigned char speed;
    bool high, inacurate, rotates;
};

class Weapon
{
public:
    Weapon(const char* wname);
    ~Weapon();
    unsigned char getReloadTime() const
    {
        return reloadtime;
    }
    unsigned char getRange() const
    {
        return range;
    }
    unsigned char getSpeed() const
    {
        return speed;
    }
    short getDamage() const
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
    void fire(UnitOrStructure* owner, unsigned short target, unsigned char subtarget);
    //unsigned int tmppif;
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
    unsigned char getFuel() const
    {
        return fuel;
    }
    unsigned char getSeekFuel() const
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
    unsigned char speed;
    unsigned char range;
    unsigned char reloadtime;
    short damage;
    unsigned char burst;
    // Fuel - how many ticks this projectile can move for until being removed.
    // Seekfuel - how many ticks can this projectile change course to track its
    // target before falling back to flying in a straight line.
    unsigned char fuel, seekfuel;
    bool heatseek,inaccurate;
    unsigned int fireimage;
    unsigned int* fireimages;
    unsigned char numfireimages,numfiredirections;
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

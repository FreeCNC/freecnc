#ifndef _GAME_PROJECTILEANIM_H
#define _GAME_PROJECTILEANIM_H

#include "../freecnc.h"
#include "actioneventqueue.h"

struct L2Overlay;
class UnitOrStructure;
class Weapon;

class ExplosionAnim : public ActionEvent
{
public:
    ExplosionAnim(unsigned int p, unsigned short pos, unsigned int startimage, unsigned char animsteps,
                  char xoff, char yoff);
    bool run();
    void finish();
private:
    ExplosionAnim() : ActionEvent(1) {};

    L2Overlay *l2o;
    unsigned short pos;
    unsigned char animsteps;
    std::multimap<unsigned short, L2Overlay*>::iterator l2entry;
};

class ProjectileAnim : public ActionEvent
{
public:
    ProjectileAnim(unsigned int p, Weapon *weap, UnitOrStructure* owner, unsigned short dest, unsigned char subdest);
    ~ProjectileAnim();
    bool run();
    void finish();
private:
    Weapon* weap;
    UnitOrStructure* owner;
    UnitOrStructure* target;
    unsigned short dest;
    unsigned char subdest;
    // Fuel - how many ticks left until projectile is removed.
    // Seekfuel - how many ticks left until this projectile change course
    // to track its target before falling back to flying in a straight line.
    unsigned char fuel, seekfuel;
    char xoffset;
    char yoffset;
    //int xmod, ymod;
    L2Overlay *l2o;
    std::multimap<unsigned short, L2Overlay*>::iterator l2entry;
    double xdiff, ydiff;
    double xmod, ymod, rxoffs, ryoffs;
    bool heatseek,inaccurate,fuelled;
    unsigned char facing;
};

#endif /* PROJECTILEANIM_H */

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
    ExplosionAnim(Uint32 p, Uint16 pos, Uint32 startimage, Uint8 animsteps,
                  Sint8 xoff, Sint8 yoff);
    ~ExplosionAnim();
    void run();
private:
    ExplosionAnim() : ActionEvent(1) {};

    L2Overlay *l2o;
    Uint16 pos;
    Uint8 animsteps;
    std::multimap<Uint16, L2Overlay*>::iterator l2entry;
};

class ProjectileAnim : public ActionEvent
{
public:
    ProjectileAnim(Uint32 p, Weapon *weap, UnitOrStructure* owner, Uint16 dest, Uint8 subdest);
    ~ProjectileAnim();
    void run();
private:
    Weapon* weap;
    UnitOrStructure* owner;
    UnitOrStructure* target;
    Uint16 dest;
    Uint8 subdest;
    // Fuel - how many ticks left until projectile is removed.
    // Seekfuel - how many ticks left until this projectile change course
    // to track its target before falling back to flying in a straight line.
    Uint8 fuel, seekfuel;
    Sint8 xoffset;
    Sint8 yoffset;
    //Sint32 xmod, ymod;
    L2Overlay *l2o;
    std::multimap<Uint16, L2Overlay*>::iterator l2entry;
    double xdiff, ydiff;
    double xmod, ymod, rxoffs, ryoffs;
    bool heatseek,inaccurate,fuelled;
    Uint8 facing;
};

#endif /* PROJECTILEANIM_H */

#ifndef _GAME_MONEYCOUNTER_H
#define _GAME_MONEYCOUNTER_H

#include "../freecnc.h"
#include "actioneventqueue.h"

class Player;
class MoneyCounter;

class MoneyCounter : public ActionEvent
{
public:
    MoneyCounter(Sint32*, Player*, MoneyCounter**);
    ~MoneyCounter();
    void run();
    Uint16 getDebt() const {return debtleft;}
    void addCredit(Uint16 amount);
    void addDebt(Uint16 amount);
    bool isScheduled() const {return queued;}

private:
    static const Uint8 delta = 5;

    Sint32& money;
    Player* player;
    bool queued;
    // Seperate because we want both credit and debit sounds being played
    Uint16 creditleft, debtleft;
    Sint32 creditsound, debitsound;

    Uint8 step(Uint16& value);
    bool sound;

    MoneyCounter** backref;

    void reshedule();
};

#endif

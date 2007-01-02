#ifndef _GAME_MONEYCOUNTER_H
#define _GAME_MONEYCOUNTER_H

#include "../freecnc.h"
#include "actioneventqueue.h"

class Player;
class MoneyCounter;

class MoneyCounter : public ActionEvent
{
public:
    MoneyCounter(int*, Player*, shared_ptr<MoneyCounter>*);
    bool run();
    unsigned short getDebt() const {return debtleft;}
    void addCredit(unsigned short amount);
    void addDebt(unsigned short amount);
    bool isScheduled() const {return queued;}

private:
    static const unsigned char delta = 5;

    int& money;
    Player* player;
    bool queued;
    // Seperate because we want both credit and debit sounds being played
    unsigned short creditleft, debtleft;
    int creditsound, debitsound;

    unsigned char step(unsigned short& value);
    bool sound;

    shared_ptr<MoneyCounter>* backref;

    void reschedule();
};

#endif

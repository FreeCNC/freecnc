#include <cassert>

#include "../sound/sound_public.h"
#include "moneycounter.h"
#include "player.h"

MoneyCounter::MoneyCounter(int* money, Player* player, shared_ptr<MoneyCounter>* backref)
    : ActionEvent(1), money(*money), player(player), queued(false), creditleft(0), debtleft(0), creditsound(-1), debitsound(-1), sound(true), backref(backref)
{
}

bool MoneyCounter::run()
{
    queued = false;
    unsigned char Dcred = step(creditleft);
    unsigned char Ddebt = step(debtleft);
    if (Dcred > 0) {
        money += Dcred;
    } else if (sound && creditsound != -1) {
        pc::sfxeng->StopLoopedSound(creditsound);
        creditsound = -1;
    }
    if (Ddebt > 0) {
        if (!player->hasInfMoney()) {
            assert(money - Ddebt >= 0);
        }
        money -= Ddebt;
    } else if (sound && debitsound != -1) {
        pc::sfxeng->StopLoopedSound(debitsound);
        debitsound = -1;
    }

    if (Dcred > 0 || Ddebt > 0) {
        queued = true;
        return true;
    }
    return false;
}

void MoneyCounter::addCredit(unsigned short amount)
{
    creditleft += amount;
    if (sound && -1 == creditsound) {
        /// @TODO Get this value from a global config object
        creditsound = pc::sfxeng->PlayLoopedSound("tone15.aud",0);
    }
    reschedule();
}

void MoneyCounter::addDebt(unsigned short amount)
{
    debtleft += amount;
    if (sound && -1 == debitsound) {
        /// @TODO Get this value from a global config object
        debitsound = pc::sfxeng->PlayLoopedSound("tone16.aud",0);
    }
    reschedule();
}

unsigned char MoneyCounter::step(unsigned short& value)
{
    if (value == 0)
        return 0;

    if (value < delta) {
        unsigned char oldvalue = static_cast<unsigned char>(value);
        value = 0;
        return oldvalue;
    } else {
        value -= delta;
        return delta;
    }
}

void MoneyCounter::reschedule() {
    if (!queued) {
        queued = true;
        p::aequeue->scheduleEvent(*backref);
    }
}

#include <cassert>

#include "../sound/sound_public.h"
#include "moneycounter.h"
#include "player.h"

MoneyCounter::MoneyCounter(Sint32* money, Player* player, MoneyCounter** backref)
    : ActionEvent(1), money(*money), player(player), queued(false), creditleft(0), debtleft(0), creditsound(-1), debitsound(-1), sound(true), backref(backref)
{
}

MoneyCounter::~MoneyCounter()
{
    *backref = 0;
}

void MoneyCounter::run()
{
    queued = false;
    Uint8 Dcred = step(creditleft);
    Uint8 Ddebt = step(debtleft);
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
        p::aequeue->scheduleEvent(this);
        queued = true;
    }
}

void MoneyCounter::addCredit(Uint16 amount)
{
    creditleft += amount;
    if (sound && -1 == creditsound) {
        /// @TODO Get this value from a global config object
        creditsound = pc::sfxeng->PlayLoopedSound("tone15.aud",0);
    }
    reshedule();
}

void MoneyCounter::addDebt(Uint16 amount)
{
    debtleft += amount;
    if (sound && -1 == debitsound) {
        /// @TODO Get this value from a global config object
        debitsound = pc::sfxeng->PlayLoopedSound("tone16.aud",0);
    }
    reshedule();
}

Uint8 MoneyCounter::step(Uint16& value)
{
    if (value == 0)
        return 0;

    if (value < delta) {
        Uint8 oldvalue = static_cast<Uint8>(value);
        value = 0;
        return oldvalue;
    } else {
        value -= delta;
        return delta;
    }
}

void MoneyCounter::reshedule() {
    if (!queued) {
        queued = true;
        p::aequeue->scheduleEvent(this);
    }
}

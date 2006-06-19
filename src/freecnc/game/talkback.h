#ifndef _GAME_TALKBACK_H
#define _GAME_TALKBACK_H

#include "../freecnc.h"

enum TalkbackType {
    TB_report, TB_ack, TB_atkun, TB_atkst, TB_die, TB_postkill, TB_invalid
};

class Talkback
{
public:
    Talkback();
    void load(std::string talkback, shared_ptr<INIFile> tbini);
    const char* getRandTalk(TalkbackType type);

private:
    Talkback(const Talkback&);
    Talkback& operator=(const Talkback&);

    static std::map<std::string, TalkbackType> talktype;
    static bool talktype_init;

    typedef std::map<TalkbackType, std::vector<std::string> > t_talkstore;
    t_talkstore talkstore;

    void merge(shared_ptr<Talkback> mergee);
    std::vector<std::string>& getTypeVector(TalkbackType type);
    TalkbackType getTypeNum(std::string name);
};

#endif

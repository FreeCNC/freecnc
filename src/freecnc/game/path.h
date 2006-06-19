#ifndef _GAME_PATH_H
#define _GAME_PATH_H

#include <stack>
#include "../freecnc.h"

/* empty, push, pop, top */
class Path : public std::stack<Uint8>
{
public:
    Path(Uint32 crBeg, Uint32 crEnd, Uint8 max_dist);
    ~Path();
};

#endif

#ifndef _GAME_PATH_H
#define _GAME_PATH_H

#include <stack>
#include "../freecnc.h"

/* empty, push, pop, top */
class Path : public std::stack<unsigned char>
{
public:
    Path(unsigned int crBeg, unsigned int crEnd, unsigned char max_dist);
    ~Path();
};

#endif

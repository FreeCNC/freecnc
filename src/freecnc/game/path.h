#ifndef _GAME_PATH_H
#define _GAME_PATH_H

#include <stack>

class Path
{
public:
    Path(unsigned int crBeg, unsigned int crEnd, unsigned char max_dist);

    bool empty() const { return result.empty(); }
    unsigned char top() const { return result.top(); }
    void pop() { result.pop(); }
private:
    std::stack<unsigned char> result;
};

#endif

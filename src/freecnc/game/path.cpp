#include <cstdlib>
#include <queue>

#include "map.h"
#include "path.h"

namespace
{
    struct TileRef
    {
        unsigned short x;
        unsigned short y;
        unsigned int g;
        unsigned int h;
    };

    class Node
    {
        public:
            friend class NodeComp;
            Node(TileRef *val, unsigned int k) {
                value = val;
                key = k;
            }
            TileRef *getValue() {
                return value;
            }
            void setKey(unsigned int k) {
                key = k;
            }
        private:
            TileRef *value;
            unsigned int key;
    };

    struct NodeComp {
            bool operator()(Node* x, Node* y) {
                return x->key > y->key;
            }
    };

    const int DIAGONAL = 14;
    const int STRAIGHT = 10;
}

Path::Path(unsigned int start_pos, unsigned int end_pos, unsigned char max_dist)
{
    if (start_pos == end_pos) {
        return;
    }
    unsigned short start_pos_x, start_pos_y;
    p::ccmap->translateFromPos(start_pos, &start_pos_x, &start_pos_y);

    unsigned short stop_pos_x, stop_pos_y;
    p::ccmap->translateFromPos(end_pos, &stop_pos_x, &stop_pos_y);

    Node *current_node, *next_node;
    Node *reusable_node;
    Node *closed;
    std::priority_queue<Node*, std::vector<Node*>, NodeComp> open;
    unsigned int current_pos, next_pos;
    unsigned short current_x, current_y, next_x, next_y;
    unsigned int cost;
    TileRef *current_node_values, *next_node_values;
    unsigned short diffx, diffy;
    unsigned char current_direction;
    unsigned int current_min_pos;
    unsigned int current_min_h;

    unsigned short mapsize = p::ccmap->getWidth()*p::ccmap->getHeight();

    current_pos = next_pos = 0;
    next_x = next_y = 0;

    Node** nodes = new Node*[mapsize];
    memset(nodes, 0, mapsize*sizeof(Node*));

    unsigned char *directions = new unsigned char[mapsize];

    current_node_values = new TileRef;
    current_node_values->x = start_pos_x;
    current_node_values->y = start_pos_y;
    current_node_values->g = 0;
    diffx = abs(start_pos_x - stop_pos_x);
    diffy = abs(start_pos_y - stop_pos_y);
    current_node_values->h = min(diffx, diffy) * DIAGONAL + abs( diffx - diffy) * STRAIGHT;

    nodes[start_pos] = current_node = new Node(current_node_values, current_node_values->h);

    current_min_pos = start_pos;
    current_min_h = current_node_values->h;

    open.push(current_node);

    closed = new Node(NULL, 0);
    reusable_node = NULL;

    while (!open.empty()) {
        current_node = open.top();
        open.pop();
        current_node_values = current_node->getValue();
        current_x = current_node_values->x;
        current_y = current_node_values->y;
        current_pos = p::ccmap->translateToPos(current_x, current_y);

        if (current_node_values->h < current_min_h) {
            current_min_h = current_node_values->h;
            current_min_pos = current_pos;
        }

        if (current_node_values->h <= ((unsigned int)max_dist) * STRAIGHT || current_node_values->g >= 100) {
            /* desired min dist to target, 0 to go all the way. Currently the length of the path is limited to 100 */
            break;
        }

        /* Walk in all directions */
        for (current_direction = 0; current_direction < 8; current_direction++) {
            switch (current_direction) {
            case 0:
                if (current_y == 0) {
                    continue;
                }
                next_x = current_x;
                next_y = current_y-1;
                next_pos = current_pos - p::ccmap->getWidth();
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * STRAIGHT;
                break;
            case 1:
                if (current_y == 0 || current_x == (p::ccmap->getWidth()-1)) {
                    continue;
                }
                next_x = current_x+1;
                next_y = current_y-1;
                next_pos = current_pos - p::ccmap->getWidth() + 1;
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * DIAGONAL;
                break;
            case 2:
                if (current_x == (p::ccmap->getWidth()-1)) {
                    cost = 0xffff;
                }
                next_y = current_y;
                next_x = current_x+1;
                next_pos = current_pos + 1;
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * STRAIGHT;
                break;
            case 3:
                if (current_y == (p::ccmap->getHeight()-1) || current_x == (p::ccmap->getWidth()-1)) {
                    continue;
                }
                next_x = current_x+1;
                next_y = current_y+1;
                next_pos = current_pos + p::ccmap->getWidth() + 1;
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * DIAGONAL;
                break;
            case 4:
                if (current_y == (p::ccmap->getHeight()-1)) {
                    continue;
                }
                next_x = current_x;
                next_y = current_y+1;
                next_pos = current_pos + p::ccmap->getWidth();
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * STRAIGHT;
                break;
            case 5:
                if (current_y == (p::ccmap->getHeight()-1) || current_x == 0) {
                    continue;
                }
                next_x = current_x-1;
                next_y = current_y+1;
                next_pos = current_pos + p::ccmap->getWidth() - 1;
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * DIAGONAL;
                break;
            case 6:
                if (current_x == 0) {
                    continue;
                }
                next_y = current_y;
                next_x = current_x-1;
                next_pos = current_pos - 1;
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * STRAIGHT;
                break;
            case 7:
                if (current_y == 0 || current_x == 0) {
                    continue;
                }
                next_x = current_x-1;
                next_y = current_y-1;
                next_pos = current_pos - p::ccmap->getWidth() - 1;
                cost = current_node_values->g + p::ccmap->getCost(next_pos) * DIAGONAL;
                break;
            default:
                cost = 0xffff;
                break;
            }

            if (cost >= 0xffff) {
                continue;
            }

            next_node = nodes[next_pos];

            if (next_node == closed) {
                continue;
            } else if (next_node == NULL) {
                if (reusable_node != NULL) {
                    next_node = reusable_node;
                    reusable_node = NULL;
                    next_node_values = next_node->getValue();
                } else {
                    next_node_values = new TileRef;
                    next_node = new Node(next_node_values, 0);
                }

                nodes[next_pos] = next_node;
                directions[next_pos] = current_direction;

                next_node_values->x = next_x;
                next_node_values->y = next_y;
                next_node_values->g = cost;
                diffx = abs(next_x - stop_pos_x);
                diffy = abs(next_y - stop_pos_y);
                next_node_values->h = min(diffx, diffy) * DIAGONAL + abs( diffx - diffy) * STRAIGHT;

                next_node->setKey(next_node_values->g + next_node_values->h);

                open.push(next_node);
            } else if (cost == next_node->getValue()->g) {
                continue;
            } else if (cost < next_node->getValue()->g) {
                next_node->getValue()->g = cost;
                directions[next_pos] = current_direction;
                next_node->setKey(cost + next_node->getValue()->h);
            }
        }

        nodes[current_pos] = closed;
        if (reusable_node != NULL) {
            delete current_node_values;
            delete current_node;
        } else {
            reusable_node = current_node;
        }
    }

    if (current_pos != end_pos) {
        end_pos = current_min_pos;
    }

    if (reusable_node != NULL) {
        delete (TileRef *)reusable_node->getValue();
        delete reusable_node;
    }

    for (int i = 0; i < mapsize; ++i) {
        if (nodes[i] != NULL && nodes[i] != closed) {
            delete (TileRef *)nodes[i]->getValue();
            delete nodes[i];
        }
    }
    delete[] nodes;
    delete closed;

    next_pos = end_pos;

    while (next_pos != start_pos) {
        result.push(directions[next_pos]);
        switch (directions[next_pos]) {
        case 0:
            next_pos = next_pos + p::ccmap->getWidth();
            break;
        case 1:
            next_pos = next_pos + p::ccmap->getWidth() - 1;
            break;
        case 2:
            next_pos = next_pos - 1;
            break;
        case 3:
            next_pos = next_pos - p::ccmap->getWidth() - 1;
            break;
        case 4:
            next_pos = next_pos - p::ccmap->getWidth();
            break;
        case 5:
            next_pos = next_pos - p::ccmap->getWidth() + 1;
            break;
        case 6:
            next_pos = next_pos + 1;
            break;
        case 7:
            next_pos = next_pos + p::ccmap->getWidth() + 1;
            break;
        default:
            delete[] directions;
            return;
        }
    }
    delete[] directions;
}


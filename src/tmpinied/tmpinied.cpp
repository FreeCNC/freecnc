#include <algorithm>
#include <iostream>
#include <string>
#include <stdexcept>

#include <boost/bind.hpp>

#include "SDL_events.h"

#include "../freecnc/freecnc.h"
#include "../freecnc/renderer/shpimage.h"
#include "../freecnc/lib/inifile.h"
#include "../freecnc/lib/fcncendian.h"
#include "../freecnc/ui/font.h"

#include "vfs_instance.h"
#include "tmpgfx.h"
#include "templatedata.h"

using boost::bind;
using std::cerr;
using std::cout;
using std::for_each;
using std::string;
using std::runtime_error;

GameEngine game;

namespace
{
    const unsigned int max_templates = 216;
}


class TmpIniEd
{
public:
    TmpIniEd();
    void run();
private:
    void save_ini();

    void key_handler(const SDL_KeyboardEvent&);

    TmpGFX gfx_;
    Font font_;

    TemplateImage marker_;

    unsigned int cur_template, curpos;
    bool done, image_updated;

    vector<TemplateData> tmps;
};

TmpIniEd::TmpIniEd() : gfx_(), font_("6point.fnt"), marker_("trans.icn", -1),
    cur_template(0), curpos(0), done(false), image_updated(true)
{
    INIFile templini("templates.ini");

    tmps.reserve(max_templates);
    for (unsigned int i = 0; i < max_templates; ++i) {
        tmps.push_back(TemplateData(templini, i));
    }
}

void TmpIniEd::key_handler(const SDL_KeyboardEvent& key)
{
    if (key.state != SDL_PRESSED) {
        return;
    }
    switch (key.keysym.sym) {
        case SDLK_0:
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
            tmps[cur_template].set_type(curpos, key.keysym.sym - SDLK_0);
            image_updated = true;
            break;

        case SDLK_RIGHT:
            curpos++;
            if (curpos >= tmps[cur_template].c_tiles()) {
                curpos = 0;
            }
            image_updated = true;
            break;

        case SDLK_LEFT:
            curpos--;
            if (curpos & 0x80000000) {
                curpos = tmps[cur_template].c_tiles()-1;
            }
            image_updated = true;
            break;

        case SDLK_UP:
            curpos-=tmps[cur_template].width();
            if (curpos & 0x80000000) {
                curpos = 0;
            }
            image_updated = true;
            break;

        case SDLK_DOWN:
            curpos += tmps[cur_template].width();
            if (curpos >= tmps[cur_template].c_tiles()) {
                curpos = tmps[cur_template].c_tiles()-1;
            }
            image_updated = true;
            break;

        case SDLK_PLUS:
            tmps[cur_template].increase_width();
            image_updated = true;
            break;

        case SDLK_MINUS:
            tmps[cur_template].decrease_width();
            image_updated = true;
            break;

        case SDLK_n:
            cur_template++;
            if (cur_template >= max_templates) {
                cur_template = 0;
            }
            curpos = 0;
            image_updated = true;
            break;

        case SDLK_p:
            cur_template--;
            if (cur_template &0x80000000) {
                cur_template = max_templates-1;
            }
            curpos = 0;
            image_updated = true;
            break;

        case SDLK_s:
            save_ini();
            break;

        default:
            break;
    }
}

void TmpIniEd::run()
{
    SDL_Event event;

    while(!done) {
        if(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_KEYDOWN:
                key_handler(event.key);
                break;
            case SDL_QUIT:
                done = true;
                break;
            default:
                break;
            }
            if (image_updated) {
                gfx_.draw(&tmps[cur_template], &marker_, &font_, curpos);
                image_updated = false;
            }
        }
    }
}

void TmpIniEd::save_ini()
{
    shared_ptr<File> output = game.vfs.open_write("new_templates.ini");
    output->writeline(";tiletype%d can be:");
    output->writeline("; 0, land (default)");
    output->writeline("; 1, water");
    output->writeline("; 2, road");
    output->writeline("; 3, rock");
    output->writeline("; 4, tree");
    output->writeline("; 5, water(blocked)");
    output->writeline("; 7, other non passable terrain thing\n");

    for_each(tmps.begin(), tmps.end(), bind(&TemplateData::write_data, _1, output));
}

int main(int argc, const char* argv[])
{
    game.vfs.add(".");
    game.vfs.add("data/settings/td");
    game.vfs.add("data/mix");

    TemplateData::load_palettes();

    try {
        TmpIniEd editor;
        editor.run();
    } catch (std::runtime_error& e) {
        cerr << "Fatal error: " << e.what() << "\n";
    }

    return 0;
}

void GameEngine::shutdown()
{
}

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/lexical_cast.hpp>

#include "SDL_types.h"

#include "templatedata.h"
#include "vfs_instance.h"
#include "../freecnc/lib/fcncendian.h"
#include "../freecnc/renderer/shpimage.h"

using std::runtime_error;
using std::string;
using std::ostringstream;
using boost::lexical_cast;

namespace
{
    const char* theatre_names[] = {".sno", ".des", ".win", ".tem", ".int"};
    const char* palette_names[] = {"snow.pal", "desert.pal", "winter.pal", "temperat.pal", "interior.pal"};

    const size_t c_theatres = sizeof(palette_names) / sizeof(char*);

    SDL_Color palettes[c_theatres][256];

    void load_palettes_impl()
    {
        for (unsigned int palnum = 0; palnum < c_theatres; ++palnum) {
            shared_ptr<VFS::File> datafile = game.vfs.open(palette_names[palnum]);
            if (!datafile) {
                continue;
            }
            vector<unsigned char> paldata(3 * 256);
            datafile->read(paldata, 3 * 256);

            for (int i = 0; i < 256; i++) {
                palettes[palnum][i].r = (read_byte(&paldata[i])) << 2;
                palettes[palnum][i].g = (read_byte(&paldata[i])) << 2;
                palettes[palnum][i].b = (read_byte(&paldata[i])) << 2;
            }
        }
    }

    bool loaded_palettes = false;

    int find_first_available(string basename)
    {
        for (size_t i = 0; i < sizeof(theatre_names); ++i) {
            shared_ptr<File> file_check = game.vfs.open(basename + theatre_names[i]);
                if (file_check) {
                    return i;
                }
        }
        return -1;
    }
}

void TemplateData::load_palettes()
{
    if (!loaded_palettes) {
        load_palettes_impl();
        loaded_palettes = true;
    }
}

void TemplateData::load_image()
{
    int index = find_first_available(name_);
    if (index == -1) {
        throw runtime_error("load_image: Unable to load " + name_);
    }

    string filename = name_ + theatre_names[index];
    std::cout << "Using: " << filename << "\n";
    image_.reset(new TemplateImage(filename.c_str(), -1));

    palnum_ = index;
}

shared_ptr<TemplateImage> TemplateData::image()
{
    SHPBase::setPalette(palettes[palnum_]);
    return image_;
}

TemplateData::TemplateData(INIFile& tmpini, unsigned int tmpnum)
{
    index_name_ = "TEM" + lexical_cast<string>(tmpnum);

    {
        char* tmp = tmpini.readString(index_name_.c_str(), "Name");
        if (tmp == NULL) {
            throw runtime_error("TemplateData: Wrong name: (" + index_name_ + ")\n");
        }
        name_ = tmp;
        delete[] tmp;
    }

    load_image();

    c_tiles_ = image_->getNumTiles();
    width_   = tmpini.readInt(index_name_.c_str(), "width");
    height_  = tmpini.readInt(index_name_.c_str(), "height");

    if (width_ == INIERROR || height_ == INIERROR) {
        width_ = c_tiles_;
        height_ = 1;
    }

    types.resize(c_tiles_);

    for (unsigned int i = 0; i < c_tiles_; ++i) {
        string type_name_("tiletype");
        type_name_ += lexical_cast<string>(i);

        int asdf = tmpini.readInt(index_name_.c_str(), type_name_.c_str());
        if (asdf == INIERROR)
            asdf = 0;
        types[i] = asdf;
    }
}

void TemplateData::increase_width()
{
    while (width_ < c_tiles_) {
        ++width_;
        if (c_tiles_ % width_ == 0) {
            break;
        }
    }
    height_ = c_tiles_ / width_;
}

void TemplateData::decrease_width()
{
    while (width_ > 1) {
        --width_;
        if (c_tiles_ % width_ == 0) {
            break;
        }
    }
    height_ = c_tiles_/width_;
}

void TemplateData::write_data(shared_ptr<VFS::File> output)
{
    ostringstream out;
    out << "[" << index_name_   << "]\n"
        << "Name="   << name_   << "\n"
        << "width="  << width_  << "\n"
        << "height=" << height_ << "\n";

    for (unsigned int i = 0; i < c_tiles_; ++i) {
        if (types[i] != 0) {
            out << "tiletype" << i << "=" << static_cast<int>(types[i]) << "\n";
        }
    }
    out << "\n";
    output->write(out.str());
}

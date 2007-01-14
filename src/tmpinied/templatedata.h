#ifndef TEMPLATEDATA_H
#define TEMPLATEDATA_H

#include "../freecnc/lib/inifile.h"

namespace VFS
{
    class File;
}

class TemplateImage;

class TemplateData
{
public:
    TemplateData(INIFile& tmpini, unsigned int tmpnum);

    static void load_palettes();

    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }

    void increase_width();
    void decrease_width();

    shared_ptr<TemplateImage> image();

    unsigned int c_tiles() const { return c_tiles_; }

    unsigned char type(unsigned int tile) const { return types.at(tile); }
    void set_type(unsigned int tile, unsigned char type) { types.at(tile) = type; }

    const string& name() const { return name_; }

    const string& index_name() const { return index_name_; }

    void write_data(shared_ptr<VFS::File> output);
private:
    void load_image();
    unsigned int width_, height_, c_tiles_;
    vector<unsigned char> types;
    string name_, index_name_;

    shared_ptr<TemplateImage> image_;
    unsigned int palnum_;
};


#endif

#ifndef _RENDERER_IMAGECACHE_H
#define _RENDERER_IMAGECACHE_H

#include "../freecnc.h"

struct SDL_Surface;

class SHPImage;

struct ImageCacheEntry
{
    ImageCacheEntry();
    ~ImageCacheEntry();
    void clear();
    SDL_Surface *image;
    SDL_Surface *shadow;
};

class ImageCache
{
public:
    ImageCache();
    ~ImageCache();
    void setImagePool(std::vector<SHPImage *> *imagepool);
    ImageCacheEntry& getImage(unsigned int imgnum);
    ImageCacheEntry& getImage(unsigned int imgnum, unsigned int frame);

    /** @TODO Arbitrary post-processing filter, e.g. colour fiddling.
     * ImageCacheEntry& getText(const char*); // Caches text
     * 1) typedef void (FilterFunc*)(unsigned int, ImageCacheEntry&);  OR
     * 2) Policy class that provides this API:
     *    struct FilterFunc : public binary_functor(?) {
     *         void operator()(unsigned int, ImageCacheEntry&);
     *    };
     * void applyFilter(const char* fname, const FilterFunc&);
     */
    /// @brief Loads the shpimage fname into the imagecache.
    unsigned int loadImage(const char* fname);
    unsigned int loadImage(const char* fname, int scaleq);

    void newCache();
    void flush();

private:
    std::map<unsigned int, ImageCacheEntry> cache, prevcache;
    std::map<std::string, unsigned int> namecache;
    std::vector<SHPImage*>* imagepool;
};

#endif

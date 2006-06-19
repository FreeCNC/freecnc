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
    ImageCacheEntry& getImage(Uint32 imgnum);
    ImageCacheEntry& getImage(Uint32 imgnum, Uint32 frame);

    /** @TODO Arbitrary post-processing filter, e.g. colour fiddling.
     * ImageCacheEntry& getText(const char*); // Caches text
     * 1) typedef void (FilterFunc*)(Uint32, ImageCacheEntry&);  OR
     * 2) Policy class that provides this API:
     *    struct FilterFunc : public binary_functor(?) {
     *         void operator()(Uint32, ImageCacheEntry&);
     *    };
     * void applyFilter(const char* fname, const FilterFunc&);
     */
    /// @brief Loads the shpimage fname into the imagecache.
    Uint32 loadImage(const char* fname);
    Uint32 loadImage(const char* fname, int scaleq);

    void newCache();
    void flush();

private:
    std::map<Uint32, ImageCacheEntry> cache, prevcache;
    std::map<std::string, Uint32> namecache;
    std::vector<SHPImage*>* imagepool;
};

#endif

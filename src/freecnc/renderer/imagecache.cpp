#include <algorithm>
#include <cctype>

#include "SDL.h"

#include "imagecache.h"
#include "shpimage.h"

using std::transform;

ImageCache::ImageCache() {}
ImageCache::~ImageCache() {}

/** @brief Assigns the source for images and purges both caches.
 * @param imagepool The imagepool to use.
 */
void ImageCache::setImagePool(std::vector<SHPImage*>* imagepool)
{
    this->imagepool = imagepool;
    cache.clear();
    prevcache.clear();
}

/** Gets an image from the cache.  If the image isn't cached, it is loaded
 * in from the imagepool.
 * @param imgnum The number of the image in the imagepool.
          The imgnum stores 3 values
          <--------------32bits------------>
          0123456701234567 01234 56701234567
          [index of image] [pal] [ frame # ]
          The index of the image, the palette to use, and the frame number
 * @returns A class containing the image and the shadow.
 */
ImageCacheEntry& ImageCache::getImage(Uint32 imgnum)
{
    std::map<Uint32, ImageCacheEntry>::iterator cachepos;

    if (imgnum == (Uint32)-1) {
        throw ImageNotFound();
    }

    //logger->debug("%d %d\n", imgnum, cache.size());

    cachepos = cache.find(imgnum);
    if (cachepos != cache.end()) {
        // Found image
        return cachepos->second;
    }
    // Didn't find it, so check old cache
    if (!prevcache.empty()) {
        cachepos = prevcache.find(imgnum);
        if (cachepos != prevcache.end()) {
            // Found image in old cache
            cache[imgnum] = cachepos->second;
            // Entry in newer cache takes ownership of the surface pointers
            prevcache[imgnum].clear();
            return cachepos->second;
        }
    }

    /* Didn't find it in either cache, so will retrieve from the imagepool.
     * This is the only part of the imagecache that uses code from elsewhere
     * i.e. change the type of the vector and how the decoded sprite data is
     * stored into the "entry" and it'll work elsewhere.
     */

    ImageCacheEntry& entry = cache[imgnum];

    // Palette is ((imgnum>>11)&0x1f).
    (*imagepool)[imgnum>>16]->getImage(imgnum&0x7FF, &(entry.image), &(entry.shadow),
                                       ((imgnum>>11)&0x1f));
    return entry;
}


ImageCacheEntry& ImageCache::getImage(Uint32 imgnum, Uint32 frame) 
{
    return getImage(imgnum | (frame &0x7FF));
}

Uint32 ImageCache::loadImage(const char* fname, int scaleq) {
    string name = fname;
    map<string, Uint32>::iterator cachentry;
    transform(name.begin(), name.end(), name.begin(), toupper);

    cachentry = namecache.find(name);
    if (cachentry == namecache.end()) {
        Uint32 size = static_cast<Uint32>(imagepool->size());
        try {
            imagepool->push_back(new SHPImage(name.c_str(), scaleq));
            namecache[name] = size<<16;
            return size<<16;
        } catch (ImageNotFound&) {
            namecache[name] = (Uint32)-1;
        }
    }
    if ((Uint32)-1 == namecache[name]) {
        throw ImageNotFound();
    }
    return cachentry->second;
}

Uint32 ImageCache::loadImage(const char* fname)
{
    return loadImage(fname, mapscaleq);
}

/** @brief Rotates caches so a new cache will be created next time an image is
 * requested.
 */
void ImageCache::newCache() {
    prevcache.clear();
    prevcache.swap(cache);
}

/** @brief Clears both the current and previous caches
*/
void ImageCache::flush() {
    prevcache.clear();
    cache.clear();
}

// ImageCacheEntry functions

/** @brief Ensure that the pointers start off pointing somewhere that's safe to
 * delete.  */
ImageCacheEntry::ImageCacheEntry() : image(0), shadow(0) {}

/** @brief Frees the surfaces.  If the destructor is invoked by a copy of the
 * main instance, the program will most likely crash or otherwise mess up
 * horribly.
 */
ImageCacheEntry::~ImageCacheEntry() {
    SDL_FreeSurface(image);
    SDL_FreeSurface(shadow);
}

/// @brief This function exists because we don't have shared pointers.
void ImageCacheEntry::clear() {
    image = 0;
    shadow = 0;
}

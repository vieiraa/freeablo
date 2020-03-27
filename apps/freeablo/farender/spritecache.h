#pragma once
#include <atomic>
#include <fa_nuklear.h>
#include <list>
#include <map>
#include <render/render.h>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>

namespace FARender
{
    class Renderer;
    class SpriteCache;
    class FASpriteGroup
    {
    public:
        bool isValid() const { return spriteCacheIndex != 0; }
        int32_t getAnimLength() const { return animLength; }
        int32_t getWidth(int frame = 0) const { return width[frame]; }
        int32_t getHeight(int frame = 0) const { return height[frame]; }
        int32_t getCacheIndex() const { return spriteCacheIndex; }
        struct nk_image getNkImage(int32_t frame = 0);

    private:
        void init(int32_t _animLength, const std::vector<int32_t>& _width, const std::vector<int32_t>& _height, int32_t _spriteCacheIndex)
        {
            animLength = _animLength;
            width = _width;
            height = _height;
            spriteCacheIndex = _spriteCacheIndex;

            frameHandles.resize(animLength);
            for (uint32_t i = 0; i < frameHandles.size(); i++)
            {
                frameHandles[i].first = spriteCacheIndex;
                frameHandles[i].second = i;
            }
        }

        int32_t animLength = 0;
        std::vector<int32_t> width = {};
        std::vector<int32_t> height = {};
        int32_t spriteCacheIndex = 0;

        std::vector<std::pair<int32_t, int32_t>> frameHandles;

        friend class SpriteCache;
        friend class SpriteManager;
    };

    struct TilesetPath
    {
        std::string celPath;
        std::string minPath;
        bool top;
        bool trim;

        TilesetPath(std::string _c, std::string _m, bool _t, bool _trim) : celPath(_c), minPath(_m), top(_t), trim(_trim) {}

        TilesetPath() {}
    };

    struct CacheEntry
    {
        Render::SpriteGroup* sprite;
        std::list<uint32_t>::iterator it;
        bool immortal;

        CacheEntry(Render::SpriteGroup* _sprite, std::list<uint32_t>::iterator _it, bool _immortal) : sprite(_sprite), it(_it), immortal(_immortal) {}

        CacheEntry() {}
    };

    ///
    /// @brief Multithread sprite cache
    ///
    /// FASpriteGroup is returned from the get() functions used in the game thread, which do not actually do image loading.
    /// get(uint32_t index) method is used to get a Render::SpriteGroup pointer in the render thread, actual image loading is done lazily here.
    /// The index value comes from FASpriteGroup.spriteCacheIndex
    ///
    class SpriteCache
    {
    public:
        explicit SpriteCache(uint32_t size);
        ~SpriteCache();

        FASpriteGroup* get(const std::string& path, bool trim); ///< To be called from the game thread

        /// Same as get(const std::string&), but for tileset sprites
        /// @brief To be called from the game thread
        FASpriteGroup* getTileset(const std::string& celPath, const std::string& minPath, bool top, bool trim);

        uint32_t newUniqueIndex(); ///< Can be called from any thread

        /// Directly inserts a sprite into the cache at the specified index.
        /// Does not touch any of the m*toCache members, so the only way to get a reference is to have one from before
        /// @brief To be called from the render thread
        void directInsert(Render::SpriteGroup* sprite, uint32_t cacheIndex);

        /// Gets a Render::SpriteGroup* from the cache if it has been loaded before, otherwise load it,
        /// insert it into the cache, and then return it.
        ///
        /// @brief To be called from the render thread
        Render::SpriteGroup* get(uint32_t index);

        /// Used when we need to guarantee that a sprite will not be evicted for a period of time
        /// @brief To be called from the render thread
        void setImmortal(uint32_t index, bool immortal);

        /// @brief To be called from the game thread
        std::string getPathForIndex(uint32_t index);

        /// The only creation point for FASpriteGroups
        /// @brief To be called from the game thread
        FASpriteGroup* allocNewSpriteGroup();

        void clear(); //< To be called from the render thread

    private:
        void moveToFront(uint32_t index);
        void evict();

        std::map<std::string, FASpriteGroup*> mStrToCache;

        struct LoadSpec
        {
            std::string path;
            bool trim = true;
        };
        std::map<uint32_t, LoadSpec> mCacheToStr;

        std::map<std::string, FASpriteGroup*> mStrToTilesetCache;
        std::map<uint32_t, TilesetPath> mCacheToTilesetPath;

        std::map<uint32_t, uint32_t> mCacheToSprite;

        std::map<uint32_t, CacheEntry> mCache;
        std::list<uint32_t> mUsedList;

        std::atomic<uint32_t> mNextCacheIndex;

        uint32_t mCurrentSize;
        uint32_t mMaxSize;

        static constexpr uint32_t SPRITEGROUP_STORE_BLOCK_SIZE = 256;
        std::vector<FASpriteGroup*> mSpriteGroupStore;
        uint32_t mSpriteGroupCurrentBlockIndex = 0;
    };
}

#pragma once
#include <Image/image.h>
#include <atomic>
#include <cstdint>
#include <map>
#include <misc/simplevec2.h>
#include <render/alignedcpubuffer.h>
#include <render/atlastexture.h>
#include <render/levelobjects.h>
#include <render/vertextypes.h>

namespace Level
{
    class Level;
}

namespace Render
{
    class SpriteGroup;
    class AtlasTextureEntry;
    class Texture;
    class VertexArrayObject;
    class Pipeline;
    class Buffer;
    class DescriptorSet;
    class Framebuffer;
    struct Tile;

    typedef const AtlasTextureEntry* Sprite;
}

namespace FARender
{
    namespace DrawLevelUniforms
    {
        struct Vertex
        {
            float screenSizeInPixels[2];

            float pad1[2];
        };

        struct Fragment
        {
            float atlasSizeInPixels[2];

            float pad2[2];
        };

        using CpuBufferType = Render::TypedAlignedCpuBuffer<Vertex, Fragment>;
    }

    namespace FullscreenUniforms
    {
        struct Vertex
        {
            float scaleOrigin[2];
            float scale;

            float pad1;
        };
    }

    class DrawLevelCache
    {
    public:
        void addSprite(Render::Sprite atlasEntry, int32_t x, int32_t y, std::optional<ByteColour> highlightColor);
        void end(DrawLevelUniforms::CpuBufferType& drawLevelUniformCpuBuffer,
                 Render::Buffer& drawLevelUniformBuffer,
                 Render::VertexArrayObject& vertexArrayObject,
                 Render::DescriptorSet& drawLevelDescriptorSet,
                 Render::Pipeline& drawLevelPipeline,
                 Render::Framebuffer* nonDefaultFramebuffer);

    private:
        void batchDrawSprite(const Render::AtlasTextureEntry& atlasEntry,
                             int32_t x,
                             int32_t y,
                             std::optional<ByteColour> highlightColor,
                             float zBufferVal,
                             DrawLevelUniforms::CpuBufferType& drawLevelUniformCpuBuffer,
                             Render::Buffer& drawLevelUniformBuffer,
                             Render::VertexArrayObject& vertexArrayObject,
                             Render::DescriptorSet& drawLevelDescriptorSet,
                             Render::Pipeline& drawLevelPipeline,
                             Render::Framebuffer* nonDefaultFramebuffer);
        void draw(DrawLevelUniforms::CpuBufferType& drawLevelUniformCpuBuffer,
                  Render::Buffer& drawLevelUniformBuffer,
                  Render::VertexArrayObject& vertexArrayObject,
                  Render::DescriptorSet& drawLevelDescriptorSet,
                  Render::Pipeline& drawLevelPipeline,
                  Render::Framebuffer* nonDefaultFramebuffer);

    private:
        struct SpriteData
        {
            const Render::AtlasTextureEntry* atlasEntry = nullptr;
            int32_t x = 0;
            int32_t y = 0;
            std::optional<ByteColour> highlightColor;
            float zBufferValue = 0;
        };

        std::vector<SpriteData> mSpritesToDraw;
        std::vector<Render::SpriteVertexPerInstance> mInstanceData;
        Render::Texture* mTexture = nullptr;
    };

    class LevelRenderer
    {
    public:
        LevelRenderer();
        ~LevelRenderer();

        void drawLevel(const Level::Level& level,
                       Render::SpriteGroup* minTops,
                       Render::SpriteGroup* minBottoms,
                       Render::SpriteGroup* specialSprites,
                       const std::map<int32_t, int32_t>& specialSpritesMap,
                       Render::LevelObjects& objs,
                       Render::LevelObjects& items,
                       const Vec2Fix& fractionalPos);

        Render::Tile getTileByScreenPos(size_t x, size_t y, const Vec2Fix& worldPositionOffset);

        void toggleTextureFiltering() { mTextureFilter = !mTextureFilter; }
        void adjustZoom(int32_t delta) { mRenderScale = std::clamp(mRenderScale + delta, 1, 5); }

    private:
        void createNewLevelDrawFramebuffer();

        void drawAtTile(Render::Sprite sprite, const Misc::Point& tileTop, int spriteW, int spriteH, std::optional<ByteColour> highlightColor = std::nullopt);
        void drawMovingSprite(Render::Sprite sprite,
                              const Vec2Fix& fractionalPos,
                              const Misc::Point& toScreen,
                              std::optional<Cel::Colour> highlightColor = std::nullopt);

    private:
        DrawLevelCache mDrawLevelCache;

        std::unique_ptr<Render::VertexArrayObject> vertexArrayObject;
        std::unique_ptr<Render::Pipeline> drawLevelPipeline;
        std::unique_ptr<Render::Buffer> drawLevelUniformBuffer;
        std::unique_ptr<DrawLevelUniforms::CpuBufferType> drawLevelUniformCpuBuffer;
        std::unique_ptr<Render::DescriptorSet> drawLevelDescriptorSet;

        std::unique_ptr<Render::Framebuffer> levelDrawFramebuffer;

        std::unique_ptr<Render::Pipeline> fullscreenPipeline;
        std::unique_ptr<Render::VertexArrayObject> fullscreenVao;
        std::unique_ptr<Render::Buffer> fullscreenUniformBuffer;
        std::unique_ptr<Render::DescriptorSet> fullscreenDescriptorSet;

        std::atomic_bool mTextureFilter = false;
        std::atomic_int mRenderScale = 2;
    };
}

#include "levelrenderer.h"
#include <level/level.h>
#include <render/commandqueue.h>
#include <render/framebuffer.h>
#include <render/levelobjects.h>
#include <render/pipeline.h>
#include <render/render.h>
#include <render/renderinstance.h>
#include <render/texture.h>
#include <render/vertexarrayobject.h>
#include <render/vertextypes.h>

class Vertex;
namespace FARender
{
    static Vec2i getCurrentResolution() { return {Render::getWindowSize().windowWidth, Render::getWindowSize().windowHeight}; }

    void DrawLevelCache::addSprite(Render::Sprite atlasEntry, int32_t x, int32_t y, std::optional<Cel::Colour> highlightColor)
    {
        mSpritesToDraw.push_back(SpriteData{atlasEntry, x, y, highlightColor});
    }

    void DrawLevelCache::end(DrawLevelUniforms::CpuBufferType& drawLevelUniformCpuBuffer,
                             Render::Buffer& drawLevelUniformBuffer,
                             Render::VertexArrayObject& vertexArrayObject,
                             Render::DescriptorSet& drawLevelDescriptorSet,
                             Render::Pipeline& drawLevelPipeline,
                             Render::Framebuffer* nonDefaultFramebuffer)
    {
        for (size_t i = 0; i < mSpritesToDraw.size(); i++)
            mSpritesToDraw[i].zBufferValue = 1.0f - ((i + 1) / float(mSpritesToDraw.size() + 2));

        // explicit z buffer values ensure the draws act like they were done in-order, so we're free to batch by texture as aggressively as possible
        auto sortByTexture = [](const SpriteData& a, const SpriteData& b) { return a.atlasEntry->mTexture < b.atlasEntry->mTexture; };
        std::sort(mSpritesToDraw.begin(), mSpritesToDraw.end(), sortByTexture);

        for (const auto& spriteData : mSpritesToDraw)
            batchDrawSprite(*spriteData.atlasEntry,
                            spriteData.x,
                            spriteData.y,
                            spriteData.highlightColor,
                            spriteData.zBufferValue,
                            drawLevelUniformCpuBuffer,
                            drawLevelUniformBuffer,
                            vertexArrayObject,
                            drawLevelDescriptorSet,
                            drawLevelPipeline,
                            nonDefaultFramebuffer);

        mSpritesToDraw.clear();

        draw(drawLevelUniformCpuBuffer, drawLevelUniformBuffer, vertexArrayObject, drawLevelDescriptorSet, drawLevelPipeline, nonDefaultFramebuffer);
    }

    void DrawLevelCache::batchDrawSprite(const Render::AtlasTextureEntry& atlasEntry,
                                         int32_t x,
                                         int32_t y,
                                         std::optional<Cel::Colour> highlightColor,
                                         float zBufferVal,
                                         DrawLevelUniforms::CpuBufferType& drawLevelUniformCpuBuffer,
                                         Render::Buffer& drawLevelUniformBuffer,
                                         Render::VertexArrayObject& vertexArrayObject,
                                         Render::DescriptorSet& drawLevelDescriptorSet,
                                         Render::Pipeline& drawLevelPipeline,
                                         Render::Framebuffer* nonDefaultFramebuffer)
    {
        if (atlasEntry.mTexture != mTexture)
            draw(drawLevelUniformCpuBuffer, drawLevelUniformBuffer, vertexArrayObject, drawLevelDescriptorSet, drawLevelPipeline, nonDefaultFramebuffer);

        mTexture = atlasEntry.mTexture;

        Render::SpriteVertexPerInstance vertexData = {};

        vertexData.v_spriteSizeInPixels[0] = atlasEntry.mTrimmedWidth;
        vertexData.v_spriteSizeInPixels[1] = atlasEntry.mTrimmedHeight;

        vertexData.v_atlasOffsetInPixels[0] = atlasEntry.mX;
        vertexData.v_atlasOffsetInPixels[1] = atlasEntry.mY;

        vertexData.v_destinationInPixels[0] = x + atlasEntry.mTrimmedOffsetX;
        vertexData.v_destinationInPixels[1] = y + atlasEntry.mTrimmedOffsetY;

        vertexData.v_zValue = zBufferVal;

        if (auto c = highlightColor)
        {
            vertexData.v_hoverColor[0] = c->r;
            vertexData.v_hoverColor[1] = c->g;
            vertexData.v_hoverColor[2] = c->b;
            vertexData.v_hoverColor[3] = 255;
        }

        mInstanceData.push_back(vertexData);
    }

    void DrawLevelCache::draw(DrawLevelUniforms::CpuBufferType& drawLevelUniformCpuBuffer,
                              Render::Buffer& drawLevelUniformBuffer,
                              Render::VertexArrayObject& vertexArrayObject,
                              Render::DescriptorSet& drawLevelDescriptorSet,
                              Render::Pipeline& drawLevelPipeline,
                              Render::Framebuffer* nonDefaultFramebuffer)
    {
        if (mInstanceData.empty())
            return;

        auto vertexUniforms = drawLevelUniformCpuBuffer.getMemberPointer<DrawLevelUniforms::Vertex>();
        vertexUniforms->screenSizeInPixels[0] = getCurrentResolution().w;
        vertexUniforms->screenSizeInPixels[1] = getCurrentResolution().h;

        auto fragmentUniforms = drawLevelUniformCpuBuffer.getMemberPointer<DrawLevelUniforms::Fragment>();
        fragmentUniforms->atlasSizeInPixels[0] = mTexture->width();
        fragmentUniforms->atlasSizeInPixels[1] = mTexture->height();

        drawLevelUniformBuffer.setData(drawLevelUniformCpuBuffer.data(), drawLevelUniformCpuBuffer.getSizeInBytes());

        vertexArrayObject.getVertexBuffer(1)->setData(mInstanceData.data(), mInstanceData.size() * sizeof(Render::SpriteVertexPerInstance));

        drawLevelDescriptorSet.updateItems({
            {2, mTexture},
        });

        Render::Bindings bindings;
        bindings.vao = &vertexArrayObject;
        bindings.pipeline = &drawLevelPipeline;
        bindings.descriptorSet = &drawLevelDescriptorSet;
        bindings.nonDefaultFramebuffer = nonDefaultFramebuffer;

        Render::mainCommandQueue->cmdDrawInstances(0, 6, mInstanceData.size(), bindings);

        mInstanceData.clear();
        mTexture = nullptr;
    }

    constexpr auto tileHeight = 32;
    constexpr auto tileWidth = tileHeight * 2;
    constexpr auto staticObjectHeight = 256;

    void LevelRenderer::drawAtTile(Render::Sprite sprite, const Misc::Point& tileTop, int spriteW, int spriteH, std::optional<Cel::Colour> highlightColor)
    {
        // centering sprite at the center of tile by width and at the bottom of tile by height
        mDrawLevelCache.addSprite(sprite, tileTop.x - spriteW / 2, tileTop.y - spriteH + tileHeight, highlightColor);
    }

    // basic transform of isometric grid to normal, (0, 0) tile coordinate maps to (0, 0) pixel coordinates
    // since eventually we're gonna shift coordinates to viewport center, it's better to keep transform itself
    // as simple as possible
    template <typename T> static Vec2<T> tileTopPoint(Vec2<T> tile, int32_t scale = 1)
    {
        return Vec2<T>(T((tileWidth * scale) / 2) * (tile.x - tile.y), (tile.y + tile.x) * ((tileHeight * scale) / 2));
    }

    // this function simply does the reverse of the above function, could be found by solving linear equation system
    // it obviously uses the fact that tileWidth = tileHeight * 2
    Render::Tile getTileFromScreenCoords(Vec2i screenPos, Vec2i screenSpaceOffset, int32_t scale = 1)
    {
        Vec2i point = screenPos - screenSpaceOffset;
        auto x = std::div(2 * point.y + point.x, tileWidth * scale);
        auto y = std::div(2 * point.y - point.x, tileWidth * scale);
        return {x.quot, y.quot, x.rem > y.rem ? Render::TileHalf::right : Render::TileHalf::left};
    }

    void
    LevelRenderer::drawMovingSprite(Render::Sprite sprite, const Vec2Fix& fractionalPos, const Misc::Point& toScreen, std::optional<Cel::Colour> highlightColor)
    {
        Vec2i point = Vec2i(tileTopPoint(fractionalPos));
        Vec2i res = point + toScreen;

        drawAtTile(sprite, Vec2i(res), sprite->mWidth, sprite->mHeight, highlightColor);
    }

    constexpr auto bottomMenuSize = 0; // 144; // TODO: pass it as a variable
    static Misc::Point worldPositionToScreenSpace(const Vec2Fix& worldPosition, int32_t scale = 1)
    {
        // centering takes in accord bottom menu size to be consistent with original game centering
        Vec2i point = Vec2i(tileTopPoint(worldPosition, scale));

        Vec2i resolution = getCurrentResolution();
        return Misc::Point{resolution.w / 2, (resolution.h - bottomMenuSize) / 2} - point;
    }

    Render::Tile LevelRenderer::getTileByScreenPos(size_t x, size_t y, const Vec2Fix& worldPositionOffset)
    {
        Misc::Point screenSpaceOffset = worldPositionToScreenSpace(worldPositionOffset, mRenderScale);
        return getTileFromScreenCoords({static_cast<int32_t>(x), static_cast<int32_t>(y)}, screenSpaceOffset, mRenderScale);
    }

    template <typename ProcessTileFunc> void drawObjectsByTiles(const Misc::Point& toScreen, ProcessTileFunc processTile)
    {
        Misc::Point start{-2 * tileWidth, -2 * tileHeight};
        Render::Tile startingTile = getTileFromScreenCoords(start, toScreen);

        Vec2i resolution = getCurrentResolution();

        Misc::Point startingPoint = Vec2i(tileTopPoint(startingTile.pos)) + toScreen;
        auto processLine = [&]() {
            Misc::Point point = startingPoint;
            Render::Tile tile = startingTile;

            while (point.x < resolution.w + tileWidth / 2)
            {
                point.x += tileWidth;
                ++tile.pos.x;
                --tile.pos.y;
                processTile(tile, point);
            }
        };

        // then from top left to top-bottom
        while (startingPoint.y < resolution.h + staticObjectHeight - tileHeight)
        {
            ++startingTile.pos.y;
            startingPoint.x -= tileWidth / 2;
            startingPoint.y += tileHeight / 2;
            processLine();
            ++startingTile.pos.x;
            startingPoint.x += tileWidth / 2;
            startingPoint.y += tileHeight / 2;
            processLine();
        }
    }

    void LevelRenderer::drawLevel(const Level::Level& level,
                                  Render::SpriteGroup* minTops,
                                  Render::SpriteGroup* minBottoms,
                                  Render::SpriteGroup* specialSprites,
                                  const std::map<int32_t, int32_t>& specialSpritesMap,
                                  Render::LevelObjects& objs,
                                  Render::LevelObjects& items,
                                  const Vec2Fix& fractionalPos)
    {
        auto toScreen = worldPositionToScreenSpace(fractionalPos);
        auto isInvalidTile = [&](const Render::Tile& tile) {
            return tile.pos.x < 0 || tile.pos.y < 0 || tile.pos.x >= static_cast<int32_t>(level.width()) || tile.pos.y >= static_cast<int32_t>(level.height());
        };

        // drawing on the ground objects
        drawObjectsByTiles(toScreen, [&](const Render::Tile& tile, const Misc::Point& topLeft) {
            if (isInvalidTile(tile))
            {
                drawAtTile((*minBottoms)[0], topLeft, tileWidth, staticObjectHeight);
                return;
            }

            size_t index = level.get(tile.pos).index();
            if (index < minBottoms->size())
                drawAtTile((*minBottoms)[index], topLeft, tileWidth, staticObjectHeight); // all static objects have the same sprite size
        });

        // drawing above the ground and moving object
        drawObjectsByTiles(toScreen, [&](const Render::Tile& tile, const Misc::Point& topLeft) {
            if (isInvalidTile(tile))
                return;

            size_t index = level.get(tile.pos).index();
            if (index < minTops->size())
            {
                drawAtTile((*minTops)[index], topLeft, tileWidth, staticObjectHeight);

                // Add special sprites (arches / open door frames) if required.
                if (specialSpritesMap.count(index))
                {
                    int32_t specialSpriteIndex = specialSpritesMap.at(index);
                    Render::Sprite& sprite = (*specialSprites)[specialSpriteIndex];
                    drawAtTile(sprite, topLeft, sprite->mWidth, sprite->mHeight);
                }
            }

            auto& itemsForTile = items.get(tile.pos.x, tile.pos.y);
            for (auto& item : itemsForTile)
            {
                const Render::Sprite& sprite = item.sprite->operator[](item.spriteFrame);
                drawAtTile(sprite, topLeft, sprite->mWidth, sprite->mHeight, item.hoverColor);
            }

            auto& objsForTile = objs.get(tile.pos.x, tile.pos.y);
            for (auto& obj : objsForTile)
            {
                if (obj.valid)
                {
                    const Render::Sprite& sprite = obj.sprite->operator[](obj.spriteFrame);
                    drawMovingSprite(sprite, obj.fractionalPos, toScreen, obj.hoverColor);
                }
            }
        });

        if (levelDrawFramebuffer->getColorBuffer().width() != getCurrentResolution().w ||
            levelDrawFramebuffer->getColorBuffer().height() != getCurrentResolution().h)
            createNewLevelDrawFramebuffer();

        Render::mainCommandQueue->cmdClearFramebuffer(Render::Colors::black, true, levelDrawFramebuffer.get());

        mDrawLevelCache.end(
            *drawLevelUniformCpuBuffer, *drawLevelUniformBuffer, *vertexArrayObject, *drawLevelDescriptorSet, *drawLevelPipeline, levelDrawFramebuffer.get());

        Render::Bindings fullscreenBindings;
        fullscreenBindings.pipeline = fullscreenPipeline.get();
        fullscreenBindings.vao = fullscreenVao.get();
        fullscreenBindings.descriptorSet = fullscreenDescriptorSet.get();

        FullscreenUniforms::Vertex fullscreenSettings = {};
        fullscreenSettings.scaleOrigin[0] = 0;
        fullscreenSettings.scaleOrigin[1] = 0;
        fullscreenSettings.scale = mRenderScale;
        fullscreenUniformBuffer->setData(&fullscreenSettings, sizeof(FullscreenUniforms::Vertex));

        Render::Texture& levelTexture = levelDrawFramebuffer->getColorBuffer();
        if (mTextureFilter && levelTexture.getInfo().magFilter == Render::Filter::Nearest)
            levelTexture.setFilter(levelTexture.getInfo().minFilter, Render::Filter::Linear);
        else if (!mTextureFilter && levelTexture.getInfo().magFilter == Render::Filter::Linear)
            levelTexture.setFilter(levelTexture.getInfo().minFilter, Render::Filter::Nearest);

        Render::mainCommandQueue->cmdDraw(0, 6, fullscreenBindings);
    }
    LevelRenderer::LevelRenderer()
    {
        Render::PipelineSpec drawLevelPipelineSpec;
        drawLevelPipelineSpec.depthTest = true;
        drawLevelPipelineSpec.vertexLayouts = {Render::SpriteVertexMain::layout(), Render::SpriteVertexPerInstance::layout()};
        drawLevelPipelineSpec.vertexShaderPath = Misc::getResourcesPath().str() + "/shaders/basic.vert";
        drawLevelPipelineSpec.fragmentShaderPath = Misc::getResourcesPath().str() + "/shaders/basic.frag";
        drawLevelPipelineSpec.descriptorSetSpec = {{
            {Render::DescriptorType::UniformBuffer, "vertexUniforms"},
            {Render::DescriptorType::UniformBuffer, "fragmentUniforms"},
            {Render::DescriptorType::Texture, "tex"},
        }};

        drawLevelPipeline = Render::mainRenderInstance->createPipeline(drawLevelPipelineSpec);
        vertexArrayObject = Render::mainRenderInstance->createVertexArrayObject({0, 0}, drawLevelPipelineSpec.vertexLayouts, 0);

        drawLevelUniformCpuBuffer = std::make_unique<DrawLevelUniforms::CpuBufferType>(Render::mainRenderInstance->capabilities().uniformBufferOffsetAlignment);

        drawLevelUniformBuffer = Render::mainRenderInstance->createBuffer(drawLevelUniformCpuBuffer->getSizeInBytes());
        drawLevelDescriptorSet = Render::mainRenderInstance->createDescriptorSet(drawLevelPipelineSpec.descriptorSetSpec);

        // clang-format off
        drawLevelDescriptorSet->updateItems(
        {
            {0, Render::BufferSlice{drawLevelUniformBuffer.get(), drawLevelUniformCpuBuffer->getMemberOffset<DrawLevelUniforms::Vertex>(), sizeof(DrawLevelUniforms::Vertex)}},
            {1, Render::BufferSlice{drawLevelUniformBuffer.get(), drawLevelUniformCpuBuffer->getMemberOffset<DrawLevelUniforms::Fragment>(), sizeof(DrawLevelUniforms::Fragment)}},
        });

        Render::SpriteVertexMain baseVertices[] =
        {
            {{0, 0},  {0, 0}},
            {{1, 0},  {1, 0}},
            {{1, 1},  {1, 1}},
            {{0, 0},  {0, 0}},
            {{1, 1},  {1, 1}},
            {{0, 1},  {0, 1}}
        };
        // clang-format on
        vertexArrayObject->getVertexBuffer(0)->setData(baseVertices, sizeof(baseVertices));

        {
            // clang-format off
            Render::BasicVertex topLeft  {{-1, -1}, {0, 0}};
            Render::BasicVertex topRight {{ 1, -1}, {1, 0}};
            Render::BasicVertex botLeft  {{-1,  1}, {0, 1}};
            Render::BasicVertex botRight {{ 1,  1}, {1, 1}};

            Render::BasicVertex fullscreenVertices[] =
            {
                topLeft, topRight, botLeft,
                topRight, botRight, botLeft,
            };
            // clang-format on

            Render::PipelineSpec fullscreenPipelineSpec;
            fullscreenPipelineSpec.vertexLayouts = {Render::BasicVertex::layout()};
            fullscreenPipelineSpec.vertexShaderPath = Misc::getResourcesPath().str() + "/shaders/fullscreen.vert";
            fullscreenPipelineSpec.fragmentShaderPath = Misc::getResourcesPath().str() + "/shaders/fullscreen.frag";
            fullscreenPipelineSpec.descriptorSetSpec = {{
                {Render::DescriptorType::UniformBuffer, "vertexUniforms"},
                {Render::DescriptorType::Texture, "tex"},
            }};

            fullscreenPipeline = Render::mainRenderInstance->createPipeline(fullscreenPipelineSpec);
            fullscreenVao = Render::mainRenderInstance->createVertexArrayObject({sizeof(fullscreenVertices)}, fullscreenPipelineSpec.vertexLayouts, 0);
            fullscreenUniformBuffer = Render::mainRenderInstance->createBuffer(sizeof(FullscreenUniforms::Vertex));
            fullscreenDescriptorSet = Render::mainRenderInstance->createDescriptorSet(fullscreenPipelineSpec.descriptorSetSpec);

            fullscreenVao->getVertexBuffer(0)->setData(fullscreenVertices, sizeof(baseVertices));
            fullscreenDescriptorSet->updateItems({{0, this->fullscreenUniformBuffer.get()}});
        }

        createNewLevelDrawFramebuffer();
    }

    void LevelRenderer::createNewLevelDrawFramebuffer()
    {
        Render::BaseTextureInfo textureInfo;
        textureInfo.width = getCurrentResolution().w;
        textureInfo.height = getCurrentResolution().h;
        textureInfo.minFilter = Render::Filter::Linear;
        textureInfo.magFilter = Render::Filter::Nearest;

        if (levelDrawFramebuffer)
            textureInfo.magFilter = levelDrawFramebuffer->getColorBuffer().getInfo().magFilter;

        textureInfo.format = Render::Format::RGBA8UNorm;
        std::unique_ptr<Render::Texture> colorBuffer = Render::mainRenderInstance->createTexture(textureInfo);

        textureInfo.format = Render::Format::Depth24Stencil8;
        std::unique_ptr<Render::Texture> depthStencilBuffer = Render::mainRenderInstance->createTexture(textureInfo);

        Render::FramebufferInfo framebufferInfo;
        framebufferInfo.colorBuffer = colorBuffer.release();
        framebufferInfo.depthStencilBuffer = depthStencilBuffer.release();
        levelDrawFramebuffer = Render::mainRenderInstance->createFramebuffer(framebufferInfo);

        fullscreenDescriptorSet->updateItems({{1, &levelDrawFramebuffer->getColorBuffer()}});
    }

    LevelRenderer::~LevelRenderer() = default;
}

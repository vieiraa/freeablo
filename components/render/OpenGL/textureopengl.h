#pragma once
#include <glad/glad.h>
#include <render/OpenGL/scopedbindgl.h>
#include <render/texture.h>

namespace Render
{
    class RenderInstanceOpenGL;

    class TextureOpenGL final : public Texture, public BindableGL
    {
        using super = Texture;

    public:
        TextureOpenGL(RenderInstanceOpenGL& instance, const BaseTextureInfo& info);
        ~TextureOpenGL() override;

        void updateImageData(int32_t x, int32_t y, int32_t layer, int32_t width, int32_t height, const uint8_t* rgba8UnormData) override;

    public:
        void bind(std::optional<GLuint> extra1, std::optional<GLuint> extra2) override;
        void unbind(std::optional<GLuint> extra1, std::optional<GLuint> extra2) override;

    private:
        GLenum getBindPoint() const;

    private:
        GLuint mId = 0;

        friend class CommandQueueOpenGL;
    };
}

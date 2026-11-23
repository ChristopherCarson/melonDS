#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLFunctions_3_2_Core>

#include "../src/frontend/qt_sdl/main_shaders.h"

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QSurfaceFormat format;
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create())
    {
        std::fputs("Could not create an OpenGL context\n", stderr);
        return 1;
    }
    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface))
    {
        std::fputs("Could not activate the offscreen surface\n", stderr);
        return 1;
    }
    auto* gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(&context);
    if (!gl || !gl->initializeOpenGLFunctions())
    {
        std::fputs("OpenGL 3.2 functions unavailable\n", stderr);
        return 1;
    }

    QOpenGLShaderProgram program;
    const char* vertex = R"(#version 140
uniform float layer;
out vec3 fTexcoord;
void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    fTexcoord = vec3(p, layer);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex) ||
        !program.addShaderFromSourceCode(QOpenGLShader::Fragment, kScreenFS) ||
        !program.link())
    {
        std::fprintf(stderr, "%s\n", qPrintable(program.log()));
        return 1;
    }

    constexpr int width = 256 * 6;
    constexpr int height = 192 * 6;
    constexpr int channels = 4;
    const size_t frameSize = width * height * channels;
    std::vector<unsigned char> data(frameSize * 2);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            size_t pos = (y * width + x) * channels;
            bool foreground = ((x / 6) + (y / 6)) % 2 == 0;
            data[pos] = foreground ? 220 : 20;
            data[pos + 1] = static_cast<unsigned char>(x % 6 * 35);
            data[pos + 2] = foreground ? 40 : 200;
            data[pos + 3] = 255;
            data[frameSize + pos] = 255;
            data[frameSize + pos + 3] = 255;
        }
    }

    GLuint texture, vao;
    gl->glGenTextures(1, &texture);
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, 2, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    gl->glGenVertexArrays(1, &vao);
    gl->glBindVertexArray(vao);
    QOpenGLFramebufferObject output(width, height);
    if (!output.isValid() || !output.bind() || !program.bind())
    {
        std::fputs("Could not bind the test framebuffer or shader\n", stderr);
        return 1;
    }
    program.setUniformValue("ScreenTex", 0);
    gl->glViewport(0, 0, width, height);
    std::vector<unsigned char> pixels(frameSize);
    auto render = [&](int layer, bool filtered)
    {
        program.setUniformValue("layer", float(layer));
        program.setUniformValue("uHQ4x", filtered);
        gl->glDrawArrays(GL_TRIANGLES, 0, 3);
        gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    };

    render(0, false);
    assert(std::equal(pixels.begin(), pixels.end(), data.begin()));
    render(0, true);
    assert(!std::equal(pixels.begin(), pixels.end(), data.begin()));
    bool retainedSubpixelDetail = false;
    for (int y = 6; y < 12; y++)
        for (int x = 6; x < 11; x++)
        {
            size_t pos = (y * width + x) * channels;
            if (pixels[pos + 1] != pixels[pos + channels + 1])
                retainedSubpixelDetail = true;
        }
    assert(retainedSubpixelDetail);
    render(1, true);
    for (size_t i = 0; i < frameSize; i += channels)
    {
        assert(pixels[i] >= 254);
        assert(pixels[i + 1] == 0 && pixels[i + 2] == 0 && pixels[i + 3] == 255);
    }
    gl->glDeleteVertexArrays(1, &vao);
    gl->glDeleteTextures(1, &texture);
    std::puts("HQ4x GPU checks passed");
    return 0;
}

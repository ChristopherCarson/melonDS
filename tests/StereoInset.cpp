#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <QFile>
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVersionFunctionsFactory>

struct BGConfig
{
    int mask, pad[3];
    struct BG { int size[2], type, pal, tile, map, clamp, pad; } bg[4];
};
struct Scanline
{
    int offset[4][4], rotscale[2][4], back;
    uint32_t windows;
    int winmask, pad, winpos[4], mosaic[4], size[4];
};
struct Config
{
    int priority[4], obj, is3d, blend, effect, coefficients[4];
};

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QSurfaceFormat format;
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create()) return 1;
    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface)) return 1;
    auto* gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(&context);
    if (!gl || !gl->initializeOpenGLFunctions()) return 1;

    QFile fragment("src/OpenGL_shaders/2DCompositorFS.glsl");
    if (!fragment.open(QIODevice::ReadOnly)) return 1;
    const char* vertex = R"(#version 140
uniform int uScaleFactor;
out vec4 fTexcoord;
void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
    fTexcoord = vec4(p * vec2(256, 192), p * vec2(256, 192) * uScaleFactor);
}
)";
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex) ||
        !program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment.readAll()) ||
        !program.link() || !program.bind())
    {
        std::fprintf(stderr, "%s\n", qPrintable(program.log()));
        return 1;
    }

    for (int i = 0; i < 4; i++)
        program.setUniformValue(qPrintable(QString("BGLayerTex[%1]").arg(i)), i);
    program.setUniformValue("OBJLayerTex", 4);
    program.setUniformValue("Capture128Tex", 5);
    program.setUniformValue("Capture256Tex", 6);
    program.setUniformValue("MosaicTex", 7);
    program.setUniformValue("uScaleFactor", 1);
    program.setUniformValue("uStereoInset", 16.0f);

    GLuint textures[8], output, framebuffer, vao, buffers[3];
    gl->glGenTextures(8, textures);
    gl->glGenTextures(1, &output);
    gl->glGenFramebuffers(1, &framebuffer);
    gl->glGenVertexArrays(1, &vao);
    gl->glGenBuffers(3, buffers);
    gl->glBindVertexArray(vao);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl->glBindTexture(GL_TEXTURE_2D, output);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output, 0);
    assert(gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    gl->glViewport(0, 0, 256, 192);

    std::vector<uint8_t> data(256 * 192 * 4 * 2);
    for (int i = 0; i < 8; i++)
    {
        gl->glActiveTexture(GL_TEXTURE0 + i);
        GLenum target = i >= 4 && i <= 6 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        gl->glBindTexture(target, textures[i]);
        gl->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl->glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        std::fill(data.begin(), data.end(), 0);
        if (i == 0)
            for (int y = 0; y < 192; y++)
                for (int x = 0; x < 256; x++)
                {
                    data[(y * 256 + x) * 4] = x;
                    data[(y * 256 + x) * 4 + 3] = 255;
                }
        if (i == 1)
            for (int p = 0; p < 256 * 192; p++)
            {
                data[p * 4 + 1] = 255;
                data[p * 4 + 3] = 255;
            }
        if (i == 4)
            for (int y = 0; y < 192; y++)
                for (int x = 0; x < 256; x++)
                {
                    if (x >= 16 && x < 240) continue;
                    int p = y * 256 + x;
                    data[p * 4 + 2] = 255;
                    data[p * 4 + 3] = 255;
                    data[(256 * 192 + p) * 4 + 2] = 255;
                }
        if (i == 7)
        {
            std::vector<int> mosaic(256 * 16);
            gl->glTexImage2D(target, 0, GL_R32I, 256, 16, 0, GL_RED_INTEGER, GL_INT, mosaic.data());
        }
        else if (target == GL_TEXTURE_2D_ARRAY)
            gl->glTexImage3D(target, 0, GL_RGBA8, 256, 192, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        else
            gl->glTexImage2D(target, 0, GL_RGBA8, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    BGConfig bg {};
    for (auto& layer : bg.bg) { layer.size[0] = 256; layer.size[1] = 192; }
    std::array<Scanline, 192> lines {};
    for (int y = 0; y < 192; y++)
    {
        lines[y].offset[0][1] = lines[y].offset[1][1] = y;
        lines[y].windows = 0x3F3F3F3F;
    }
    Config cfg {};
    cfg.priority[0] = 1;
    cfg.priority[1] = cfg.priority[2] = cfg.priority[3] = -1;
    cfg.is3d = 1;
    const char* blocks[] = {"ubBGConfig", "ubScanlineConfig", "ubCompositorConfig"};
    const void* values[] = {&bg, lines.data(), &cfg};
    const size_t sizes[] = {sizeof(bg), sizeof(lines), sizeof(cfg)};
    for (int i = 0; i < 3; i++)
    {
        gl->glBindBuffer(GL_UNIFORM_BUFFER, buffers[i]);
        gl->glBufferData(GL_UNIFORM_BUFFER, sizes[i], values[i], GL_DYNAMIC_DRAW);
        gl->glBindBufferBase(GL_UNIFORM_BUFFER, i, buffers[i]);
        gl->glUniformBlockBinding(program.programId(), gl->glGetUniformBlockIndex(program.programId(), blocks[i]), i);
    }
    auto upload = [&](int index)
    {
        gl->glBindBuffer(GL_UNIFORM_BUFFER, buffers[index]);
        gl->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizes[index], values[index]);
    };
    auto pixel = [&](int x)
    {
        std::array<uint8_t, 4> color {};
        gl->glReadPixels(x, 100, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color.data());
        return color;
    };

    // The 3D gradient stays in its original position.
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(pixel(32)[0] == 32 && pixel(32)[3] == 128);

    // An edge sprite moves into the visible area without moving the 3D behind it.
    cfg.obj = 1;
    upload(2);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(pixel(16)[2] == 252 && pixel(16)[3] == 255);
    assert(pixel(239)[2] == 252 && pixel(239)[3] == 255);
    assert(pixel(32)[0] == 32 && pixel(32)[3] == 128);
    program.setUniformValue("uStereoInset", 0.0f);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(pixel(0)[2] == 252 && pixel(16)[2] == 0);

    // 3D windows use original coordinates; 2D windows follow the inset.
    program.setUniformValue("uStereoInset", 16.0f);
    cfg.obj = 0;
    cfg.priority[1] = 0;
    for (auto& line : lines)
    {
        line.winmask = 1 << 1;
        line.winpos[1] = 32;
        line.windows = (1u << 24) | (1u << 1);
    }
    upload(1);
    upload(2);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(pixel(20)[0] == 20 && pixel(20)[3] == 128);
    assert(pixel(35)[0] == 0 && pixel(35)[1] == 0);
    assert(pixel(45)[1] == 252 && pixel(45)[3] == 255);

    // An object window must not move the 3D clipping region.
    cfg.priority[1] = -1;
    for (auto& line : lines)
    {
        line.winmask = 0;
        line.windows = 1;
    }
    upload(1);
    upload(2);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(pixel(16)[0] == 16 && pixel(16)[3] == 128);
    assert(pixel(8)[0] == 0 && pixel(8)[3] == 255);

    // Flat dual-screen 3D must use the same inset as alternating captured 2D frames.
    program.setUniformValue("uStereoInset3D", true);
    for (auto& line : lines)
        line.windows = 0x3F3F3F3F;
    upload(1);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(pixel(16)[0] == 0 && pixel(16)[3] == 128);
    assert(pixel(128)[0] == 128 && pixel(128)[3] == 128);
    assert(pixel(239)[0] == 252 && pixel(239)[3] == 128);
    assert(gl->glGetError() == GL_NO_ERROR);
    std::puts("Stereo 2D inset compositor checks passed");
    return 0;
}

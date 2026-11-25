#include <cassert>
#include <cstdint>
#include <cstdio>

#include <QFile>
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVersionFunctionsFactory>

struct FinalPassConfig
{
    uint32_t swap[192];
    uint32_t scale, auxLayer, dispModeA, dispModeB;
    uint32_t brightModeA, brightModeB, brightFactorA, brightFactorB;
    float auxColorFactor;
    uint32_t padding[3];
};

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
        std::fputs("Could not create OpenGL context\n", stderr);
        return 1;
    }
    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface))
    {
        std::fputs("Could not activate offscreen surface\n", stderr);
        return 1;
    }
    auto* gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(&context);
    if (!gl || !gl->initializeOpenGLFunctions())
    {
        std::fputs("OpenGL 3.2 functions unavailable\n", stderr);
        return 1;
    }

    QFile fragment("src/OpenGL_shaders/FinalPassFS.glsl");
    if (!fragment.open(QIODevice::ReadOnly))
    {
        std::fputs("Could not open final-pass shader\n", stderr);
        return 1;
    }
    const char* vertex = R"(#version 140
out vec3 fTexcoord;
void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    fTexcoord = vec3(p, p.y);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex) ||
        !program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment.readAll()))
    {
        std::fprintf(stderr, "%s\n", qPrintable(program.log()));
        return 1;
    }
    gl->glBindFragDataLocation(program.programId(), 0, "oTopColor");
    gl->glBindFragDataLocation(program.programId(), 1, "oBottomColor");
    if (!program.link() || !program.bind())
    {
        std::fprintf(stderr, "%s\n", qPrintable(program.log()));
        return 1;
    }

    GLuint inputs[3], output, framebuffer, vao, ubo;
    gl->glGenTextures(3, inputs);
    const uint8_t colors[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    for (int i = 0; i < 3; i++)
    {
        gl->glBindTexture(GL_TEXTURE_2D, inputs[i]);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors[i]);
    }
    gl->glGenTextures(1, &output);
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, output);
    gl->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 1, 1, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl->glGenFramebuffers(1, &framebuffer);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    const GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    gl->glDrawBuffers(2, attachments);
    gl->glGenVertexArrays(1, &vao);
    gl->glBindVertexArray(vao);
    gl->glGenBuffers(1, &ubo);
    gl->glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    FinalPassConfig config {};
    config.scale = 1;
    config.dispModeA = config.dispModeB = 1;
    gl->glBufferData(GL_UNIFORM_BUFFER, sizeof(config), &config, GL_DYNAMIC_DRAW);
    gl->glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    gl->glUniformBlockBinding(program.programId(), gl->glGetUniformBlockIndex(program.programId(), "ubFinalPassConfig"), 0);
    program.setUniformValue("MainInputTexA", 0);
    program.setUniformValue("MainInputTexB", 1);
    program.setUniformValue("AuxInputTex", 2);
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_2D, inputs[2]);
    gl->glViewport(0, 0, 1, 1);

    for (int swap = 0; swap < 2; swap++)
    {
        for (auto& line : config.swap) line = swap;
        gl->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(config), &config);
        for (int eye = 0; eye < 2; eye++)
        {
            gl->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, output, 0, eye + 4);
            gl->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, output, 0, eye + 2);
            if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::fputs("Stereo framebuffer incomplete\n", stderr);
                return 1;
            }
            gl->glActiveTexture(GL_TEXTURE0);
            gl->glBindTexture(GL_TEXTURE_2D, inputs[eye]);
            gl->glDrawArrays(GL_TRIANGLES, 0, 3);
            for (int screen = 0; screen < 2; screen++)
            {
                uint8_t pixel[4] {};
                gl->glReadBuffer(attachments[screen]);
                gl->glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                int expectedChannel = (screen == swap) ? 2 : eye;
                for (int c = 0; c < 3; c++)
                    assert(pixel[c] == (c == expectedChannel ? 252 : 0));
                assert(pixel[3] == 255);
            }
        }
    }

    uint8_t gradient[256 * 4];
    for (int x = 0; x < 256; x++)
    {
        gradient[x * 4] = x;
        gradient[x * 4 + 1] = 0;
        gradient[x * 4 + 2] = 0;
        gradient[x * 4 + 3] = 255;
    }
    for (int i : {0, 2})
    {
        gl->glBindTexture(GL_TEXTURE_2D, inputs[i]);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, gradient);
    }
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, output);
    gl->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 256, 1, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, output, 0, 4);
    gl->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, output, 0, 2);
    gl->glViewport(0, 0, 256, 1);
    for (auto& line : config.swap) line = 0;
    gl->glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    gl->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(config), &config);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, inputs[0]);
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_2D, inputs[2]);
    program.setUniformValue("uStereoInset", 16.0f);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    for (int x : {16, 32, 128, 239})
    {
        uint8_t pixel[4] {};
        int sourceX = static_cast<int>((x + 0.5f - 16.0f) * 256.0f / 224.0f);
        gl->glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl->glReadPixels(x, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        assert(pixel[0] == (sourceX & ~3));
        gl->glReadBuffer(GL_COLOR_ATTACHMENT1);
        gl->glReadPixels(x, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        assert(pixel[0] == (x & ~3));
    }
    gl->glDeleteBuffers(1, &ubo);
    gl->glDeleteVertexArrays(1, &vao);
    gl->glDeleteFramebuffers(1, &framebuffer);
    gl->glDeleteTextures(1, &output);
    gl->glDeleteTextures(3, inputs);
    std::puts("Stereo final-pass routing checks passed");
    return 0;
}

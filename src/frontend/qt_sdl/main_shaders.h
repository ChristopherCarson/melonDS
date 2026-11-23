/*
    Copyright 2016-2026 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef MAIN_SHADERS_H
#define MAIN_SHADERS_H

const char* kScreenVS = R"(#version 140

uniform vec2 uScreenSize;
uniform mat2x3 uTransform;

in vec2 vPosition;
in vec3 vTexcoord;

smooth out vec3 fTexcoord;

void main()
{
    vec4 fpos;

    fpos.xy = vec3(vPosition, 1.0) * uTransform;

    fpos.xy = ((fpos.xy * 2.0) / uScreenSize) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;

    gl_Position = fpos;
    fTexcoord = vTexcoord;
}
)";

const char* kScreenFS = R"(#version 140

uniform sampler2DArray ScreenTex;
uniform int uUpscaleFilter; // 0 = off, 1 = HQ4x, 2 = xBRZ
uniform bool uUpscale2DOnly; // skip pixels tagged as 3D (alpha < 1)
uniform vec2 uCrop; // left/right edge crop in texcoord units

smooth in vec3 fTexcoord;

out vec4 oColor;

// 4xGLSLHqFilter by guest(r), Copyright (C) 2005.
// Distributed under the GNU GPL, version 2 or (at your option) any later version.
// Adapted from melonDS Android's HQ4x display shader.
vec3 HQ4xSample(vec2 offset)
{
    return texture(ScreenTex, vec3(fTexcoord.xy + offset, fTexcoord.z)).rgb;
}

vec3 HQ4x()
{
    vec2 halfPixel = vec2(0.5 / 256.0, 0.5 / 192.0);
    vec2 d1 = halfPixel;
    vec2 d2 = vec2(-halfPixel.x, halfPixel.y);
    vec3 c = HQ4xSample(vec2(0.0));
    vec3 i1 = HQ4xSample(-d1 * 0.5);
    vec3 i2 = HQ4xSample(-d2 * 0.5);
    vec3 i3 = HQ4xSample(d1 * 0.5);
    vec3 i4 = HQ4xSample(d2 * 0.5);
    vec3 o1 = HQ4xSample(-d1);
    vec3 o2 = HQ4xSample(-d2);
    vec3 o3 = HQ4xSample(d1);
    vec3 o4 = HQ4xSample(d2);
    vec3 s1 = HQ4xSample(vec2(0.0, -halfPixel.y));
    vec3 s2 = HQ4xSample(vec2(halfPixel.x, 0.0));
    vec3 s3 = HQ4xSample(vec2(0.0, halfPixel.y));
    vec3 s4 = HQ4xSample(vec2(-halfPixel.x, 0.0));
    vec3 dt = vec3(1.0);
    float ko1 = dot(abs(o1 - c), dt);
    float ko2 = dot(abs(o2 - c), dt);
    float ko3 = dot(abs(o3 - c), dt);
    float ko4 = dot(abs(o4 - c), dt);
    float k1 = min(dot(abs(i1 - i3), dt), max(ko1, ko3));
    float k2 = min(dot(abs(i2 - i4), dt), max(ko2, ko4));
    float w1 = k2; if (ko3 < ko1) w1 *= ko3 / ko1;
    float w2 = k1; if (ko4 < ko2) w2 *= ko4 / ko2;
    float w3 = k2; if (ko1 < ko3) w3 *= ko1 / ko3;
    float w4 = k1; if (ko2 < ko4) w4 *= ko2 / ko4;
    c = (w1*o1 + w2*o2 + w3*o3 + w4*o4 + 0.001*c) /
        (w1 + w2 + w3 + w4 + 0.001);
    w1 = -1.10 * dot(abs(i1-c) + abs(i3-c), dt) / (0.125*dot(i1+i3, dt) + 0.33);
    w2 = -1.10 * dot(abs(i2-c) + abs(i4-c), dt) / (0.125*dot(i2+i4, dt) + 0.33);
    w3 = -1.10 * dot(abs(s1-c) + abs(s3-c), dt) / (0.125*dot(s1+s3, dt) + 0.33);
    w4 = -1.10 * dot(abs(s2-c) + abs(s4-c), dt) / (0.125*dot(s2+s4, dt) + 0.33);
    w1 = clamp(w1 + 1.0, 0.03, 0.75);
    w2 = clamp(w2 + 1.0, 0.03, 0.75);
    w3 = clamp(w3 + 1.0, 0.03, 0.75);
    w4 = clamp(w4 + 1.0, 0.03, 0.75);
    return (w1*(i1+i3) + w2*(i2+i4) + w3*(s1+s3) + w4*(s2+s4) + c) /
           (2.0*(w1+w2+w3+w4) + 1.0);
}

// Resolution-independent implementation of the xBRZ rules (Zenju's algorithm):
// per-corner edge detection on the native 256x192 grid, then line or corner
// blending with analytically antialiased edges, so it works at any display size.
const vec2 kNativeSize = vec2(256.0, 192.0);
const int BLEND_NONE = 0;
const int BLEND_NORMAL = 1;
const int BLEND_DOMINANT = 2;

vec2 XBRZCell;

vec3 XBRZFetch(vec2 offset)
{
    return texture(ScreenTex, vec3((XBRZCell + offset + 0.5) / kNativeSize, fTexcoord.z)).rgb;
}

float XBRZDist(vec3 a, vec3 b)
{
    // YCbCr distance (BT.2020 weights)
    vec3 d = a - b;
    float y = dot(d, vec3(0.2627, 0.6780, 0.0593));
    float cb = 0.5 / (1.0 - 0.0593) * (d.b - y);
    float cr = 0.5 / (1.0 - 0.2627) * (d.r - y);
    return sqrt(y*y + cb*cb + cr*cr);
}

bool XBRZEq(vec3 a, vec3 b) { return XBRZDist(a, b) < 30.0 / 255.0; }
bool XBRZSame(vec3 a, vec3 b) { return all(lessThan(abs(a - b), vec3(0.5 / 255.0))); }

// Blend type at the corner of pixel 'o' that touches the center of the 2x2
// block whose top-left pixel is at 'o + blockOffset'. 'which' picks the pixel
// of the block that 'o' is: 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right.
int XBRZCornerType(vec2 blockOffset, int which)
{
    //     B C
    //   E F G H
    //   I J K L
    //     N O
    vec2 o = blockOffset;
    vec3 B = XBRZFetch(o + vec2(0, -1)), C = XBRZFetch(o + vec2(1, -1));
    vec3 E = XBRZFetch(o + vec2(-1, 0)), F = XBRZFetch(o), G = XBRZFetch(o + vec2(1, 0)), H = XBRZFetch(o + vec2(2, 0));
    vec3 I = XBRZFetch(o + vec2(-1, 1)), J = XBRZFetch(o + vec2(0, 1)), K = XBRZFetch(o + vec2(1, 1)), L = XBRZFetch(o + vec2(2, 1));
    vec3 N = XBRZFetch(o + vec2(0, 2)), O = XBRZFetch(o + vec2(1, 2));

    if ((XBRZSame(F, G) && XBRZSame(J, K)) || (XBRZSame(F, J) && XBRZSame(G, K)))
        return BLEND_NONE;

    float jg = XBRZDist(I, F) + XBRZDist(F, C) + XBRZDist(N, K) + XBRZDist(K, H) + 4.0 * XBRZDist(J, G);
    float fk = XBRZDist(E, J) + XBRZDist(J, O) + XBRZDist(B, G) + XBRZDist(G, L) + 4.0 * XBRZDist(F, K);

    if (jg < fk)
    {
        int type = (3.6 * jg < fk) ? BLEND_DOMINANT : BLEND_NORMAL;
        if (which == 0 && !XBRZSame(F, G) && !XBRZSame(F, J)) return type;
        if (which == 3 && !XBRZSame(K, J) && !XBRZSame(K, G)) return type;
    }
    else if (fk < jg)
    {
        int type = (3.6 * fk < jg) ? BLEND_DOMINANT : BLEND_NORMAL;
        if (which == 2 && !XBRZSame(J, F) && !XBRZSame(J, K)) return type;
        if (which == 1 && !XBRZSame(G, F) && !XBRZSame(G, K)) return type;
    }
    return BLEND_NONE;
}

// corner types of the center pixel, indexed by direction
ivec4 XBRZCorners; // x = bottom-right, y = bottom-left, z = top-left, w = top-right

int XBRZCornerAt(vec2 dir)
{
    if (dir.x > 0.0) return dir.y > 0.0 ? XBRZCorners.x : XBRZCorners.w;
    return dir.y > 0.0 ? XBRZCorners.y : XBRZCorners.z;
}

float XBRZCoverage(float dist, float aa)
{
    return clamp(dist / aa + 0.5, 0.0, 1.0);
}

// Blend toward the corner at (+dirX, +dirY); 'p' is the position inside the
// source pixel, relative to its center.
vec3 XBRZBlendCorner(vec3 color, vec2 dirX, vec2 dirY, vec2 p, float aa)
{
    int type = XBRZCornerAt(dirX + dirY);
    if (type == BLEND_NONE)
        return color;

    vec3 b = XBRZFetch(-dirY), c = XBRZFetch(dirX - dirY);
    vec3 d = XBRZFetch(-dirX), e = XBRZFetch(vec2(0.0)), f = XBRZFetch(dirX);
    vec3 g = XBRZFetch(-dirX + dirY), h = XBRZFetch(dirY), i = XBRZFetch(dirX + dirY);

    bool lineBlend = true;
    if (type != BLEND_DOMINANT)
    {
        // avoid double blending of insular pixels and full blending of L-shapes
        if (XBRZCornerAt(dirX - dirY) != BLEND_NONE && !XBRZEq(e, g)) lineBlend = false;
        else if (XBRZCornerAt(-dirX + dirY) != BLEND_NONE && !XBRZEq(e, c)) lineBlend = false;
        else if (!XBRZEq(e, i) && XBRZEq(g, h) && XBRZEq(h, i) && XBRZEq(i, f) && XBRZEq(f, c)) lineBlend = false;
    }

    vec3 px = XBRZDist(e, f) <= XBRZDist(e, h) ? f : h;
    float x = dot(p, dirX), y = dot(p, dirY);
    float w;
    if (lineBlend)
    {
        float fg = XBRZDist(f, g), hc = XBRZDist(h, c);
        bool shallow = 2.2 * fg <= hc && !XBRZSame(e, g) && !XBRZSame(d, g);
        bool steep = 2.2 * hc <= fg && !XBRZSame(e, c) && !XBRZSame(b, c);
        float wShallow = XBRZCoverage((x + 2.0*y - 0.5) / sqrt(5.0), aa);
        float wSteep = XBRZCoverage((2.0*x + y - 0.5) / sqrt(5.0), aa);
        if (shallow && steep) w = max(wShallow, wSteep);
        else if (shallow) w = wShallow;
        else if (steep) w = wSteep;
        else w = XBRZCoverage((x + y - 0.5) / sqrt(2.0), aa);
    }
    else
        w = XBRZCoverage((x + y - 0.67) / sqrt(2.0), aa);

    return mix(color, px, w);
}

vec3 XBRZ(float aa)
{
    vec2 src = fTexcoord.xy * kNativeSize;
    XBRZCell = floor(src);
    vec2 p = src - XBRZCell - 0.5;

    XBRZCorners = ivec4(XBRZCornerType(vec2(0, 0), 0),
                        XBRZCornerType(vec2(-1, 0), 1),
                        XBRZCornerType(vec2(-1, -1), 3),
                        XBRZCornerType(vec2(0, -1), 2));

    vec3 color = XBRZFetch(vec2(0.0));
    if (XBRZCorners == ivec4(BLEND_NONE))
        return color;

    // the four rotations of the bottom-right corner rule
    color = XBRZBlendCorner(color, vec2(1, 0), vec2(0, 1), p, aa);
    color = XBRZBlendCorner(color, vec2(0, 1), vec2(-1, 0), p, aa);
    color = XBRZBlendCorner(color, vec2(-1, 0), vec2(0, -1), p, aa);
    color = XBRZBlendCorner(color, vec2(0, -1), vec2(1, 0), p, aa);
    return color;
}

void main()
{
    // xBRZ edge antialiasing width: output pixel footprint in native pixels
    // (derivatives must be taken before any per-pixel branching)
    vec2 nativePos = fTexcoord.xy * kNativeSize;
    float aa = max(max(fwidth(nativePos.x), fwidth(nativePos.y)), 0.01);

    if (fTexcoord.x < uCrop.x || fTexcoord.x > 1.0 - uCrop.y)
    {
        oColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec4 base = texture(ScreenTex, fTexcoord);
    int upscale = (uUpscale2DOnly && base.a <= 0.75) ? 0 : uUpscaleFilter;
    vec3 color = base.rgb;
    if (upscale == 1) color = HQ4x();
    else if (upscale == 2) color = XBRZ(aa);
    oColor = vec4(color, 1.0);
}
)";

#endif // MAIN_SHADERS_H

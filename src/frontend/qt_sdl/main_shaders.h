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
uniform bool uHQ4x;

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

void main()
{
    oColor = vec4(uHQ4x ? HQ4x() : texture(ScreenTex, fTexcoord).rgb, 1.0);
}
)";

#endif // MAIN_SHADERS_H

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
*/

#pragma once

#include <array>
#include <cmath>
#include "types.h"

namespace melonDS
{

struct ComputeSpanVertex
{
    s32 X, Y, Z, W;
    s32 Color[3];
    s32 Texcoord[2];
};

// A malformed ten-vertex polygon may cross each clipping plane repeatedly.
using ComputeSpanVertices = std::array<ComputeSpanVertex, 32>;

inline s32 ComputeStereoShiftX(s32 clipW, int scale, double separation, int eye)
{
    if (!eye || !clipW) return 0;
    double shift = (eye == 1 ? 1.5 : -1.5) * separation * scale;
    return static_cast<s32>(std::lround(shift * (65536.0 / clipW - 1.0)));
}

inline int ClipComputeStereoPolygon(ComputeSpanVertices& vertices, int count, int width, bool wbuffer)
{
    auto interpolate = [wbuffer](const ComputeSpanVertex& a, const ComputeSpanVertex& b, int x)
    {
        double t = (static_cast<double>(x) - a.X) / (static_cast<double>(b.X) - a.X);
        double denominator = t * a.W + (1.0 - t) * b.W;
        double perspective = denominator != 0.0 ? t * a.W / denominator : 0.0;
        auto lerp = [](s32 a, s32 b, double f)
        {
            return static_cast<s32>(std::lround(a + (static_cast<double>(b) - a) * f));
        };
        ComputeSpanVertex v {};
        v.X = x;
        v.Y = lerp(a.Y, b.Y, t);
        v.Z = lerp(a.Z, b.Z, wbuffer ? perspective : t);
        v.W = lerp(a.W, b.W, perspective);
        for (int i = 0; i < 3; i++)
            v.Color[i] = lerp(a.Color[i], b.Color[i], perspective);
        for (int i = 0; i < 2; i++)
            v.Texcoord[i] = lerp(a.Texcoord[i], b.Texcoord[i], perspective);
        return v;
    };

    for (int side = 0; side < 2 && count; side++)
    {
        ComputeSpanVertices clipped {};
        int nextCount = 0;
        int boundary = side ? width : 0;
        auto inside = [side, boundary](const ComputeSpanVertex& v)
        {
            return side ? v.X <= boundary : v.X >= boundary;
        };
        ComputeSpanVertex previous = vertices[count-1];
        bool previousInside = inside(previous);
        for (int i = 0; i < count; i++)
        {
            const ComputeSpanVertex& current = vertices[i];
            bool currentInside = inside(current);
            if (currentInside != previousInside && current.X != boundary && previous.X != boundary)
                clipped[nextCount++] = interpolate(previous, current, boundary);
            if (currentInside)
                clipped[nextCount++] = current;
            previous = current;
            previousInside = currentInside;
        }
        vertices = clipped;
        count = nextCount;
    }
    return count;
}

}

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

#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "GPU3D.h"

namespace melonDS
{

class AdaptiveStereo
{
public:
    void Reset() { factor = 1.0; }

    double Update(const Polygon* const* polygons, u32 count, int separation, int target)
    {
        struct Sample { double disparity; double area; };
        std::vector<Sample> samples;
        samples.reserve(count);
        double totalArea = 0.0;

        for (u32 i = 0; i < count; i++)
        {
            const Polygon& polygon = *polygons[i];
            if (polygon.Degenerate || polygon.IsShadowMask || polygon.NumVertices < 3)
                continue;

            int minX = 256, maxX = 0, minY = 192, maxY = 0;
            double disparity = 0.0;
            int valid = 0;
            for (u32 j = 0; j < polygon.NumVertices; j++)
            {
                const Vertex& vertex = *polygon.Vertices[j];
                minX = std::min(minX, std::clamp(vertex.FinalPosition[0], 0, 256));
                maxX = std::max(maxX, std::clamp(vertex.FinalPosition[0], 0, 256));
                minY = std::min(minY, std::clamp(vertex.FinalPosition[1], 0, 192));
                maxY = std::max(maxY, std::clamp(vertex.FinalPosition[1], 0, 192));
                if (vertex.Position[3] > 0)
                {
                    disparity += std::abs(65536.0 / vertex.Position[3] - 1.0);
                    valid++;
                }
            }
            double area = static_cast<double>(maxX - minX) * (maxY - minY);
            if (!valid || area <= 0.0) continue;

            // Both eyes shift by 1.5 * separation in opposite directions.
            samples.push_back({3.0 * separation * disparity / valid, area});
            totalArea += area;
        }

        if (samples.empty()) return factor;

        std::sort(samples.begin(), samples.end(), [](const Sample& a, const Sample& b)
        {
            return a.disparity < b.disparity;
        });
        double accumulated = 0.0;
        double disparity = 0.0;
        for (const Sample& sample : samples)
        {
            accumulated += sample.area;
            disparity = sample.disparity;
            if (accumulated >= totalArea * 0.9) break;
        }

        double desired = disparity > 0.0 ? std::min(1.0, target / disparity) : 1.0;
        if (desired < factor)
            factor = desired;
        else
            factor += (desired - factor) * 0.1;
        if (std::abs(desired - factor) < 0.001)
            factor = desired;
        return factor;
    }

private:
    double factor = 1.0;
};

}

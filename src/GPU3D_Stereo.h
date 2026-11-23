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

// Per-vertex stereo depth term: 0 at the convergence plane (W = 1.0), positive in
// front of the screen, negative behind it. Eye shift is +/-1.5 * separation * term.
inline double StereoDepthTerm(s32 clipW)
{
    return clipW > 0 ? 65536.0 / clipW - 1.0 : 0.0;
}

class AdaptiveStereo
{
public:
    void Reset() { factor = 1.0; convergence = 0.0; }

    // Depth term subtracted from every vertex so that near 3D content does not
    // appear in front of the flat 2D layers (which sit at the screen plane).
    double Convergence() const { return convergence; }

    // Returns the separation factor. With keepFlatInFront, also updates Convergence().
    double Update(const Polygon* const* polygons, u32 count, int separation, int target,
                  bool adaptive = true, bool keepFlatInFront = false)
    {
        struct Sample { double depth; double area; u32 first, num; };
        std::vector<Sample> samples;
        std::vector<double> terms;
        samples.reserve(count);
        double totalArea = 0.0;

        for (u32 i = 0; i < count; i++)
        {
            const Polygon& polygon = *polygons[i];
            if (polygon.Degenerate || polygon.IsShadowMask || polygon.NumVertices < 3)
                continue;

            int minX = 256, maxX = 0, minY = 192, maxY = 0;
            double depth = 0.0;
            u32 first = static_cast<u32>(terms.size());
            for (u32 j = 0; j < polygon.NumVertices; j++)
            {
                const Vertex& vertex = *polygon.Vertices[j];
                minX = std::min(minX, std::clamp(vertex.FinalPosition[0], 0, 256));
                maxX = std::max(maxX, std::clamp(vertex.FinalPosition[0], 0, 256));
                minY = std::min(minY, std::clamp(vertex.FinalPosition[1], 0, 192));
                maxY = std::max(maxY, std::clamp(vertex.FinalPosition[1], 0, 192));
                if (vertex.Position[3] > 0)
                {
                    double term = StereoDepthTerm(vertex.Position[3]);
                    terms.push_back(term);
                    depth += term;
                }
            }
            u32 num = static_cast<u32>(terms.size()) - first;
            double area = static_cast<double>(maxX - minX) * (maxY - minY);
            if (!num || area <= 0.0)
            {
                terms.resize(first);
                continue;
            }

            samples.push_back({depth / num, area, first, num});
            totalArea += area;
        }

        if (!keepFlatInFront)
            convergence = 0.0;
        else if (!samples.empty())
        {
            // Converge on the nearest content covering a meaningful share of the
            // scene, ignoring small specks close to the camera.
            std::sort(samples.begin(), samples.end(), [](const Sample& a, const Sample& b)
            {
                return a.depth > b.depth;
            });
            double accumulated = 0.0;
            double nearest = 0.0;
            for (const Sample& sample : samples)
            {
                accumulated += sample.area;
                nearest = sample.depth;
                if (accumulated >= totalArea * 0.05) break;
            }
            double desired = std::max(0.0, nearest);
            convergence += (desired - convergence) * (desired > convergence ? 0.5 : 0.05);
            if (std::abs(desired - convergence) < 0.001)
                convergence = desired;
        }

        if (!adaptive) return 1.0;
        if (samples.empty()) return factor;

        struct Disparity { double disparity; double area; };
        std::vector<Disparity> disparities;
        disparities.reserve(samples.size());
        for (const Sample& sample : samples)
        {
            double sum = 0.0;
            for (u32 j = 0; j < sample.num; j++)
                sum += std::abs(terms[sample.first + j] - convergence);
            // Both eyes shift by 1.5 * separation in opposite directions.
            disparities.push_back({3.0 * separation * sum / sample.num, sample.area});
        }

        std::sort(disparities.begin(), disparities.end(), [](const Disparity& a, const Disparity& b)
        {
            return a.disparity < b.disparity;
        });
        double accumulated = 0.0;
        double disparity = 0.0;
        for (const Disparity& sample : disparities)
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
    double convergence = 0.0;
};

}
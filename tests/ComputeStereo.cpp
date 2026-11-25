#include <cassert>
#include <cmath>

#include "GPU3D_ComputeStereo.h"
#include "GPU3D_Stereo.h"

using namespace melonDS;

int main()
{
    assert(ComputeStereoShiftX(65536, 6, 32, 1) == 0);
    assert(ComputeStereoShiftX(32768, 6, 32, 1) == 288);
    assert(ComputeStereoShiftX(32768, 6, 32, 2) == -288);
    assert(ComputeStereoShiftX(0, 6, 32, 1) == 0);
    assert(ComputeStereoShiftX(32768, 6, 32, 0) == 0);
    assert(ComputeStereoShiftX(32768, 6, 2.5, 1) == 23);

    Vertex sceneVertices[3] {};
    sceneVertices[1].FinalPosition[0] = 256;
    sceneVertices[2].FinalPosition[1] = 192;
    Polygon scene {};
    scene.NumVertices = 3;
    for (int i = 0; i < 3; i++)
    {
        scene.Vertices[i] = &sceneVertices[i];
        sceneVertices[i].Position[3] = 65536;
    }
    const Polygon* polygons[] = {&scene};
    AdaptiveStereo adaptive;
    assert(adaptive.Update(polygons, 1, 24, 24) == 1.0);
    for (auto& vertex : sceneVertices) vertex.Position[3] = 8192;
    double closeFactor = adaptive.Update(polygons, 1, 24, 24);
    assert(std::fabs(closeFactor - 1.0 / 21.0) < 0.00001);
    assert(std::fabs(3.0 * 24.0 * closeFactor * 7.0 - 24.0) < 0.00001);
    assert(adaptive.Update(nullptr, 0, 24, 24) == closeFactor);
    adaptive.Reset();
    assert(std::fabs(adaptive.Update(polygons, 1, 24, 72) - 1.0 / 7.0) < 0.00001);
    for (auto& vertex : sceneVertices) vertex.Position[3] = 65536;
    double recovering = adaptive.Update(polygons, 1, 24, 72);
    assert(recovering > 1.0 / 7.0 && recovering < 1.0);

    Vertex speckVertices[3] {};
    speckVertices[1].FinalPosition[0] = 1;
    speckVertices[2].FinalPosition[1] = 1;
    Polygon speck {};
    speck.NumVertices = 3;
    for (int i = 0; i < 3; i++)
    {
        speck.Vertices[i] = &speckVertices[i];
        speckVertices[i].Position[3] = 1;
    }
    const Polygon* withSpeck[] = {&scene, &speck};
    adaptive.Reset();
    assert(adaptive.Update(withSpeck, 2, 24, 24) == 1.0);

    // A scene close to the camera converges at its depth, leaving no disparity.
    for (auto& vertex : sceneVertices) vertex.Position[3] = 32768;
    adaptive.Reset();
    assert(adaptive.Update(polygons, 1, 24, 24, false, true) == 1.0);
    assert(std::fabs(adaptive.Convergence() - 0.5) < 0.00001);
    for (int i = 0; i < 9; i++) adaptive.Update(polygons, 1, 24, 24, true, true);
    assert(adaptive.Convergence() == 1.0);
    assert(adaptive.Update(polygons, 1, 24, 24, true, true) == 1.0);
    assert(ComputeStereoShiftX(32768, 6, 32, 1, adaptive.Convergence()) == 0);
    // Content behind the screen plane needs no convergence shift.
    for (auto& vertex : sceneVertices) vertex.Position[3] = 131072;
    adaptive.Reset();
    adaptive.Update(polygons, 1, 24, 24, true, true);
    assert(adaptive.Convergence() == 0.0);
    adaptive.Update(polygons, 1, 24, 24, true, false);
    assert(adaptive.Convergence() == 0.0);

    StereoEdgeCrop crop;
    for (auto& vertex : sceneVertices)
    {
        vertex.FinalPosition[0] = 128;
        vertex.Position[3] = 32768;
    }
    assert(crop.Update(polygons, 1, 10.0, 0.0) == 0);
    sceneVertices[0].FinalPosition[0] = 0;
    assert(crop.Update(polygons, 1, 10.0, 0.0) == 16);
    // Both depth directions and both original edges use the same common crop.
    sceneVertices[0].FinalPosition[0] = 255;
    sceneVertices[0].Position[3] = 131072;
    assert(crop.Update(polygons, 1, 40.0, 0.0) == 31);
    assert(crop.Update(polygons, 1, 40.0, -0.5) == 28);
    assert(crop.Update(polygons, 1, 400.0, 0.0) == 64);
    assert(crop.Update(nullptr, 0, 0.0, 0.0) == 58);
    assert(crop.Update(polygons, 1, 400.0, 0.0) == 64);
    crop.Reset();
    assert(crop.Update(polygons, 1, 40.0, -0.5) == 0);
    sceneVertices[0].FinalPosition[0] = 300;
    assert(crop.Update(polygons, 1, 40.0, 0.0) == 0);
    sceneVertices[0].FinalPosition[0] = 0;
    sceneVertices[0].Position[3] = 0;
    assert(crop.Update(polygons, 1, 40.0, 0.0) == 0);
    sceneVertices[0].Position[3] = 131072;
    scene.IsShadowMask = true;
    assert(crop.Update(polygons, 1, 40.0, 0.0) == 0);
    scene.IsShadowMask = false;

    ComputeSpanVertices vertices {};
    vertices[0] = {-100, 0, 100, 100, {0, 10, 20}, {0, 10}};
    vertices[1] = {100, 0, 300, 200, {90, 100, 110}, {90, 100}};
    vertices[2] = {100, 100, 300, 200, {90, 100, 110}, {90, 100}};
    vertices[3] = {-100, 100, 100, 100, {0, 10, 20}, {0, 10}};
    int count = ClipComputeStereoPolygon(vertices, 4, 80, false);
    assert(count == 4);
    assert(vertices[0].X == 0 && vertices[0].Y == 0);
    assert(vertices[0].Z == 200);
    assert(vertices[0].W == 133);
    assert(vertices[0].Color[0] == 30);
    assert(vertices[0].Texcoord[0] == 30);
    for (int i = 0; i < count; i++)
        assert(vertices[i].X >= 0 && vertices[i].X <= 80);

    ComputeSpanVertices outside {};
    outside[0] = {-100, 0, 0, 1, {}, {}};
    outside[1] = {-50, 0, 0, 1, {}, {}};
    outside[2] = {-50, 10, 0, 1, {}, {}};
    assert(ClipComputeStereoPolygon(outside, 3, 80, false) == 0);

    ComputeSpanVertices boundary {};
    boundary[0] = {-1, 0, 0, 1, {}, {}};
    boundary[1] = {0, 10, 0, 1, {}, {}};
    boundary[2] = {1, 0, 0, 1, {}, {}};
    assert(ClipComputeStereoPolygon(boundary, 3, 80, false) == 3);

    ComputeSpanVertices wbuffer {};
    wbuffer[0] = {-100, 0, 100, 100, {}, {}};
    wbuffer[1] = {100, 0, 300, 200, {}, {}};
    wbuffer[2] = {100, 100, 300, 200, {}, {}};
    wbuffer[3] = {-100, 100, 100, 100, {}, {}};
    assert(ClipComputeStereoPolygon(wbuffer, 4, 200, true) == 4);
    assert(wbuffer[0].Z == 167);

    ComputeSpanVertices crossings {};
    for (int i = 0; i < 10; i++)
        crossings[i] = {i % 2 ? 200 : -100, i * 10, 0, 1, {}, {}};
    count = ClipComputeStereoPolygon(crossings, 10, 80, false);
    assert(count <= static_cast<int>(crossings.size()));
    for (int i = 0; i < count; i++)
        assert(crossings[i].X >= 0 && crossings[i].X <= 80);
}

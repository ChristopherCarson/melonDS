#include <cassert>
#include <cmath>

#include "ScreenLayout.h"

int main()
{
    ScreenLayout layout;
    float matrices[kMaxScreenTransforms * 6];
    int kinds[kMaxScreenTransforms];

    layout.SetupStereo(1920, 1080, 0, false, false, false);
    assert(layout.GetScreenTransforms(matrices, kinds) == 3);
    assert(kinds[0] == 0 && kinds[1] == 2 && kinds[2] == 3);
    assert(std::fabs(matrices[0] - 2.8125f) < 0.001f);
    assert(std::fabs(matrices[4] - 600.0f) < 0.001f);
    assert(std::fabs(matrices[10] - 120.0f) < 0.001f);
    assert(std::fabs(matrices[16] - 1080.0f) < 0.001f);

    int x = 480, y = 810;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);
    x = 1440; y = 810;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);
    x = 1000; y = 810;
    assert(!layout.GetTouchCoords(x, y, false));

    x = 480; y = 810;
    assert(layout.GetTouchCoords(x, y, false));
    x = 1440; y = 1200;
    assert(layout.GetTouchCoords(x, y, true) && x == 255 && y == 191);

    layout.SetupStereo(1024, 800, 16, true, false, false);
    assert(layout.GetScreenTransforms(matrices, kinds) == 3);
    assert(matrices[0] == 2.0f);

    layout.SetupStereo(1920, 1080, 0, false, true, false);
    assert(layout.GetScreenTransforms(matrices, kinds) == 2);
    assert(kinds[0] == 2 && kinds[1] == 3);
    assert(matrices[0] == 3.75f && matrices[3] == 3.75f);
    assert(matrices[4] == 0.0f && matrices[5] == 180.0f);
    assert(matrices[10] == 960.0f);

    layout.SetupStereo(1920, 1080, 0, false, true, true);
    assert(layout.GetScreenTransforms(matrices, kinds) == 2);
    assert(kinds[0] == 2 && kinds[1] == 3);
    assert(matrices[0] == 2.8125f && matrices[3] == 5.625f);
    assert(matrices[4] == 120.0f && matrices[10] == 1080.0f);
    x = 480; y = 540;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);
    x = 1440; y = 540;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);
    x = 210; y = 540;
    assert(layout.GetTouchCoords(x, y, false, 16) && x == 18 && y == 96);
    x = 1170; y = 540;
    assert(layout.GetTouchCoords(x, y, false, 16) && x == 18 && y == 96);
    x = 130; y = 540;
    assert(!layout.GetTouchCoords(x, y, false, 16));
    x = 130; y = 540;
    assert(layout.GetTouchCoords(x, y, true, 16) && x == 0 && y == 96);
    x = 0; y = 1200;
    assert(layout.GetTouchCoords(x, y, true) && x == 0 && y == 191);

    layout.SetupStereo(1920, 1080, 0, false, false, true);
    assert(layout.GetScreenTransforms(matrices, kinds) == 3);
    assert(kinds[0] == 0 && kinds[1] == 2 && kinds[2] == 3);
    assert(matrices[6] == 1.40625f && matrices[9] == 2.8125f);
    assert(matrices[10] == 300.0f && matrices[16] == 1260.0f);

    layout.SetupStereo(1920, 1080, 0, false, false, false, true);
    assert(layout.GetScreenTransforms(matrices, kinds) == 4);
    assert(kinds[0] == 4 && kinds[1] == 5 && kinds[2] == 2 && kinds[3] == 3);
    assert(matrices[0] == 2.8125f && matrices[4] == 120.0f);
    assert(matrices[10] == 1080.0f && matrices[16] == 120.0f && matrices[22] == 1080.0f);
    x = 480; y = 270;
    assert(!layout.GetTouchCoords(x, y, false));
    x = 1440; y = 810;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);

    layout.SetupStereo(1920, 1080, 0, false, false, true, true);
    assert(layout.GetScreenTransforms(matrices, kinds) == 4);
    assert(matrices[0] == 1.40625f && matrices[3] == 2.8125f);
    assert(matrices[4] == 300.0f && matrices[10] == 1260.0f);
    x = 480; y = 810;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);

    layout.SetupStereo(1920, 1080, 0, false, false, true, true, true);
    assert(layout.GetScreenTransforms(matrices, kinds) == 2);
    assert(kinds[0] == 4 && kinds[1] == 5);
    assert(matrices[0] == 2.8125f && matrices[3] == 5.625f);
    assert(matrices[4] == 120.0f && matrices[10] == 1080.0f);
    x = 480; y = 540;
    assert(!layout.GetTouchCoords(x, y, false));

    layout.SetupStereo(1920, 1080, 0, false, true, false, true);
    assert(layout.GetScreenTransforms(matrices, kinds) == 2);
    assert(kinds[0] == 2 && kinds[1] == 3);

    layout.SetupStereo(1920, 1080, 0, false, false, false, true, false, false);
    assert(layout.GetScreenTransforms(matrices, kinds) == 3);
    assert(kinds[0] == 4 && kinds[1] == 5 && kinds[2] == 1);
    assert(matrices[0] == 2.8125f && matrices[4] == 120.0f);
    assert(matrices[10] == 1080.0f && matrices[12] == 2.8125f && matrices[16] == 600.0f);
    x = 960; y = 810;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);
    x = 690; y = 810;
    assert(layout.GetTouchCoords(x, y, false, 16) && x == 32 && y == 96);
    x = 480; y = 810;
    assert(!layout.GetTouchCoords(x, y, false));
    x = 0; y = 1200;
    assert(layout.GetTouchCoords(x, y, true) && x == 0 && y == 191);

    layout.SetupStereo(1920, 1080, 0, false, false, true, true, false, false);
    assert(layout.GetScreenTransforms(matrices, kinds) == 3);
    assert(kinds[0] == 4 && kinds[1] == 5 && kinds[2] == 1);
    assert(matrices[0] == 1.40625f && matrices[12] == 2.8125f);
    x = 960; y = 810;
    assert(layout.GetTouchCoords(x, y, false) && x == 128 && y == 96);

    layout.SetupStereo(1920, 1080, 0, false, false, true, true, true, false);
    assert(layout.GetScreenTransforms(matrices, kinds) == 2);
    assert(kinds[0] == 4 && kinds[1] == 5);
    x = 960; y = 540;
    assert(!layout.GetTouchCoords(x, y, false));

    layout.Setup(512, 768, screenLayout_Natural, screenRot_0Deg,
                 screenSizing_TopOnly, 0, false, false, 1.0f, 1.0f);
    assert(layout.GetScreenTransforms(matrices, kinds) == 1 && kinds[0] == 0);
    x = 256; y = 384;
    assert(!layout.GetTouchCoords(x, y, false));

    layout.Setup(512, 768, screenLayout_Natural, screenRot_0Deg,
                 screenSizing_Even, 0, false, false, 1.0f, 1.0f);
    assert(layout.GetScreenTransforms(matrices, kinds) == 2);
    assert(kinds[0] == 0 && kinds[1] == 1);
}

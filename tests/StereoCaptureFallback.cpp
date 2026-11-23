#include <cassert>

#include "StereoCaptureFallback.h"

using namespace melonDS;

int main()
{
    StereoCaptureFallback fallback;
    fallback.Reset(false);

    // Capture alone, or a single screen swap, must not disable stereo.
    for (int i = 0; i < 10; i++) fallback.EndFrame(false, true);
    assert(!fallback.Active());
    fallback.EndFrame(true, true);
    assert(!fallback.Active());
    for (int i = 0; i < 8; i++) fallback.EndFrame(true, true);
    fallback.EndFrame(false, true);
    assert(!fallback.Active());

    // Rapid swaps without recent capture also leave stereo enabled.
    fallback.Reset(false);
    for (int i = 0; i < 10; i++) fallback.EndFrame((i & 1) != 0, false);
    assert(!fallback.Active());

    // Alternating screens and intermittent capture select the flat view.
    fallback.Reset(false);
    fallback.EndFrame(true, true);
    fallback.EndFrame(false, false);
    assert(fallback.Active());
    for (int i = 0; i < 100; i++)
    {
        fallback.EndFrame((i & 1) == 0, (i & 1) == 0);
        assert(fallback.Active());
    }

    // Restore stereo after the last rapid swap, and reset on state load.
    for (int i = 0; i < 59; i++) fallback.EndFrame(false, false);
    assert(fallback.Active());
    fallback.EndFrame(false, false);
    assert(!fallback.Active());
    fallback.EndFrame(true, true);
    fallback.EndFrame(false, true);
    assert(fallback.Active());
    fallback.Reset(false);
    assert(!fallback.Active());
}

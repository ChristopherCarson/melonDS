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

#ifndef STEREO_CAPTURE_FALLBACK_H
#define STEREO_CAPTURE_FALLBACK_H

namespace melonDS
{

class StereoCaptureFallback
{
public:
    void Reset(bool screenSwap)
    {
        LastScreenSwap = screenSwap;
        RecentCapture = 0;
        RecentSwap = 0;
        FlatFrames = 0;
    }

    void EndFrame(bool screenSwap, bool captured)
    {
        if (RecentCapture > 0) --RecentCapture;
        if (RecentSwap > 0) --RecentSwap;
        if (FlatFrames > 0) --FlatFrames;

        if (captured) RecentCapture = HoldDuration;
        if (screenSwap != LastScreenSwap)
        {
            if (RecentSwap > 0 && RecentCapture > 0)
                FlatFrames = HoldDuration;
            RecentSwap = SwapWindow;
        }
        LastScreenSwap = screenSwap;
    }

    bool Active() const { return FlatFrames > 0; }

private:
    static constexpr int HoldDuration = 60;
    static constexpr int SwapWindow = 8;
    bool LastScreenSwap {};
    int RecentCapture {};
    int RecentSwap {};
    int FlatFrames {};
};

}

#endif // STEREO_CAPTURE_FALLBACK_H

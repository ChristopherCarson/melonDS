<p align="center"><img src="https://raw.githubusercontent.com/melonDS-emu/melonDS/master/res/icon/melon_128x128.png"></p>
<h2 align="center"><b>melonDS</b></h2>
<p align="center">
<a href="http://melonds.kuribo64.net/" alt="melonDS website"><img src="https://img.shields.io/badge/website-melonds.kuribo64.net-%2331352e.svg"></a>
<a href="http://melonds.kuribo64.net/downloads.php" alt="Release: 1.1"><img src="https://img.shields.io/badge/release-1.1-%235c913b.svg"></a>
<a href="https://www.gnu.org/licenses/gpl-3.0" alt="License: GPLv3"><img src="https://img.shields.io/badge/License-GPL%20v3-%23ff554d.svg"></a>
<a href="https://kiwiirc.com/client/irc.badnik.net/?nick=IRC-Source_?#melonds" alt="IRC channel: #melonds"><img src="https://img.shields.io/badge/IRC%20chat-%23melonds-%23dd2e44.svg"></a>
<a href="https://discord.gg/pAMAtExcqV" alt="Discord"><img src="https://img.shields.io/badge/Discord-Kuribo64-7289da?logo=discord&logoColor=white"></a>
<br>
<a href="https://github.com/melonDS-emu/melonDS/actions/workflows/build-windows.yml?query=event%3Apush"><img src="https://github.com/melonDS-emu/melonDS/actions/workflows/build-windows.yml/badge.svg" /></a>
<a href="https://github.com/melonDS-emu/melonDS/actions/workflows/build-ubuntu.yml?query=event%3Apush"><img src="https://github.com/melonDS-emu/melonDS/actions/workflows/build-ubuntu.yml/badge.svg" /></a>
<a href="https://github.com/melonDS-emu/melonDS/actions/workflows/build-macos.yml?query=event%3Apush"><img src="https://github.com/melonDS-emu/melonDS/actions/workflows/build-macos.yml/badge.svg" /></a>
<a href="https://github.com/melonDS-emu/melonDS/actions/workflows/build-bsd.yml?query=event%3Apush"><img src="https://github.com/melonDS-emu/melonDS/actions/workflows/build-bsd.yml/badge.svg" /></a>
</p>
DS emulator, sorta

The goal is to do things right and fast, akin to blargSNES (but hopefully better). But also to, you know, have a fun challenge :)
<hr>

## How to use

Firmware boot (not direct boot) requires a BIOS/firmware dump from an original DS or DS Lite.
DS firmwares dumped from a DSi or 3DS aren't bootable and only contain configuration data, thus they are only suitable when booting games directly.

### Possible firmware sizes

 * 128KB: DSi/3DS DS-mode firmware (reduced size due to lacking bootcode)
 * 256KB: regular DS firmware
 * 512KB: iQue DS firmware

DS BIOS dumps from a DSi or 3DS can be used with no compatibility issues. DSi BIOS dumps (in DSi mode) are not compatible. Or maybe they are. I don't know.

As for the rest, the interface should be pretty straightforward. If you have a question, don't hesitate to ask, though!

### Stereoscopic screens

In Config > Video settings, select OpenGL (Classic) or OpenGL (Compute shader) and enable
Stereoscopic bottom screen, Stereoscopic top screen, or both. Compute can avoid shadow
flickering seen with Classic.
Adjust Stereo separation to control the depth effect. Separation is 50% stronger than in
the initial stereo build, including for previously saved settings.

Optionally enable Adaptive stereo strength to reduce separation in scenes with large
estimated eye disparity. Stereo separation remains the maximum strength; Depth target
sets the approximate disparity target in native DS pixels. Lower targets reduce the
effect more aggressively. This scene-based estimate is not a strict limit or a direct
camera-zoom detector, so adjust it for the game. Strength returns gradually as the
scene changes.

Keep 2D in front of 3D (on by default) helps when the camera moves close, such as
zoomed-in dialogue. The game's flat 2D elements, like text boxes and the HUD, sit at
the screen plane, so close-up 3D would otherwise appear in front of them. This option
pushes the whole 3D scene back just far enough that the nearest large 3D objects sit
at the screen plane, then eases back as the camera pulls away. Scenes that stay behind
the screen are unchanged. Use it together with Adaptive stereo strength, which accounts
for the extra depth when reducing separation; without it, distant scenery can end up
with large disparity during close-ups.

The stereo shift leaves thin columns without scene content at the side edges of each
eye view, which can show the backdrop color or artifacts. Raise Stereo edge crop in
Video settings to black out that many native DS pixel columns on both the left and
right edges of every eye view; use the smallest value that hides them. The default
of 0 disables manual cropping. Optional Automatic stereo edge crop estimates the
necessary width from 3D geometry at the screen edges. It grows once a wider crop is
needed for several consecutive frames, ignoring brief camera-transition spikes, and
shrinks gradually. The manual crop remains the minimum, and the automatic estimate is capped
at 64 native pixels per edge. It is not exact: geometry already clipped by the DS
cannot always be detected, and the black bars also cover 2D content near the edges.

Optional Fit 2D graphics inside stereo crop squeezes the displayed 2D layers between
the bars, helping keep edge HUD elements visible. It follows the current crop and
holds the last crop width when no stereo 3D scene is displayed, so full-screen 2D
maps and menus do not widen after a screen swap. Automatic adjustment resumes when
stereo 3D returns. It does not change display capture or the stereo 3D view; when a
game shows 3D on both screens and the emulator uses its flat fallback, that flat 3D is
squeezed with the alternating captured image to prevent flicker.
Other 2D graphics are also squeezed,
including captured images shown as 2D. Touch input follows the squeezed 2D image;
touch targets based on the unchanged 3D scene may be misaligned.

When only bottom stereo is enabled, the top screen appears once above the two bottom
views. To separate the screens, open another window with View > Open new window, then
use View > Screen sizing to set one window to Top only and the other to Bottom only.
Both bottom views accept touch input.
While a stereo layout is shown with the OpenGL display, melonDS replaces the system
mouse pointer with a green cursor drawn at the same position in both eyes, so it
appears at screen depth like the 2D UI. Outside the eye views it is drawn once. The
normal pointer returns in non-stereo layouts and when no game is running; Hide mouse
after inactivity applies to the stereo cursor too.
For a display that expands half side-by-side 3D, enable Half SBS in Video settings;
each eye is squeezed horizontally so the display can expand it to the correct shape.
Enable Stereoscopic top screen to show the top screen as a side-by-side pair,
including in a Top only window. The top pair has depth when the game directs 3D
geometry to that screen; 2D content appears identically in both eyes.
Keyboard and joystick shortcuts for toggling each screen's SBS 3D can be assigned
under Config > Input and hotkeys > General hotkeys. Each shortcut operates independently.

The renderer detects rapid screen swaps near display capture as a sign of dual-screen
3D and temporarily uses the same flat image for both eyes to reduce stereo ghosting.
This heuristic may not recognize every game. Stereo returns shortly after swapping stops.

This experimental mode renders two views of directly displayed 3D geometry. 2D layers
and display captures remain flat, and geometry clipped by the original DS view can be
missing at the edges, especially with larger separation. Stereo layouts override rotation
and screen swapping. Without the top-screen option, a display that splits the whole
picture will also split the single top screen unless it is shown in a separate window.
Stereo renders only the 3D views that are on screen: two eye passes while the 3D
engine is on a stereo screen, or one normal pass when it is on the other screen. Games
that use display capture, or that just swapped screens, briefly render all three passes,
which can be substantially slower, particularly at high internal resolutions. Enabling
the top pair also uses additional graphics memory.

Both OpenGL renderers share a streamlined 2D compositor that selects the top two
visible layers in one background pass and reuses scanline/configuration uploads across
stereo views. Layer priority, blending and emulated scanline timing are unchanged.

With OpenGL (Compute shader), Video settings also offers an optional upscaling filter for
pixel art: the Android-style HQ4x, or xBRZ, which keeps edges sharper. It smooths the
displayed image without reducing the internal rendering resolution. By default
("Upscaling filter: 2D graphics only") it is applied only to 2D graphics such as
backgrounds, sprites and text, keeping 3D models sharp; uncheck that option to filter the
complete image, including 3D. It replaces the regular screen filtering while enabled and
does not affect DS display capture. It can reduce performance, xBRZ more so than HQ4x.

## How to build
See [BUILD.md](./BUILD.md) for build instructions.

## TODO LIST

 * better DSi emulation
 * better OpenGL rendering
 * netplay
 * the impossible quest of pixel-perfect 3D graphics
 * support for rendering screens to separate windows
 * emulating some fancy addons
 * other non-core shit (debugger, graphics viewers, etc)

### TODO LIST FOR LATER (low priority)

 * big-endian compatibility (Wii, etc)
 * LCD refresh time (used by some games for blending effects)
 * any feature you can eventually ask for that isn't outright stupid

## Credits

 * Martin for GBAtek, a good piece of documentation
 * Cydrak for the extra 3D GPU research
 * limittox for the icon
 * All of you comrades who have been testing melonDS, reporting issues, suggesting shit, etc

## Licenses

[![GNU GPLv3 Image](https://www.gnu.org/graphics/gplv3-127x51.png)](http://www.gnu.org/licenses/gpl-3.0.en.html)

melonDS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

### External
* Images used in the Input Config Dialog - see `src/frontend/qt_sdl/InputConfig/resources/LICENSE.md`

To run dynamic build:
The packaged Release build can be started by double-clicking `build\melonDS.exe`.
Keep its DLLs and plugin folders alongside it; the existing `build\melonDS.toml`
continues to hold its settings. The PowerShell launch below also remains available.

$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
& "C:\Users\chris\repos\melonDS\build\melonDS.exe"
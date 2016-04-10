# fhDOOM

## About

fhDOOM is one of my little side projects. I am working on this because its fun and a great learning experience. Its interesting to explore the code and to test out how such an old game can be improved by new features and more modern techniques. I have no plans to develop a real game or something like that with it. Using Unity, Cry Engine or Unreal Engine might be a better choice for that anyways.

### Goals
  * Keep it easy to build
    * simple build system
    * compiles with modern compilers
    * minimal dependencies
  * Keep the original game working, all of it!
    * The game itself
    * The expansion pack Resurrection of Evil
    * The Tools
  * Stay true to the original game. Enhancements are fine, as long as they don't look out of place or destroy the mood of the game.
  * Support for Windows and Linux

### Similar Projects
  There exist other forks of the DOOM3 engine and even id software released a modernized version of DOOM3 and its engine as "DOOM 3 - BFG Edition".
 * DOOM-3-BFG: https://github.com/id-Software/DOOM-3-BFG
 * iodoom3 (discontinued?): https://github.com/iodoom/iod3
 * dhewm3: https://github.com/dhewm/dhewm3
 * RBDOOM-3-BFG (based on DOOM-3-BFG): https://github.com/RobertBeckebans/RBDOOM-3-BFG
 * Dark Mod (total conversion, not sure if it can still run the original DOOM3 game): http://www.thedarkmod.com/

## Changes
 * CMake build system
 * Compiles fine with Visual Studio 2013 (you need to have the MFC Multibyte Addon installed) and Visual Studio 2015
 * Replaced deprecated DirectX SDK by Windows 8.1 SDK 
 * Use GLEW for OpenGL extension handling (replaced all qgl\* calls by normal gl\* calls)
 * Contains all game assets from latest official patch 1.31
 * OpenGL 4.3 core profile  
  * Fixed function and ARB2 assembly shaders are completely gone
  * Re-wrote everything with GLSL  
  * Game and Tools!
 * Soft shadows via shadow mapping
  * Alpha-tested surfaces can cast shadows 
  * Configurable globally as a default (useful for original maps) and for each light individually
    * Softness
    * Brightness
  * Shadow mapping and Stencil Shadows can be freely mixed
 * Soft particles
 * Parallax occlusion mapping (expects height data in the alpha channel of the specular map)
 * Added support for Qt based tools
   * optional (can be excluded from the build, so you are not forced to install Qt to build fhDOOM)
   * Implemented some basic helpers
     * RenderWidget, so the engine can render easily to the GUI
     * several input widgets for colors, numbers and vectors
     * utility functions to convert between Qt and id types (Strings, Colors, etc.)
   * re-implemented the light editor as an example (+some improvements)
     * useable ingame and from editor/radiant
     * cleaned up layout 
     * removed unused stuff
     * added advanced shadow options (related to shadow mapping)
     * show additional material properties (eg the file where it is defined)
 * Several smaller changes
  * Detect base directory automatically by walking up current directory path
  * Added new r_mode's for HD/widescreen resolutions (r_mode also sets the aspect ratio)
  * Set font size of console via con_fontScale
  * Set size of console via con_size
  * Minor fixes and cleanups in editor code (e.g. fixed auto save, removed some unused options)
  * Renamed executable and game dll (the intention was to have fhDOOM installed next to the original doom binaries. Not sure if that still works)
  * Moved maya import stuff from dll to a stand-alone command line tool (maya2md5)
  * Compile most 3rd party stuff as separate libraries
  * Fixed tons of warnings
  * image_usePrecompressedTextures is 0 by default, so HD texture packs (like Wulfen) can be used more easily.

### Screenshots

<div style="text-align:center">
Shadow Mapping
<br/>
[(https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping1_off_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping1_off.jpg)
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping1_on_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping1_on.jpg)
<br/>
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping2_off_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping2_off.jpg)
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping2_on_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping2_on.jpg)
<br/>
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping3_off_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping3_off.jpg)
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping3_on_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping3_on.jpg)
<br/>
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping4_off_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping4_off.jpg)
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping4_on_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/shadowmapping4_on.jpg)
<br/>
Soft Particles
<br/>
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/softparticles_off_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/softparticles_off.jpg)
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/softparticles_on_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/softparticles_on.jpg)
<br/>
Parallax Occlusion Mapping
<br/>
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/pom_off_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/pom_off.jpg)
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/pom_on_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/pom_on.jpg)
<br/>
Enhanced Light Editor (Qt based)
<br/>
[![Foo](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/lighteditor_tn.jpg)](https://github.com/existence/fhDOOM/raw/master/doc/screenshots/lighteditor.jpg)

</div>

### Installation

 * Download Binaries here: http://www.facinghell.com/fhdoom/fhDOOM-1.5.0-1405.zip
 * Extract zip to a location of your choice (e.g. c:\games)
 * Copy 5 files from the original DOOM3 (CD or Steam) to base directory of fhDOOM (e.g. c:\games\fhDOOM\base):
    * pak000.pk4
    * pak001.pk4
    * pak002.pk4
    * pak003.pk4
    * pak004.pk4
 * Start game by clicking on fhDOOM.exe
 * Don't forget to change the game's resolution (fhDOOM does not yet select your native resolution automatically).


### Benchmarks
System:
  * Windows 8.1 (64bit)
  * Intel Core2Quad Q9550 (2.83GHz)
  * 8GB RAM
  * Geforce GTX 570 (353.30)

Settings:
  * 1920x1200 (r_mode 15)
  * r_multiSamples 8
  * image_anisotropy 16
  * fullscreen

time demo (demo1.demo) results with original doom textures, POM disabled (r_pomEnabled 0):
  * stencil shadows: ~190 
  * shadow mapping: ~185
  * shadow mapping + AMD workaround: ~145 (workaround enforced on nVidia with r_amdWorkaround 2)

time demo (demo1.demo) results with HD textures (shadow mapping enabled):
  * POM disabled: ~180
  * POM enabled: ~160

### Notes
  * Only fully tested on nVidia hardware. Unfortunately a quick test on AMD hardware revealed some serious issues. There is a workaround in place, but it causes a pretty big performance hit (the workaround is only enabled for AMD, nVidia users won't suffer from it).
    I am going to investigate these issues more closely to find an actual solution and enable full performance for AMD.
    Not tested on Intel at all.
  * The maps of the original game were not designed with shadow mapping in mind. I tried to find sensible default shadow parameters, but those parameters are not the perfect fit in every case, so if you look closely enough you will notice a few glitches here and there
    * light bleeding
    * low-res/blocky shadows
  * Parallax Occlusion Mapping is disabled by default, because it looks just weird and wrong in a lot of places (I haven't put much work into it).
  * Shadow maps are not yet implemented for parallel/directional lights (fall back to stencil shadows)
  * Shaders from RoE expansion pack not rewritten yet
  * There was a time, where you could switch from core profile to compatibility profile (r_glCoreProfile). In compatibility profile, 
    you were still able to use ARB2 shaders for special effects (like heat haze). This feature stopped working for some reason.
    Not sure if i will ever investigate this issue, i am fine with core profile.  
  * The font size of the console is scalable (via con_fontScale), but the width of each line is still fixed (will be fixed later).
  * Doom3's editor code is still a giant mess  
  * fhDOOM is 32bit only at this point
  * I already had a linux version up and running, but its not tested very well. I am currently focussed on windows.
  * I added support for Qt based tools purely out of curiosity. I have currently no plans to port more tools to Qt.  
  * Other stuff on my ToDo list (no particular order):
    * Some ideas to improve render performance (related to shadow mapping). The engine runs pretty decent on my (fairly dated) machine, not sure if its worth the effort. Might change with more advanced rendering features.
    * Poisson sampling for shadow maps 
    * Expose different sampling techniques (currently hard coded)
    * HDR rendering/Bloom
    * Tesselation/Displacement Mapping
    * Multi threading (primarily to improve loading times. Not sure entirely yet how to parallelize the rendering or game code)    
    * Hardware/GPU skinning
    * Port deprecated MBCS MFC stuff to unicode, so you don't need that MBCS Addon für VS2013. 
    * Doom3 contains a very early and simple form of Megatexturing. Might be interesting to re-enable that for modern OpenGL and GLSL.
    * Rework available graphic options 
      * not sure if they all work properly
      * some of them make no sense on a modern PC (e.g. you don't want to disable specular maps)    
    * 64bit support would simplify the build process on linux (totally irrelevant on windows, the actual game doesn't really need that much memory...)
      * Get rid of ASM code. Rewrite with compiler intrinsics? Not sure if its worth the effort... the generic C/C++ implementation might be sufficient (needs some testing)
      * Get rid of EAX stuff
    * Look into Doom3's texture compression and modern alternatives. Does the wulfen texture pack (and others) really need to be that big? How does it effect rendering performance? Pretty sure it could improve loading times...
    * Render everything to an offscreen buffer
      * simple super sampling
      * fast switching between different r_modes, simplifies messy fullscreen switch
   * i am pretty sure i forgot a lot of things, so you might discover more things that are pretty hacky or not working at all ;)

### Building fhDOOM

Dependencies:
  * Visual Studio 2013 or Visual Studio 2015. Community versions are just fine, but VS2013 needs MBCS-Addon.
  * cmake 3.2
  * Windows 8.1 SDK
  * optional: Qt 5.4 (32bit)
  * optional: Maya 2011 SDK (32bit)




# bsnes-mercury

bsnes-mercury is a fork of higan, aiming to restore some useful features that have been removed, as well as improving performance a bit.
Maximum accuracy is still uncompromisable; anything that affects accuracy is optional and off by default.

## Changes to upstream:
- The biggest change is getting the interface sane, which is accomplished through libretro.
- 'inline' was added all across the PPU subclasses. Framerate went up by 20% from such a trivial change! (Spamming inline elsewhere does little if anything, so it wasn't done.)
- A fast path was added to various parts of the CPU bus, improving framerate by about 2.5%.
- libco has been changed quite a bit, both for performance, readability, and portability to more platforms.
- A seven-line function in sfc/memory/memory-inline.hpp was replaced, which speeds up ROM load by roughly a factor 6 (1.2s -> 0.2s). (This change was developed for upstream, and the author of higan was mostly positive; it is likely to appear in bsnes v095.)
- A section in sfc/memory/memory.cpp was specialized for the common, easy, case. This speeds up ROM load time by about a third.
- HLE emulation of some special chips is optionally restored (defaults to LLE), to improve performance and reduce reliance on those chip ROMs (they're not really easy to find). Chips for which no HLE emulation was developed (ST-0011 and ST-0018) are still LLE.
- SuperFX overclock is now available (off by default, of course); if enabled, it makes Star Fox look quite a lot smoother.

## Todo list:
- Is the compat core's DSP identical to that of the accuracy core, but a bit faster?
- Examine if ST-0011 is confused by attempts to HLE it.

The name is because metals are shiny, like the accuracy promises of bsnes, and mercury is the fastest metal.
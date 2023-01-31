# Otto Matic changelog

## **4.0.1** (Jan 2023)

Quality of life improvements:
- Better Italian translation thanks to @orazioedoardo (#2)
- New settings: infobar centering and scaling (#17)
- New setting: toggle automatic alignment of the camera behind Otto (I like to turn this off when playing with a controller)
- For the completionists out there, the bonus screen now says if you've rescued all humans in the level
- Hold down the jump button to fast-forward through bonus screen tallies
- Cheat code now gives ammo for all weapons, have fun trying out the growth potion in hilarious places (#3)
- Alternate cheat code for keyboards with bad rollover (#12)
- Toggle fullscreen with Alt+Enter
- Restored monochrome anaglyph setting
- macOS: Prevent quitting the game by mistake if fat-fingering ⌘Q or ⌘W
- macOS: Retina support, preferred display setting

Gameplay:
- Saucer abductions are now fairer. The saucer won't come if a closed gate stands between Otto and the human, or if Otto is unable to move freely due to level-specific gimmicks (zipline, water skiing, teleportation, etc). A fair amount of humans were previously impossible to rescue; it is now possible to rescue them all, in theory.
- Fix flaky fence collisions at high framerates — keeps critical enemies from getting out of bounds (Tractor, etc.) (#15)
- Fix flaky platform physics at high framerates (especially in level 3 at more than 130 fps)
- Non-vsync framerate cap raised to 500 FPS (up from 100 previously) thanks to stabler physics

Cosmetic:
- Seamless terrain texturing
- Buttery-smooth motion of spline-bound humans, enemies, platforms and boats along their spline
- When escaping a level, align Otto to the center of the ramp so he doesn't clip through the hull once inside the rocket
- Fix rough cropping of rocket flame (especially apparent in bonus screen)
- Starfield now visible through rocket's interior windows in bonus screen
- Fix graphical glitches when using supernova
- Raise particle system caps (fixes missing jump-jet smoke in busy areas)
- Fix jitter in particle orientation (especially apparent in smoke/steam emanating from the ground)

Level-specific tweaks:
- Level 1: Reset tractor to initial position when player dies as long as the metal gate is shut
- Level 1: Fix lopsided tractor back wheels
- Level 1: Fix softlock if Otto runs out of jump-jet fuel in onion area (to continue, Otto had to die and restart at checkpoint 1)
- Level 2: Fix softlock if Otto runs out of jump-jet fuel at checkpoints 6 and 7 (to continue, you had start the whole level over)
- Level 2: Otto can now stand on top of BubblePump plungers instead of clipping through them
- Level 4: Fix some teleporters unexpectedly catching Otto as he walks out
- Level 4: Restored 2 teleporters that were removed way back in version 1.0.2 for technical reasons (#20)
- Level 5: Fix inconsistent belly slide force after rocket sled (this sometimes caused Otto to slip into the bottomless pit unfairly)
- Level 5: Camera attempts to be smarter than to stare at the floor when Otto falls through a trapdoor
- Level 5: Limit amount of enemy bumper cars, making the bumper car zones less tedious to complete — like original PowerPC version when played on a CPU weaker than a G4
- Level 6: Fix missing GiantLizards and Flytraps (which are critical to level flow) due to enemy caps getting hit more often than in 1.0.0 (#10)
- Level 6: GiantLizard won't try to chase the player through an unbroken wooden gate (this used to cause ugly clipping)
- Level 6: Fix scale of tossed weapon when player is giant
- Level 8: Tone down cacophony when getting hit by a SwingerBot
- Level 9: Stop Otto's walk anim once he's inside the rocket

Technical improvements:
- Reuse a single GL draw context throughout the game
- Better fade out implementation
- Fix rare crash caused by accumulating too many open file handles (especially on macOS during a long play session)
- Stability fixes in audio mixer
- Reduce memory allocations in-game
- Update SDL to 2.26.2 (notably fixes v-sync in macOS Ventura)
- New build target: Linux/aarch64 (ARM64)

## **4.0.0** (Aug 2021) - Updated the game to run on modern systems

- Cross-platform support
- Many, many gameplay and cosmetic bugfixes and improvements
- Reworked UI and scenes for widescreen support
- Modern controller support
- All-new menu system

---

## **3.0.9** (Feb 2008)

- Last Mac version officially published by Pangea for PowerPC and 32-bit Intel Macs.

---

## **2.0** (Apr. 2004)

---

## **1.0** (Dec 2001) - Initial Pangea release

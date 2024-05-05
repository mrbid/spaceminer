### ⚠️ This has been superceeded by the latest version [here](https://github.com/mrbid/NotABug-Mirror/tree/main/SpaceMiner).

# spaceminer
A 3D space mining game.

[![Screenshot of the Space Miner game](https://res.cloudinary.com/canonical/image/fetch/f_auto,q_auto,fl_sanitize,w_819,h_637/https://dashboard.snapcraft.io/site_media/appmedia/2022/01/Screenshot_2022-01-02_18-12-12.png)](https://www.youtube.com/watch?v=PKAjwRyGCS0 "Space Miner Game Video")

You just float around and collect space rocks.

So asteroids have 5 elements:
- `[orange, top left]` **Break** _(this allows you to collect/break asteroids, without this, you are essentially dead)_
- `[green, top right]` **Shield** _(getting too close to asteroids damages your shield, with no shield the damage goes straight to your fuel tank)_
- `[purple, bottom left]` **Stop** _(asteroids generally don't move that fast but if you want to stop all the ones near you, this will do just that)_
- `[dark, bottom right]` **Repel** _(allows you to push asteroids away from you)_
- `[aqua, back of player]` **Fuel** _(allows you to move around, without this, you are essentially dead)_

The quantity of each element is represented by coloured cubic segments on the spaceman. You can see roughly what each asteroid has by the colour tints on the asteroid surface, but it's just an indicator, you never know, there could be 'gold' below surface level.

The aim of the game, whatever you want, try and mine every asteroid if you can. Or just see how long you can survive in space.

It can be hard to appreciate how many triangles are rendered in each frame, excluding the astronaut, all of the asteroids make up 1.3 Million (1,310,720) triangles spread across 16,384 draw calls. This is an amicable amount of triangles per frame for most modern computers to render meaning that this game should render at a high framerate across the board.

## General Controls
 - `W` = Thrust Forward
 - `S` = Thrust Backward
 - `Left Shift` = Thrust Down
 - `Space` = Thrust Up
 - `A` = Turn Left
 - `D` = Turn Right
 - `Left Click` = Break Asteroid
 - `Right Click` = Repel Asteroid
 - `Mouse 4 Click` = Stop all Asteroids nearby
 - `Middle Scroll` = Zoom in/out

## Keyboard
 - `F` = FPS to console
 - `P` = Player stats to console
 - `N` = New Game
 - `Q` = Break Asteroid
 - `E` = Stop all nearby Asteroids
 - `R` = Repel nearby Asteroid
 - `W` = Thrust Forward
 - `A` = Turn Left
 - `S` = Thrust Backward
 - `D` = Turn Right
 - `Left Shift` = Thrust Down
 - `Space` = Thrust Up
 - `ESCAPE` = Unfocus/Focus mouse cursor

## Mouse
 - `Left Click` = Break Asteroid
 - `Right Click` = Repel Asteroid
 - `Mouse 4 Click` = Stop all Asteroids nearby
 - `Middle Scroll` = Zoom in/out

## Downloads

### Web
https://mrbid.github.io/spaceminer/

### Snapcraft
https://snapcraft.io/spaceminer

### AppImage
https://github.com/mrbid/spaceminer/raw/main/Space_Miner-x86_64.AppImage

### [x86_64] Linux Binary (Ubuntu 21.10)
https://github.com/mrbid/spaceminer/raw/main/spaceminer

### [ARM64] Linux Binary (Raspbian 10)
https://github.com/mrbid/spaceminer/raw/main/spaceminer_arm<br>
_(This version is set to a total of 2048 asteroids, 8x less than the x86 version, and runs between 30-40 FPS on a Raspberry PI 4B+ at a resolution of 1024x768)_

### Windows Binary
https://github.com/mrbid/spaceminer/raw/main/spaceminer.exe<br>
https://github.com/mrbid/spaceminer/raw/main/glfw3.dll

## Attributions
http://www.forrestwalter.com/icons/<br>

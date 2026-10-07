## Debug Menu

This fork has an in-game debug menu that lists all CVars, all console commands and a few ready-made
actions, and it can be opened while the game is running. A config file of its own can add a fourth
list of actions to it (see Custom Actions below). It is drawn with the console font
(`textures/bigchars`, through `idRenderSystem::DrawSmallStringExt`) and is toggled with `F11` by default.

It is engine code and not game code: the menu is `neo/framework/DebugMenu.cpp`, compiled into the
executable itself alongside the rest of `framework/` in `neo/CMakeLists.txt`, and the CVars it adds
are static in that file. Where the menu lives is decided by the file it is compiled into and not by a
flag: it does carry `CVAR_SYSTEM`, but so do `com_forceGenericSIMD` and `net_clientMaxPrediction` in
`neo/game` and `neo/d3xp`, and the flag only picks the `SYS` label in `listCvar -flags`. It reads what
it lists out of the engine's systems (`cvarSystem`, `cmdSystem`), so no game module is involved and
the same menu serves the base game, RoE and a mod alike. The debug free camera further down is the
other way round - that one is game logic.

While the menu is open in a Single Player game, the game is stopped (through `g_stopTime`, the same
CVar the dhewm3 Settings Menu uses), so you're safe from monsters while browsing - the game is still
on the screen behind the menu. In multiplayer the menu doesn't stop the game.

The menu has a filter line: whatever you type narrows the list down, and the matches of that line are
shown in a panel over the list. `TAB` completes what you typed the way the console does (including
the arguments of a command), and can also be used to walk through the matches one by one.

These CVars configure the menu:

- `dbgmenu_key` the key that opens and closes the menu. Defaults to `F11`, because `F10` opens the
  dhewm3 Settings Menu. If the key name can't be parsed, `F11` is used instead.
- `dbgmenu_showValues` if set to `1` (the default), the current value of every CVar is shown next to
  its name in the list.
- `dbgmenu_bgAlpha` how opaque the background of the menu is, from `0` (fully see-through, only the
  text is drawn) to `1` (solid). Defaults to `0.55`.
- `dbgmenu_tabCompletesFilter` what `TAB` does when the edit line is *not* open:  
  `1` = complete what you typed in the filter line (then `CTRL-TAB` switches the lists),  
  `0` = always switch the lists (this is the default).  
  The `cmd:`/`val:` edit line always completes on `TAB`, no matter what this CVar is set to.

- `dbgmenu_gamepad` if set to `1`, a gamepad drives the menu, including the
  on-screen keyboard described below.
- `dbgmenu_gamepadCombo` the gamepad buttons that open the menu when they are held together, as key
  names separated by `+`. Defaults to
  `JOY_BTN_LSHOULDER+JOY_BTN_RSHOULDER+JOY_BTN_BACK+JOY_BTN_START`, that is `LB`+`RB`+`Back`+`Start`.
- `dbgmenu_padRepeatDelay` how long a direction on the DPad or on a stick has to be held down
  before the menu starts repeating it, in milliseconds. Defaults to `400`; `0` turns the repeat off.
- `dbgmenu_padRepeatRate` how many milliseconds pass between those repeats once they have
  started. Defaults to `50`, which walks a page of the list in about a second.
- `dbgmenu_osk` if set to `1` (the default), the on-screen keyboard ('Y' on the gamepad opens it) is offered for the `filter:`
  line and for the `cmd:`/`val:` edit lines.


Keys while the menu is open:

- `F11` (or whatever `dbgmenu_key` is set to) or `ESC` close the menu; `ESC` first cancels an open
  edit line.
- `UP`/`DOWN` move through the list, or through the matches of the line while the match panel is up.
- `PGUP`/`PGDN` move a whole page (`19` entries) in the list, or a whole panel of matches.
- `HOME`/`END` jump to the first/last entry.
- `TAB` switches to the next list (`CVARS` -> `COMMANDS` -> `ACTIONS`), `SHIFT-TAB` to the previous
  one; with `dbgmenu_tabCompletesFilter 1` it completes in the filter line instead, and `CTRL-TAB`
  switches the lists.
- `ENTER` runs the selected command, edits the selected CVar, or runs the selected action. Booleans
  are toggled right away, other CVars get an edit line (`val:`) prefixed with their name. Inside an
  edit line, `ENTER` runs what you typed.

### Gamepad

With a gamepad (`dbgmenu_gamepad 1`) the menu also opens on a combination of pad buttons, so it can
be reached without a keyboard: hold the buttons named in `dbgmenu_gamepadCombo` down, `LB`+`RB`+`Back`
by default, and press `Start` as the last one. The button that completes the combination is swallowed,
so the game menu doesn't open behind the debug menu. The combination is read by key name, and the
pad's `Start` is special: the SDL event code turns it into `ESC` so that it can open and close the game
menu, so the default combination is really `LB`+`RB`+`Back`+`ESC` - `JOY_BTN_START` in the CVar is
mapped to that key. A name the engine doesn't know is skipped, so a typo costs one button and not the
whole combination, and `ESC` used by itself still goes to the menu and the game as always.
While the menu is open the pad belongs to it: its buttons and axes no longer reach the player, and
the axis values the engine sends for the cursor of an in-game GUI (the PDA, another interactive
GUI) stop here too, so neither the player nor that cursor moves while you walk the lists.

Buttons while the menu is open:

- `A` runs the selected entry, or applies the line; `B` goes back and closes the menu; `X` completes
  what you typed the way `TAB` does; `Y` and `Back` bring up the on-screen keyboard.
- the DPad and the left stick walk the list (or the matches of the line), the DPad left/right switch
  the lists; `LB`/`RB` move a whole page; pressing the right stick completes the line. The right
  stick's up and down walk the matches of the line the way `UP`/`DOWN` do - on the list and, with the
  on-screen keyboard up, over the matches shown under the keys.
- a direction that is held down keeps moving: the list scrolls like it does with a held arrow key,
  the direction repeating after `dbgmenu_padRepeatDelay` and then every `dbgmenu_padRepeatRate`.
  In the list only up and down repeat - left and right switch the lists - and on the on-screen
  keyboard all four directions slide the cursor along the keys, while the right stick repeats its
  up and down over the matches both there and in the list. `LB`/`RB` repeat as well, even
  though they are part of `dbgmenu_gamepadCombo`: a combination button reaches the menu when it
  is released, so holding one down with no chord coming waits out `dbgmenu_padRepeatDelay` first
  and only then starts paging - a held `LB`/`RB` scrolls pages at the same pace as a held
  `PGUP`/`PGDN`. With the on-screen keyboard up they do not repeat, since there they step to the
  next block.
- the on-screen keyboard: `A` types the highlighted key, `X` deletes one character, `Y` applies the
  line and puts the keyboard away, `B` puts it away and keeps typing in the line, `LB`/`RB` jump to the next block, and all three of them (`abc` / `ABC` / `sym`) are on the
  screen at once, the DPad and the left stick walk the keys, pressing the left
  stick types a space, and the right stick completes into the line. The matches of the line are shown
  under the keys, and the keyboard is drawn where the list and the details normally are.
- the physical keyboard keeps working while the on-screen keyboard is up; `ESC` puts the keyboard away
  first (`ENTER` applies the line and puts it away with it).


### Details panel

Under the list, the selected entry shows its name and type, its current value and up to
**three** lines of its description, so a long description is not cut off after one line any more.
For a CVar the `flags:` and the possible range sit on the value line, which is what frees the third
line; for a command or an action the `ENTER` hint moves into the first description line the text
did not need.

### Custom Actions

The actions compiled into the menu are one list; a config file of your own can add a second one.
`dmCustomActions.cfg` is looked for in the config folder first (`fs_configpath`: `~/.config/dhewm3` on
Linux, the `My Games/dhewm3` folder in Documents on Windows) and in the game resources after that
(`base/`, `d3xp/`, a mod or a `.pk4`, packed or loose), so a file of your own wins over one a game or a
mod ships. What it holds shows up on a **CUSTOM ACTIONS** tab of its own, after `ACTIONS`, and `TAB`
and the DPad's left and right walk to it like to any other list.

A line of the file is

```
kind label = target
```

with the kind one of

- `command` (or `cmd`) - the target is a console command line, run right away:
  `command Give all = give all`
- `cvarbool` (or `toggle`) - the target is the name of a boolean CVar, which gets flipped:
  `cvarbool Show FPS = com_showFPS`
- `internal` (or `menu`) - the target is one of the codes the menu handles itself, `#refresh`,
  `#clearfilter`, `#close` or `#reload`: `internal Reload = #reload`

`#reload` reads this file over again, which is what a file being worked on needs. Empty lines and whole
lines starting with `//` or `#` are comments - there are none behind the `=`, where a command line may
hold anything.

The menu's font draws plain ASCII only, so a label or a target that carries anything else would come
out as nonsense, and a line with one is refused together with a line without a kind, a label or a
target, one with a kind or a code the menu does not have, one whose label is already on the list, and
everything past the 64th action. Each of them says why it was skipped in the log.

Without the file, and with a file that holds no usable action either, there is nothing the tab could
show, so it is not offered at all and the menu has the three lists it always had - `CVARS`,
`COMMANDS`, `ACTIONS`. Every attempt to read the file says what came of it: the first one of a session
and every one that says something new go to the console, a repeat of the last one only to the log with
`developer 1` set, so that opening the menu again does not fill the console with the same line. The
file is read every time the menu is opened, so one put in place - or taken away - after the menu was
first opened is picked up by closing and opening it, and a change to a file that is already there by
an `internal ... = #reload` action or by reopening the menu.

## Debug Free Camera

The `freeCam` command and the `dbg_freeCam` CVar let you freeze the view and fly it around, also
while a cinematic is running. They're meant for debugging and are marked as cheats, so in multiplayer
they can only be changed if `net_allowCheats` is set (in Single Player they always work).

Unlike the debug menu, the free camera is game logic and not engine code. Each game module carries its
own copy - `neo/game` for the base game and `neo/d3xp` for RoE - with the CVars in that module's
`gamesys/SysCvar.cpp` under `CVAR_GAME` and the behaviour in its `Player.cpp`, `Entity.cpp` and
`gamesys/SysCmds.cpp`, so a change here has to be made in both modules for the base game and RoE to
get it. These CVars become known when the engine loads the module, and that happens at startup, not
with a map: `idCommonLocal::InitGame()` reaches `LoadGameDLL()` and `game->Init()`, and a map is
loaded after that. So `dbg_freeCam` is registered as soon as the game starts, and
`cvarSystem->Find( "dbg_freeCam" )` comes back empty only until the module is up, or while a
different module is the one running. The only part of it that is engine code is the stick input:
`dbg_freeCam_pad` in `neo/framework/UsercmdGen.cpp`, whose `FreeCamStickMoves()` writes the move the
camera flies off and looks the game's `dbg_freeCam` up by name, since the engine cannot link to a
symbol of the game module.

```
freeCam 0|1|2              mode
freeCam here               re-anchor the camera at what you see right now
freeCam pos <x> <y> <z>    move the camera to those coordinates
freeCam angles <p> <y> <r> point the camera at those angles
freeCam speed <u/s>        set dbg_freeCam_speed
freeCamTeleport [1|2]      put the player body where the debug camera is looking from
```

Entered without arguments, `freeCam` prints the current mode, whether the camera is anchored, whether
it's flying, and the position and angles it's at.

`freeCamTeleport` puts the player body at the point the debug camera is looking from and gives it the
camera's angles - fly somewhere, look at a spot you want to reach and type it. The camera stays anchored
where it was, so the body comes to the camera, not the other way round. It needs the camera to be on and
anchored, and it refuses while a cinematic camera owns the view (see `dbg_freeCam_cine`); the optional
`1|2` argument is the freeCam mode the call was meant for and only decides which warning you get. The
body goes through the game's own teleport path, so it lands on the floor if there is one within 16 units
under the camera point, and in mode `2` it stays there instead of falling - frozen physics never moves it
again.

The CVars behind it:

- `dbg_freeCam` `0` = off (the default); `1` = the view is pinned where it is, while the player keeps
  playing as usual; `2` = the view is pinned *and* can be flown with the usual movement keys and the
  mouse, and the player's body is frozen in place.
- `dbg_freeCam_cine` what happens while a cinematic is running: `1` = a cinematic camera owns the view
  as long as it runs and the frozen view comes back afterwards; `0` (the default) = the frozen view
  always wins.
- `dbg_freeCam_look` if set to `1` (the default), mouse look turns the frozen camera. If set to `0`,
  its angles are pinned as well.
- `dbg_freeCam_body` if set to `1` (the default), the eye is detached from the player: the player's
  own body is drawn and there's no first-person weapon.
- `dbg_freeCam_speed` the fly speed in units per second. Defaults to `400`.
- `dbg_freeCam_pad` if set to `1` (the default), the tilt of a stick is what the speed of that camera is: how
  far the left stick is pushed past `joy_deadZone` is how much of `dbg_freeCam_speed` the camera
  flies at, so half a stick flies at half speed, and the triggers fly it up and down the same way.
  The left stick does not have to be bound for this. With the `0` the binds alone fly the
  camera, and since it normalizes the direction they give it and takes the speed from
  `dbg_freeCam_speed`, any tilt past `joy_deadZone` flies at all of it. Looking is left to the look actions
  the right stick is bound to, which already scale their turn rate by the tilt (`joy_yawSpeed`,
  `joy_pitchSpeed`, `joy_powerScale`). Defaults to `1`; it only does anything while the `dbg_freeCam 2`
  camera is flying and the debug menu is closed. Works in the base game and in `d3xp` alike.
- `dbg_freeCam_visible` if set to `1`, the player model is not hidden when a cinematic starts while
  the debug camera is on. Defaults to `0` (it is hidden, like cinematics normally do).
- `dbg_freeCam_freezeAnim` if set to `1` (the default), the player's animation is held on the frame it
  was at while the `dbg_freeCam 2` camera is flying - so no animation frame commands (footsteps, gear,
  voice lines) fire while you fly. Set to `0` to keep the animation cycling (only the sounds stay off).

While the mode `2` camera is flying, the player gets no input at all: movement keys and mouse only move
the camera, the body doesn't fire or reload, its weapon stays silent, and its physics is frozen (so it
neither falls nor gets pushed around). The air/vacuum logic is bypassed as well - `newAirless` is
forced to `false`, so `airTics` doesn't drain, `damage_noair` never fires, the oxygen indicator doesn't
come up and the breathing sounds stay off. See also `pm_ignoreVacuum`.

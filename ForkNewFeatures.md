## Debug Menu

This fork has an in-game debug menu that lists all CVars, all console commands and a few ready-made
actions, and it can be opened while the game is running. It is drawn with the console font
(`textures/bigchars`, through `idRenderSystem::DrawSmallStringExt`) and is toggled with `F11` by default.  

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

Keys while the menu is open:

- `F11` (or whatever `dbgmenu_key` is set to) or `ESC` close the menu; `ESC` first cancels an open
  edit line.
- `UP`/`DOWN` move through the list, or through the matches of the line while the match panel is up.
- `PGUP`/`PGDN` move a whole page (`20` entries) in the list, or a whole panel of matches.
- `HOME`/`END` jump to the first/last entry.
- `TAB` switches to the next list (`CVARS` -> `COMMANDS` -> `ACTIONS`), `SHIFT-TAB` to the previous
  one; with `dbgmenu_tabCompletesFilter 1` it completes in the filter line instead, and `CTRL-TAB`
  switches the lists.
- `ENTER` runs the selected command, edits the selected CVar, or runs the selected action. Booleans
  are toggled right away, other CVars get an edit line (`val:`) prefixed with their name. Inside an
  edit line, `ENTER` runs what you typed.

## Debug Free Camera

The `freeCam` command and the `dbg_freeCam` CVar let you freeze the view and fly it around, also
while a cinematic is running. They're meant for debugging and are marked as cheats, so in multiplayer
they can only be changed if `net_allowCheats` is set (in Single Player they always work).

```
freeCam 0|1|2              mode
freeCam here               re-anchor the camera at what you see right now
freeCam pos <x> <y> <z>    move the camera to those coordinates
freeCam angles <p> <y> <r> point the camera at those angles
freeCam speed <u/s>        set dbg_freeCam_speed
```

Entered without arguments, `freeCam` prints the current mode, whether the camera is anchored, whether
it's flying, and the position and angles it's at. The CVars behind it:

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

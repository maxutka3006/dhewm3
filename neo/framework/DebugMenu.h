/*
========================================================
 Debug menu for the dhewm3 fork
 Copyright (c) 2026 maxutka_3006
 Generated with assistance from DeepSeek / AI Assistant
 Licensed under GNU GPLv3
========================================================
*/

#ifndef __DEBUGMENU_H__
#define __DEBUGMENU_H__

#include "framework/Session.h"

/*
===============================================================================

	In-game debug menu.

	Browses every CVar and console command the engine knows about, edits CVars,
	executes commands (with arguments) and calls a few engine functions
	directly. It is drawn with the "textures/bigchars" font through the render
	system, so it needs no GUI files and works before any map is loaded.

	The menu is toggled with the key named by the "dbgmenu_key" CVar (F11 by
	default; F10 is taken by the dhewm3 settings menu) or with the "debugMenu"
	console command.

	While it is open, a single player game is stopped (via "g_stopTime"), just
	like the Dear ImGui settings menu does, so the player cannot be hurt while
	browsing.

===============================================================================
*/

class idDebugMenu {
public:
	virtual					~idDebugMenu( void ) {}

	virtual void			Init( void ) = 0;
	virtual void			Shutdown( void ) = 0;
	virtual void			LoadGraphics( void ) = 0;

	// returns true if the event was handled and must not reach the game
	virtual bool			ProcessEvent( const sysEvent_t *event ) = 0;
	virtual void			Draw( void ) = 0;

	virtual bool			Active( void ) const = 0;
	virtual void			Toggle( void ) = 0;
	virtual void			Close( void ) = 0;
	virtual void			Open( const char *initialFilter = NULL ) = 0;
};

extern idDebugMenu *		debugMenu;

#endif /* !__DEBUGMENU_H__ */

/*
========================================================
 Debug menu for the dhewm3 fork
 Copyright (c) 2026 maxutka_3006
 Generated with assistance from DeepSeek / AI Assistant
 Licensed under GNU GPLv3
========================================================
*/

#include "sys/platform.h"
#include "framework/DebugMenu.h"

#include "idlib/containers/StrList.h"
#include "idlib/math/Vector.h"
#include "idlib/Str.h"
#include "framework/BuildVersion.h"
#include "framework/CVarSystem.h"
#include "framework/CmdSystem.h"
#include "framework/Common.h"
#include "framework/Console.h"
#include "framework/DeclManager.h"
#include "framework/EditField.h"
#include "framework/KeyInput.h"
#include "framework/Session.h"
#include "renderer/RenderSystem.h"

/*
===============================================================================

	idDebugMenuLocal

	All text is drawn with the bigchars font, which only knows ASCII, so every
	string the menu renders must stay ASCII.

===============================================================================
*/

static idCVar	dm_key( "dbgmenu_key", "F11", CVAR_ARCHIVE | CVAR_SYSTEM | CVAR_NOCHEAT, "key that opens the in-game debug menu (F10 belongs to the dhewm3 settings menu)" );
static idCVar	dm_showValues( "dbgmenu_showValues", "1", CVAR_ARCHIVE | CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "show the current CVar values in the debug menu list" );
static idCVar	dm_bgAlpha( "dbgmenu_bgAlpha", "0.55", CVAR_ARCHIVE | CVAR_FLOAT | CVAR_SYSTEM | CVAR_NOCHEAT,
					"opacity of the debug menu background (0 = see-through, 1 = solid)", 0.0f, 1.0f );
static idCVar	dm_tabCompletesFilter( "dbgmenu_tabCompletesFilter", "0", CVAR_ARCHIVE | CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "cmd:/val: TAB completes; filter: 1 = completes (CTRL-TAB lists), 0 = lists" );

// the render system always draws 2D into a 640x480 virtual screen
static const int	DM_WIDTH			= 640;
static const int	DM_HEIGHT			= 480;
// The menu reuses the console font (textures/bigchars). Its "small" variant
// draws each 16x16 glyph into an 8x16 cell - that is exactly the console look
// and doubles the number of columns (40 -> 80).
// Set this to 0 to get the chunky 16x16 cells back.
#define DM_USE_SMALL_FONT	1

#if DM_USE_SMALL_FONT
static const int	DM_CHAR_W			= SMALLCHAR_WIDTH;					// 8 pixels
static const int	DM_CHAR_H			= SMALLCHAR_HEIGHT;					// 16 pixels
#else
static const int	DM_CHAR_W			= BIGCHAR_WIDTH;					// 16 pixels
static const int	DM_CHAR_H			= BIGCHAR_HEIGHT;					// 16 pixels
#endif

static const int	DM_COLS				= DM_WIDTH / DM_CHAR_W;				// 80 characters
static const int	DM_ROWS				= DM_HEIGHT / DM_CHAR_H;			// 30 lines

static const int	DM_ROW_TITLE		= 0;
static const int	DM_ROW_TABS			= 1;
static const int	DM_ROW_FILTER		= 2;
static const int	DM_ROW_LIST			= 3;
static const int	DM_LIST_ROWS		= 20;								// rows 3 .. 22
static const int	DM_ROW_INFO			= DM_ROW_LIST + DM_LIST_ROWS;		// 23
static const int	DM_ROW_VALUE		= DM_ROW_INFO + 1;					// 24
static const int	DM_ROW_DESC1		= DM_ROW_VALUE + 1;					// 25
static const int	DM_ROW_DESC2		= DM_ROW_DESC1 + 1;					// 26
static const int	DM_ROW_STATUS		= DM_ROW_DESC2 + 1;					// 27
static const int	DM_ROW_HINT1		= DM_ROW_STATUS + 1;				// 28
static const int	DM_ROW_HINT2		= DM_ROWS - 1;						// 29
static const int	DM_MATCH_ROWS	= DM_ROW_DESC2 - DM_ROW_LIST;	// matches the panel shows at once

static const idVec4	DM_COLOR_BG( 0.03f, 0.05f, 0.08f, 1.0f );
static const idVec4	DM_COLOR_BAR( 0.07f, 0.13f, 0.19f, 1.0f );
static const idVec4	DM_COLOR_TITLE( 0.40f, 0.90f, 1.00f, 1.0f );
static const idVec4	DM_COLOR_TEXT( 0.80f, 0.84f, 0.88f, 1.0f );
static const idVec4	DM_COLOR_DIM( 0.45f, 0.50f, 0.55f, 1.0f );
static const idVec4	DM_COLOR_SEL_BG( 0.12f, 0.34f, 0.50f, 1.0f );
static const idVec4	DM_COLOR_SEL( 1.00f, 1.00f, 1.00f, 1.0f );
static const idVec4	DM_COLOR_EDIT( 1.00f, 0.70f, 0.30f, 1.0f );
static const idVec4	DM_COLOR_STATUS( 1.00f, 0.85f, 0.35f, 1.0f );
static const idVec4	DM_COLOR_SEP( 0.20f, 0.45f, 0.60f, 1.0f );
static const idVec4	DM_COLOR_PANEL( 0.02f, 0.04f, 0.07f, 0.94f );				// under the match list

static const int	DM_NAME_COLS		= DM_COLS / 2;								// width of the name column in the list
static const int	DM_VALUE_COLS		= DM_COLS - DM_NAME_COLS - 1;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

/*
================
DM_CopyTruncated

Copies at most maxChars characters, replacing the last one with '>' if the text
did not fit. '^' is dropped because the font renderer would eat the character
following a color escape.
================
*/
static void DM_CopyTruncated( char *dst, int dstSize, const char *src, int maxChars ) {
	int		i, n;
	bool	truncated;

	if ( !dst || dstSize <= 0 ) {
		return;
	}
	dst[0] = '\0';
	if ( !src ) {
		return;
	}

	n = (int)strlen( src );
	truncated = false;
	if ( n > maxChars ) {
		n = maxChars;
		truncated = true;
	}
	if ( n > dstSize - 1 ) {
		n = dstSize - 1;
		truncated = true;
	}

	for ( i = 0; i < n; i++ ) {
		char c = src[i];
		if ( c == '^' ) {
			c = ' ';
		}
		dst[i] = c;
	}
	if ( truncated && n > 0 ) {
		dst[n - 1] = '>';
	}
	dst[n] = '\0';
}

/*
================
DM_ContainsNoCase

Substring search, both strings are expected to be lowercase in needle.
================
*/
static bool DM_ContainsNoCase( const char *haystack, const char *needle ) {
	int		i, j, hayLen, needleLen;

	if ( !needle || !needle[0] ) {
		return true;
	}
	if ( !haystack ) {
		return false;
	}

	hayLen = (int)strlen( haystack );
	needleLen = (int)strlen( needle );
	if ( needleLen > hayLen ) {
		return false;
	}

	for ( i = 0; i <= hayLen - needleLen; i++ ) {
		for ( j = 0; j < needleLen; j++ ) {
			char h = haystack[i + j];
			if ( h >= 'A' && h <= 'Z' ) {
				h = (char)( h - 'A' + 'a' );
			}
			if ( h != needle[j] ) {
				break;
			}
		}
		if ( j == needleLen ) {
			return true;
		}
	}
	return false;
}

/*
================
DM_StrListCompare

Used for the alphabetical sorting of the CVar and command lists.
================
*/
static int DM_StrListCompare( const idStr *a, const idStr *b ) {
	return a->Icmp( *b );
}

/*
================
DM_CommonPrefix

The longest beginning all matches share. This is what the console puts into
the line on the first TAB, so the next typed character narrows the list down.
================
*/
static void DM_CommonPrefix( const idStrList &list, idStr &out ) {
	int		i, j;

	out.Clear();
	if ( list.Num() <= 0 ) {
		return;
	}

	out = list[0];
	for ( i = 1; i < list.Num(); i++ ) {
		const char *a = out.c_str();
		const char *b = list[i].c_str();

		for ( j = 0; a[j] && b[j]; j++ ) {
			char	ca = a[j];
			char	cb = b[j];

			if ( ca >= 'A' && ca <= 'Z' ) {
				ca = (char)( ca - 'A' + 'a' );
			}
			if ( cb >= 'A' && cb <= 'Z' ) {
				cb = (char)( cb - 'A' + 'a' );
			}
			if ( ca != cb ) {
				break;
			}
		}
		out.CapLength( j );
	}
}

/*
================
DM_CollectMatch

The callback the command and the CVar system call for every candidate they
have. Both of them hand back everything they know, and the console keeps only
what starts with the line that is being completed; this filters in exactly the
same way, so the menu shows what the console would have printed.
================
*/
static idStrList *	dmMatchSink = NULL;
static const char *	dmMatchBase = NULL;

static void DM_CollectMatch( const char *s ) {
	idStr	match;
	int		i;

	if ( dmMatchSink == NULL || dmMatchBase == NULL || s == NULL || s[0] == '\0' ) {
		return;
	}
	if ( idStr::Icmpn( s, dmMatchBase, (int)strlen( dmMatchBase ) ) != 0 ) {
		return;
	}

	match = s;
	for ( i = 0; i < dmMatchSink->Num(); i++ ) {
		if ( ( *dmMatchSink )[i].Icmp( match ) == 0 ) {
			return;				// a name that is both a command and a CVar
		}
	}
	dmMatchSink->Append( match );
}

/*
================
DM_CollectCompletions

Fills out with the names ("arguments" false) or with the arguments of the
command that is being typed ("arguments" true). Every entry is a complete
command line - "map game/admin.map" and not just "game/admin.map" - because
the engine builds it that way, so a match can be run as it stands.
================
*/
static void DM_CollectCompletions( idStrList &out, const char *base, bool arguments ) {
	dmMatchSink = &out;
	dmMatchBase = base;

	if ( arguments ) {
		cmdSystem->ArgCompletion( base, DM_CollectMatch );
		cvarSystem->ArgCompletion( base, DM_CollectMatch );
	} else {
		cmdSystem->CommandCompletion( DM_CollectMatch );
		cvarSystem->CommandCompletion( DM_CollectMatch );
	}

	dmMatchSink = NULL;
	dmMatchBase = NULL;
}

/*
================
DM_WrapText

Copies up to width characters of text into out (breaking at a space if
possible) and returns the rest of the text, or NULL if everything fit.
================
*/
static const char *DM_WrapText( const char *text, int width, char *out, int outSize ) {
	int			i, take, space, len;

	if ( !out || outSize <= 0 ) {
		return NULL;
	}
	out[0] = '\0';
	if ( !text || !text[0] ) {
		return NULL;
	}

	len = (int)strlen( text );
	take = ( len <= width ) ? len : width;
	if ( take < len ) {
		space = -1;
		for ( i = take - 1; i > 0; i-- ) {
			if ( text[i] == ' ' ) {
				space = i;
				break;
			}
		}
		if ( space > 0 ) {
			take = space;
		}
	}
	if ( take > outSize - 1 ) {
		take = outSize - 1;
	}

	for ( i = 0; i < take; i++ ) {
		char c = text[i];
		out[i] = ( c == '^' ) ? ' ' : c;
	}
	out[take] = '\0';

	if ( take >= len ) {
		return NULL;
	}
	text += take;
	while ( *text == ' ' ) {
		text++;
	}
	return ( *text ) ? text : NULL;
}

/*
================
DM_InsertCaret

Inserts the text cursor into a copy of the given line.
================
*/
static void DM_InsertCaret( const char *src, int cursor, char *dst, int dstSize ) {
	int		i, len, j;

	if ( !dst || dstSize <= 0 ) {
		return;
	}
	dst[0] = '\0';
	if ( !src ) {
		src = "";
	}

	len = (int)strlen( src );
	if ( cursor < 0 ) {
		cursor = 0;
	}
	if ( cursor > len ) {
		cursor = len;
	}

	j = 0;
	for ( i = 0; i < len && j < dstSize - 2; i++ ) {
		if ( i == cursor ) {
			dst[j++] = '_';
		}
		char c = src[i];
		dst[j++] = ( c == '^' ) ? ' ' : c;
	}
	if ( cursor >= len && j < dstSize - 1 ) {
		dst[j++] = '_';
	}
	dst[j] = '\0';
}

/*
================
DM_TailOfString

Returns the part of the string that fits into maxChars columns.
================
*/
static const char *DM_TailOfString( const char *src, int maxChars ) {
	int		len;

	if ( !src ) {
		return "";
	}
	len = (int)strlen( src );
	if ( len <= maxChars || maxChars <= 0 ) {
		return src;
	}
	return src + ( len - maxChars );
}

// ---------------------------------------------------------------------------
// flag names
// ---------------------------------------------------------------------------

typedef struct {
	int			flag;
	const char *name;
} dmFlagName_t;

static const dmFlagName_t dmCVarFlagNames[] = {
	{ CVAR_BOOL, "BOOL" },
	{ CVAR_INTEGER, "INTEGER" },
	{ CVAR_FLOAT, "FLOAT" },
	{ CVAR_SYSTEM, "SYSTEM" },
	{ CVAR_RENDERER, "RENDERER" },
	{ CVAR_SOUND, "SOUND" },
	{ CVAR_GUI, "GUI" },
	{ CVAR_GAME, "GAME" },
	{ CVAR_TOOL, "TOOL" },
	{ CVAR_USERINFO, "USERINFO" },
	{ CVAR_SERVERINFO, "SERVERINFO" },
	{ CVAR_NETWORKSYNC, "NETSYNC" },
	{ CVAR_CHEAT, "CHEAT" },
	{ CVAR_NOCHEAT, "NOCHEAT" },
	{ CVAR_INIT, "INIT" },
	{ CVAR_ROM, "READONLY" },
	{ CVAR_ARCHIVE, "ARCHIVE" },
	{ CVAR_STATIC, "STATIC" }
};
static const int dmNumCVarFlagNames = sizeof( dmCVarFlagNames ) / sizeof( dmCVarFlagNames[0] );

static const dmFlagName_t dmCmdFlagNames[] = {
	{ CMD_FL_SYSTEM, "SYSTEM" },
	{ CMD_FL_RENDERER, "RENDERER" },
	{ CMD_FL_SOUND, "SOUND" },
	{ CMD_FL_GAME, "GAME" },
	{ CMD_FL_TOOL, "TOOL" },
	{ CMD_FL_CHEAT, "CHEAT" }
};
static const int dmNumCmdFlagNames = sizeof( dmCmdFlagNames ) / sizeof( dmCmdFlagNames[0] );

/*
================
DM_FlagsToString

Builds a space separated list of the names of all set flags.
================
*/
static void DM_FlagsToString( const dmFlagName_t *names, int numNames, int flags, char *dst, int dstSize ) {
	int		i;
	size_t	len;

	if ( !dst || dstSize <= 0 ) {
		return;
	}
	dst[0] = '\0';

	for ( i = 0; i < numNames; i++ ) {
		if ( !( flags & names[i].flag ) ) {
			continue;
		}
		len = strlen( dst );
		if ( len > 0 ) {
			strncat( dst, " ", dstSize - len - 1 );
			len = strlen( dst );
		}
		strncat( dst, names[i].name, dstSize - len - 1 );
	}
}

/*
================
DM_RangeString

Returns the range of a CVar as "<min> - <max>" if it has one.
================
*/
static void DM_RangeString( const idCVar *cvar, char *dst, int dstSize ) {
	dst[0] = '\0';
	if ( !cvar ) {
		return;
	}
	if ( cvar->GetMinValue() >= cvar->GetMaxValue() ) {
		return;
	}
	sprintf( dst, "range %.4g - %.4g", cvar->GetMinValue(), cvar->GetMaxValue() );
}

// ---------------------------------------------------------------------------
// the menu itself
// ---------------------------------------------------------------------------

typedef enum {
	DM_LIST_CVARS = 0,
	DM_LIST_COMMANDS,
	DM_LIST_ACTIONS,
	DM_LIST_NUM
} dmList_t;

static const char *dmListNames[DM_LIST_NUM] = { "CVARS", "COMMANDS", "ACTIONS" };

typedef enum {
	DM_ACTION_COMMAND = 0,		// target is a console command line
	DM_ACTION_CVAR_BOOL,		// target is the name of a boolean CVar that gets toggled
	DM_ACTION_INTERNAL			// handled inside the menu, target is a "#..." code
} dmActionKind_t;

typedef struct {
	const char *	label;
	const char *	target;
	int				kind;
} dmActionDef_t;

static const dmActionDef_t dmActions[] = {
	{ "Screenshot",			"screenshot",				DM_ACTION_COMMAND },
	{ "Restart renderer",	"vid_restart",				DM_ACTION_COMMAND },
	{ "Reload engine",	    "reloadEngine",				DM_ACTION_COMMAND },
	{ "Reload images",		"reloadImages",				DM_ACTION_COMMAND },
	{ "Reload models",		"reloadModels",				DM_ACTION_COMMAND },
	{ "Reload sounds",		"reloadSounds",				DM_ACTION_COMMAND },
	{ "Restart sound",		"s_restart",				DM_ACTION_COMMAND },
	{ "Reload declarations", "reloadDecls",				DM_ACTION_COMMAND },
	{ "Reload GUIs",		"reloadGuis",				DM_ACTION_COMMAND },
	{ "Write config",		"writeConfig",				DM_ACTION_COMMAND },
	{ "GPU info",			"gfxInfo",					DM_ACTION_COMMAND },
	{ "Memory info",		"printMemInfo",				DM_ACTION_COMMAND },
	{ "List CVars",			"listCvars",				DM_ACTION_COMMAND },
	{ "List commands",		"listCmds",					DM_ACTION_COMMAND },
	{ "List declarations",	"listDecls",				DM_ACTION_COMMAND },
	{ "Reset all CVars",	"cvar_restart",				DM_ACTION_COMMAND },
	{ "Toggle com_showFPS",	"com_showFPS",				DM_ACTION_CVAR_BOOL },
	{ "Toggle mem usage",	"com_showMemoryUsage",		DM_ACTION_CVAR_BOOL },
	{ "Toggle sound stats",	"com_showSoundDecoders",	DM_ACTION_CVAR_BOOL },
	{ "Refresh lists",		"#refresh",					DM_ACTION_INTERNAL },
	{ "Clear filter",		"#clearfilter",				DM_ACTION_INTERNAL },
	{ "Close menu",			"#close",					DM_ACTION_INTERNAL }
};
static const int dmNumActions = sizeof( dmActions ) / sizeof( dmActions[0] );

class idDebugMenuLocal : public idDebugMenu {
public:
							idDebugMenuLocal( void );

	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			LoadGraphics( void );

	virtual bool			ProcessEvent( const sysEvent_t *event );
	virtual void			Draw( void );

	virtual bool			Active( void ) const { return active; }
	virtual void			Toggle( void );
	virtual void			Close( void );
	virtual void			Open( const char *initialFilter );

private:
	int						ToggleKeyNum( void ) const;
	void					SetGamePause( bool pause );
	void					SetList( int newList );
	void					RefreshLists( void );
	void					MarkFilterDirty( void );
	void					ApplyFilter( void );
	void					SyncFilterFromEditField( void );
	void					SetFilter( const char *text );
	void					MoveSelection( int delta );
	void					EnsureSelectionVisible( void );
	int						SelectedListIndex( void ) const;
	const char *			SelectedName( void ) const;

	void					KeyDownEvent( int key );
	void					CharEvent( int ch );
	void					ExecuteSelected( void );
	void					RunAction( const dmActionDef_t &action );
	void					BeginEdit( const char *initialText, const char *target, bool isCommand );
	void					EndEdit( void );
	void					CancelEdit( void );
	void					CompleteEdit( void );
	void					RunEditLine( void );
	void					CollectMatches( const char *line );
	void					SetEditBuffer( const char *text );
	void					ApplyMatch( void );
	void					StepMatch( int delta );
	void					StepMatchPage( int page );
	void					GotoMatch( int index );
	void					ClearMatches( void );
	bool					MatchesActive( void ) const { return completeLive && completeMatches.Num() > 0; }

	void					SetStatus( const char *text );

	void					DrawText( int col, int row, const char *text, const idVec4 &color );
	void					DrawTextClipped( int col, int row, const char *text, const idVec4 &color, int maxChars );
	void					DrawRectPixels( float x, float y, float w, float h, const idVec4 &color );
	void					DrawRowBar( int row, const idVec4 &color );

	void					DrawList( void );
	void					DrawMatches( void );
	void					DrawCvarInfo( const char *name );
	void					DrawCommandInfo( const char *name );
	void					DrawActionInfo( int actionIndex );

	idStrList				listNames;			// everything in the current list, sorted
	idList<int>				filtered;			// indices into listNames matching the filter
	idStr					filterText;
	idStr					filterLower;
	idEditField				editField;			// used for the filter line and for value/argument editing
	idStr					editTarget;
	bool					editIsCommand;
	bool					editing;
	bool					filterDirty;
	int						list;
	int						selection;
	int						scroll;
	bool					active;
	bool					pauseSetByMenu;
	idStr					status;
	idStrList				completeMatches;	// candidates the engine offers for the typed line
	idStr					completeStrip;		// text dropped when a match goes back into the line
	int						completeIndex;		// match the line holds now, -1 = the common beginning
	bool					completeLive;		// the line came from the completion, not from typing
	bool					completeNarrowed;	// the matches share this beginning
									// and it is already in the line

	const idMaterial *		bigCharShader;
	const idMaterial *		whiteShader;
};

static idDebugMenuLocal	debugMenuLocal;
idDebugMenu *			debugMenu = &debugMenuLocal;

/*
================
DM_DebugMenu_f

"debugMenu [filter]" - toggles (or opens with a filter) the debug menu.
================
*/
static void DM_DebugMenu_f( const idCmdArgs &args ) {
	if ( !debugMenu ) {
		return;
	}
	if ( args.Argc() > 1 ) {
		debugMenu->Open( args.Argv( 1 ) );
	} else {
		debugMenu->Toggle();
	}
}

/*
================
idDebugMenuLocal::idDebugMenuLocal
================
*/
idDebugMenuLocal::idDebugMenuLocal( void ) {
	active = false;
	editing = false;
	editIsCommand = false;
	filterDirty = true;
	list = DM_LIST_CVARS;
	selection = 0;
	scroll = 0;
	bigCharShader = NULL;
	whiteShader = NULL;
	pauseSetByMenu = false;
}

/*
================
idDebugMenuLocal::Init
================
*/
void idDebugMenuLocal::Init( void ) {
	ClearMatches();
	active = false;
	editing = false;
	editIsCommand = false;
	list = DM_LIST_CVARS;
	selection = 0;
	scroll = 0;
	filterDirty = true;
	status.Clear();
	filterText.Clear();
	filterLower.Clear();
	listNames.Clear();
	filtered.Clear();
	editField.Clear();
	editField.SetWidthInChars( 40 );

	pauseSetByMenu = false;

	cmdSystem->AddCommand( "debugMenu", DM_DebugMenu_f, CMD_FL_SYSTEM, "toggles the in-game debug menu" );
}

/*
================
idDebugMenuLocal::Shutdown
================
*/
void idDebugMenuLocal::Shutdown( void ) {
	active = false;
	editing = false;
	listNames.Clear();
	filtered.Clear();

	// never leave a stopped game behind
	SetGamePause( false );

	cmdSystem->RemoveCommand( "debugMenu" );
}

/*
================
idDebugMenuLocal::LoadGraphics
================
*/
void idDebugMenuLocal::LoadGraphics( void ) {
	bigCharShader = declManager->FindMaterial( "textures/bigchars" );
	whiteShader = declManager->FindMaterial( "_white" );
}

/*
================
idDebugMenuLocal::ToggleKeyNum
================
*/
int idDebugMenuLocal::ToggleKeyNum( void ) const {
	int key = idKeyInput::StringToKeyNum( dm_key.GetString() );
	if ( key <= 0 ) {
		key = K_F11;
	}
	return key;
}

/*
================
idDebugMenuLocal::Toggle
================
*/
void idDebugMenuLocal::Toggle( void ) {
	if ( active ) {
		Close();
	} else {
		Open( NULL );
	}
}

/*
================
idDebugMenuLocal::SetGamePause

The Dear ImGui settings menu stops the single player game while it is open,
so the player stays safe from monsters while browsing it.  It does that with
"g_stopTime", the CVar the game code (declared in game/gamesys/SysCvar.cpp)
checks in its frame loop, and this menu does exactly the same.

The CVar is only written when it really has to change its value, and on close
only if this menu was the one that set it: a game that was already stopped
(cinematic, script) or a session where  the CVar does not exist yet is left
alone.  Multiplayer is skipped as well - there the game must keep running.
================
*/
void idDebugMenuLocal::SetGamePause( bool pause ) {
	if ( session == NULL || session->IsMultiplayer() ) {
		pauseSetByMenu = false;
		return;
	}

	idCVar *stopTime = cvarSystem->Find( "g_stoptime" );
	if ( stopTime == NULL ) {
		// the game code is not loaded (main menu, or no game at all)
		pauseSetByMenu = false;
		return;
	}

	if ( pause ) {
		if ( stopTime->GetBool() ) {
			// already stopped by the game itself - do not undo that on close
			pauseSetByMenu = false;
		} else {
			stopTime->SetBool( true );
			pauseSetByMenu = true;
		}
	} else if ( pauseSetByMenu ) {
		stopTime->SetBool( false );
		pauseSetByMenu = false;
	}
}

/*
================
idDebugMenuLocal::Close
================
*/
void idDebugMenuLocal::Close( void ) {
	ClearMatches();
	active = false;
	editing = false;
	editField.Clear();
	editField.ClearAutoComplete();

	SetGamePause( false );
}

/*
================
idDebugMenuLocal::Open
================
*/
void idDebugMenuLocal::Open( const char *initialFilter ) {
	if ( !bigCharShader ) {
		LoadGraphics();
	}

	active = true;
	editing = false;
	editIsCommand = false;
	selection = 0;
	scroll = 0;
	status.Clear();

	SetFilter( initialFilter ? initialFilter : "" );
	RefreshLists();

	SetGamePause( true );
}

/*
================
idDebugMenuLocal::SetFilter
================
*/
void idDebugMenuLocal::SetFilter( const char *text ) {
	ClearMatches();
	filterText = text ? text : "";
	filterLower = filterText;
	filterLower.ToLower();
	editField.SetBuffer( filterText.c_str() );
	editField.SetCursor( (int)strlen( editField.GetBuffer() ) );
	filterDirty = true;
}

/*
================
idDebugMenuLocal::MarkFilterDirty
================
*/
void idDebugMenuLocal::MarkFilterDirty( void ) {
	filterDirty = true;
}

/*
================
idDebugMenuLocal::SyncFilterFromEditField
================
*/
void idDebugMenuLocal::SyncFilterFromEditField( void ) {
	filterText = editField.GetBuffer();
	filterLower = filterText;
	filterLower.ToLower();
	filterDirty = true;
}

/*
================
idDebugMenuLocal::CollectMatches

Asks cmdSystem and cvarSystem what the engine can offer for the line. Which of
the two things is completed follows from the line itself: its first word alone
is a command or CVar name, anything after that is an argument of that command
(file names, maps, sounds, decls, values), and a CVar value is completed as
"name value" even though the edit line only holds the value. A space
behind the name is the arguments already: the tokenizer drops it, but
"spawn " lists the entityDefs and not the names that begin with spawn.
================
*/
void idDebugMenuLocal::CollectMatches( const char *line ) {
	idCmdArgs	args;
	idStr		base;

	completeMatches.Clear();
	completeStrip.Clear();

	if ( line == NULL || line[0] == '\0' ) {
		return;
	}

	// a CVar value: the engine needs the name of the CVar in front of it
	if ( editing && !editIsCommand ) {
		if ( editTarget.Length() <= 0 ) {
			return;
		}
		completeStrip = editTarget;
		completeStrip += " ";
		base = completeStrip;
		base += line;
		DM_CollectCompletions( completeMatches, base.c_str(), true );
		return;
	}

	args.TokenizeString( line, false );

	base = args.Argv( 0 );
	if ( base.Length() <= 0 ) {
		return;
	}

	if ( args.Argc() > 1 ) {
		// the arguments of the command that is being typed
		base += " ";
		base += args.Args();
		DM_CollectCompletions( completeMatches, base.c_str(), true );
		return;
	}

	// a space behind the name is an argument position, however little has
	// been typed after it: "spawn " lists "spawn <entityDef>" like "spawn n"
	for ( const char *c = line; *c != '\0'; c++ ) {
		if ( *c != ' ' && *c != '\t' ) {
			continue;
		}
		while ( *c == ' ' || *c == '\t' ) {
			c++;
		}
		base += " ";
		if ( *c != '\0' ) {
			idStr	argPart( c );
			argPart.StripTrailing( ' ' );
			base += argPart;
		}
		DM_CollectCompletions( completeMatches, base.c_str(), true );
		if ( completeMatches.Num() > 0 ) {
			return;
		}

		// nothing for an empty argument: the names keep the line
		completeMatches.Clear();
		base = args.Argv( 0 );
		break;
	}

	// the name itself, what the console offers first
	DM_CollectCompletions( completeMatches, base.c_str(), false );

	// a name that is already complete ("map" say) has no name matches left, and
	// then the console moves on to the arguments of that command
	if ( completeMatches.Num() == 1 && idStr::Icmp( completeMatches[0].c_str(), base.c_str() ) == 0 ) {
		completeMatches.Clear();
		base += " ";
		DM_CollectCompletions( completeMatches, base.c_str(), true );
	}
}

/*
================
idDebugMenuLocal::SetEditBuffer

Puts a match into the line. A CVar value is completed as "name value" while
the line holds the value only, so the name is dropped here again.
================
*/
void idDebugMenuLocal::SetEditBuffer( const char *text ) {
	idStr	line = text ? text : "";

	if ( completeStrip.Length() > 0 && idStr::Icmpn( line.c_str(), completeStrip.c_str(), completeStrip.Length() ) == 0 ) {
		idStr	tail( line.c_str() + completeStrip.Length() );
		line = tail;
	}

	editField.SetBuffer( line.c_str() );
	editField.ClearAutoComplete();
	editField.SetCursor( (int)strlen( editField.GetBuffer() ) );

	if ( !editing ) {
		SyncFilterFromEditField();
	}
}

/*
================
idDebugMenuLocal::ClearMatches

Forgets the collected matches: the line is typed or moved again, so every
candidate gathered for the old one is stale.
================
*/
void idDebugMenuLocal::ClearMatches( void ) {
	completeMatches.Clear();
	completeStrip.Clear();
	completeIndex = -1;
	completeLive = false;
	completeNarrowed = false;
}

/*
================
idDebugMenuLocal::CompleteEdit

TAB completion, the very same one the console has: the same cmdSystem and
cvarSystem calls, the same narrowing down to what all matches share, and the
same cycling through them on every further TAB. The difference is only where
the list goes - the console prints it, the menu draws it, because the console
is not visible while the menu is open. What lands in the line is a complete
command line, so ENTER runs it as it stands.
================
*/
void idDebugMenuLocal::CompleteEdit( void ) {
	idStr	shown;

	if ( !completeLive ) {
		// a fresh TAB collects what the engine offers for the line, the TABs
		// after it walk through what has been collected
		CollectMatches( editField.GetBuffer() );
		completeIndex = -1;
		completeNarrowed = false;
		completeLive = ( completeMatches.Num() > 0 );
		if ( !completeLive ) {
			SetStatus( "no completion" );
			return;
		}
	}

	if ( completeMatches.Num() == 1 ) {
		completeIndex = 0;
		SetEditBuffer( completeMatches[0].c_str() );
	} else if ( completeIndex < 0 && !completeNarrowed ) {
		// more than one match: narrow the line down to what all of them share,
		// exactly like the console does on the first TAB
		idStr	prefix;
		DM_CommonPrefix( completeMatches, prefix );
		SetEditBuffer( prefix.c_str() );
		completeNarrowed = true;	// the next TAB walks the matches
		completeIndex = -1;
	} else {
		completeIndex = ( completeIndex + 1 ) % completeMatches.Num();
		SetEditBuffer( completeMatches[completeIndex].c_str() );
	}

	shown = editField.GetBuffer();
	if ( shown.Length() > 24 ) {
		shown.CapLength( 24 );
	}

	if ( completeMatches.Num() == 1 ) {
		SetStatus( va( "completed: %s", shown.c_str() ) );
	} else if ( completeIndex >= 0 ) {
		SetStatus( va( "match %d/%d: %s", completeIndex + 1, completeMatches.Num(), shown.c_str() ) );
	} else {
		SetStatus( va( "completed: %s (%d matches, TAB walks them)", shown.c_str(), completeMatches.Num() ) );
	}
}

/*
================
idDebugMenuLocal::ApplyMatch

Puts the match the index points at into the line and reports it. Every step
and every jump ends up here, so they all land the same way.
================
*/
void idDebugMenuLocal::ApplyMatch( void ) {
	idStr	shown;

	if ( completeIndex < 0 || completeIndex >= completeMatches.Num() ) {
		return;
	}

	SetEditBuffer( completeMatches[completeIndex].c_str() );

	shown = editField.GetBuffer();
	if ( shown.Length() > 24 ) {
		shown.CapLength( 24 );
	}
	SetStatus( va( "match %d/%d: %s", completeIndex + 1, completeMatches.Num(), shown.c_str() ) );
}

/*
================
idDebugMenuLocal::StepMatch

Walks through the collected matches without leaving the line: UP and DOWN do
it one by one, TAB does the same.
================
*/
void idDebugMenuLocal::StepMatch( int delta ) {
	if ( completeMatches.Num() <= 1 ) {
		return;
	}

	if ( completeIndex < 0 ) {
		completeIndex = ( delta > 0 ) ? 0 : completeMatches.Num() - 1;
	} else {
		// a page can be longer than the list, so take the offset modulo the
		// number of matches and never leave the range
		completeIndex += delta % completeMatches.Num();
		completeIndex %= completeMatches.Num();
		if ( completeIndex < 0 ) {
			completeIndex += completeMatches.Num();
		}
	}

	ApplyMatch();
}

/*
================
idDebugMenuLocal::StepMatchPage
================
*/
void idDebugMenuLocal::StepMatchPage( int page ) {
	if ( completeMatches.Num() <= 1 || page == 0 ) {
		return;
	}

	// one panel holds DM_MATCH_ROWS matches, so a page steps that many
	StepMatch( page * DM_MATCH_ROWS );
}

/*
================
idDebugMenuLocal::GotoMatch

HOME and END take the first and the last match of the line, the way they take
the first and the last entry of the list. The index is clamped instead of
wrapped, so END on the last match stays where it is.
================
*/
void idDebugMenuLocal::GotoMatch( int index ) {
	if ( completeMatches.Num() <= 0 ) {
		return;
	}

	if ( index < 0 ) {
		index = 0;
	}
	if ( index >= completeMatches.Num() ) {
		index = completeMatches.Num() - 1;
	}

	completeIndex = index;
	ApplyMatch();
}

/*
================
idDebugMenuLocal::RunEditLine

Runs what stands in the filter row as a command line. The completion puts a
complete command line there ("map game/admin.map"), so ENTER runs it as it
stands, the way the console does, and the row goes back to filtering.
================
*/
void idDebugMenuLocal::RunEditLine( void ) {
	idStr	text = editField.GetBuffer();

	if ( text.Length() <= 0 ) {
		return;
	}

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "%s\n", text.c_str() ) );
	SetStatus( va( "executed: %s", text.c_str() ) );

	// the line has been used: the matches are stale and the filter goes back
	// to the whole list, the way the console clears its input
	ClearMatches();
	if ( !editing ) {
		editField.SetBuffer( "" );
		editField.SetCursor( 0 );
		SyncFilterFromEditField();
	}
}

/*
================
idDebugMenuLocal::SetList
================
*/
void idDebugMenuLocal::SetList( int newList ) {
	if ( newList < 0 || newList >= DM_LIST_NUM || newList == list ) {
		return;
	}
	ClearMatches();
	list = newList;
	selection = 0;
	scroll = 0;
	RefreshLists();
}

/*
================
idDebugMenuLocal::RefreshLists
================
*/
void idDebugMenuLocal::RefreshLists( void ) {
	int		i, num;

	listNames.Clear();

	switch ( list ) {
		case DM_LIST_CVARS: {
			num = cvarSystem->GetNumCVars();
			for ( i = 0; i < num; i++ ) {
				const idCVar *cvar = cvarSystem->GetCVarByIndex( i );
				if ( cvar && cvar->GetName() && cvar->GetName()[0] ) {
					listNames.Append( idStr( cvar->GetName() ) );
				}
			}
			break;
		}
		case DM_LIST_COMMANDS: {
			num = cmdSystem->GetNumCommands();
			for ( i = 0; i < num; i++ ) {
				const char *name = cmdSystem->GetCommandName( i );
				if ( name && name[0] ) {
					listNames.Append( idStr( name ) );
				}
			}
			break;
		}
		default: {
			for ( i = 0; i < dmNumActions; i++ ) {
				listNames.Append( idStr( dmActions[i].label ) );
			}
			break;
		}
	}

	// the hand written action list keeps its own order, everything else is sorted
	if ( list != DM_LIST_ACTIONS && listNames.Num() > 1 ) {
		listNames.Sort( &DM_StrListCompare );
	}

	filterDirty = true;
}

/*
================
idDebugMenuLocal::ApplyFilter
================
*/
void idDebugMenuLocal::ApplyFilter( void ) {
	int		i, num;

	filtered.Clear();

	num = listNames.Num();
	for ( i = 0; i < num; i++ ) {
		if ( DM_ContainsNoCase( listNames[i].c_str(), filterLower.c_str() ) ) {
			filtered.Append( i );
		}
	}

	filterDirty = false;

	if ( filtered.Num() == 0 ) {
		selection = 0;
		scroll = 0;
		return;
	}
	if ( selection < 0 ) {
		selection = 0;
	}
	if ( selection > filtered.Num() - 1 ) {
		selection = filtered.Num() - 1;
	}
	EnsureSelectionVisible();
}

/*
================
idDebugMenuLocal::MoveSelection
================
*/
void idDebugMenuLocal::MoveSelection( int delta ) {
	if ( filtered.Num() <= 0 ) {
		selection = 0;
		return;
	}
	selection += delta;
	if ( selection < 0 ) {
		selection = 0;
	}
	if ( selection > filtered.Num() - 1 ) {
		selection = filtered.Num() - 1;
	}
	EnsureSelectionVisible();
}

/*
================
idDebugMenuLocal::EnsureSelectionVisible
================
*/
void idDebugMenuLocal::EnsureSelectionVisible( void ) {
	if ( selection < scroll ) {
		scroll = selection;
	}
	if ( selection >= scroll + DM_LIST_ROWS ) {
		scroll = selection - DM_LIST_ROWS + 1;
	}
	if ( scroll < 0 ) {
		scroll = 0;
	}
	if ( scroll > filtered.Num() - DM_LIST_ROWS && filtered.Num() > DM_LIST_ROWS ) {
		scroll = filtered.Num() - DM_LIST_ROWS;
	}
}

/*
================
idDebugMenuLocal::SelectedListIndex
================
*/
int idDebugMenuLocal::SelectedListIndex( void ) const {
	if ( selection < 0 || selection >= filtered.Num() ) {
		return -1;
	}
	return filtered[selection];
}

/*
================
idDebugMenuLocal::SelectedName
================
*/
const char *idDebugMenuLocal::SelectedName( void ) const {
	int index = SelectedListIndex();
	if ( index < 0 || index >= listNames.Num() ) {
		return NULL;
	}
	return listNames[index].c_str();
}

/*
================
idDebugMenuLocal::SetStatus
================
*/
void idDebugMenuLocal::SetStatus( const char *text ) {
	status = text ? text : "";
}

// ---------------------------------------------------------------------------
// input handling
// ---------------------------------------------------------------------------

/*
================
idDebugMenuLocal::ProcessEvent

Called for every system event before the game sees it. While the menu is open
it swallows key, character and mouse events so the player does not move around.
================
*/
bool idDebugMenuLocal::ProcessEvent( const sysEvent_t *event ) {
	if ( !event ) {
		return false;
	}

	if ( !active ) {
		if ( event->evType == SE_KEY && event->evValue2 == 1 && event->evValue == ToggleKeyNum() ) {
			Open( NULL );
			return true;
		}
		return false;
	}

	switch ( event->evType ) {
		case SE_KEY:
			if ( event->evValue2 == 1 && event->evValue ) {
				KeyDownEvent( event->evValue );
			}
			return true;
		case SE_CHAR:
			CharEvent( event->evValue );
			return true;
		case SE_MOUSE:
			return true;		// keep the view from spinning while browsing
		default:
			return false;
	}
}

/*
================
idDebugMenuLocal::KeyDownEvent
================
*/
void idDebugMenuLocal::KeyDownEvent( int key ) {
	if ( editing ) {
		if ( key == K_ESCAPE ) {
			CancelEdit();
		} else if ( key == K_ENTER || key == K_KP_ENTER ) {
			EndEdit();
		} else if ( key == K_TAB ) {
			CompleteEdit();
		} else if ( completeLive && ( key == K_UPARROW || key == K_DOWNARROW
				|| key == K_PGUP || key == K_PGDN ) ) {
			// walk through the matches of the line that is being edited;
			// PGUP and PGDN move a whole panel of them at once
			if ( key == K_PGUP || key == K_PGDN ) {
				StepMatchPage( ( key == K_PGDN ) ? 1 : -1 );
			} else {
				StepMatch( ( key == K_DOWNARROW ) ? 1 : -1 );
			}
		} else if ( completeLive && completeMatches.Num() > 1 && ( key == K_HOME || key == K_END ) ) {
			// the ends of the collected matches, the way HOME and END take the ends
			// of the list; with a single match the keys keep moving the cursor
			GotoMatch( ( key == K_END ) ? completeMatches.Num() - 1 : 0 );
		} else {
			ClearMatches();
			editField.KeyDownEvent( key );
		}
		return;
	}

	// the matches of the line are on screen: UP and DOWN walk them the way
	// TAB does, PGUP and PGDN walk a whole panel of them, HOME and END jump
	// to the first and the last of them, instead of scrolling the selection
	// behind the panel
	if ( completeLive && completeMatches.Num() > 1
			&& ( key == K_UPARROW || key == K_DOWNARROW || key == K_PGUP || key == K_PGDN
				|| key == K_HOME || key == K_END ) ) {
		if ( key == K_PGUP || key == K_PGDN ) {
			StepMatchPage( ( key == K_PGDN ) ? 1 : -1 );
		} else if ( key == K_HOME || key == K_END ) {
			GotoMatch( ( key == K_END ) ? completeMatches.Num() - 1 : 0 );
		} else {
			StepMatch( ( key == K_DOWNARROW ) ? 1 : -1 );
		}
		return;
	}

	// browsing the list takes over from a match list that is still on screen
	if ( key == K_UPARROW || key == K_DOWNARROW || key == K_PGUP || key == K_PGDN
			|| key == K_HOME || key == K_END ) {
		ClearMatches();
	}

	if ( key == ToggleKeyNum() || key == K_ESCAPE ) {
		Close();
		return;
	}

	switch ( key ) {
		case K_UPARROW:
			MoveSelection( -1 );
			break;
		case K_DOWNARROW:
			MoveSelection( 1 );
			break;
		case K_PGUP:
			MoveSelection( -DM_LIST_ROWS );
			break;
		case K_PGDN:
			MoveSelection( DM_LIST_ROWS );
			break;
		case K_HOME:
			selection = 0;
			EnsureSelectionVisible();
			break;
		case K_END:
			selection = filtered.Num() - 1;
			if ( selection < 0 ) {
				selection = 0;
			}
			EnsureSelectionVisible();
			break;
		case K_ENTER:
		case K_KP_ENTER:
			if ( completeLive && completeMatches.Num() > 0 ) {
				RunEditLine();		// the line holds a match, ENTER runs it as it is
			} else {
				ExecuteSelected();
			}
			break;
		case K_TAB:
			// this CVar only decides TAB in the filter row; the cmd:/val: line
			// is handled above and completes whatever this is set to
			if ( dm_tabCompletesFilter.GetBool() && !idKeyInput::IsDown( K_CTRL ) ) {
				CompleteEdit();
			} else if ( idKeyInput::IsDown( K_SHIFT ) ) {
				SetList( ( list + DM_LIST_NUM - 1 ) % DM_LIST_NUM );
			} else {
				SetList( ( list + 1 ) % DM_LIST_NUM );
			}
			break;
		case K_BACKSPACE:
		case K_DEL:
		case K_LEFTARROW:
		case K_RIGHTARROW:
			editField.KeyDownEvent( key );
			SyncFilterFromEditField();
			break;
		default:
			break;
	}
}

/*
================
idDebugMenuLocal::CharEvent
================
*/
void idDebugMenuLocal::CharEvent( int ch ) {
	ClearMatches();				// the line is typed again, the matches are stale
	editField.CharEvent( ch );
	if ( !editing ) {
		SyncFilterFromEditField();
	}
}

/*
================
idDebugMenuLocal::ExecuteSelected
================
*/
void idDebugMenuLocal::ExecuteSelected( void ) {
	const char *name = SelectedName();
	if ( !name ) {
		SetStatus( "nothing selected" );
		return;
	}

	if ( list == DM_LIST_CVARS ) {
		idCVar *cvar = cvarSystem->Find( name );
		if ( !cvar ) {
			SetStatus( va( "%s: not found", name ) );
			return;
		}
		if ( cvar->GetFlags() & ( CVAR_ROM | CVAR_INIT ) ) {
			SetStatus( va( "%s is read only", name ) );
			return;
		}
		if ( cvar->GetFlags() & CVAR_BOOL ) {
			cvarSystem->SetCVarBool( name, !cvar->GetBool(), 0 );
			SetStatus( va( "%s = %s", name, cvarSystem->GetCVarString( name ) ) );
			return;
		}
		BeginEdit( cvar->GetString(), name, false );
		return;
	}

	if ( list == DM_LIST_COMMANDS ) {
		BeginEdit( va( "%s ", name ), name, true );
		return;
	}

	int index = SelectedListIndex();
	if ( index >= 0 && index < dmNumActions ) {
		RunAction( dmActions[index] );
	}
}

/*
================
idDebugMenuLocal::RunAction
================
*/
void idDebugMenuLocal::RunAction( const dmActionDef_t &action ) {
	if ( !action.target ) {
		return;
	}

	switch ( action.kind ) {
		case DM_ACTION_COMMAND: {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "%s\n", action.target ) );
			SetStatus( va( "executed: %s", action.target ) );
			break;
		}
		case DM_ACTION_CVAR_BOOL: {
			idCVar *cvar = cvarSystem->Find( action.target );
			if ( !cvar ) {
				SetStatus( va( "%s: not found", action.target ) );
				break;
			}
			cvarSystem->SetCVarBool( action.target, !cvar->GetBool(), 0 );
			SetStatus( va( "%s = %s", action.target, cvarSystem->GetCVarString( action.target ) ) );
			break;
		}
		default: {
			if ( idStr::Icmp( action.target, "#refresh" ) == 0 ) {
				RefreshLists();
				SetStatus( "lists refreshed" );
			} else if ( idStr::Icmp( action.target, "#clearfilter" ) == 0 ) {
				SetFilter( "" );
				ApplyFilter();
				SetStatus( "filter cleared" );
			} else if ( idStr::Icmp( action.target, "#close" ) == 0 ) {
				Close();
			}
			break;
		}
	}
}

/*
================
idDebugMenuLocal::BeginEdit
================
*/
void idDebugMenuLocal::BeginEdit( const char *initialText, const char *target, bool isCommand ) {
	ClearMatches();
	editing = true;
	editIsCommand = isCommand;
	editTarget = target ? target : "";
	editField.SetBuffer( initialText ? initialText : "" );
	editField.ClearAutoComplete();
	editField.SetCursor( (int)strlen( editField.GetBuffer() ) );
	SetStatus( isCommand ? "ENTER runs the command, ESC cancels" : "ENTER applies the value, ESC cancels" );
}

/*
================
idDebugMenuLocal::CancelEdit
================
*/
void idDebugMenuLocal::CancelEdit( void ) {
	ClearMatches();
	editing = false;
	editField.SetBuffer( filterText.c_str() );
	editField.ClearAutoComplete();
	editField.SetCursor( (int)strlen( editField.GetBuffer() ) );
	SetStatus( "edit cancelled" );
}

/*
================
idDebugMenuLocal::EndEdit
================
*/
void idDebugMenuLocal::EndEdit( void ) {
	idStr text = editField.GetBuffer();

	ClearMatches();

	if ( editIsCommand ) {
		if ( text.Length() > 0 ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "%s\n", text.c_str() ) );
			SetStatus( va( "executed: %s", text.c_str() ) );
		}
	} else {
		idCVar *cvar = cvarSystem->Find( editTarget.c_str() );
		if ( cvar ) {
			cvarSystem->SetCVarString( cvar->GetName(), text.c_str(), 0 );
			SetStatus( va( "%s = %s", cvar->GetName(), cvarSystem->GetCVarString( cvar->GetName() ) ) );
		} else {
			SetStatus( va( "%s: not found", editTarget.c_str() ) );
		}
	}

	editing = false;
	editField.SetBuffer( filterText.c_str() );
	editField.SetCursor( (int)strlen( editField.GetBuffer() ) );
	editField.ClearAutoComplete();
}

// ---------------------------------------------------------------------------
// drawing
// ---------------------------------------------------------------------------

/*
================
idDebugMenuLocal::DrawText
================
*/
void idDebugMenuLocal::DrawText( int col, int row, const char *text, const idVec4 &color ) {
	if ( !text || !text[0] || !bigCharShader ) {
		return;
	}
	if ( row < 0 || row >= DM_ROWS ) {
		return;
	}
	if ( col < 0 ) {
		col = 0;
	}
	if ( col >= DM_COLS ) {
		return;
	}
#if DM_USE_SMALL_FONT
	renderSystem->DrawSmallStringExt( col * DM_CHAR_W, row * DM_CHAR_H, text, color, true, bigCharShader );
#else
	renderSystem->DrawBigStringExt( col * DM_CHAR_W, row * DM_CHAR_H, text, color, true, bigCharShader );
#endif
}

/*
================
idDebugMenuLocal::DrawTextClipped
================
*/
void idDebugMenuLocal::DrawTextClipped( int col, int row, const char *text, const idVec4 &color, int maxChars ) {
	char buffer[512];
	DM_CopyTruncated( buffer, sizeof( buffer ), text, maxChars );
	DrawText( col, row, buffer, color );
}

/*
================
idDebugMenuLocal::DrawRectPixels
================
*/
void idDebugMenuLocal::DrawRectPixels( float x, float y, float w, float h, const idVec4 &color ) {
	if ( !whiteShader ) {
		return;
	}
	if ( x < 0.0f ) {
		w += x;
		x = 0.0f;
	}
	if ( y < 0.0f ) {
		h += y;
		y = 0.0f;
	}
	if ( x + w > (float)DM_WIDTH ) {
		w = DM_WIDTH - x;
	}
	if ( y + h > (float)DM_HEIGHT ) {
		h = DM_HEIGHT - y;
	}
	if ( w <= 0.0f || h <= 0.0f ) {
		return;
	}
	renderSystem->SetColor( color );
	renderSystem->DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, whiteShader );
}

/*
================
idDebugMenuLocal::DrawRowBar
================
*/
void idDebugMenuLocal::DrawRowBar( int row, const idVec4 &color ) {
	DrawRectPixels( 0.0f, (float)( row * DM_CHAR_H ), (float)DM_WIDTH, (float)DM_CHAR_H, color );
}

/*
================
idDebugMenuLocal::Draw
================
*/
void idDebugMenuLocal::Draw( void ) {
	char	buffer[512];
	char	line[128];
	int		i, x;
	idVec4	bgColor;

	if ( !active ) {
		return;
	}
	if ( !bigCharShader || !whiteShader ) {
		return;
	}

	// Keep a single player game stopped while this menu is open. The dhewm3
	// settings menu (or a script) may have cleared g_stopTime in the meantime,
	// and while this menu is open the player must stay safe.
	if ( pauseSetByMenu ) {
		idCVar *stopTime = cvarSystem->Find( "g_stoptime" );
		if ( stopTime != NULL && !stopTime->GetBool() ) {
			stopTime->SetBool( true );
		}
	}

	if ( filterDirty ) {
		ApplyFilter();
	}

	// background, title bar and separators
	bgColor = DM_COLOR_BG;
	bgColor[3] = idMath::ClampFloat( 0.0f, 1.0f, dm_bgAlpha.GetFloat() );
	DrawRectPixels( 0.0f, 0.0f, (float)DM_WIDTH, (float)DM_HEIGHT, bgColor );
	DrawRowBar( DM_ROW_TITLE, DM_COLOR_BAR );
	DrawRectPixels( 0.0f, (float)( DM_ROW_LIST * DM_CHAR_H - 2 ), (float)DM_WIDTH, 2.0f, DM_COLOR_SEP );
	DrawRectPixels( 0.0f, (float)( DM_ROW_STATUS * DM_CHAR_H - 2 ), (float)DM_WIDTH, 2.0f, DM_COLOR_SEP );

	// title and item count
	DrawText( 1, DM_ROW_TITLE, "DEBUG MENU", DM_COLOR_TITLE );
	sprintf( buffer, "%d/%d", filtered.Num(), listNames.Num() );
	DrawText( DM_COLS - 3 - (int)strlen( buffer ), DM_ROW_TITLE, buffer, DM_COLOR_DIM );

	// list tabs
	x = 1;
	for ( i = 0; i < DM_LIST_NUM; i++ ) {
		bool isCurrent = ( i == list );
		if ( isCurrent ) {
			sprintf( buffer, "[%s]", dmListNames[i] );
		} else {
			sprintf( buffer, " %s ", dmListNames[i] );
		}
		DrawText( x, DM_ROW_TABS, buffer, isCurrent ? DM_COLOR_SEL : DM_COLOR_DIM );
		x += (int)strlen( buffer ) + 1;
		if ( x >= DM_COLS - 12 ) {
			break;
		}
	}

	// filter / value edit line
	if ( editing ) {
		DrawText( 1, DM_ROW_FILTER, editIsCommand ? "cmd: " : "val: ", DM_COLOR_EDIT );
		DM_InsertCaret( editField.GetBuffer(), editField.GetCursor(), buffer, sizeof( buffer ) );
		DrawTextClipped( 6, DM_ROW_FILTER, DM_TailOfString( buffer, DM_COLS - 8 ), DM_COLOR_EDIT, DM_COLS - 8 );
	} else {
		DrawText( 1, DM_ROW_FILTER, "filter:", DM_COLOR_DIM );
		DM_InsertCaret( editField.GetBuffer(), editField.GetCursor(), buffer, sizeof( buffer ) );
		DrawTextClipped( 9, DM_ROW_FILTER, DM_TailOfString( buffer, DM_COLS - 11 ), DM_COLOR_TEXT, DM_COLS - 11 );
	}

	// the list itself
	DrawList();

	if ( MatchesActive() ) {
		// the matches of the line being completed take the room of the list and
		// of the details, which is where the console prints its own candidates
		DrawMatches();
	} else {
		// details of the selected entry
		const char *name = SelectedName();
		if ( !name ) {
			DrawText( 1, DM_ROW_INFO, "nothing matches the filter", DM_COLOR_DIM );
		} else if ( list == DM_LIST_CVARS ) {
			DrawCvarInfo( name );
		} else if ( list == DM_LIST_COMMANDS ) {
			DrawCommandInfo( name );
		} else {
			DrawActionInfo( SelectedListIndex() );
		}
	}

	// last action and the key hints
	DrawText( 1, DM_ROW_STATUS, status.c_str(), DM_COLOR_STATUS );
	if ( MatchesActive() && editing ) {
		DrawText( 1, DM_ROW_HINT1, "TAB next  UP/DOWN move  PGUP/PGDN page  HOME/END first/last  ENTER runs", DM_COLOR_DIM );
	} else if ( MatchesActive() ) {
		DrawText( 1, DM_ROW_HINT1, "filter matches  TAB cycles  PGUP/PGDN page  HOME/END first/last", DM_COLOR_DIM );
	} else if ( editing ) {
		DrawText( 1, DM_ROW_HINT1, "TAB completes command and arguments", DM_COLOR_DIM );
	} else {
		DrawText( 1, DM_ROW_HINT1, "UP/DOWN browse  PGUP/PGDN page  HOME/END first/last  TAB list", DM_COLOR_DIM );
	}
	sprintf( line, "TYPE filter  ENTER run/edit  ESC/%s close", idKeyInput::KeyNumToString( ToggleKeyNum(), true ) );
	DrawTextClipped( 1, DM_ROW_HINT2, line, DM_COLOR_DIM, DM_COLS - 1 );
}

/*
================
idDebugMenuLocal::DrawList
================
*/
void idDebugMenuLocal::DrawList( void ) {
	char	namePart[64];
	char	valuePart[64];
	char	value[128];
	int		row, index, listIndex;
	bool	selected;
	idStr	line;

	if ( filtered.Num() <= 0 ) {
		return;
	}

	for ( row = 0; row < DM_LIST_ROWS; row++ ) {
		index = scroll + row;
		if ( index >= filtered.Num() ) {
			break;
		}

		listIndex = filtered[index];
		if ( listIndex < 0 || listIndex >= listNames.Num() ) {
			continue;
		}

		selected = ( index == selection );
		if ( selected ) {
			DrawRowBar( DM_ROW_LIST + row, DM_COLOR_SEL_BG );
		}

		value[0] = '\0';
		if ( dm_showValues.GetBool() ) {
			if ( list == DM_LIST_CVARS ) {
				const idCVar *cvar = cvarSystem->Find( listNames[listIndex].c_str() );
				if ( cvar && cvar->GetString() ) {
					idStr::Copynz( value, cvar->GetString(), sizeof( value ) );
				}
			} else if ( list == DM_LIST_ACTIONS && listIndex < dmNumActions ) {
				idStr::Copynz( value, dmActions[listIndex].target ? dmActions[listIndex].target : "", sizeof( value ) );
			}
		}

		DM_CopyTruncated( namePart, sizeof( namePart ), listNames[listIndex].c_str(), DM_NAME_COLS );
		DM_CopyTruncated( valuePart, sizeof( valuePart ), value, DM_VALUE_COLS );

		line = namePart;
		while ( line.Length() < DM_NAME_COLS ) {
			line += " ";
		}
		line += " ";
		line += valuePart;

		DrawTextClipped( 0, DM_ROW_LIST + row, line.c_str(), selected ? DM_COLOR_SEL : DM_COLOR_TEXT, DM_COLS );
	}
}

/*
================
idDebugMenuLocal::DrawMatches

Draws the candidates the engine offered for the line that is being completed,
the list the console would have printed. It covers the list and the details,
their rows are simply the room for it, and the match that is in the line right
now is highlighted. Every entry can be run as it stands.
================
*/
void idDebugMenuLocal::DrawMatches( void ) {
	char	buffer[512];
	char	line[256];
	int		row, i, num, rows, first, last, start, col;
	bool	current;
	idVec4	panelColor;

	if ( !MatchesActive() ) {
		return;
	}

	num = completeMatches.Num();
	first = DM_ROW_LIST;
	last = DM_ROW_DESC2;				// the panel covers list and details
	rows = last - first;				// every row below its own header

	// the panel follows dbgmenu_bgAlpha the way the menu background does
	panelColor = DM_COLOR_PANEL;
	panelColor[3] = DM_COLOR_PANEL[3] * idMath::ClampFloat( 0.0f, 1.0f, dm_bgAlpha.GetFloat() );
	DrawRectPixels( 0.0f, (float)( first * DM_CHAR_H ), (float)DM_WIDTH,
			(float)( ( last - first + 1 ) * DM_CHAR_H ), panelColor );
	DrawRowBar( first, DM_COLOR_BAR );

	if ( completeIndex >= 0 ) {
		sprintf( buffer, "MATCHES  %d/%d", completeIndex + 1, num );
	} else {
		sprintf( buffer, "MATCHES  %d", num );
	}
	DrawText( 1, first, buffer, DM_COLOR_TITLE );

	// the line these matches belong to
	col = DM_COLS / 2;
	DM_CopyTruncated( line, sizeof( line ), editField.GetBuffer(), DM_COLS - col - 5 );
	sprintf( buffer, "for: %s", line );
	DrawTextClipped( col, first, buffer, DM_COLOR_DIM, DM_COLS - col );

	// the window of matches follows the one the line holds
	start = 0;
	if ( num > rows ) {
		start = ( completeIndex >= 0 ) ? completeIndex - rows / 2 : 0;
		if ( start < 0 ) {
			start = 0;
		}
		if ( start > num - rows ) {
			start = num - rows;
		}
	}

	for ( row = 0; row < rows; row++ ) {
		i = start + row;
		if ( i >= num ) {
			break;
		}
		current = ( i == completeIndex );
		if ( current ) {
			DrawRowBar( first + 1 + row, DM_COLOR_SEL_BG );
		}
		DrawTextClipped( 1, first + 1 + row, completeMatches[i].c_str(),
				current ? DM_COLOR_SEL : DM_COLOR_TEXT, DM_COLS - 2 );
	}
}

/*
================
idDebugMenuLocal::DrawCvarInfo
================
*/
void idDebugMenuLocal::DrawCvarInfo( const char *name ) {
	char		buffer[512];
	char		flags[256];
	char		range[64];
	const idCVar *cvar = cvarSystem->Find( name );

	if ( !cvar ) {
		DrawTextClipped( 1, DM_ROW_INFO, name, DM_COLOR_TITLE, DM_COLS - 2 );
		DrawText( 1, DM_ROW_VALUE, "CVar was removed", DM_COLOR_DIM );
		return;
	}

	DM_FlagsToString( dmCVarFlagNames, dmNumCVarFlagNames, cvar->GetFlags(), flags, sizeof( flags ) );
	DM_RangeString( cvar, range, sizeof( range ) );

	// name and type
	sprintf( buffer, "%s [%s]", cvar->GetName(),
			( cvar->GetFlags() & CVAR_BOOL ) ? "bool" :
			( cvar->GetFlags() & CVAR_INTEGER ) ? "int" :
			( cvar->GetFlags() & CVAR_FLOAT ) ? "float" : "string" );
	DrawTextClipped( 1, DM_ROW_INFO, buffer, DM_COLOR_TITLE, DM_COLS - 2 );

	// current value
	if ( range[0] ) {
		sprintf( buffer, "value: %s   %s", cvar->GetString(), range );
	} else {
		sprintf( buffer, "value: %s", cvar->GetString() );
	}
	DrawTextClipped( 1, DM_ROW_VALUE, buffer, DM_COLOR_TEXT, DM_COLS - 2 );

	// flags and the possible values of an enum style CVar
	sprintf( buffer, "flags: %s", flags );
	DrawTextClipped( 1, DM_ROW_DESC1, buffer, DM_COLOR_DIM, DM_COLS - 2 );

	// description
	DM_WrapText( cvar->GetDescription(), DM_COLS - 2, buffer, sizeof( buffer ) );
	DrawTextClipped( 1, DM_ROW_DESC2, buffer, DM_COLOR_TEXT, DM_COLS - 2 );
}

/*
================
idDebugMenuLocal::DrawCommandInfo
================
*/
void idDebugMenuLocal::DrawCommandInfo( const char *name ) {
	char	buffer[512];
	char	flags[256];
	int		i, num;
	bool	found = false;

	DrawTextClipped( 1, DM_ROW_INFO, name, DM_COLOR_TITLE, DM_COLS - 2 );

	num = cmdSystem->GetNumCommands();
	for ( i = 0; i < num; i++ ) {
		const char *cmdName = cmdSystem->GetCommandName( i );
		if ( !cmdName || idStr::Icmp( cmdName, name ) != 0 ) {
			continue;
		}
		found = true;
		DM_FlagsToString( dmCmdFlagNames, dmNumCmdFlagNames, cmdSystem->GetCommandFlags( i ), flags, sizeof( flags ) );
		sprintf( buffer, "command  flags: %s", flags );
		DrawTextClipped( 1, DM_ROW_VALUE, buffer, DM_COLOR_DIM, DM_COLS - 2 );
		DM_WrapText( cmdSystem->GetCommandDescription( i ), DM_COLS - 2, buffer, sizeof( buffer ) );
		DrawTextClipped( 1, DM_ROW_DESC1, buffer, DM_COLOR_TEXT, DM_COLS - 2 );
		break;
	}

	if ( !found ) {
		DrawTextClipped( 1, DM_ROW_VALUE, "command was removed", DM_COLOR_DIM, DM_COLS - 2 );
	}

	DrawText( 1, DM_ROW_DESC2, "ENTER prepares the command line", DM_COLOR_DIM );
}

/*
================
idDebugMenuLocal::DrawActionInfo
================
*/
void idDebugMenuLocal::DrawActionInfo( int actionIndex ) {
	char	buffer[512];
	char	temp[128];

	if ( actionIndex < 0 || actionIndex >= dmNumActions ) {
		return;
	}

	const dmActionDef_t &action = dmActions[actionIndex];

	DrawTextClipped( 1, DM_ROW_INFO, action.label, DM_COLOR_TITLE, DM_COLS - 2 );

	switch ( action.kind ) {
		case DM_ACTION_COMMAND:
			sprintf( buffer, "console command: %s", action.target ? action.target : "" );
			break;
		case DM_ACTION_CVAR_BOOL:
			sprintf( buffer, "toggles CVar: %s = %s", action.target ? action.target : "",
					cvarSystem->GetCVarString( action.target ? action.target : "" ) );
			break;
		default:
			sprintf( buffer, "menu action: %s", action.target ? action.target : "" );
			break;
	}
	DrawTextClipped( 1, DM_ROW_VALUE, buffer, DM_COLOR_TEXT, DM_COLS - 2 );

	if ( action.kind == DM_ACTION_COMMAND ) {
		DM_WrapText( "Runs right away through the command buffer.", DM_COLS - 2, temp, sizeof( temp ) );
		DrawTextClipped( 1, DM_ROW_DESC1, temp, DM_COLOR_DIM, DM_COLS - 2 );
	} else if ( action.kind == DM_ACTION_CVAR_BOOL ) {
		DM_WrapText( "Flips the value of an engine CVar.", DM_COLS - 2, temp, sizeof( temp ) );
		DrawTextClipped( 1, DM_ROW_DESC1, temp, DM_COLOR_DIM, DM_COLS - 2 );
	} else {
		DM_WrapText( "Handled inside the debug menu.", DM_COLS - 2, temp, sizeof( temp ) );
		DrawTextClipped( 1, DM_ROW_DESC1, temp, DM_COLOR_DIM, DM_COLS - 2 );
	}

	DrawTextClipped( 1, DM_ROW_DESC2, "ENTER executes", DM_COLOR_TEXT, DM_COLS - 2 );
}

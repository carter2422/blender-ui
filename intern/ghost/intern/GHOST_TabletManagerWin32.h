// safe & friendly Wintab wrapper
// by Mike Erwin, July 2010

#ifndef GHOST_TABLET_MANAGER_WIN32_H
#define GHOST_TABLET_MANAGER_WIN32_H

#define _WIN32_WINNT 0x501 // require Windows XP or newer
#define WIN32_LEAN_AND_MEAN
#include	<windows.h>
#include "wintab.h"
#include <map>

class GHOST_WindowWin32;

// BEGIN from Wacom's Utils.h
typedef UINT ( API * WTINFOA ) ( UINT, UINT, LPVOID );
typedef HCTX ( API * WTOPENA ) ( HWND, LPLOGCONTEXTA, BOOL );
typedef BOOL ( API * WTCLOSE ) ( HCTX );
typedef BOOL ( API * WTQUEUESIZESET ) ( HCTX, int );
typedef int  ( API * WTPACKETSGET ) ( HCTX, int, LPVOID );
// END

typedef enum { TABLET_NONE, TABLET_PEN, TABLET_ERASER, TABLET_MOUSE } TabletToolType;

typedef struct
	{
	TabletToolType type : 6; // plenty of room to grow

	// capabilities
	bool hasPressure : 1;
	bool hasTilt : 1;
	
	} TabletTool;


typedef struct
	{
	TabletTool tool;

	float pressure;
	float tilt_x, tilt_y;

	} TabletToolData;


class GHOST_TabletManagerWin32
	{
	// the Wintab library
	HINSTANCE lib_Wintab; // or HMODULE?

	// WinTab function pointers
	WTOPENA func_Open;
	WTCLOSE func_Close;
	WTINFOA func_Info;
	WTQUEUESIZESET func_QueueSizeSet;
	WTPACKETSGET func_PacketsGet;

	// tablet attributes
	bool hasPressure;
	float pressureScale;
	bool hasTilt;
	float tiltScale;

	// candidates for a base class:
	TabletTool activeTool;
	void resetActiveTool();

	// book-keeping
	std::map<GHOST_WindowWin32*,HCTX> contexts;
	HCTX contextForWindow(GHOST_WindowWin32*);

public:
	GHOST_TabletManagerWin32();
	~GHOST_TabletManagerWin32();
	bool available(); // another base class candidate
	void openForWindow(GHOST_WindowWin32* window);
	void closeForWindow(GHOST_WindowWin32* window);
	void processPackets(GHOST_WindowWin32* window);
	void changeTool(GHOST_WindowWin32* window);
	};

#endif

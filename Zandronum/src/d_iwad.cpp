/*
** d_iwad.cpp
** IWAD detection code
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** Copyright 2009 CHristoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include "d_main.h"
#include "gi.h"
#include "cmdlib.h"
#include "doomstat.h"
#include "i_system.h"
#include "w_wad.h"
#include "w_zip.h"
#include "v_palette.h"
#include "m_argv.h"
#include "m_misc.h"
#include "c_cvars.h"
#include "gameconfigfile.h"
// [BB] New #includes.
#include "doomerrors.h"
#include "version.h"


EIWADType gameiwad;

CVAR (Bool, queryiwad, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG);
CVAR (String, defaultiwad, "", CVAR_ARCHIVE|CVAR_GLOBALCONFIG);

// If autoname is NULL, that's either because that game doesn't allow
// loading of external wads or because it's already caught by the
// general game-specific wads section.
const IWADInfo IWADInfos[NUM_IWAD_TYPES] =
{
	// banner text,								autoname,	fg color,				bg color
	{ "Final Doom: TNT - Evilution",			"TNT",		MAKERGB(168,0,0),		MAKERGB(168,168,168),	GAME_Doom,		"mapinfo/tnt.txt",		GI_MAPxx },
	{ "Final Doom: Plutonia Experiment",		"Plutonia",	MAKERGB(168,0,0),		MAKERGB(168,168,168),	GAME_Doom,		"mapinfo/plutonia.txt",	GI_MAPxx },
	{ "Hexen: Beyond Heretic",					NULL,		MAKERGB(240,240,240),	MAKERGB(107,44,24),		GAME_Hexen,		"mapinfo/hexen.txt",	GI_MAPxx },
	{ "Hexen: Deathkings of the Dark Citadel",	"HexenDK",	MAKERGB(240,240,240),	MAKERGB(139,68,9),		GAME_Hexen,		"mapinfo/hexen.txt",	GI_MAPxx },
	{ "Hexen: Demo Version",					"HexenDemo",MAKERGB(240,240,240),	MAKERGB(107,44,24),		GAME_Hexen,		"mapinfo/hexen.txt",	GI_MAPxx | GI_SHAREWARE },
	{ "DOOM 2: Hell on Earth",					"Doom2",	MAKERGB(168,0,0),		MAKERGB(168,168,168),	GAME_Doom,		"mapinfo/doom2.txt",	GI_MAPxx },
	{ "Heretic Shareware",						NULL,		MAKERGB(252,252,0),		MAKERGB(168,0,0),		GAME_Heretic,	"mapinfo/hereticsw.txt",GI_SHAREWARE },
	{ "Heretic: Shadow of the Serpent Riders",	NULL,		MAKERGB(252,252,0),		MAKERGB(168,0,0),		GAME_Heretic,	"mapinfo/heretic.txt",	GI_MENUHACK_EXTENDED },
	{ "Heretic",								NULL,		MAKERGB(252,252,0),		MAKERGB(168,0,0),		GAME_Heretic,	"mapinfo/heretic.txt" },
	{ "DOOM Shareware",							NULL,		MAKERGB(168,0,0),		MAKERGB(168,168,168),	GAME_Doom,		"mapinfo/doom1.txt",	GI_SHAREWARE },
	{ "The Ultimate DOOM",						"Doom1",	MAKERGB(84,84,84),		MAKERGB(168,168,168),	GAME_Doom,		"mapinfo/ultdoom.txt" },
	{ "DOOM Registered",						"Doom1",	MAKERGB(84,84,84),		MAKERGB(168,168,168),	GAME_Doom,		"mapinfo/doom1.txt" },
	{ "Strife: Quest for the Sigil",			NULL,		MAKERGB(224,173,153),	MAKERGB(0,107,101),		GAME_Strife,	"mapinfo/strife.txt",	GI_MAPxx },
	{ "Strife: Teaser (Old Version)",			NULL,		MAKERGB(224,173,153),	MAKERGB(0,107,101),		GAME_Strife,	"mapinfo/strife.txt",	GI_MAPxx | GI_SHAREWARE },
	{ "Strife: Teaser (New Version)",			NULL,		MAKERGB(224,173,153),	MAKERGB(0,107,101),		GAME_Strife,	"mapinfo/strife.txt",	GI_MAPxx | GI_SHAREWARE | GI_TEASER2 },
	{ "Freedoom",								"Freedoom",	MAKERGB(50,84,67),		MAKERGB(198,220,209),	GAME_Doom,		"mapinfo/doom2.txt",	GI_MAPxx },
	{ "Ultimate Freedoom",						"Freedoom1",MAKERGB(50,84,67),		MAKERGB(198,220,209),	GAME_Doom,		"mapinfo/doom1.txt" },
	{ "Freedoom \"Demo\"",						NULL,		MAKERGB(50,84,67),		MAKERGB(198,220,209),	GAME_Doom,		"mapinfo/doom1.txt" },
	{ "FreeDM",									"FreeDM",	MAKERGB(50,84,67),		MAKERGB(198,220,209),	GAME_Doom,		"mapinfo/doom2.txt",	GI_MAPxx },
	{ "Chex(R) Quest",							"Chex1",	MAKERGB(255,255,0),		MAKERGB(0,192,0),		GAME_Chex,		"mapinfo/chex.txt" },
	{ "Chex(R) Quest 3",						"Chex3",	MAKERGB(255,255,0),		MAKERGB(0,192,0),		GAME_Chex,		"mapinfo/chex3.txt" },
	//{ "ZDoom Engine",							NULL,		MAKERGB(168,0,0),		MAKERGB(168,168,168) },
};

static const char *IWADNames[] =
{
	NULL,
	"doom2f.wad",
	"doom2.wad",
	"plutonia.wad",
	"tnt.wad",
	"doomu.wad", // Hack from original Linux version. Not necessary, but I threw it in anyway.
	"doom.wad",
	"doom1.wad",
	"heretic.wad",
	"heretic1.wad",
	"hexen.wad",
	"hexdd.wad",
	"hexendemo.wad",
	"hexdemo.wad",
	"strife1.wad",
	"strife0.wad",
	"freedoom.wad", // Freedoom.wad is distributed as Doom2.wad, but this allows to have both in the same directory.
	"freedoom1.wad",
	"freedoomu.wad",
	"freedm.wad",
	"chex.wad",
	"chex3.wad",
#ifdef unix
	"DOOM2.WAD",    // Also look for all-uppercase names
	"PLUTONIA.WAD",
	"TNT.WAD",
	"DOOM.WAD",
	"DOOM1.WAD",
	"HERETIC.WAD",
	"HERETIC1.WAD",
	"HEXEN.WAD",
	"HEXDD.WAD",
	"HEXENDEMO.WAD",
	"HEXDEMO.WAD",
	"STRIFE1.WAD",
	"STRIFE0.WAD",
	"FREEDOOM.WAD",
	"FREEDOOM1.WAD",
	"FREEDOOMU.WAD",
	"FREEDM.WAD",
	"CHEX.WAD",
	"CHEX3.WAD",
#endif
	NULL
};

//==========================================================================
//
// ScanIWAD
//
// Scan the contents of an IWAD to determine which one it is
//==========================================================================

static EIWADType ScanIWAD (const char *iwad)
{
	static const char checklumps[][8] =
	{
		"E1M1",
		"E4M2",
		"MAP01",
		"MAP40",
		"MAP60",
		"TITLE",
		"REDTNT2",
		"CAMO1",
		{ 'E','X','T','E','N','D','E','D'},
		"ENDSTRF",
		"MAP33",
		"INVCURS",
		{ 'F','R','E','E','D','O','O','M' },
		"W94_1",
		{ 'P','O','S','S','H','0','M','0' },
		"CYCLA1",
		"FLMBA1",
		"MAPINFO",
		{ 'G','A','M','E','I','N','F','O' },
		"E2M1","E2M2","E2M3","E2M4","E2M5","E2M6","E2M7","E2M8","E2M9",
		"E3M1","E3M2","E3M3","E3M4","E3M5","E3M6","E3M7","E3M8","E3M9",
		"DPHOOF","BFGGA0","HEADA1","CYBRA1",
		{ 'S','P','I','D','A','1','D','1' },

	};
#define NUM_CHECKLUMPS (sizeof(checklumps)/8)
	enum
	{
		Check_e1m1,
		Check_e4m1,
		Check_map01,
		Check_map40,
		Check_map60,
		Check_title,
		Check_redtnt2,
		Check_cam01,
		Check_Extended,
		Check_endstrf,
		Check_map33,
		Check_invcurs,
		Check_FreeDoom,
		Check_W94_1,
		Check_POSSH0M0,
		Check_Cycla1,
		Check_Flmba1,
		Check_Mapinfo,
		Check_Gameinfo,
		Check_e2m1
	};
	int lumpsfound[NUM_CHECKLUMPS];
	size_t i;
	wadinfo_t header;
	FILE *f;

	memset (lumpsfound, 0, sizeof(lumpsfound));
	if ( (f = fopen (iwad, "rb")) )
	{
		fread (&header, sizeof(header), 1, f);
		if (header.Magic == IWAD_ID || header.Magic == PWAD_ID)
		{
			header.NumLumps = LittleLong(header.NumLumps);
			if (0 == fseek (f, LittleLong(header.InfoTableOfs), SEEK_SET))
			{
				for (i = 0; i < (size_t)header.NumLumps; i++)
				{
					wadlump_t lump;
					size_t j;

					if (0 == fread (&lump, sizeof(lump), 1, f))
						break;
					for (j = 0; j < NUM_CHECKLUMPS; j++)
						if (strnicmp (lump.Name, checklumps[j], 8) == 0)
							lumpsfound[j]++;
				}
			}
		}

		fclose (f);
	}

	// Always check for custom iwads first.
#if 0
	if (lumpsfound[Check_Gameinfo])
	{
		return IWAD_Custom;
	}
#endif
	if (lumpsfound[Check_title] && lumpsfound[Check_map60])
	{
		return IWAD_HexenDK;
	}
	else if (lumpsfound[Check_map33] && lumpsfound[Check_endstrf])
	{
		if (lumpsfound[Check_map01])
		{
			return IWAD_Strife;
		}
		else if (lumpsfound[Check_invcurs])
		{
			return IWAD_StrifeTeaser2;
		}
		else
		{
			return IWAD_StrifeTeaser;
		}
	}
	else if (lumpsfound[Check_map01])
	{
		if (lumpsfound[Check_FreeDoom])
		{
			// Is there a 100% reliable way to tell FreeDoom and FreeDM
			// apart based solely on the lump names?
			if (strstr(iwad, "freedm.wad") || strstr(iwad, "FREEDM.WAD"))
			{
				return IWAD_FreeDM;
			}
			else
			{
				return IWAD_FreeDoom;
			}
		}
		else if (lumpsfound[Check_redtnt2])
		{
			return IWAD_Doom2TNT;
		}
		else if (lumpsfound[Check_cam01])
		{
			return IWAD_Doom2Plutonia;
		}
		else
		{
			if (lumpsfound[Check_title])
			{
				if (lumpsfound[Check_map40])
				{
					return IWAD_Hexen;
				}
				else
				{
					return IWAD_HexenDemo;
				}
			}
			else
			{
				return IWAD_Doom2;
			}
		}
	}
	else if (lumpsfound[Check_e1m1])
	{
		if (lumpsfound[Check_title])
		{
			if (!lumpsfound[Check_e2m1])
			{
				return IWAD_HereticShareware;
			}
			else
			{
				if (lumpsfound[Check_Extended])
				{
					return IWAD_HereticExtended;
				}
				else
				{
					return IWAD_Heretic;
				}
			}
		}
		else if (lumpsfound[Check_Cycla1] && lumpsfound[Check_Flmba1])
		{
			if (!lumpsfound[Check_Mapinfo])
			{
				// The original release won't work without its hacked custom EXE.
				//I_FatalError("Found an incompatible version of Chex Quest 3");
				return NUM_IWAD_TYPES;	// Can't use it.
			}
			return IWAD_ChexQuest3;
		}
		else
		{
			if (lumpsfound[Check_FreeDoom])
			{
				if (!lumpsfound[Check_e2m1])
				{
					return IWAD_FreeDoom1;
				}
				else
				{
					return IWAD_FreeDoomU;
				}
			}
			for (i = Check_e2m1; i < NUM_CHECKLUMPS; i++)
			{
				if (!lumpsfound[i])
				{
					return IWAD_DoomShareware;
				}
			}
			if (i == NUM_CHECKLUMPS)
			{
				if (lumpsfound[Check_e4m1])
				{
					if (lumpsfound[Check_W94_1] && lumpsfound[Check_POSSH0M0])
					{
						return IWAD_ChexQuest;
					}
					else
					{
						return IWAD_UltimateDoom;
					}
				}
				else
				{
					return IWAD_DoomRegistered;
				}
			}
		}
	}
	return NUM_IWAD_TYPES;	// Don't know
}

//==========================================================================
//
// CheckIWAD
//
// Tries to find an IWAD from a set of known IWAD names, and checks the
// contents of each one found to determine which game it belongs to.
// Returns the number of new wads found in this pass (does not count wads
// found from a previous call).
// 
//==========================================================================

static int CheckIWAD (const char *doomwaddir, WadStuff *wads)
{
	const char *slash;
	int i;
	int numfound;

	numfound = 0;

	slash = (doomwaddir[0] && doomwaddir[strlen (doomwaddir)-1] != '/') ? "/" : "";

	// Search for a pre-defined IWAD
	for (i = IWADNames[0] ? 0 : 1; IWADNames[i]; i++)
	{
		if (wads[i].Path.IsEmpty())
		{
			FString iwad;
			
			iwad.Format ("%s%s%s", doomwaddir, slash, IWADNames[i]);
			FixPathSeperator (iwad.LockBuffer());
			iwad.UnlockBuffer();
			if (FileExists (iwad))
			{
				wads[i].Type = ScanIWAD (iwad);
				if (wads[i].Type != NUM_IWAD_TYPES)
				{
					wads[i].Path = iwad;
					numfound++;
				}
			}
		}
	}

	return numfound;
}

//==========================================================================
//
//==========================================================================
//
// [RC] D_DoesDirectoryHaveIWADs
//
// Checks if a directory contains any IWADs.
// 
//==========================================================================

bool D_DoesDirectoryHaveIWADs( const char *pszPath )
{
	WadStuff wads[countof( IWADNames )];
	return ( CheckIWAD( pszPath, wads ) > 0 );
}
// IdentifyVersion
//
// Tries to find an IWAD in one of four directories under DOS or Win32:
//	  1. Current directory
//	  2. Executable directory
//	  3. $DOOMWADDIR
//	  4. $HOME
//
// Under UNIX OSes, the search path is:
//	  1. Current directory
//	  2. $DOOMWADDIR
//	  3. $HOME/.zdoom
//	  4. The share directory defined at compile time (/usr/local/share/zdoom)
//
// The search path can be altered by editing the IWADSearch.Directories
// section of the config file.
//
//==========================================================================

static EIWADType IdentifyVersion (const char *zdoom_wad)
{
	WadStuff wads[countof(IWADNames)];
	size_t foundwads[NUM_IWAD_TYPES] = { 0 };
	const char *iwadparm = Args->CheckValue ("-iwad");
	size_t numwads;
	int pickwad;
	size_t i;
	bool iwadparmfound = false;
	FString custwad;

	if (iwadparm)
	{
		custwad = iwadparm;
		FixPathSeperator (custwad.LockBuffer());
		if (CheckIWAD (custwad, wads))
		{ // -iwad parameter was a directory
			iwadparm = NULL;
		}
		else
		{
			DefaultExtension (custwad, ".wad");
			iwadparm = custwad;
			IWADNames[0] = iwadparm;
			CheckIWAD ("", wads);
		}
	}

	if (iwadparm == NULL || wads[0].Path.IsEmpty())
	{
		if (GameConfig->SetSection ("IWADSearch.Directories"))
		{
			const char *key;
			const char *value;

			while (GameConfig->NextInSection (key, value))
			{
				if (stricmp (key, "Path") == 0)
				{
					FString nice = NicePath(value);
					FixPathSeperator(nice.LockBuffer());
					nice.UnlockBuffer();
					CheckIWAD(nice, wads);
				}
			}
		}
#ifdef _WIN32
		FString steam_path = I_GetSteamPath();
		if (steam_path.IsNotEmpty())
		{
			static const char *const steam_dirs[] =
			{
				"doom 2/base",
				"final doom/base",
				"heretic shadow of the serpent riders/base",
				"hexen/base",
				"hexen deathkings of the dark citadel/base",
				"ultimate doom/base"
			};
			steam_path += "/SteamApps/common/";
			for (i = 0; i < countof(steam_dirs); ++i)
			{
				CheckIWAD (steam_path + steam_dirs[i], wads);
			}
		}
#endif
	}

	if (iwadparm != NULL && !wads[0].Path.IsEmpty())
	{
		iwadparmfound = true;
	}

	for (i = numwads = 0; i < countof(IWADNames); i++)
	{
		if (!wads[i].Path.IsEmpty())
		{
			if (i != numwads)
			{
				wads[numwads] = wads[i];
			}
			foundwads[wads[numwads].Type] = numwads + 1;
			numwads++;
		}
	}

	if (foundwads[IWAD_HexenDK] && !foundwads[IWAD_Hexen])
	{ // Cannot play Hexen DK without Hexen
		size_t kill = foundwads[IWAD_HexenDK];
		for (i = kill; i < numwads; ++i)
		{
			wads[i - 1] = wads[i];
		}
		numwads--;
		foundwads[IWAD_HexenDK] = 0;
		for (i = 0; i < NUM_IWAD_TYPES; ++i)
		{
			if (foundwads[i] > kill)
			{
				foundwads[i]--;
			}
		}
	}

	if (numwads == 0)
// [BB] Skulltag uses Rivecoder's IWAD setup screen now (only available under Windows).
#ifdef _WIN32
		throw CNoIWADError(); // [RC]
#else
	{
		I_FatalError ("Cannot find a game IWAD (doom.wad, doom2.wad, heretic.wad, etc.).\n"
					  "Did you install "GAMENAME" properly? You can do either of the following:\n"
					  "\n"
					  "1. Place one or more of these wads in the same directory as "GAMENAME".\n"
					  "2. Edit your "GAMENAMELOWERCASE"-username.ini and add the directories of your iwads\n"
					  "to the list beneath [IWADSearch.Directories]");
	}
#endif

	pickwad = 0;

	if (!iwadparmfound && numwads > 1)
	{
		int defiwad = 0;

		// Locate the user's prefered IWAD, if it was found.
		if (defaultiwad[0] != '\0')
		{
			for (i = 0; i < numwads; ++i)
			{
				FString basename = ExtractFileBase (wads[i].Path);
				if (stricmp (basename, defaultiwad) == 0)
				{
					defiwad = (int)i;
					break;
				}
			}
		}
		pickwad = I_PickIWad (wads, (int)numwads, queryiwad, defiwad);
		if (pickwad >= 0)
		{
			// The newly selected IWAD becomes the new default
			FString basename = ExtractFileBase (wads[pickwad].Path);
			defaultiwad = basename;
		}
	}

	if (pickwad < 0)
		exit (0);

	// zdoom.pk3 must always be the first file loaded and the IWAD second.
	D_AddFile (zdoom_wad);

	if (wads[pickwad].Type == IWAD_HexenDK)
	{ // load hexen.wad before loading hexdd.wad
		D_AddFile (wads[foundwads[IWAD_Hexen]-1].Path);
	}

	D_AddFile (wads[pickwad].Path);

	if (wads[pickwad].Type == IWAD_Strife)
	{ // Try to load voices.wad along with strife1.wad
		long lastslash = wads[pickwad].Path.LastIndexOf ('/');
		FString path;

		if (lastslash == -1)
		{
			path = "";//  wads[pickwad].Path;
		}
		else
		{
			path = FString (wads[pickwad].Path.GetChars(), lastslash + 1);
		}
		path += "voices.wad";
		D_AddFile (path);
	}

	return wads[pickwad].Type;
}


const IWADInfo *D_FindIWAD(const char *basewad)
{
	gameiwad = IdentifyVersion(basewad);
	const IWADInfo *iwad_info = &IWADInfos[gameiwad];
	I_SetIWADInfo(iwad_info);
	return iwad_info;
}
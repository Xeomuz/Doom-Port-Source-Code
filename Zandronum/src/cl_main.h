//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2003 Brad Carney
// Copyright (C) 2007-2012 Skulltag Development Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the Skulltag Development Team nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 4. Redistributions in any form must be accompanied by information on how to
//    obtain complete source code for the software and any accompanying
//    software that uses the software. The source code must either be included
//    in the distribution or be available for no more than the cost of
//    distribution plus a nominal fee, and must be freely redistributable
//    under reasonable conditions. For an executable file, complete source
//    code means the source code for all modules it contains. It does not
//    include source code for modules or files that typically accompany the
//    major components of the operating system on which the executable file
//    runs.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Date created:  2/3/03
//
//
// Filename: cl_main.h
//
// Description: 
//
//-----------------------------------------------------------------------------

#ifndef __CL_MAIN_H__
#define __CL_MAIN_H__

#include "i_net.h"
#include "d_ticcmd.h"
#include "sv_main.h"

//*****************************************************************************
//	DEFINES

#define	CONNECTION_RESEND_TIME		( 3 * TICRATE )
#define	GAMESTATE_RESEND_TIME		( 3 * TICRATE )

// For prediction (?)
#define MAXSAVETICS					70

// Display "couldn't find thing" messages.
//#define	CLIENT_WARNING_MESSAGES

//*****************************************************************************
typedef enum 
{
	// Full screen console with no connection.
	CTS_DISCONNECTED,

	// We are currently attempting to connect to the server.
	CTS_ATTEMPTINGCONNECTION,

	// We've gotten a response from the server, and are now attempting to authenticate
	// the level.
	CTS_ATTEMPTINGAUTHENTICATION,

	// We've successfully authenticated the level and loaded the level. Now we're waiting
	// for a snapshot.
	CTS_REQUESTINGSNAPSHOT,
	
	// We're currently receiving a snapshot of the level.
	CTS_RECEIVINGSNAPSHOT,

    // Snapshot is finished! Everything is done, fully in the level.
	CTS_ACTIVE,

} CONNECTIONSTATE_e;

//*****************************************************************************
//	STRUCTURES

typedef struct
{
	// This array of bytes is the storage for the packet data.
	BYTE	abData[MAX_UDP_PACKET * PACKET_BUFFER_SIZE];

	// This is the number of bytes in paData.
	LONG	lMaxSize;

} PACKETBUFFER_s;

//*****************************************************************************
//	PROTOTYPES

// Standard API.
void				CLIENT_Construct( void );
void				CLIENT_Destruct( void );
void				CLIENT_Tick( void );
void				CLIENT_EndTick( void );

// Access functions.
CONNECTIONSTATE_e	CLIENT_GetConnectionState( void );
void				CLIENT_SetConnectionState( CONNECTIONSTATE_e State );
NETBUFFER_s			*CLIENT_GetLocalBuffer( void );
void				CLIENT_SetLocalBuffer( NETBUFFER_s *pBuffer );
ULONG				CLIENT_GetLastServerTick( void );
void				CLIENT_SetLastServerTick( ULONG ulTick );
ULONG				CLIENT_GetLastConsolePlayerUpdateTick( void );
void				CLIENT_SetLastConsolePlayerUpdateTick( ULONG ulTick );
bool				CLIENT_GetServerLagging( void );
void				CLIENT_SetServerLagging( bool bLagging );
bool				CLIENT_GetClientLagging( void );
void				CLIENT_SetClientLagging( bool bLagging );
NETADDRESS_s		CLIENT_GetServerAddress( void );
void				CLIENT_SetServerAddress( NETADDRESS_s Address );
bool				CLIENT_GetAllowSendingOfUserInfo( void );
void				CLIENT_SetAllowSendingOfUserInfo( bool bAllow );

// Functions necessary to carry out client-side operations.
void				CLIENT_SendServerPacket( void );
void				CLIENT_AttemptConnection( void );
void				CLIENT_AttemptAuthentication( char *pszMapName );
void				CLIENT_RequestSnapshot( void );
bool				CLIENT_GetNextPacket( void );
void				CLIENT_GetPackets( void );
void				CLIENT_CheckForMissingPackets( void );
bool				CLIENT_ReadPacketHeader( BYTESTREAM_s *pByteStream );
void				CLIENT_ParsePacket( BYTESTREAM_s *pByteStream, bool bSequencedPacket );
void				CLIENT_ProcessCommand( LONG lCommand, BYTESTREAM_s *pByteStream );
#ifdef _DEBUG
void				CLIENT_PrintCommand( LONG lCommand );
#endif
void				CLIENT_QuitNetworkGame( const char *pszError );
void				CLIENT_SendCmd( void );
void				CLIENT_WaitForServer( void );

// Support functions to make things work more smoothly.
void				CLIENT_AuthenticateLevel( const char *pszMapName );
AActor				*CLIENT_SpawnThing( const PClass *pType, fixed_t X, fixed_t Y, fixed_t Z, LONG lNetID );
void				CLIENT_SpawnMissile( const PClass *pType, fixed_t X, fixed_t Y, fixed_t Z, fixed_t MomX, fixed_t MomY, fixed_t MomZ, LONG lNetID, LONG lTargetNetID );
void				CLIENT_MoveThing( AActor *pActor, fixed_t X, fixed_t Y, fixed_t Z );
AActor				*CLIENT_FindThingByNetID( LONG lID );
void				CLIENT_DisplayMOTD( void );
void				CLIENT_RestoreSpecialPosition( AActor *pActor );
void				CLIENT_RestoreSpecialDoomThing( AActor *pActor, bool bFog );
AInventory			*CLIENT_FindPlayerInventory( ULONG ulPlayer, const PClass *pType );
AInventory			*CLIENT_FindPlayerInventory( ULONG ulPlayer, const char *pszName );
//void				CLIENT_RemoveMonsterCorpses( void );
sector_t			*CLIENT_FindSectorByID( ULONG ulID );
bool				CLIENT_IsParsingPacket( void );
void				CLIENT_ResetConsolePlayerCamera( void );
void				CLIENT_ResetPlayerData( player_t *pPlayer );
LONG				CLIENT_AdjustDoorDirection( LONG lDirection );
LONG				CLIENT_AdjustFloorDirection( LONG lDirection );
LONG				CLIENT_AdjustCeilingDirection( LONG lDirection );
LONG				CLIENT_AdjustElevatorDirection( LONG lDirection );
void				CLIENT_LogHUDMessage( char *pszString, LONG lColor );
void				CLIENT_UpdatePendingWeapon( const player_t *pPlayer );
void				CLIENT_ClearAllPlayers( void );
void				CLIENT_LimitProtectedCVARs( void );
bool				CLIENT_CanClipMovement( AActor *pActor );

void				CLIENT_PREDICT_Construct( void );
void				CLIENT_PREDICT_PlayerPredict( void );
void				CLIENT_PREDICT_SaveCmd( void );
void				CLIENT_PREDICT_PlayerTeleported( void );
bool				CLIENT_PREDICT_IsPredicting( void );

//*****************************************************************************
//	EXTERNAL CONSOLE VARIABLES

EXTERN_CVAR( Bool, cl_predict_players )
//EXTERN_CVAR( Int, cl_maxmonstercorpses )
EXTERN_CVAR( Float, cl_motdtime )
EXTERN_CVAR( Bool, cl_taunts )
EXTERN_CVAR( Int, cl_showcommands )
EXTERN_CVAR( Int, cl_showspawnnames )
EXTERN_CVAR( Bool, cl_startasspectator )
EXTERN_CVAR( Bool, cl_dontrestorefrags )
EXTERN_CVAR( String, cl_password )
EXTERN_CVAR( String, cl_joinpassword )
EXTERN_CVAR( Bool, cl_hitscandecalhack )

// Not in cl_main.cpp, but this seems like a good enough place for it.
EXTERN_CVAR( Int, cl_skins )

#endif // __CL_MAIN__
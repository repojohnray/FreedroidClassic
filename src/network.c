/* 
 *
 *   Copyright (c) 1994, 2002 Johannes Prix
 *   Copyright (c) 1994, 2002 Reinhard Prix
 *
 *
 *  This file is part of Freedroid
 *
 *  Freedroid is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Freedroid is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Freedroid; see the file COPYING. If not, write to the 
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *  MA  02111-1307  USA
 *
 */
/* ----------------------------------------------------------------------
 * This file contains all functions for the networking client-server
 * interaction, preparation, initialisation and interpretation back into
 * the classical data structures.
 * ---------------------------------------------------------------------- */
/*
 * This file has been checked for remains of german comments in the code
 * I you still find some, please just kill it mercilessly.
 */
#define _network_c

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"

#include "SDL_net.h"

#define FREEDROID_NET_PORT (5676)

Uint16 port = FREEDROID_NET_PORT;
Uint32 ipaddr;
IPaddress ip,*remoteip; // this is for the server (and for the client?)
// TCPsocket client; // this is for the server
TCPsocket TheServerSocket; // this is for the server only
TCPsocket sock; // this is for the client 
int len;

//--------------------
// Now we define a short message, that is designed to
// be sent from the server to the client, best periodically,
// no matter whether whether there was some change or not.
//
typedef struct
{
  int status;			/* attacking, defense, dead, ... */
  finepoint speed;		/* the current speed of the druid */
  gps pos;		        /* current position in the whole ship */
  double health;		/* the max. possible energy in the moment */
  double energy;		/* current energy level */
  double mana;                  // current mana level 
}
player_engram, *Player_Engram;

player_engram PlayerEngram [ MAX_PLAYERS ] ;

//--------------------
// Now we define a short message, that is designed to
// be sent from the server to the client, best periodically,
// no matter whether whether there was some change or not.
//
typedef struct
{
  int EnemyIndex; // this is an information not included in the original data!!!
                  // but it's required, so that the client knows what to do with
                  // this enemy.

  int type;			// the number of the droid specifications in Druidmap 
  int Status;			// current status like OUT=TERMINATED or not OUT
  gps pos;		        // coordinates of the current position in the level
  finepoint speed;		// current speed  
  double energy;		// current energy of this droid
  double feindphase;		// current phase of rotation of this droid
  char friendly;

}
enemy_engram, *Enemy_Engram;

enemy_engram EnemyEngram [ MAX_ENEMYS_ON_SHIP ] ;

//--------------------
// Now we define a command buffer.
// This buffer is intended to contain a full command, meaning a
// command indication area and a command data area.
// Command indication area always comes first.
//
#define COMMAND_BUFFER_MAXLEN 500000
typedef struct
{
  int command_code ;
  int data_chunk_length ;
  char command_data_buffer [ COMMAND_BUFFER_MAXLEN ] ;
}
network_command, *Network_Command;

network_command CommandFromPlayer [ MAX_PLAYERS ];
network_command CommandFromServer [ 1 ];

//--------------------
// Now we define all the known server and client commands
//
enum
  { 

    PLAYER_SERVER_ERROR = 0,

    PLAYER_TELL_ME_YOUR_NAME ,
    SERVER_THIS_IS_MY_NAME ,

    PLAYER_SERVER_TAKE_THIS_TEXT_MESSAGE ,

    PLAYER_ACCEPT_ALL_ME_UPDATES ,
    PLAYER_ACCEPT_PLAYER_ENGRAM ,

    PLAYER_ACCEPT_FULL_ENEMY_ENGRAM ,
    PLAYER_ACCEPT_UPDATE_ENEMY_ENGRAM ,

    PLAYER_DELETE_ALL_YOUR_ENEMYS,

    SERVER_ACCEPT_THIS_KEYBOARD_EVENT ,
    NUMBER_OF_KNOWN_COMMANDS

  };

#define MAX_KEYCODES (10000)

typedef struct
{
  TCPsocket ThisPlayersSocketAtTheServer; // this is for the server too
  int command_buffer_fill_status;
  int network_status;
  unsigned char server_keyboard [ MAX_KEYCODES ] ;
}
remote_player, *Remote_Player;

remote_player AllPlayers[ MAX_PLAYERS ] = {
  { NULL, 0 } ,
  { NULL, 0 } ,
  { NULL, 0 } ,
  { NULL, 0 } ,
  { NULL, 0 } 
};

int server_buffer_fill_status = 0 ;

//--------------------
// This is the set of at most MAX_PLAYERS socket, that the server has.
// Any client socket at the server is added to this set.
// That way we can easily query if there was some data sent from one
// of the clients or not.
//
SDLNet_SocketSet The_Set_Of_All_Client_Sockets;

//--------------------
// This is the one socket, that the client has and that connets him to
// the server.  In order to correctly query if there war activity in this
// socket or not, we put it into it's own set.  This set then has only
// one element, never more, but that's not a problem, is it?
//
SDLNet_SocketSet The_Set_Of_The_One_Server_Socket;

void SendPlayerNameToServer ( void );

/* ----------------------------------------------------------------------
 * This function initializes the networking libraty SDL_net.  Without or
 * before this initialisation, no networking operations should be 
 * attempted.
 * ---------------------------------------------------------------------- */
void 
Init_Network(void)
{
  int PlayerNum; // and index for the clinet sockets.
  int i;

  //--------------------
  // This code was taken from the SDL_Net manual and then
  // slightly modified.
  //
  if ( SDLNet_Init() == (-1) ) 
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET SUBSYSTEM COULD NOT BE INITIALIZED.\n\
\n\
The reason for this as reportet by the SDL Net is as follows:\n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the network code problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) );
      Terminate( ERR );
    }
  else
    {
      DebugPrintf( 0 , "\n--------------------\nInitialisation of SDL_net subsystem successful...\n--------------------\n" );
    }

  //--------------------
  // Next we make sure the clinet sockets at the server are empty.  This is
  // for the server, so he knows, that there are no active sockets
  // from some clients or something.
  //
  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer = NULL ; 
      AllPlayers [ PlayerNum ] . network_status = UNCONNECTED ; 
      for ( i = 0 ; i < MAX_KEYCODES ; i ++ )
	AllPlayers [ PlayerNum ] . server_keyboard [ i ] = FALSE ;
    }

  //--------------------
  // We will later want to read out only those client sockets, that really contain
  // some data.  Therefore we must create a socket set, which will allow
  // to query if there is some data there or not.
  //
  The_Set_Of_All_Client_Sockets = SDLNet_AllocSocketSet ( MAX_PLAYERS ) ;

  if( ! The_Set_Of_All_Client_Sockets ) 
    {
      printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
      Terminate ( ERR );
    }
  else
    {
      DebugPrintf ( 0 , "\nSET OF CLIENT SOCKETS ESTABLISHED SUCCESSFULLY!!!\n" );
    }

  //--------------------
  // We will later want to read out the server information from a socket,
  // but only if there is some data of course.  Therefore we must create 
  // a socket set, which will allow us to query if the one socket to the
  // server contains some data or not.
  //
  The_Set_Of_The_One_Server_Socket = SDLNet_AllocSocketSet ( 1 ) ;

  if( ! The_Set_Of_The_One_Server_Socket ) 
    {
      printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
      Terminate ( ERR );
    }
  else
    {
      DebugPrintf ( 0 , "\nSET OF THE ONE SERVER SOCKETS ESTABLISHED SUCCESSFULLY!!!\n" );
    }


}; // void Init_Network(void)

/* ----------------------------------------------------------------------
 * This function reports if the server thinks that the cursor right key
 * was pressed or not.  If not in server mode, this function returns just
 * the local keyboard status.
 * ---------------------------------------------------------------------- */
int
ServerThinksRightPressed ( int PlayerNum )
{

  if ( ServerMode ) 
    {
      return ( AllPlayers [ PlayerNum ] . server_keyboard [ SDLK_RIGHT ] );
    }
  else 
    {
      if ( PlayerNum == 0 )
	return ( RightPressed ( ) ) ;
      else
	return ( FALSE );
    }

}; // int ServerThinksRightPressed ( int Playernum )


/* ----------------------------------------------------------------------
 * This function reports if the server thinks that the cursor left key
 * was pressed or not.  If not in server mode, this function returns just
 * the local keyboard status.
 * ---------------------------------------------------------------------- */
int
ServerThinksLeftPressed ( int PlayerNum )
{

  if ( ServerMode ) 
    {
      return ( AllPlayers [ PlayerNum ] . server_keyboard [ SDLK_LEFT ] );
    }
  else 
    {
      if ( PlayerNum == 0 )
	return ( LeftPressed ( ) ) ;
      else
	return ( FALSE );
    }

}; // int ServerThinksLeftPressed ( int Playernum )


/* ----------------------------------------------------------------------
 * This function reports if the server thinks that the cursor up key
 * was pressed or not.  If not in server mode, this function returns just
 * the local keyboard status.
 * ---------------------------------------------------------------------- */
int
ServerThinksUpPressed ( int PlayerNum )
{

  if ( ServerMode ) 
    {
      return ( AllPlayers [ PlayerNum ] . server_keyboard [ SDLK_UP ] );
    }
  else 
    {
      if ( PlayerNum == 0 )
	return ( UpPressed ( ) ) ;
      else
	return ( FALSE );
    }

}; // int ServerThinksUpPressed ( int Playernum )


/* ----------------------------------------------------------------------
 * This function reports if the server thinks that the cursor down key
 * was pressed or not.  If not in server mode, this function returns just
 * the local keyboard status.
 * ---------------------------------------------------------------------- */
int
ServerThinksDownPressed ( int PlayerNum )
{

  if ( ServerMode ) 
    {
      return ( AllPlayers [ PlayerNum ] . server_keyboard [ SDLK_DOWN ] );
    }
  else 
    {
      if ( PlayerNum == 0 )
	return ( DownPressed ( ) ) ;
      else
	return ( FALSE );
    }

}; // int ServerThinksDownPressed ( int Playernum )


/* ----------------------------------------------------------------------
 * This function creates a copy of the Me structure, that is reduced to
 * those parts, that the client must know.
 * ---------------------------------------------------------------------- */
void 
CopyMeToNetworkMe( int PlayerNum )
{
  int i;
  int WriteIndex=0;

  //--------------------
  // At first we copy this particular player to position 0 within
  // the networking me.
  //
  memcpy ( & NetworkMe[ WriteIndex ] , & Me [ PlayerNum ] , sizeof ( NetworkMe [ WriteIndex ] ) );
  WriteIndex ++;

  //--------------------
  // Now we copy the other existing or nonexisting players into the
  // networking me, but only in file after the zero-entry.
  //
  for ( i = 0 ; i < MAX_PLAYERS ; i ++ )
    {
      if ( PlayerNum != i )
	{
	  memcpy ( & NetworkMe[ WriteIndex ] , & Me [ i ] , sizeof ( NetworkMe [ WriteIndex ] ) );
	  WriteIndex ++;
	}
    }
}; // void CopyMeToNetworkMe( void )

/* ----------------------------------------------------------------------
 * This function prepares a player engram for player number PlayerNum.
 * ---------------------------------------------------------------------- */
void
PreparePlayerEngramForPlayer ( PlayerNum ) 
{
  int WriteIndex = 0 ;
  int i;

  PlayerEngram [ 0 ] . status    = Me [ PlayerNum ] . status ;
  PlayerEngram [ 0 ] . speed . x = Me [ PlayerNum ] . speed . x ;
  PlayerEngram [ 0 ] . speed . y = Me [ PlayerNum ] . speed . y ;
  PlayerEngram [ 0 ] . pos . x   = Me [ PlayerNum ] . pos . x ;
  PlayerEngram [ 0 ] . pos . y   = Me [ PlayerNum ] . pos . y ;
  PlayerEngram [ 0 ] . pos . z   = Me [ PlayerNum ] . pos . z ;
  PlayerEngram [ 0 ] . health    = Me [ PlayerNum ] . health ;
  PlayerEngram [ 0 ] . energy    = Me [ PlayerNum ] . energy ;
  PlayerEngram [ 0 ] . mana      = Me [ PlayerNum ] . mana ;

  WriteIndex ++;

  //--------------------
  // Now we copy the other existing or nonexisting players into the
  // networking me, but only in file after the zero-entry.
  //
  for ( i = 0 ; i < MAX_PLAYERS ; i ++ )
    {
      if ( PlayerNum != i )
	{
	  
	  PlayerEngram [ WriteIndex ] . status    = Me [ i ] . status ;
	  PlayerEngram [ WriteIndex ] . speed . x = Me [ i ] . speed . x ;
	  PlayerEngram [ WriteIndex ] . speed . y = Me [ i ] . speed . y ;
	  PlayerEngram [ WriteIndex ] . pos . x   = Me [ i ] . pos . x ;
	  PlayerEngram [ WriteIndex ] . pos . y   = Me [ i ] . pos . y ;
	  PlayerEngram [ WriteIndex ] . pos . z   = Me [ i ] . pos . z ;
	  PlayerEngram [ WriteIndex ] . health    = Me [ i ] . health ;
	  PlayerEngram [ WriteIndex ] . energy    = Me [ i ] . energy ;
	  PlayerEngram [ WriteIndex ] . mana      = Me [ i ] . mana ;

	  WriteIndex ++;

	}
    }
  
}; // void PreparePlayerEngramForPlayer ( PlayerNum ) 

/* ----------------------------------------------------------------------
 * This function fills data from AllEnemys into the enemy engram.
 * ---------------------------------------------------------------------- */
void
FillDataIntoEnemyEngram ( int WriteIndex , int EnemyIndex )
{

  EnemyEngram [ WriteIndex ] . EnemyIndex = EnemyIndex ;
  EnemyEngram [ WriteIndex ] . type       = AllEnemys [ EnemyIndex ] . type ;
  EnemyEngram [ WriteIndex ] . Status     = AllEnemys [ EnemyIndex ] . Status ;
  EnemyEngram [ WriteIndex ] . speed . x  = AllEnemys [ EnemyIndex ] . speed . x ;
  EnemyEngram [ WriteIndex ] . speed . y  = AllEnemys [ EnemyIndex ] . speed . y ;
  EnemyEngram [ WriteIndex ] . pos . x    = AllEnemys [ EnemyIndex ] . pos . x ;
  EnemyEngram [ WriteIndex ] . pos . y    = AllEnemys [ EnemyIndex ] . pos . y ;
  EnemyEngram [ WriteIndex ] . pos . z    = AllEnemys [ EnemyIndex ] . pos . z ;
  EnemyEngram [ WriteIndex ] . energy     = AllEnemys [ EnemyIndex ] . energy ;
  EnemyEngram [ WriteIndex ] . feindphase = AllEnemys [ EnemyIndex ] . feindphase ;
  EnemyEngram [ WriteIndex ] . friendly   = AllEnemys [ EnemyIndex ] . Friendly ;
  
}; // void FillDataIntoEnemyEngram ( int WriteIndex , int EnemyIndex )

/* ----------------------------------------------------------------------
 * This function prepares a player engram for player number PlayerNum.
 * ---------------------------------------------------------------------- */
void
PrepareFullEnemysEngram ( void ) 
{
  int i;

  //--------------------
  // Now we copy the existing information about the enemys
  // into the short engram.
  //
  for ( i = 0 ; i < MAX_ENEMYS_ON_SHIP ; i ++ )
    {
      FillDataIntoEnemyEngram ( i , i ) ;
    }
  
}; // void PrepareFullEnemysEngram ( void ) 


/* ----------------------------------------------------------------------
 * This function prepares a player engram for player number PlayerNum.
 * ---------------------------------------------------------------------- */
void
EnforceServersPlayerEngram ( void ) 
{
  int PlayerNum;

  DebugPrintf ( 0 , "\nvoid EnforceServersAllMeUpdate ( void ) : real function call confirmed. " );

  memcpy ( PlayerEngram , CommandFromServer [ 0 ] . command_data_buffer , sizeof ( PlayerEngram ) );

  DebugPrintf ( 0 , "\nPlayerEngram[0].energy : %f." , PlayerEngram [ 0 ] . energy );
  DebugPrintf ( 0 , "\nPlayerEngram[0].status : %d." , PlayerEngram [ 0 ] . status );

  //--------------------
  // Now we copy the players information of the engram to the
  // Me structures.
  //
  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
	  
      Me [ PlayerNum ] . status    = PlayerEngram [ PlayerNum ] . status    ;
      Me [ PlayerNum ] . speed . x = PlayerEngram [ PlayerNum ] . speed . x ;
      Me [ PlayerNum ] . speed . y = PlayerEngram [ PlayerNum ] . speed . y ;
      Me [ PlayerNum ] . pos . x   = PlayerEngram [ PlayerNum ] . pos . x   ;
      Me [ PlayerNum ] . pos . y   = PlayerEngram [ PlayerNum ] . pos . y   ;
      Me [ PlayerNum ] . pos . z   = PlayerEngram [ PlayerNum ] . pos . z   ;
      Me [ PlayerNum ] . health    = PlayerEngram [ PlayerNum ] . health    ;
      Me [ PlayerNum ] . energy    = PlayerEngram [ PlayerNum ] . energy    ;
      Me [ PlayerNum ] . mana      = PlayerEngram [ PlayerNum ] . mana      ;


    }

  DebugPrintf ( 0 , "\nvoid EnforceServersPlayerEngram ( void ) : end of function reached. " );
  
}; // void EnforceServersPlayerEngram ( void ) 

/* ----------------------------------------------------------------------
 * This function prepares a player engram for player number PlayerNum.
 * ---------------------------------------------------------------------- */
void
EnforceServersFullEnemysEngram ( void ) 
{
  int i ;

  DebugPrintf ( 0 , "\nvoid EnforceServersFullEnemysEngram ( void ) : real function call confirmed. " );

  memcpy ( EnemyEngram , CommandFromServer [ 0 ] . command_data_buffer , sizeof ( EnemyEngram ) );

  //--------------------
  // Now we copy the existing information about the enemys
  // into the short engram.
  //
  for ( i = 0 ; i < MAX_ENEMYS_ON_SHIP ; i ++ )
    {
	  
      // AllEnemys [ i ] . EnemyIndex = i ;
      AllEnemys [ i ] . type       = EnemyEngram [ i ] . type ;
      AllEnemys [ i ] . Status     = EnemyEngram [ i ] . Status ;
      AllEnemys [ i ] . speed . x  = EnemyEngram [ i ] . speed . x ;
      AllEnemys [ i ] . speed . y  = EnemyEngram [ i ] . speed . y ;
      AllEnemys [ i ] . pos . x    = EnemyEngram [ i ] . pos . x ;
      AllEnemys [ i ] . pos . y    = EnemyEngram [ i ] . pos . y ;
      AllEnemys [ i ] . pos . z    = EnemyEngram [ i ] . pos . z ;
      AllEnemys [ i ] . energy     = EnemyEngram [ i ] . energy ;
      AllEnemys [ i ] . feindphase = EnemyEngram [ i ] . feindphase ;
      AllEnemys [ i ] . Friendly   = EnemyEngram [ i ] . friendly ;

    }

  DebugPrintf ( 0 , "\nvoid EnforceServersFullEnemysEngram ( void ) : end of function reached. " );
  
}; // void EnforceServersFullEnemysEngram ( void ) 

/* ----------------------------------------------------------------------
 * This function prepares a player engram for player number PlayerNum.
 * ---------------------------------------------------------------------- */
void
EnforceServersUpdateEnemysEngram ( int NumberOfTargets ) 
{
  int i ;
  int WriteIndex;

  DebugPrintf ( 0 , "\nvoid EnforceServersUpdateEnemysEngram ( void ) : real function call confirmed. " );
  DebugPrintf ( 0 , "\nvoid EnforceServersUpdateEnemysEngram ( void ) : %d enemys to update. " , NumberOfTargets );

  memcpy ( EnemyEngram , CommandFromServer [ 0 ] . command_data_buffer , sizeof ( EnemyEngram ) );

  //--------------------
  // Now we copy the existing information about the enemys
  // into the short engram.
  //
  for ( i = 0 ; i < NumberOfTargets ; i ++ )
    {
	  
      WriteIndex = EnemyEngram [ i ] . EnemyIndex ;
	
      AllEnemys [ WriteIndex ] . type       = EnemyEngram [ i ] . type ;
      AllEnemys [ WriteIndex ] . Status     = EnemyEngram [ i ] . Status ;
      AllEnemys [ WriteIndex ] . speed . x  = EnemyEngram [ i ] . speed . x ;
      AllEnemys [ WriteIndex ] . speed . y  = EnemyEngram [ i ] . speed . y ;
      AllEnemys [ WriteIndex ] . pos . x    = EnemyEngram [ i ] . pos . x ;
      AllEnemys [ WriteIndex ] . pos . y    = EnemyEngram [ i ] . pos . y ;
      AllEnemys [ WriteIndex ] . pos . z    = EnemyEngram [ i ] . pos . z ;
      AllEnemys [ WriteIndex ] . energy     = EnemyEngram [ i ] . energy ;
      AllEnemys [ WriteIndex ] . feindphase = EnemyEngram [ i ] . feindphase ;
      AllEnemys [ WriteIndex ] . Friendly   = EnemyEngram [ i ] . friendly ;

    }

  DebugPrintf ( 0 , "\nvoid EnforceServersUpdateEnemysEngram ( void ) : end of function reached. " );
  
}; // void EnforceServersUpdateEnemysEngram ( void ) 

/* ----------------------------------------------------------------------
 * This function creates a copy of the Me structure, that is reduced to
 * those parts, that the client must know.
 * ---------------------------------------------------------------------- */
void 
CopyNetworkMeToMe( void )
{
  int PlayerNum;

  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      memcpy ( & Me[ PlayerNum ] , & NetworkMe [ PlayerNum ] , sizeof ( NetworkMe [ PlayerNum ] ) );
    }

}; // void CopyMeToNetworkMe( void )

/* ----------------------------------------------------------------------
 * This function creates a copy of the AllEnemys array, that is reduced to
 * those parts, that the client must know.
 * ---------------------------------------------------------------------- */
void 
CopyAllEnemysToNetworkAllEnemys ( void )
{
  int EnemyNum;

  for ( EnemyNum = 0 ; EnemyNum < MAX_ENEMYS_ON_SHIP ; EnemyNum ++ )
    {
      memcpy ( & NetworkAllEnemys[ EnemyNum ] , & AllEnemys [ EnemyNum ] , sizeof ( NetworkAllEnemys [ EnemyNum ] ) );
    }

}; // void CopyAllEnemysToNetworkAllEnemys ( void )

/* ----------------------------------------------------------------------
 * This function creates a copy of the Me structure, that is reduced to
 * those parts, that the client must know.
 * ---------------------------------------------------------------------- */
void 
CopyNetworkAllEnemysToAllEnemys ( void )
{
  int EnemyNum;

  for ( EnemyNum = 0 ; EnemyNum <  MAX_ENEMYS_ON_SHIP ; EnemyNum ++ )
    {
      memcpy ( & AllEnemys[ EnemyNum ] , & NetworkAllEnemys [ EnemyNum ] , sizeof ( NetworkAllEnemys [ EnemyNum ] ) );
    }
}; // void CopyNetworkAllEnemysToAllEnemys ( void )

/* ----------------------------------------------------------------------
 * This function assembles the information the server wants to send to
 * the client
 * ---------------------------------------------------------------------- */
void
AssembleServerInformationForClient ( void )
{
  
};

/* ----------------------------------------------------------------------
 *
 *
 * ---------------------------------------------------------------------- */
void
CheckForClientsConnectingToMe ( void )
{
  
}; // void CheckForClientsConnectingToMe ( void )

/* ----------------------------------------------------------------------
 * This function sends a text message to a client in command form.
 * ---------------------------------------------------------------------- */
void
SendTextMessageToClient ( int PlayerNum , char* message )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending text message to client in command form.  \nMessage is : %s\n" , message ) ;
  len = strlen ( message ) + 1 ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long message to server... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_SERVER_TAKE_THIS_TEXT_MESSAGE ;
  LocalCommandBuffer . data_chunk_length = len ;
  strcpy ( LocalCommandBuffer . command_data_buffer , message );

  

  CommunicationResult = SDLNet_TCP_Send ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending TCP message returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A TEXT MESSAGE TO THE CLIENT SUCCESSFULLY\n\
in the function void SendTextMessageToClient ( int PlayerNum , char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendTextMessageToClient ( int PlayerNum , char* message )


/* ----------------------------------------------------------------------
 * This function sends a text message to a client in command form.
 * ---------------------------------------------------------------------- */
void
SendFullMeUpdateToClient ( int PlayerNum )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending full me update to client in command form.\n" ) ;
  len = sizeof ( NetworkMe ) ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long full NetworkMe update to client... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // We prepare the NetworkMe we want to send...
  //
  CopyMeToNetworkMe ( PlayerNum ) ;

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_ACCEPT_ALL_ME_UPDATES ;
  LocalCommandBuffer . data_chunk_length = len ;
  memcpy ( LocalCommandBuffer . command_data_buffer , NetworkMe , len );

  CommunicationResult = SDLNet_TCP_Send ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending Full Me Update returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A FULL ME UPDATE TO THE CLIENT SUCCESSFULLY\n\
in the function void SendTextMessageToClient ( int PlayerNum , char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendFullMeUpdateToClient ( int PlayerNum )

/* ----------------------------------------------------------------------
 * This function sends a text message to a client in command form.
 * ---------------------------------------------------------------------- */
void
SendFullPlayerEngramToClient ( int PlayerNum )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending full player engram to client in command form.\n" ) ;
  len = sizeof ( PlayerEngram ) ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long full NetworkMe update to client... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // We prepare the NetworkMe we want to send...
  //
  PreparePlayerEngramForPlayer ( PlayerNum ) ;

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_ACCEPT_PLAYER_ENGRAM ;
  LocalCommandBuffer . data_chunk_length = len ;
  memcpy ( LocalCommandBuffer . command_data_buffer , PlayerEngram , len );

  CommunicationResult = SDLNet_TCP_Send ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending Full PlayerEngram returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A FULL PLAYER ENGRAM TO THE CLIENT SUCCESSFULLY\n\
in the function void SendTextMessageToClient ( int PlayerNum , char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendFullPlayerEngramToClient ( int PlayerNum )

/* ----------------------------------------------------------------------
 * This function sends a text message to a client in command form.
 * ---------------------------------------------------------------------- */
void
SendFullEnemyEngramToClient ( int PlayerNum )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending full player engram to client in command form.\n" ) ;
  len = sizeof ( EnemyEngram ) ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long full NetworkMe update to client... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // We prepare the network information we want to send...
  //
  PrepareFullEnemysEngram (  ) ;

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_ACCEPT_FULL_ENEMY_ENGRAM ;
  LocalCommandBuffer . data_chunk_length = len ;
  memcpy ( LocalCommandBuffer . command_data_buffer , EnemyEngram , len );

  CommunicationResult = SDLNet_TCP_Send ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending Full PlayerEngram returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A FULL PLAYER ENGRAM TO THE CLIENT SUCCESSFULLY\n\
in the function void SendTextMessageToClient ( int PlayerNum , char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendFullPlayerEngramToClient ( int PlayerNum )

/* ----------------------------------------------------------------------
 * This function sends an update engram about the enemys to a client in command form.
 * ---------------------------------------------------------------------- */
void
SendEnemyUpdateEngramToClient ( int PlayerNum )
{
  int CommunicationResult;
  int len;
  int EnemyIndex;
  int WriteIndex = 0 ;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending enemy update engram to client in command form.\n" ) ;

  for ( EnemyIndex = 0 ; EnemyIndex < MAX_ENEMYS_ON_SHIP ; EnemyIndex ++ )
    {
      if ( AllEnemys [ EnemyIndex ] . pos . z == Me [ PlayerNum ] . pos . z )
	{
	  //--------------------
	  // We also ignore the deactivated enemys on level 0
	  //
	  if ( ( AllEnemys [ EnemyIndex ] . pos . z == 0 ) && ( AllEnemys [ EnemyIndex ] . Status == OUT ) )
	    continue;

	  FillDataIntoEnemyEngram ( WriteIndex , EnemyIndex );
	  WriteIndex ++ ;
	}
    };


  len = WriteIndex * sizeof ( EnemyEngram[0] ) ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long full NetworkMe update to client... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_ACCEPT_UPDATE_ENEMY_ENGRAM ;
  LocalCommandBuffer . data_chunk_length = len ;
  memcpy ( LocalCommandBuffer . command_data_buffer , EnemyEngram , len );

  CommunicationResult = SDLNet_TCP_Send ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending Full PlayerEngram returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A FULL PLAYER ENGRAM TO THE CLIENT SUCCESSFULLY\n\
in the function void SendTextMessageToClient ( int PlayerNum , char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendEnemyUpdateEngramToClient ( int PlayerNum )

/* ----------------------------------------------------------------------
 * This function sends a request for the client to erase all enemys in
 * the enemy array.
 * ---------------------------------------------------------------------- */
void
SendEnemyDeletionRequestToClient ( int PlayerNum )
{
  int CommunicationResult;
  int len;
  char dummy_byte;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending full player engram to client in command form.\n" ) ;
  len = sizeof ( dummy_byte ) ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long detetion request to client... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_DELETE_ALL_YOUR_ENEMYS ;
  LocalCommandBuffer . data_chunk_length = len ;
  // memcpy ( LocalCommandBuffer . command_data_buffer , PlayerEngram , len );

  CommunicationResult = SDLNet_TCP_Send ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending Delete All Enemys Request to Client returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A REQUEST FOR DELETION OF ALL ENEMYS TO THE CLIENT SUCCESSFULLY\n\
in the function void SendEnemyDeletionRequestToClient ( int PlayerNum ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendEnemyDeletionRequestToClient ( int PlayerNum )

/* ----------------------------------------------------------------------
 * When the server sends us update about the complete (well, with the
 * networking reductions of course) Me array, this new information must
 * be enforced, i.e. written into the really used Me structures.
 * This is what this function should do.  It assumes, that the data
 * has been completely received and the command correctly interpeted
 * by the calling function before.
 * ---------------------------------------------------------------------- */
void 
EnforceServersAllMeUpdate ( void )
{

  DebugPrintf ( 0 , "\nvoid EnforceServersAllMeUpdate ( void ) : real function call confirmed. " );

  memcpy ( NetworkMe , CommandFromServer [ 0 ] . command_data_buffer , MAX_PLAYERS * sizeof ( NetworkMe[0] ) );

  DebugPrintf ( 0 , "\nNetworkMe[0].energy : %f." , NetworkMe[0].energy );
  DebugPrintf ( 0 , "\nNetworkMe[0].status : %d." , NetworkMe[0].status );

  CopyNetworkMeToMe ( ) ;

  DebugPrintf ( 0 , "\nvoid EnforceServersAllMeUpdate ( void ) : end of function reached. " );

}; // void EnforceServersAllMeUpdate ( void )


/* ----------------------------------------------------------------------
 * This function should execute the command the server has sent us.  This
 * command is assumed to be stored in the appropriate command structure
 * already.
 * ---------------------------------------------------------------------- */
void
ExecuteServerCommand ( void )
{
  int TransmittedEnemys;

  DebugPrintf ( 0 , "\nExecuteServerCommand ( void ): real function call confirmed." ) ;

  switch ( CommandFromServer [ 0 ] . command_code )
    {
    case PLAYER_SERVER_ERROR:
      DebugPrintf ( 0 , "\nPLAYER_SERVER_ERROR command received...Terminting..." );
      Terminate ( ERR );
      break;
      
    case SERVER_THIS_IS_MY_NAME:
      //--------------------
      // This command is not allowed to be given to a Player
      //
      DebugPrintf ( 0 , "\nSERVER_THIS_IS_MY_NAME command received AS PLAYER!!!... Terminating..." );
      Terminate ( ERR );
      break;

    case PLAYER_SERVER_TAKE_THIS_TEXT_MESSAGE:
      DebugPrintf ( 0 , "\nPLAYER_SERVER_TAKE_THIS_IS_TEXT_MESSAGE command received... AS CLIENT." );
      DebugPrintf ( 0 , "\nMessage transmitted : %s." , & CommandFromServer [ 0 ] . command_data_buffer );
      // Terminate ( ERR );
      break;
  
    case PLAYER_ACCEPT_ALL_ME_UPDATES:
      DebugPrintf ( 0 , "\nPLAYER_ACCEPT_ALL_ME_UPDATES command received... " );
      EnforceServersAllMeUpdate ( );
      // Terminate ( ERR );
      break;

    case PLAYER_ACCEPT_PLAYER_ENGRAM:
      DebugPrintf ( 0 , "\nPLAYER_ACCEPT_PLAYER_ENGRAM command received... " );
      EnforceServersPlayerEngram ( ) ;
      // Terminate ( ERR );
      break;

    case PLAYER_ACCEPT_FULL_ENEMY_ENGRAM:
      DebugPrintf ( 0 , "\nPLAYER_ACCEPT_FULL_ENEMY_ENGRAM command received... " );
      EnforceServersFullEnemysEngram ( ) ;
      // Terminate ( ERR );
      break;

    case PLAYER_ACCEPT_UPDATE_ENEMY_ENGRAM:
      DebugPrintf ( 0 , "\nPLAYER_ACCEPT_UPDATE_ENEMY_ENGRAM command received... " );

      TransmittedEnemys = CommandFromServer [ 0 ] . data_chunk_length / sizeof ( EnemyEngram [ 0 ] ) ;

      EnforceServersUpdateEnemysEngram ( TransmittedEnemys ) ;
      // Terminate ( ERR );
      break;

    case PLAYER_DELETE_ALL_YOUR_ENEMYS:
      DebugPrintf ( 0 , "\nPLAYER_DELETE_ALL_YOUR_ENEMYS command received... " );
      ClearEnemys ( ) ;
      break;

    default:
      DebugPrintf ( 0 , "\nUNKNOWN COMMAND CODE ERROR!!!! Received as PLAYER!!!  Terminating..." );
      Terminate ( ERR );
      break;

    };

}; // void ExecuteServerCommand ( void )


/* ----------------------------------------------------------------------
 * This function reads the signals from the client socket of player
 * PlayerNum at the server.  Depending on the current fill and 
 * command completition status of this players command buffer, be data
 * will be read into the buffer at the right location and also the right
 * amount of data will be read.
 * ---------------------------------------------------------------------- */
void
Read_Command_Buffer_From_Server ( void )
{
  int len;

  //--------------------
  // Depending on the current fill status of the command buffer, we read in the 
  // command code and command buffer length or not.
  //
  if ( ! server_buffer_fill_status )
    {
      // --------------------
      // Here we clearly expect a completely new command sequence to begin.
      //
      DebugPrintf ( 0 , "\nvoid Read_Command_Buffer_From_Server ( void ): New Command expected." );

      //--------------------
      // At first we read in the command code.
      //
      len = SDLNet_TCP_Recv ( sock , 
			      & ( CommandFromServer [ 0 ] . command_code ) , 
			      sizeof ( CommandFromServer [ 0 ] . command_code ) ) ;
      if ( len == 0 ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command code failed.... Terminating... " );
	  Terminate ( ERR );
	}
      if ( len < sizeof ( CommandFromServer [ 0 ] . command_code ) ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command:  command code did not completely come over... " );
	  Terminate ( ERR );
	}

      //--------------------
      // Now we read in the command data chunk length to expect.
      //
      len = SDLNet_TCP_Recv ( sock , 
			      & ( CommandFromServer [ 0 ] . data_chunk_length ) , 
			      sizeof ( CommandFromServer [ 0 ] . data_chunk_length ) ) ;
      
      if ( len == 0 ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command data buffer length failed.... Terminating... " );
	  Terminate ( ERR );
	}
      if ( len < sizeof ( CommandFromServer [ 0 ] . data_chunk_length ) ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command:  data buffer length info did not completely come over... " );
	  Terminate ( ERR );
	}
    }

  //--------------------
  // At this point we know, that either an old command code and chunk exists
  // and must be appended or a new command chunk is being read
  //
  len = SDLNet_TCP_Recv ( sock , 
			  & ( CommandFromServer [ 0 ] . command_data_buffer [ server_buffer_fill_status ] ) , 
			  CommandFromServer [ 0 ] . data_chunk_length - server_buffer_fill_status ) ;

  if ( len == 0 ) 
    {
      DebugPrintf ( 0 , "\n\nReading command data buffer itself failed.... Terminating... " );
      Terminate ( ERR );
    }

  if ( ( len + server_buffer_fill_status ) < ( CommandFromServer [ 0 ] . data_chunk_length ) ) 
    {
      DebugPrintf ( 0 , "\n\nWARNING!! Reading command:  data (still) did not come completely over... " );
      DebugPrintf ( 0 , "\nAppending to existing data and continuing..." );

      //--------------------
      // So we advance our fill pointer and wait for the next function call...
      //
      server_buffer_fill_status += len;

      // Terminate ( ERR );

    }
  else
    {
      DebugPrintf ( 0 , "\nReading command:  (Finally) Data did come completely over... " );

      //--------------------
      // Now at this point we know, that we have received a complete and full
      // command from the client.  That means we can really execute the command
      // that has just been filled into the command buffer.
      //
      ExecuteServerCommand (  );
      
      server_buffer_fill_status = 0 ;

    }

      
}; // void Read_Command_Buffer_From_Server ( void )

/* ----------------------------------------------------------------------
 * This function should establish a connection to a known server, so that
 * the game can run as a client now.
 * ---------------------------------------------------------------------- */
void
ConnectToFreedroidServer ( void )
{
  int UsedSockets;

  // Resolve the argument into an IPaddress type
  if( SDLNet_ResolveHost( &ip , "localhost" , port ) == ( -1 ) )
    {
      DebugPrintf( 0 , "\n--------------------\nERROR!!! SDLNet_ResolveHost: %s\n--------------------\n " ,
		   SDLNet_GetError ( ) ) ;
      Terminate ( ERR );
    }
  else
    {
      DebugPrintf( 0 , "\n--------------------\nSDLNet_ResolveHost was successful...\n--------------------\n " );
    }
  
  // open the server socket
  sock = SDLNet_TCP_Open( &ip ) ;
  if( ! sock )
    {
      DebugPrintf( 0 , "\n--------------------\nERROR!!! SDLNet_TCP_Open: %s\n--------------------\n" , 
		   SDLNet_GetError ( ) ) ;
      Terminate ( ERR );
    }
  else
    {
      DebugPrintf( 0 , "\n--------------------\nSDLNet_TCP_Open was successful...\n--------------------\n " );
    }

  //--------------------
  // Now that we have successfully opened a socket for information
  // exchange with the server, we add this socket to a socket set of
  // one socket, so that we can later easily query if there was some
  // transmission and data ready for reading from the socket or not.
  //
  if ( ( UsedSockets = SDLNet_TCP_AddSocket( The_Set_Of_The_One_Server_Socket , sock ) ) == ( -1 ) )
    {
      fprintf( stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET SUBSYSTEM WAS UNABLE TO ADD A NEW SOCKET TO THE SOCKET SET.\n\
\n\
The reason for this as reported by the SDL is : \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError( ) ) ;
      Terminate(ERR);
    }
  else
    {
      DebugPrintf ( 0 , "\nSOCKETS IN SOCKET SET OF NO MORE THAN ONE SOCKET IS NOW : %d.\n" , UsedSockets );
    }

  //--------------------
  // Now we tell the server our name
  //
  SendPlayerNameToServer ( ) ;

}; // void ConnectToFreedroidServer ( void )

/* ----------------------------------------------------------------------
 * This function will go out soon in favour of the command format based
 * client-server-communication.
 * ---------------------------------------------------------------------- */
void
Send1024MessageToServer ( char message[1024] )
{
  int CommunicationResult;
  int len;

  // print out the message
  DebugPrintf ( 0 , "\nSending message : %s\n" , message ) ;
  len = strlen ( message );

  CommunicationResult = SDLNet_TCP_Send ( sock , message , len ); // add 1 for the NULL

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending TCP message returned : %d . " , CommunicationResult );

  /*
  if(result<len)
    printf("SDLNet_TCP_Send: %s\n",SDLNet_GetError());
  */
  
};
  
/* ----------------------------------------------------------------------
 * This opens the TCP/IP sockets, so that some clients can connect
 * to this server...
 * ---------------------------------------------------------------------- */
void
OpenTheServerSocket ( void )
{

  // Resolve the argument into an IPaddress type
  if ( SDLNet_ResolveHost ( &ip , NULL , port ) == (-1) )
    {
      DebugPrintf ( 0 , "SDLNet_ResolveHost: %s\n" , SDLNet_GetError ( ) ) ;
      Terminate ( ERR );
    }
  else
    {
      DebugPrintf ( 0 , "\n--------------------\nSDLNet_ResolveHost was successful...\n--------------------\n" );
    }
  
  // open the server socket
  TheServerSocket = SDLNet_TCP_Open ( & ip ) ;

  if ( ! TheServerSocket )
    {
      DebugPrintf ( 0 , "\n--------------------\nERROR!!! SDLNet_TCP_Open: %s\n--------------------\n" , 
		    SDLNet_GetError ( ) ) ;
      Terminate ( ERR );
    }
  else
    {
      DebugPrintf ( 0 , "\n--------------------\nSDLNet_TCP_Open was successful...\n--------------------\n" );
    }

}; // void OpenTheServerSocket ( void )
  
/* ----------------------------------------------------------------------
 * This function does what?? 
 *
 * ---------------------------------------------------------------------- */
void
AcceptConnectionsFromClients ( void )
{
  int PlayerNum;
  int UsedSockets;

  //--------------------
  // We plan to offer MAX_PLAYERS connections to remote machines.
  // Therefore we check for new connections in this loop.
  //
  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      //--------------------
      // Of course we MUST NOT use an existing and already used connections for
      // connecting a new player!!  So we avoid this...
      //
      if ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer ) continue;

      //--------------------
      // So having reached an unused position for the client now,
      // we can see if someone wants to join at this socket position
      // into our running server game.
      // 
      AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer = SDLNet_TCP_Accept ( TheServerSocket );

      if ( ! AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer )
	{ 
	  // no connection accepted
	  // printf("SDLNet_TCP_Accept: %s\n",SDLNet_GetError());
	  // SDL_Delay(100); //sleep 1/10th of a second
	  return;
	  // continue;
	}
      else
	{
	  DebugPrintf( 0 , "\nvoid AcceptConnectionsFromClinets ( void ) : NEW PLAYER HAS JUST CONNECTED !!!! \n" );
	  if ( ( UsedSockets = SDLNet_TCP_AddSocket( The_Set_Of_All_Client_Sockets , 
						     AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer ) 
		 ) == ( -1 ) )
	    {
	      fprintf( stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET SUBSYSTEM WAS UNABLE TO ADD A NEW SOCKET TO THE SOCKET SET.\n\
\n\
The reason for this as reported by the SDL is : \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError( ) ) ;
	      Terminate(ERR);
	    }
	  else
	    {
	      DebugPrintf ( 0 , "\nSOCKETS IN SET OF ALL CLIENT SOCKETS NOW : %d.\n" , UsedSockets );
	      AllPlayers [ PlayerNum ] . network_status = CONNECTION_FRESHLY_OPENED;
	    }
	}
  
      //--------------------
      // So now that we know that a new player has connected this instant,
      // we go and try to get the clients IP and port number
      //
      remoteip=SDLNet_TCP_GetPeerAddress( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer );
      if ( ! remoteip )
	{
	  DebugPrintf ( 0 , "\n--------------------\nERROR!!! SDLNet_TCP_GetPeerAddress: %s\n--------------------\n" , 
			SDLNet_GetError ( ) ) ;
	  Terminate( ERR );
	}
      else
	{
	  DebugPrintf ( 0 , "\n--------------------\nSDLNet_TCP_GetPeerAddress was successful... \n" ) ;

	  //--------------------
	  // print out the clients IP and port number
	  //
	  ipaddr=SDL_SwapBE32(remoteip->host);
	  printf("Accepted a connection from %d.%d.%d.%d port %hu\n",
		 ipaddr>>24,
		 (ipaddr>>16)&0xff,
		 (ipaddr>>8)&0xff,
		 ipaddr&0xff,
		 remoteip->port);
	}

      //--------------------
      // Now that the new client has been accepted, we send a welcome 
      // message to the client program.
      //
      SendTextMessageToClient ( PlayerNum , "\nMesg. f.t. server: WELCOME to the game!!!\n" );

    } // for .. test all MAX_PLAYER sockets for new clients wanting to connect.
  
}; // void AcceptConnectionsFromClients ( void )


/* ----------------------------------------------------------------------
 * When the server experiences problems and can't help but shut down, e.g.
 * due to a segfault or something or if the running multiplayer game 
 * should be terminated, then all existing clients should (perhaps be
 * informed? and then) disconnected.
 * ---------------------------------------------------------------------- */
void 
DisconnectAllRemoteClinets ( void )
{
  int PlayerNum;

  //--------------------
  // We close all the players sockets at the server...
  //
  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      if ( AllPlayers[ PlayerNum ].ThisPlayersSocketAtTheServer )
	SDLNet_TCP_Close ( AllPlayers[ PlayerNum ].ThisPlayersSocketAtTheServer );
    }

}; // void DisconnectAllRemoteClinets ( void )

/* ----------------------------------------------------------------------
 *
 *
 * ---------------------------------------------------------------------- */
void
ExecutePlayerCommand ( int PlayerNum )
{
  SDL_Event *KeyEventPointer;

  DebugPrintf ( 0 , "\nExecutePlayerCommand ( int PlayerNum ): real function call confirmed." ) ;

  switch ( CommandFromPlayer [ PlayerNum ] . command_code )
    {
    case PLAYER_SERVER_ERROR:
      DebugPrintf ( 0 , "\nPLAYER_SERVER_ERROR command received...Terminting..." );
      Terminate ( ERR );
      break;
      
    case SERVER_THIS_IS_MY_NAME:
      DebugPrintf ( 0 , "\nSERVER_THIS_IS_MY_NAME command received... " );
      DebugPrintf ( 0 , "\nNew name string has length : %d. " , 
		    strlen ( CommandFromPlayer [ PlayerNum ] . command_data_buffer ) );
      strcpy ( Me [ PlayerNum ] . character_name , CommandFromPlayer [ PlayerNum ] . command_data_buffer ) ;
      DebugPrintf ( 0 , "\nNew name has been set for Player %d. " , PlayerNum ) ;

      //--------------------
      // At this point, for a test, we also initiate a new character at the server
      // Hope this doesn't crash everything.
      //
      InitiateNewCharacter ( PlayerNum , WAR_BOT );

      //--------------------
      // Since this is a completely new client or new character connecting, the first thing
      // it will need is a full update about the current me structure at
      // the server.
      //
      // I hope this will at least not overwrite the first Me entry as well
      // or come too early or something...
      //
      SendFullMeUpdateToClient ( PlayerNum );

      AllPlayers [ PlayerNum ] . network_status = GAME_ON ;

      SendEnemyDeletionRequestToClient ( PlayerNum ) ;

      SendFullEnemyEngramToClient ( PlayerNum ) ;

      break;

    case SERVER_ACCEPT_THIS_KEYBOARD_EVENT:
      DebugPrintf ( 0 , "\nSERVER_ACCEPT_THIS_KEYBOARD_EVENT command received..." );

      KeyEventPointer = ( SDL_Event* ) CommandFromPlayer [ PlayerNum ] . command_data_buffer ;
      
      switch ( KeyEventPointer -> type )
	{

	case SDL_KEYDOWN:
	  //--------------------
	  // In case the client just reportet a key being pressed, we enter
	  // this change into the server keyboard pressing table for this
	  // client.
	  //
	  DebugPrintf ( 0 , "\nKEYBOARD EVENT: Key down received... " );
	  AllPlayers [ PlayerNum ] . server_keyboard [ KeyEventPointer -> key.keysym.sym ] = TRUE ;
	  break;
	case SDL_KEYUP:
	  //--------------------
	  // In case the client just reportet a key being released, we enter
	  // this change into the server keyboard pressing table for this
	  // client.
	  //
	  DebugPrintf ( 0 , "\nKEYBOARD EVENT: Key up received... " );
	  AllPlayers [ PlayerNum ] . server_keyboard [ KeyEventPointer -> key.keysym.sym ] = FALSE ;
	  break;

	default:
	  //--------------------
	  // There should no be any other keyboard events than keys being pressed
	  // or keys being released.  It some other event is sent, this is assumed to
	  // be a major error and terminates the server.
	  //
	  DebugPrintf ( 0 , "\nKEYBOARD EVENT DATA RECEIVED IS BULLSHIT!! TERMINATING... " );
	  Terminate ( ERR ) ;
	  break;
	}
      // switch( event.key.keysym.sym )

      // Terminate ( ERR );
      break;

    case PLAYER_TELL_ME_YOUR_NAME:
      DebugPrintf ( 0 , "\nPLAYER_TELL_ME_YOUR_NAME command received...Terminting..." );
      Terminate ( ERR );
      break;
      
    case PLAYER_SERVER_TAKE_THIS_TEXT_MESSAGE:
      DebugPrintf ( 0 , "\nPLAYER_SERVER_TAKE_THIS_IS_TEXT_MESSAGE command received... AS SERVER." );
      DebugPrintf ( 0 , "\nMessage transmitted : %s." , & CommandFromPlayer [ PlayerNum ] . command_data_buffer );
      // Terminate ( ERR );
      break;

      
    case PLAYER_ACCEPT_ALL_ME_UPDATES:
      DebugPrintf ( 0 , "\nPLAYER_ACCEPT_ALL_ME_UPDATES command received AS SERVER!!!! Terminating... " );
      Terminate ( ERR );
      break;

    default:
      DebugPrintf ( 0 , "\nUNKNOWN COMMAND CODE ERROR!!!! Terminating..." );
      Terminate ( ERR );
      break;

    };

}; // void ExecutePlayerCommand ( PlayerNum )

/* ----------------------------------------------------------------------
 * This function reads the signals from the client socket of player
 * PlayerNum at the server.  Depending on the current fill and 
 * command completition status of this players command buffer, be data
 * will be read into the buffer at the right location and also the right
 * amount of data will be read.
 * ---------------------------------------------------------------------- */
void
Read_Command_Buffer_From_Player_No ( int PlayerNum )
{
  int len;

  //--------------------
  // Depending on the current fill status of the command buffer, we read in the 
  // command code and command buffer length or not.
  //
  if ( ! AllPlayers [ PlayerNum ] . command_buffer_fill_status )
    {

      DebugPrintf ( 0 , "\nvoid Read_Command_Buffer_From_Player_No ( int PlayerNum ): New Command expected." );

      len = SDLNet_TCP_Recv ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
			      & ( CommandFromPlayer [ PlayerNum ] . command_code ) , 
			      sizeof ( CommandFromPlayer [ PlayerNum ] . command_code ) ) ;

      if ( len == 0 ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command code failed.... Terminating... " );
	  Terminate ( ERR );
	}
      if ( len < sizeof ( CommandFromPlayer [ PlayerNum ] . command_code ) ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command:  command code did not completely come over... " );
	  Terminate ( ERR );
	}

      len = SDLNet_TCP_Recv ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
			      & ( CommandFromPlayer [ PlayerNum ] . data_chunk_length ) , 
			      sizeof ( CommandFromPlayer [ PlayerNum ] . data_chunk_length ) ) ;
      
      if ( len == 0 ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command data buffer length failed.... Terminating... " );
	  Terminate ( ERR );
	}
      if ( len < sizeof ( CommandFromPlayer [ PlayerNum ] . data_chunk_length ) ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command:  data buffer length info did not completely come over... " );
	  Terminate ( ERR );
	}

      len = SDLNet_TCP_Recv ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer , 
			      & ( CommandFromPlayer [ PlayerNum ] . command_data_buffer ) , 
			      CommandFromPlayer [ PlayerNum ] . data_chunk_length ) ;

      if ( len == 0 ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command data buffer itself failed.... Terminating... " );
	  Terminate ( ERR );
	}
      if ( len < CommandFromPlayer [ PlayerNum ] . data_chunk_length ) 
	{
	  DebugPrintf ( 0 , "\n\nReading command:  data did not come completely over... " );
	  Terminate ( ERR );
	}

      //--------------------
      // Now at this point we know, that we have received a complete and full
      // command from the client.  That means we can really execute the command
      // that has just been filled into the command buffer.
      //
      ExecutePlayerCommand ( PlayerNum );

    }
  else
    {

      DebugPrintf ( 0 , "\nvoid Read_Command_Buffer_From_Player_No ( int PlayerNum ): APPENDING OLD COMMAND." );

    }

}; // void Read_Command_Buffer_From_Player_No ( int PlayerNum )

/* ----------------------------------------------------------------------
 * This function listens if some signal just came from one of the clinets
 * ---------------------------------------------------------------------- */
void
ListenToAllRemoteClients ( void )
{
  int PlayerNum;
  int ActivityInTheSet;

  DebugPrintf( 2 , "\nvoid ListenToAllRemoteClients ( void ) : real function call confirmed ..." );

  //--------------------
  // At first we check if something is going on in the set of all
  // the clints sockets at the server.  If not, we're done here and
  // need not do anything more...
  //
  // ActivityInTheSet = SDLNet_SocketReady ( The_Set_Of_All_Client_Sockets ) ;
  ActivityInTheSet = SDLNet_CheckSockets ( The_Set_Of_All_Client_Sockets , 0 ) ;
  if ( ActivityInTheSet == 0 )
    {
      DebugPrintf ( 2 , "\nNo Activity in the whole set detected.... Returning.... " );
      return;
    }
  else
    {
      DebugPrintf ( 0 , "\nvoid ListenToAllRemoteClients ( void ) : Activity in the set detected." );
    }

  //--------------------
  // Now we check in which one of the sockets the acitivty has
  // taken place.
  //
  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      //--------------------
      // Of course we should not try to read some info from a
      // client socket, that has not yet been occupied.  Therefore
      // we introduce this protection.
      //
      if ( ! AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer ) continue;

      //--------------------
      // Since we do not know which of the sockets in the set has caused the 
      // activity, we check if it was perhaps this one, before we start a blocking
      // read receive command, which would halt the whole server, cause it's blocking
      // if this socket didn't really have something to say...
      //
      if ( ! SDLNet_SocketReady( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer ) )
	{
	  DebugPrintf ( 0 , "\nSocked Nr. %d didn't cause the activity..." , PlayerNum );
	  continue;
	}

      //--------------------
      // Now that we know, that there is at least a socket, where 
      // perhaps some information might be ready for us to read, we
      // try to get this piece of information.
      //
      Read_Command_Buffer_From_Player_No ( PlayerNum );

    }

  DebugPrintf( 0 , "\nvoid ListenToAllRemoteClients ( void ) : end of function reached..." );

}; // void ListenToAllRemoteClients ( void )

/* ---------------------------------------------------------------------- 
 * This function sends something from the server to all of the clients
 * currently connected to this server.
 * ---------------------------------------------------------------------- */
void
ServerSendMessageToAllClients ( char ServerMessage[1024] )
{
  int PlayerNum;
  int BytesSent;
  int MessageLength;

  MessageLength = strlen ( ServerMessage );

  DebugPrintf( 0 , "\n--------------------" ) ;
  DebugPrintf( 0 , "\nSending message to all clinets: %s." , ServerMessage );
  DebugPrintf( 0 , "\nMessage length is found to be : %d." , MessageLength );

  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      if ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer )
	{
	  DebugPrintf( 0 , "\nNow sending to Player %d." , PlayerNum );
	  
	  BytesSent = SDLNet_TCP_Send( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer, 
				       ServerMessage , MessageLength ) ;
	  DebugPrintf( 0 , "\nNumber of Bytes sent to Player %d : %d." , PlayerNum , BytesSent );

	}
    }

}; // void ServerSendMessageToAllClients ( char ServerMessage[1024] );

/* ----------------------------------------------------------------------
 * This function listens, if the server has perhaps said something to
 * us.  In this case, the received data is printed out.
 * ---------------------------------------------------------------------- */
void
ListenForServerMessages ( void ) 
{
  int ActivityInTheSet;

  //--------------------
  // Here we check if the set of the one and only server socket
  // perhaps contains some data in its only socket.
  //
  ActivityInTheSet = SDLNet_CheckSockets ( The_Set_Of_The_One_Server_Socket , 0 ) ;
  if ( ActivityInTheSet == 0 )
    {
      DebugPrintf ( 2 , "\nNo Activity in the whole set of the one server socket at the client detected.... Returning.... " );
      return;
    }
  else
    {
      DebugPrintf ( 0 , "\nvoid ListenForServerMessages ( void ) : Something was sent from the server!!!" );
    }

  //--------------------
  // This is somewhat redundant, cause since there is never more than one socket
  // in this socket set, it must be this one socket, where the activity came from.
  // But nevertheless, being a networking newbie, I'll check again if the activity
  // has really been observed in the socket itself.
  //
  if ( ! SDLNet_SocketReady( sock ) )
    {
      DebugPrintf ( 0 , "\n\
--------------------\n\
WARNING!! SEVERE ERROR ENCOUNTERED! ACTIVITY THERE, BUT NOT DETECTABLE!! ERROR!!\n\
Termination....\n\
--------------------\n" );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now that we know, that we really got some message from the server,
  // we try to read that information.
  //
  Read_Command_Buffer_From_Server (  ) ;

}; // void ListenForServerMessages ( void ) 

/* ----------------------------------------------------------------------
 * This function sends a text message to the server in command form.
 * ---------------------------------------------------------------------- */
void
SendTextMessageToServer ( char* message )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending to server in command form.  Message is : %s\n" , message ) ;
  len = strlen ( message ) + 1 ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long message to server... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = PLAYER_SERVER_TAKE_THIS_TEXT_MESSAGE ;
  LocalCommandBuffer . data_chunk_length = len ;
  strcpy ( LocalCommandBuffer . command_data_buffer , message );

  

  CommunicationResult = SDLNet_TCP_Send ( sock , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending TCP message returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A TEXT MESSAGE TO THE SERVER SUCCESSFULLY\n\
in the function void SendTextMessageToServer ( char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendTextMessageToServer ( char* message )

/* ----------------------------------------------------------------------
 * This function sends a text message to the server in command form.
 * ---------------------------------------------------------------------- */
void
SendPlayerNameToServer ( void )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending player name to server in command form. " ) ;
  len = strlen ( Me [ 0 ] . character_name ) + 1 ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= MAX_CHARACTER_NAME_LENGTH )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long character name to server... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = SERVER_THIS_IS_MY_NAME ;
  LocalCommandBuffer . data_chunk_length = len ;
  strcpy ( LocalCommandBuffer . command_data_buffer , Me [ 0 ] . character_name );

  CommunicationResult = SDLNet_TCP_Send ( sock , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending character name to server returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A TEXT MESSAGE TO THE SERVER SUCCESSFULLY\n\
in the function void SendTextMessageToServer ( char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendPlayerNameToServer ( void )

/* ----------------------------------------------------------------------
 * This function sends a keyboard event to the server in command form.
 * ---------------------------------------------------------------------- */
void
SendPlayerKeyboardEventToServer ( SDL_Event event )
{
  int CommunicationResult;
  int len;
  network_command LocalCommandBuffer;

  // print out the message
  DebugPrintf ( 0 , "\nSending keyboard event to server in command form. " ) ;
  len = sizeof ( event ) ; // the amount of bytes in the data buffer

  //--------------------
  // We check against sending too long messages to the server.
  //
  if ( len >= COMMAND_BUFFER_MAXLEN )
    {
      DebugPrintf ( 0 , "\nAttempted to send too long keyboard event to server... Terminating..." );
      Terminate ( ERR ) ;
    }

  //--------------------
  // Now we prepare our command buffer.
  //
  LocalCommandBuffer . command_code = SERVER_ACCEPT_THIS_KEYBOARD_EVENT ;
  LocalCommandBuffer . data_chunk_length = len ;
  memcpy ( LocalCommandBuffer . command_data_buffer , & ( event ) , sizeof ( event ) );

  CommunicationResult = SDLNet_TCP_Send ( sock , 
					  & ( LocalCommandBuffer ) , 
					  2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length ); 

  //--------------------
  // Now we print out the success or return value of the sending operation
  //
  DebugPrintf ( 0 , "\nSending keyboard event to server returned : %d . " , CommunicationResult );
  if ( CommunicationResult < 2 * sizeof ( int ) + LocalCommandBuffer . data_chunk_length )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The SDL NET COULD NOT SEND A KEYBOARD EVENT TO THE SERVER SUCCESSFULLY\n\
in the function void SendTextMessageToServer ( char* message ).\n\
\n\
The cause of this problem as reportet by the SDL_net was: \n\
%s\n\
\n\
Freedroid will terminate now to draw attention \n\
to the networking problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , SDLNet_GetError ( ) ) ;
      Terminate(ERR);
    }

}; // void SendPlayerKeyboardEventToServer ( void )

/* ----------------------------------------------------------------------
 * This prints out the server status.  Well, it will print anyway, but
 * only the server will know the full and correct information.
 * ---------------------------------------------------------------------- */
void
PrintServerStatusInformation ( void )
{
  int PlayerNum;

  DebugPrintf ( 0 , "\n----------------------------------------" );
  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      DebugPrintf ( 0 , "\nPlayer Nr : %d Character Name : %s. Status : %d NW-Status: %s." ,
		    PlayerNum , Me [ PlayerNum ] . character_name , Me [ PlayerNum ] . status , 
		    NetworkClientStatusNames[ AllPlayers [ PlayerNum ] . network_status ] );
      DebugPrintf ( 0 , "\nPos:X=%d Y=%d Z=%d Energy=%d." ,
		    ( int ) Me [ PlayerNum ] . pos.x , 
		    ( int ) Me [ PlayerNum ] . pos.y ,
		    ( int ) Me [ PlayerNum ] . pos.z ,
		    ( int ) Me [ PlayerNum ] . energy );
    }
  DebugPrintf ( 0 , "\n----------------------------------------" );

}; // void PrintServerStatusInformation ( void )

/* ----------------------------------------------------------------------
 * This function sends the periodic updates to all connected clients.
 * ---------------------------------------------------------------------- */
void
SendPeriodicServerMessagesToAllClients ( void )
{
  int PlayerNum;

  static int DelayCounter = 0;

  DelayCounter ++ ;

#define SEND_PACKET_ON_EVERY_FRAME_CONG_MOD (20)

  if ( DelayCounter < SEND_PACKET_ON_EVERY_FRAME_CONG_MOD ) return;

  DelayCounter = 0;


  DebugPrintf( 0 , "\n--------------------" ) ;
  DebugPrintf( 0 , "\nSending periodic server messages to all clinets...." );

  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
    {
      if ( AllPlayers [ PlayerNum ] . ThisPlayersSocketAtTheServer )
	{
	  DebugPrintf( 0 , "\nNow sending periodic server message to Player %d." , PlayerNum );
	  
	  SendFullPlayerEngramToClient ( PlayerNum ) ;

	  SendEnemyUpdateEngramToClient ( PlayerNum ) ;

	}
    }

}; // void SendPeriodicServerMessagesToAllClients ( void )

#undef _network_c

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
 * Desc: the main program
 * ---------------------------------------------------------------------- */
/*
 * This file has been checked for remains of german comments in the code
 * I you still find some, please just kill it mercilessly.
 */

#define _main_c

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"
#include "text.h"
#include "vars.h"
#include "ship.h"

int ThisMessageTime;
float LastGotIntoBlastSound = 2;
float LastRefreshSound = 2;

void UpdateCountersForThisFrame (void);

/* -----------------------------------------------------------------
 * This function is the heart of the game.  It contains the main
 * game loop.
 * ----------------------------------------------------------------- */
int
main (int argc, char *const argv[])
{
  int i;
  int PlayerNum;

  GameOver = FALSE;
  QuitProgram = FALSE;

  /*
   *  Parse command line and set global switches 
   *  this function exits program when error, so we don't need to 
   *  check its success  (dunno if that's good design?)
   */
  sound_on = TRUE;	 /* default value, can be overridden by command-line */
  debug_level = 0;       /* 0=no debug 1=first debug level (at the moment=all) */
  fullscreen_on = TRUE; /* use X11-window or full screen */
  joy_sensitivity = 1;
  mouse_control = TRUE;
  classic_user_rect = FALSE;

  parse_command_line (argc, argv); 

  InitFreedroid ();   // Initialisation of global variables and arrays

  while (!QuitProgram)
    {

      StartupMenu ( );
      // MissionSelectMenu ( );
      // InitNewMission ( STANDARD_MISSION );
      // InitNewMission ( NEW_MISSION );

      GameOver = FALSE;

      while ( (!GameOver && !QuitProgram) || ServerMode )
	{

	  if ( ServerMode ) AcceptConnectionsFromClients ( ) ;

	  if ( ServerMode ) ListenToAllRemoteClients ( ) ; 

	  if ( ClientMode ) ListenForServerMessages ( ) ;

	  if ( ServerMode ) SendPeriodicServerMessagesToAllClients (  ) ;

	  StartTakingTimeForFPSCalculation(); 

	  UpdateCountersForThisFrame ();

	  CollectAutomapData (); // this is a pure client issue.  Only do it for the main player...

	  ReactToSpecialKeys();

	  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ ) 
	    {
	      MoveLevelDoors ( PlayerNum ) ; // this is also a pure client issue, but done for all players...
	    }

	  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ ) 
	    CheckForTriggeredEventsAndStatements ( PlayerNum ) ;

	  AnimateRefresh (); // this is a pure client issue.  Not dependent upon the players.

	  AnimateTeleports (); // this is a pure client issue.  Not dependent upon the players.

	  ExplodeBlasts ();	// move blasts to the right current "phase" of the blast

	  //--------------------
	  // Now, for multiplayer, we update all player images...
	  //
	  for ( PlayerNum = 0 ; PlayerNum < MAX_PLAYERS ; PlayerNum ++ )
	    Update_Tux_Working_Copy ( PlayerNum ); // do this for player Nr. 

	  // Assemble_Combat_Picture ( DO_SCREEN_UPDATE ); 
	  Assemble_Combat_Picture ( 0 ); 

	  MoveBullets ();   // please leave this in front of graphics output, so that time_in_frames always starts with 1

	  DisplayBanner (NULL, NULL,  0 ); // this is a pure client issue

	  SDL_Flip ( Screen );

	  for (i = 0; i < MAXBULLETS; i++) CheckBulletCollisions (i);

	  if ( ! ClientMode )
	    for ( i = 0 ; i < MAX_PLAYERS ; i ++ ) MoveInfluence ( i );	// change Influ-speed depending on keys pressed, but
	                        // also change his status and position and "phase" of rotation

	  // MoveInfluence ( 0 ) ; // for now, we move only influence nr. 0 

	  UpdateAllCharacterStats ( 0 );

	  // Move_Influencers_Friends (); // Transport followers to next level

	  if ( ! ClientMode ) MoveEnemys ();	// move all the enemys:
	                        // also do attacks on influ and also move "phase" or their rotation

	  for ( i = 0 ; i < MAX_PLAYERS ; i ++ ) CheckInfluenceWallCollisions ( i );	// test if influs way is blocked by walls...

	  CheckInfluenceEnemyCollision ();

	  if (CurLevel->empty == 2)
	    {
	      LevelGrauFaerben ();
	      CurLevel->empty = TRUE;
	    }		    

	  CheckIfMissionIsComplete (); 

	  ComputeFPSForThisFrame();

	} /* while !GameOver */
    } /* while !QuitProgram */
  Terminate (0);
  return (0);
}; // void main ( void )

/* -----------------------------------------------------------------
 * This function updates counters and is called ONCE every frame.
 * The counters include timers, but framerate-independence of game speed
 * is preserved because everything is weighted with the Frame_Time()
 * function.
 * ----------------------------------------------------------------- */
void
UpdateCountersForThisFrame (void)
{
  static long Overall_Frames_Displayed=0;
  int i;

  GameConfig.Mission_Log_Visible_Time += Frame_Time();
  GameConfig.Inventory_Visible_Time += Frame_Time();
  // if (ShipEmptyCounter == 1) GameOver = TRUE;

  LastBlastHit++;

  Total_Frames_Passed_In_Mission++;
  Me[0].FramesOnThisLevel++;
  // The next couter counts the frames displayed by freedroid during this
  // whole run!!  DO NOT RESET THIS COUNTER WHEN THE GAME RESTARTS!!
  Overall_Frames_Displayed++;
  Overall_Average = (Overall_Average*(Overall_Frames_Displayed-1)
		     + Frame_Time()) / Overall_Frames_Displayed;

  // Here are some things, that were previously done by some periodic */
  // interrupt function
  ThisMessageTime++;

  LastGotIntoBlastSound += Frame_Time ();
  LastRefreshSound += Frame_Time ();
  Me[0].LastCrysoundTime += Frame_Time ();
  Me[0].MissionTimeElapsed += Frame_Time();
  Me[0].LastTransferSoundTime += Frame_Time();
  Me[0].TextVisibleTime += Frame_Time();
  if ( Me[0].weapon_swing_time != (-1) ) Me[0].weapon_swing_time += Frame_Time();
  if ( Me[0].got_hit_time != (-1) ) Me[0].got_hit_time += Frame_Time();

  LevelDoorsNotMovedTime += Frame_Time();

  if ( SkipAFewFrames ) SkipAFewFrames--;

  if ( Me[0].firewait > 0 )
    {
      Me[0].firewait-=Frame_Time();
      if (Me[0].firewait < 0) Me[0].firewait=0;
    }
  if (ShipEmptyCounter > 1)
    ShipEmptyCounter--;
  if (CurLevel->empty > 2)
    CurLevel->empty--;
  if (Me[0].Experience > ShowScore)
    ShowScore++;
  if (Me[0].Experience < ShowScore)
    ShowScore--;

  for (i = 0; i < MAX_ENEMYS_ON_SHIP ; i++)
    {

      if (AllEnemys[i].Status == OUT ) continue;

      if (AllEnemys[i].warten > 0) 
	{
	  AllEnemys[i].warten -= Frame_Time() ;
	  if (AllEnemys[i].warten < 0) AllEnemys[i].warten = 0;
	}

      if (AllEnemys[i].firewait > 0) 
	{
	  AllEnemys[i].firewait -= Frame_Time() ;
	  if (AllEnemys[i].firewait <= 0) AllEnemys[i].firewait=0;
	}

      AllEnemys[i].TextVisibleTime += Frame_Time();
    } // for (i=0;...

} /* UpdateCountersForThisFrame() */


#undef _main_c

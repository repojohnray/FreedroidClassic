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
 * This file contains all the functions managing the things one gets to see.
 * That includes assembling of enemys, assembling the currently
 * relevant porting of the map (the bricks I mean), drawing all visible
 * elements like bullets, blasts, enemys or influencer in a nonvisible
 * place in memory at first, and finally drawing them to the visible
 * screen for the user.
 * ---------------------------------------------------------------------- */

/*
 * This file has been checked for remnants of german comments.  If you still find
 * any, please let me know.
 */

#define _view_c

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "map.h"
#include "proto.h"
#include "colodefs.h"

#include "SDL_rotozoom.h"

void FlashWindow (SDL_Color Flashcolor);
void RecFlashFill (int LX, int LY, int Color, unsigned char *Parameter_Screen,
		   int SBreite);
int Cent (int);

char *Affected;
EXTERN int MyCursorX;
EXTERN int MyCursorY;

SDL_Color flashcolor1 = {100, 100, 100};
SDL_Color flashcolor2 = {0, 0, 0};

//
// POSSIBLY OUTDATED AND UNUSED FUNCTION
// PLEASE CHECK FOR REMOVAL POSSIBLE
//
int
Cent (int Val)
{
  Val = Val - (Val % Block_Width) + Block_Width / 2;
  return Val;
}

/* ----------------------------------------------------------------------
 * There is more than one approach to the problem of disruptor flashes.
 * (*) One solution is to just completely fill the visible screen white and
 *     black altenatingly.
 * (*) The other solution is to start in the center and then recursively
 *     proceed through the passable tiles and using this method fill 
 *     exactly the whole room where you're currently in.  That is perhaps
 *     the more sophisticated method.  Right now however, it's disabled.
 * 
 * ---------------------------------------------------------------------- */
void
RecFlashFill (int LX, int LY, int Color, unsigned char *Parameter_Screen, int SBreite)
{
  int i;
  static int num;

  LY = LY;
  LX = LX;
  num++;

//      gotoxy(1,1);
//      printf(" RFF: X=%d Y=%d.\n",LX,LY);
//      getchar();

  // mark this square as within the area of effect
  Affected[LY / Block_Height * CurLevel->xlen + LX / Block_Width] = TRUE;

  // fill this square up
  for (i = LY / 4 - ((LY / 4) % 8); i < (LY / 4 - ((LY / 4) % 8) + 8); i++)
    {
      memset (Parameter_Screen + i * SBreite + LX / 4 - ((LX / 4) % 8), Color, 8);
    }
  i -= 4;

  // fill the square to the right also up
  if ((*(Parameter_Screen + i * SBreite + LX / 4 + 8) != Color) &&
      (IsPassable (Cent (LX + Block_Width), Cent (LY), CENTER) == CENTER))
    RecFlashFill (LX + Block_Width, LY, Color, Parameter_Screen, SBreite);

  // fill the square to the left
  if (LX > Block_Width)
    {
      if ((*(Parameter_Screen + i * SBreite + LX / 4 - 8) != Color) &&
	  (IsPassable (Cent (LX - Block_Width), Cent (LY), CENTER) == CENTER))
	RecFlashFill (LX - Block_Width, LY, Color, Parameter_Screen, SBreite);
    }

  // fill the square above
  if ((i > 8) && (LY > Block_Height))
    {
      if ((*(Parameter_Screen + (i - 8) * SBreite + LX / 4) != Color) &&
	  (IsPassable (Cent (LX), Cent (LY - Block_Height), CENTER) == CENTER))
	RecFlashFill (LX, LY - Block_Height, Color, Parameter_Screen, SBreite);
    }

  // fill the square below
  if ((*(Parameter_Screen + (i + 8) * SBreite + LX / 4) != Color) &&
      (IsPassable (Cent (LX), Cent (LY + Block_Height), CENTER) == CENTER))
    RecFlashFill (LX, LY + Block_Height, Color, Parameter_Screen, SBreite);
}

/* ----------------------------------------------------------------------
 *
 *
 * ---------------------------------------------------------------------- */
void 
ShowMissionCompletitionMessages( void )
{
  int MissNum;

  //--------------------
  // If the log is not set to visible right now, we do not need to 
  // do anything more
  //
  if ( GameConfig.Mission_Log_Visible == FALSE ) return;
  if ( GameConfig.Mission_Log_Visible_Time >= GameConfig.Mission_Log_Visible_Max_Time ) return;

  //--------------------
  // At this point we know, that the quest log is desired and
  // therefore we display it in-game:
  //
  DisplayText( "See quest log: \n" , User_Rect.x , User_Rect.y , &User_Rect );

  for ( MissNum = 0 ; MissNum < MAX_MISSIONS_IN_GAME; MissNum ++ )
    {
      // In case the mission does not exist at all, we need not do anything more...
      if ( Me.AllMissions[ MissNum ].MissionExistsAtAll != TRUE ) continue;

      // In case the mission was not yet assigned, we need not do anything more...
      if ( Me.AllMissions[ MissNum ].MissionWasAssigned != TRUE ) continue;

      // In case the message is rather old, we need not do anything more...
      // if ( Me.AllMissions[ MissNum ].MissionLastStatusChangeTime > 1000 ) continue;

      // At this point we know, that the mission has recently been completed or failed

      if ( Me.AllMissions[ MissNum ].MissionIsComplete == TRUE )
	{
	  DisplayText( "\n* Mission completed: " , -1 , -1 , &User_Rect );
	}
      else if ( Me.AllMissions[ MissNum ].MissionWasFailed == TRUE )
	{
	  DisplayText( "\n* Mission completed: " , -1 , -1 , &User_Rect );
	}
      else
	  DisplayText( "\n* Mission assigned: " , -1 , -1 , &User_Rect );

      DisplayText( Me.AllMissions[ MissNum ].MissionName , -1 , -1 , &User_Rect );

    }
};

/* ----------------------------------------------------------------------
 *
 *
 * ---------------------------------------------------------------------- */
void 
ShowInventoryMessages( void )
{
  int SlotNum;
  char InventoryText[2000];
  SDL_Rect InventoryRect;
  SDL_Rect TargetRect;
  SDL_Surface *InventoryImage;
  char *fpath;
  char fname[]="inventory.png";

  DebugPrintf (2, "\nvoid ShowInventoryMessages( ... ): Function call confirmed.");

  InventoryRect.x = 0;
  InventoryRect.y = User_Rect.y;
  InventoryRect.w = SCREENLEN/2;
  InventoryRect.h = User_Rect.h;

  User_Rect.x = 0;
  User_Rect.w = SCREENLEN;
  User_Rect_Center_x=( User_Rect.x + User_Rect.w/2 );
  User_Rect_Center_y=( User_Rect.y + User_Rect.h/2 );

  //--------------------
  // If the log is not set to visible right now, we do not need to 
  // do anything more
  //
  if ( GameConfig.Inventory_Visible == FALSE ) return;
  // if ( GameConfig.Inventory_Visible_Time >= GameConfig.Inventory_Visible_Max_Time ) return;

  //--------------------
  // At this point we know, that the quest log is desired and
  // therefore we display it in-game:
  //

  // We split the screen to half, so that one half can be used for the inventory
  // and the other 
  User_Rect.x = SCREENLEN/2;
  User_Rect.w = SCREENLEN/2;
  User_Rect_Center_x=( User_Rect.x + User_Rect.w/2 );
  User_Rect_Center_y=( User_Rect.y + User_Rect.h/2 );
  SDL_SetClipRect( Screen, &InventoryRect );
  SDL_FillRect( Screen, & InventoryRect , 0x0FFFFFF );
 
  fpath = find_file ( fname , GRAPHICS_DIR, FALSE);

  InventoryImage = IMG_Load( fpath );

  SDL_BlitSurface ( InventoryImage , NULL , Screen , &InventoryRect );

  SDL_FreeSurface( InventoryImage );

  for ( SlotNum = 0 ; SlotNum < MAX_ITEMS_IN_INVENTORY; SlotNum ++ )
    {
      // In case the item does not exist at all, we need not do anything more...
      if ( Me.Inventory[ SlotNum ].type == ( -1 ) ) 
	{
	  // DisplayText( "\n--- Slot empty ---" , -1 , -1 , &InventoryRect );
	  continue;
	}

      TargetRect.x = 16 + 32 * Me.Inventory[ SlotNum ].inventory_position.x;
      TargetRect.y = 480 - 16 - 32 * 6 + 32 * Me.Inventory[ SlotNum ].inventory_position.y;
      TargetRect.w = 50;
      TargetRect.h = 50;

      SDL_BlitSurface( ItemSurfaceList[ ItemMap[ Me.Inventory[ SlotNum ].type ].picture_number ] , NULL , Screen , &TargetRect );
      
    }


  /*
  MyCursorX=InventoryRect.x;
  MyCursorY=InventoryRect.y;
  DisplayText( "See inventory: \n" , InventoryRect.x , InventoryRect.y , &InventoryRect );
  
  for ( SlotNum = 0 ; SlotNum < MAX_ITEMS_IN_INVENTORY; SlotNum ++ )
    {
      // In case the item does not exist at all, we need not do anything more...
      if ( Me.Inventory[ SlotNum ].type == ( -1 ) ) 
	{
	  DisplayText( "\n--- Slot empty ---" , -1 , -1 , &InventoryRect );
	  continue;
	}

      sprintf( InventoryText , "\n Item Nr. %d is %s." , SlotNum , ItemMap[ Me.Inventory[ SlotNum ]. type ].ItemName );
      DisplayText( InventoryText , -1 , -1 , &InventoryRect );

    }

  DisplayText( "\n\nNo more items beyond that." , -1 , -1 , &InventoryRect );
  */

  SDL_UpdateRect( Screen , InventoryRect.x , InventoryRect.y , InventoryRect.w , InventoryRect.h );

}; // void ShowInventoryMessages();

/* -----------------------------------------------------------------
 * This function assembles the contents of the combat window 
 * in Screen.
 *
 * Several FLAGS can be used to control its behaviour:
 *
 * (*) ONLY_SHOW_MAP = 1:  This flag indicates not do draw any
 *     game elements but the map blocks
 *
 * (*) DO_SCREEN_UPDATE = 2: This flag indicates for the function
 *     to also cause an SDL_Update of the portion of the screen
 *     that has been modified
 *
 * ----------------------------------------------------------------- */
void
Assemble_Combat_Picture (int mask)
{
  int MapBrick;
  int line, col;
  int i;
  int minutes;
  int seconds;
  static float TimeSinceLastFPSUpdate=10;
  static int FPS_Displayed=1;
  SDL_Rect TargetRectangle;
#define UPDATE_FPS_HOW_OFTEN 0.75

  DebugPrintf (2, "\nvoid Assemble_Combat_Picture(...): Real function call confirmed.");
  
  // Why not blit the WHOLE map?  Lets try it!
  // THAT IS A VERY POWERFUL AND VERY ABSTRACT PROCEDURE:
  // * THE COMBATSCREENSIZE COULD *EASYLY* BE CHANGED WITHOUT HAVING TO CHANGE THE CODE!!!
  // 
  // Recently there were complaints about garbage outside the ship.  This was because
  // outside the ship, nothing was blittet.  Now the blitting starts at -5 tiles outside
  // the ship and ends +5 tiles outside the other end of the ship.  That should do it.
  //

  SDL_SetColorKey (Screen, 0, 0);
  // SDL_SetAlpha( Screen , 0 , SDL_ALPHA_OPAQUE ); 

                         
  SDL_SetClipRect (Screen , &User_Rect);

  for (line = -5; line < CurLevel->ylen + 5; line++)
    {
      for (col = -5; col < CurLevel->xlen + 5; col++)
	{
	  if ((MapBrick = GetMapBrick( CurLevel, col , line )) != INVISIBLE_BRICK)
	    {
	      TargetRectangle.x = User_Rect_Center_x 
		+ ( -Me.pos.x+col-0.5 )*Block_Width;
	      TargetRectangle.y = User_Rect_Center_y
		+ ( -Me.pos.y+line-0.5 )*Block_Height;
	      // Screen, &TargetRectangle);
	      SDL_BlitSurface( MapBlockSurfacePointer[ CurLevel->color ][MapBrick] , NULL ,
 			       Screen, &TargetRectangle);
	    }			// if !INVISIBLE_BRICK 
	}			// for(col) 
    }				// for(line) 

  if (mask & ONLY_SHOW_MAP) 
    {
      // in case we only draw the map, we are done here.  But
      // of course we must check if we should update the screen too.
      if ( mask & DO_SCREEN_UPDATE ) 
	SDL_UpdateRect( Screen , User_Rect.x , User_Rect.y , User_Rect.w , User_Rect.h );
      return;
    }

  // At this point we know that now only the map is to be drawn.
  // so we start drawing the rest of the INTERIOR of the combat window:

  for ( i = 0 ; i < MAX_ITEMS_PER_LEVEL ; i ++ )
    {
      PutItem( i );
    }

  for (i = 0; i < MAX_ENEMYS_ON_SHIP ; i++)
    PutEnemy (i , -1 , -1 );

  if (Me.energy > 0)
    PutInfluence ( -1 , -1 );

  for (i = 0; i < (MAXBULLETS); i++)
    if (AllBullets[i].type != OUT)
      PutBullet (i);

  for (i = 0; i < (MAXBLASTS); i++)
    if (AllBlasts[i].type != OUT)
      PutBlast (i);

  if ( GameConfig.Draw_Framerate )
    {
      TimeSinceLastFPSUpdate += Frame_Time();
      if ( TimeSinceLastFPSUpdate > UPDATE_FPS_HOW_OFTEN )
	{
	  FPS_Displayed=(int)(1.0/Frame_Time());
	  TimeSinceLastFPSUpdate=0;
	}

      PrintStringFont( Screen , FPS_Display_BFont , User_Rect.x , 
		       User_Rect.y+User_Rect.h - FontHeight( FPS_Display_BFont ), 
		       "FPS: %d " , FPS_Displayed );

      PrintStringFont( Screen , FPS_Display_BFont , User_Rect.x + 100, 
		       User_Rect.y+User_Rect.h - FontHeight( FPS_Display_BFont ), 
		       "Axis: %d %d" , input_axis.x, input_axis.y);
    }

  if ( GameConfig.Draw_Energy )
    {
      PrintStringFont( Screen , FPS_Display_BFont , User_Rect.x+User_Rect.w/2 , 
		       User_Rect.y+User_Rect.h - FontHeight( FPS_Display_BFont ), 
		       "Energy: %d " , (int) (Me.energy) );
      PrintStringFont( Screen , FPS_Display_BFont , User_Rect.x+User_Rect.w/2 , 
		       User_Rect.y+User_Rect.h - 2 * FontHeight( FPS_Display_BFont ), 
		       "Resistance: %f " , (Me.Current_Victim_Resistance_Factor) );
    }

  if ( GameConfig.Draw_Position )
    {
      PrintStringFont( Screen , FPS_Display_BFont , User_Rect.x+2*User_Rect.w/3 , 
		       User_Rect.y+User_Rect.h - FontHeight( FPS_Display_BFont ), 
		       "GPS: X=%d Y=%d Lev=%d" , (int) rintf(Me.pos.x) , (int) rintf(Me.pos.y) , CurLevel->levelnum );
    }

  if ( Me.AllMissions[0].MustLiveTime != (-1) )
    {
      minutes=floor( ( Me.AllMissions[0].MustLiveTime - Me.MissionTimeElapsed ) / 60 );
      seconds= rintf( Me.AllMissions[0].MustLiveTime - Me.MissionTimeElapsed ) - 60 * minutes;
      if ( minutes < 0 ) 
	{
	  minutes = 0;
	  seconds = 0;
	}
      PrintStringFont( Screen , FPS_Display_BFont , User_Rect.x , 
		       User_Rect.y + 0*FontHeight( FPS_Display_BFont ), 
		       "Time to hold out still: %2d:%2d " , minutes , seconds );
    }

  ShowMissionCompletitionMessages();
  ShowInventoryMessages();

  //--------------------
  // At this point we are done with the drawing procedure
  // and all that remains to be done is updating the screen.
  // Depending on where we did our modifications, we update
  // an according portion of the screen.
  if ( mask & DO_SCREEN_UPDATE )
    {
      SDL_UpdateRect( Screen , User_Rect.x , User_Rect.y , User_Rect.w , User_Rect.h );
    }


  DebugPrintf (2, "\nvoid Assemble_Combat_Picture(...): end of function reached.");
}; // void Assemble_Combat_Picture(...)

/* -----------------------------------------------------------------
 * This function draws the influencer to the screen, either
 * to the center of the combat window if (-1,-1) was specified, or
 * to the specified coordinates anywhere on the screen, useful e.g.
 * for using the influencer as a cursor in the menus.
 * ----------------------------------------------------------------- */
void
PutInfluence ( int x , int y )
{
  SDL_Rect TargetRectangle;
  SDL_Rect Text_Rect;
  int alpha_value;
  int i;

  Text_Rect.x=User_Rect.x+(User_Rect.w/2) + Block_Width/3;
  Text_Rect.y=User_Rect.y+(User_Rect.h/2) - Block_Height/2;
  Text_Rect.w=User_Rect.w/2 - Block_Width/3;
  Text_Rect.h=User_Rect.h/2;

  DebugPrintf (2, "\nvoid PutInfluence(void): real function call confirmed.");

  if ( x == -1 ) 
    {
      TargetRectangle.x=User_Rect_Center_x - Block_Width/2;
      TargetRectangle.y=User_Rect_Center_y - Block_Height/2;
    }
  else
    {
      TargetRectangle.x=x ;
      TargetRectangle.y=y ;
    }

  //--------------------
  // Maybe the influencer is fading due to low energy?
  // to achive this, is might be nescessary to add some 
  // alpha to the surface, that will later be
  // removed again.  We do this here:
  //
  
#define alpha_offset 80
  if ( ( (Me.energy*100/Druidmap[Me.type].maxenergy) <= BLINKENERGY) && ( x == (-1) ) ) 
    {

      // In case of low energy, do the fading effect...
      alpha_value = (int) ( ( 256 - alpha_offset ) * 
			    fabsf( 0.5 * Me.MissionTimeElapsed - floor( 0.5 * Me.MissionTimeElapsed ) - 0.5 ) + 
			    ( alpha_offset ) );

      for ( i = 0 ; i < DIGITNUMBER ; i++ )
	SDL_SetAlpha( InfluDigitSurfacePointer[i] , SDL_SRCALPHA , alpha_value );

      // ... and also maybe start a new cry-sound

      if ( Me.LastCrysoundTime > CRY_SOUND_INTERVAL )
	{
	  Me.LastCrysoundTime = 0;
	  CrySound();
	}
    }
  else
    {
      for ( i = 0 ; i < DIGITNUMBER ; i++ )
	SDL_SetAlpha( InfluDigitSurfacePointer[i] , SDL_SRCALPHA , SDL_ALPHA_OPAQUE );
    }

  //--------------------
  // In case of transfer mode, we produce the transfer mode sound
  // but of course only in some periodic intervall...

  if ( Me.status == TRANSFERMODE )
    {
      if ( Me.LastTransferSoundTime > TRANSFER_SOUND_INTERVAL )
	{
	  Me.LastTransferSoundTime = 0;
	  TransferSound();
	}
    }


  // Now we draw the hat and shoes of the influencer
  SDL_BlitSurface( InfluencerSurfacePointer[ (int) floorf (Me.phase) ], NULL , Screen, &TargetRectangle );


  // Now we draw the first digit of the influencers current number.
  // SDL SOMETIMES MODIFIES THE TARGET ENTRY, THEREFORE IT HAS TO BE 
  // COMPUTED ANEW!!!!
  if ( x == -1 ) 
    {
      TargetRectangle.x=User_Rect_Center_x - Block_Width/2 + First_Digit_Pos_X;
      TargetRectangle.y=User_Rect_Center_y - Block_Height/2 + First_Digit_Pos_Y;
    }
  else
    {
      TargetRectangle.x=x + First_Digit_Pos_X ;
      TargetRectangle.y=y + First_Digit_Pos_Y ;
    }
  SDL_BlitSurface( InfluDigitSurfacePointer[ Druidmap[Me.type].druidname[0]-'1'+1 ] , NULL, Screen, &TargetRectangle );

  // Now we draw the second digit of the influencers current number.
  // SDL SOMETIMES MODIFIES THE TARGET ENTRY, THEREFORE IT HAS TO BE 
  // COMPUTED ANEW!!!!
  if ( x == -1 ) 
    {
      // TargetRectangle.x=User_Rect_Center_x - Block_Width/2 + Digit_Pos_X + Digit_Length;
      // TargetRectangle.y=User_Rect_Center_y - Block_Height/2 + Digit_Pos_Y;
      TargetRectangle.x=User_Rect_Center_x - Block_Width/2 + Second_Digit_Pos_X;
      TargetRectangle.y=User_Rect_Center_y - Block_Height/2 + Second_Digit_Pos_Y;
    }
  else
    {
      TargetRectangle.x=x + Second_Digit_Pos_X ;
      TargetRectangle.y=y + Second_Digit_Pos_Y ;
    }
  SDL_BlitSurface( InfluDigitSurfacePointer[ Druidmap[Me.type].druidname[1]-'1'+1 ] , NULL, Screen, &TargetRectangle );

  // Now we draw the third digit of the influencers current number.
  // SDL SOMETIMES MODIFIES THE TARGET ENTRY, THEREFORE IT HAS TO BE 
  // COMPUTED ANEW!!!!
  if ( x == -1 ) 
    {
      TargetRectangle.x=User_Rect_Center_x - Block_Width/2 + Third_Digit_Pos_X ;
      TargetRectangle.y=User_Rect_Center_y - Block_Height/2 + Third_Digit_Pos_Y;
    }
  else
    {
      TargetRectangle.x=x + Third_Digit_Pos_X ;
      TargetRectangle.y=y + Third_Digit_Pos_Y ;
    }
  SDL_BlitSurface( InfluDigitSurfacePointer[ Druidmap[Me.type].druidname[2]-'1'+1 ] , NULL, Screen, &TargetRectangle );

  //--------------------
  // Now that all fading effects are done, we can restore the blocks surface to OPAQUE,
  // which is the oposite of TRANSPARENT :)
  //

  //--------------------
  // Maybe the influencer has something to say :)
  // so let him say it..
  //
  if ( ( x == (-1) ) && ( Me.TextVisibleTime < GameConfig.WantedTextVisibleTime ) && GameConfig.All_Texts_Switch )
    {
      //      PutStringFont ( Screen , FPS_Display_BFont , 
      //		      User_Rect.x+(User_Rect.w/2) + Block_Width/3 , 
      //		      User_Rect.y+(User_Rect.h/2) - Block_Height/2 ,  
      //		      Me.TextToBeDisplayed );
      SetCurrentFont( FPS_Display_BFont );
      DisplayText( Me.TextToBeDisplayed , User_Rect.x+(User_Rect.w/2) + Block_Width/3 , User_Rect.y+(User_Rect.h/2) - Block_Height/2 , &Text_Rect );
    }

  DebugPrintf (2, "\nvoid PutInfluence(void): enf of function reached.");

}; // void PutInfluence( int x , int y )


/* ----------------------------------------------------------------------
 * This function draws an enemy into the combat window.
 * The only parameter given is the number of the enemy within the
 * AllEnemys array. Everything else is computed in here.
 * ---------------------------------------------------------------------- */
void
PutEnemy (int Enum , int x , int y)
{
  const char *druidname;	/* the number-name of the Enemy */
  int phase;
  int alpha_value;
  int i;
  int HumanModifier;
  SDL_Rect TargetRectangle;

  DebugPrintf (3, "\nvoid PutEnemy(int Enum): real function call confirmed...\n");

  if ( Druidmap[AllEnemys[ Enum ].type ].IsHuman ) HumanModifier =  'A' - '1' - 13;
  else HumanModifier = 0;

  /* if enemy is on other level, return */
  if (AllEnemys[Enum].levelnum != CurLevel->levelnum)
    {
      // DebugPrintf (3, "\nvoid PutEnemy(int Enum): DIFFERENT LEVEL-->usual end of function reached.\n");
      return;
    }

  // if this enemy is dead, we need not do anything more here
  if (AllEnemys[Enum].Status == OUT)
    {
      // DebugPrintf (3, "\nvoid PutEnemy(int Enum): STATUS==OUT --> usual end of function reached.\n");
      return;
    }

  // if the enemy is out of signt, we need not do anything more here
  if ((!show_all_droids) && (!IsVisible (&AllEnemys[Enum].pos)) )
    {
      DebugPrintf (3, "\nvoid PutEnemy(int Enum): ONSCREEN=FALSE --> usual end of function reached.\n");
      return;
    }

  DebugPrintf (3, "\nvoid PutEnemy(int Enum): it seems that we must draw this one on the screen....\n");

  // We check for incorrect droid types, which sometimes might occor, especially after
  // heavy editing of the crew initialisation functions ;)
  if ( AllEnemys[Enum].type >= Number_Of_Droid_Types )
    {
      fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
There was a droid type on this level, that does not really exist.\n\
\n\
We might use a fallback to shortly work around this problem.  That would\n\
not be difficult.  But for now Freedroid will terminate to draw attention \n\
to the sound problem it could not resolve.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" );
      AllEnemys[Enum].type = 0;
      Terminate(ERR);
    }

  //--------------------
  // First blit just the enemy hat and shoes.
  // If no coordinates, i.e. "-1", were given, we blit to the correct location in
  // the combat window, else we blit to the given location.
  // The number will be blittet later

  druidname = Druidmap[AllEnemys[Enum].type].druidname;
  phase = AllEnemys[Enum].feindphase;

  if ( x == (-1) ) 
    {
      TargetRectangle.x=User_Rect_Center_x+ 
	( (-Me.pos.x+AllEnemys[Enum].pos.x ) ) * Block_Width  -Block_Width/2;
      TargetRectangle.y=User_Rect_Center_y+ 
	( (-Me.pos.y+AllEnemys[Enum].pos.y ) ) * Block_Height -Block_Height/2;
    }
  else
    {
      TargetRectangle.x=x;
      TargetRectangle.y=y;
    }

  if ( AllEnemys[Enum].Friendly == 0 ) 
    {
      SDL_BlitSurface( EnemySurfacePointer[ phase ] , NULL , Screen, &TargetRectangle);
    }
  else
    {
      SDL_BlitSurface( InfluencerSurfacePointer[ phase ] , NULL , Screen, &TargetRectangle);

      if ( ( ( AllEnemys[Enum].energy*100/Druidmap[AllEnemys[Enum].type].maxenergy) <= BLINKENERGY) ) 
	{
	  
	  // In case of low energy, do the fading effect...
	  alpha_value = (int) ( ( 256 - alpha_offset ) * 
				fabsf( 0.5 * Me.MissionTimeElapsed - floor( 0.5 * Me.MissionTimeElapsed ) - 0.5 ) + 
				( alpha_offset ) );
	  
	  for ( i = 0 ; i < DIGITNUMBER ; i++ )
	    SDL_SetAlpha( InfluDigitSurfacePointer[i] , SDL_SRCALPHA , alpha_value );
	  
	  // ... and also maybe start a new cry-sound
	  
	  // if ( Me.LastCrysoundTime > CRY_SOUND_INTERVAL )
	  // {
	  // Me.LastCrysoundTime = 0;
	  // CrySound();
	  // }
	}
      else
	{
	  for ( i = 0 ; i < DIGITNUMBER ; i++ )
	    SDL_SetAlpha( InfluDigitSurfacePointer[i] , SDL_SRCALPHA , SDL_ALPHA_OPAQUE );
	}
    }

  //--------------------
  // Now the numbers should be blittet.
  // again, if no coordinates, i.e. "-1", were given, we blit to the correct location in
  // the combat window, else we blit to the given location.
  if ( x == (-1) )
    {
      TargetRectangle.x=User_Rect_Center_x - 
	(Me.pos.x-AllEnemys[Enum].pos.x) * Block_Width + First_Digit_Pos_X  - Block_Width/2; 
      TargetRectangle.y=User_Rect_Center_y - 
	(Me.pos.y-AllEnemys[Enum].pos.y) * Block_Height + First_Digit_Pos_Y - Block_Height/2;
    }
  else
    {
     TargetRectangle.x=x + First_Digit_Pos_X;
     TargetRectangle.y=y + First_Digit_Pos_Y;
    }

  if ( AllEnemys[Enum].Friendly == 0 )
    {
      SDL_BlitSurface( EnemyDigitSurfacePointer[ Druidmap[AllEnemys[Enum].type].druidname[0]-'1'+1 ] , NULL, Screen, &TargetRectangle );
    }
  else
    {
      
      SDL_BlitSurface( InfluDigitSurfacePointer[ Druidmap[AllEnemys[Enum].type].druidname[0]-'1'+1+HumanModifier ] , NULL, Screen, &TargetRectangle );
    }

  if ( x == (-1) )
    {
  TargetRectangle.x=User_Rect_Center_x - 
    (Me.pos.x-AllEnemys[Enum].pos.x)*Block_Height + Second_Digit_Pos_X - Block_Width/2;
  TargetRectangle.y=User_Rect_Center_y - 
    (Me.pos.y-AllEnemys[Enum].pos.y)*Block_Height + Second_Digit_Pos_Y - Block_Height/2 ;
    }
  else
    {
     TargetRectangle.x=x + Second_Digit_Pos_X;
     TargetRectangle.y=y + Second_Digit_Pos_Y;
    }

  if ( AllEnemys[Enum].Friendly == 0 )
    {
      SDL_BlitSurface( EnemyDigitSurfacePointer[ Druidmap[AllEnemys[Enum].type].druidname[1]-'1'+1 ] , NULL, Screen, &TargetRectangle );
    }
  else
    {
      SDL_BlitSurface( InfluDigitSurfacePointer[ Druidmap[AllEnemys[Enum].type].druidname[1]-'1'+1 +HumanModifier ] , NULL, Screen, &TargetRectangle );
    }

  if ( x == (-1) )
    {
      TargetRectangle.x=User_Rect_Center_x - (Me.pos.x-AllEnemys[Enum].pos.x)*Block_Width - Block_Width/2 + Third_Digit_Pos_X ;
      TargetRectangle.y=User_Rect_Center_y - (Me.pos.y-AllEnemys[Enum].pos.y)*Block_Width - Block_Height/2 + Third_Digit_Pos_Y;
    }
  else
    {
     TargetRectangle.x=x + Third_Digit_Pos_X ;
     TargetRectangle.y=y + Third_Digit_Pos_Y;
    }

  if ( AllEnemys[Enum].Friendly == 0 )
    {
      SDL_BlitSurface( EnemyDigitSurfacePointer[ Druidmap[AllEnemys[Enum].type].druidname[2]-'1'+1 ] , NULL, Screen, &TargetRectangle );
    }
  else
    {
      SDL_BlitSurface( InfluDigitSurfacePointer[ Druidmap[AllEnemys[Enum].type].druidname[2]-'1'+1 +HumanModifier ] , NULL, Screen, &TargetRectangle );
    }


  //--------------------
  // At this point we can assume, that the enemys has been blittet to the
  // screen, whether it's a friendly enemy or not.
  // 
  // So now we can add some text the enemys says.  That might be fun.
  //
  if ( (x == -1)
       && ( AllEnemys[Enum].TextVisibleTime < GameConfig.WantedTextVisibleTime )
       && GameConfig.All_Texts_Switch )
    {
      PutStringFont ( Screen , FPS_Display_BFont , 
		      User_Rect.x+(User_Rect.w/2) + Block_Width/3 + (AllEnemys[Enum].pos.x - Me.pos.x) * Block_Width , 
		      User_Rect.y+(User_Rect.h/2) - Block_Height/2 + (AllEnemys[Enum].pos.y - Me.pos.y) * Block_Height ,  
		      AllEnemys[Enum].TextToBeDisplayed );
    }

  

  DebugPrintf (2, "\nvoid PutEnemy(int Enum): ENEMY HAS BEEN PUT --> usual end of function reached.\n");

}	// void PutEnemy(int Enum , int x , int y) 

/* ----------------------------------------------------------------------
 * This function draws a Bullet into the combat window.  The only 
 * parameter given is the number of the bullet in the AllBullets 
 * array. Everything else is computed in here.
 * ---------------------------------------------------------------------- */
void
PutBullet (int BulletNummer)
{
  Bullet CurBullet = &AllBullets[BulletNummer];
  SDL_Rect TargetRectangle;
  int PhaseOfBullet;
  int i;

  DebugPrintf (2, "\nvoid PutBullet(int BulletNummer): real function call confirmed.\n");

  //--------------------
  // in case our bullet is of the type "FLASH", we only
  // draw a big white or black rectangle right over the 
  // combat window, white for even frames and black for 
  // odd frames.
  if (CurBullet->type == FLASH)
    {
      // Now the whole window will be filled with either white
      // or black each frame until the flash is over.  (Flash 
      // deletion after some time is done in CheckBulletCollisions.)
      if ( (CurBullet->time_in_frames % 2) == 1)
	{
	  FlashWindow (flashcolor1);
	  return;
	}
      if ( (CurBullet->time_in_frames % 2) == 0)
	{
	  FlashWindow (flashcolor2);
	  return;
	}
    } // if type == FLASH



  PhaseOfBullet = (CurBullet->time_in_seconds * Bulletmap[ CurBullet->type ].phase_changes_per_second );

  PhaseOfBullet = PhaseOfBullet % Bulletmap[CurBullet->type].phases ;

  // DebugPrintf( 0 , "\nPhaseOfBullet: %d.", PhaseOfBullet );

#define ONE_ROTATION_ONLY
#ifdef ONE_ROTATION_ONLY
  //--------------------
  // Maybe it's the first time this bullet is displayed.  But then, the images
  // of the rotated bullet in all phases are not yet attached to the bullet.
  // Then, we'll have to generate these
  //
  //if ( CurBullet->time_in_frames == 1 )
  if ( !CurBullet->Surfaces_were_generated )
    {
      for ( i=0; i<Bulletmap[ CurBullet->type ].phases ; i++ )
	{
	  CurBullet->SurfacePointer[i] = 
	    rotozoomSurface( Bulletmap[CurBullet->type].SurfacePointer[ i ] , CurBullet->angle , 1.0 , FALSE );
	}
      DebugPrintf( 1 , "\nvoid PutBullet(i): This was the first time for this bullet, so images were generated... angle=%f" , CurBullet->angle);
      CurBullet->Surfaces_were_generated=TRUE;
    }

  // WARNING!!! PAY ATTENTION HERE!! After the rotozoom was applied to the image, it is NO
  // LONGER of dimension Block_Width times Block_Height, but of the dimesions of the smallest
  // rectangle containing the full rotated Block_Height x Block_Width rectangle!!!
  // This has to be taken into account when calculating the target position for the 
  // blit of these surfaces!!!!
  TargetRectangle.x = User_Rect_Center_x
    - (Me.pos.x-CurBullet->pos.x)*Block_Width-CurBullet->SurfacePointer[ PhaseOfBullet ]->w/2;
  TargetRectangle.y = User_Rect_Center_y
    - (Me.pos.y-CurBullet->pos.y)*Block_Width-CurBullet->SurfacePointer[ PhaseOfBullet ]->h/2;

  SDL_BlitSurface( CurBullet->SurfacePointer[ PhaseOfBullet ] , NULL, Screen , &TargetRectangle );
#else
  tmp = rotozoomSurface( Bulletmap[CurBullet->type].SurfacePointer[ PhaseOfBullet ] , CurBullet->angle , 1.0 , FALSE );

  // WARNING!!! PAY ATTENTION HERE!! After the rotozoom was applied to the image, it is NO
  // LONGER of dimension Block_Width times Block_Height, but of the dimesions of the smallest
  // rectangle containing the full rotated Block_Height x Block_Width rectangle!!!
  // This has to be taken into account when calculating the target position for the 
  // blit of these surfaces!!!!
  TargetRectangle.x = User_Rect_Center_x
    - (Me.pos.x-CurBullet->pos.x)*Block_Width-CurBullet->SurfacePointer[ PhaseOfBullet ]->w/2;
  TargetRectangle.y = User_Rect_Center_y
    - (Me.pos.y-CurBullet->pos.y)*Block_Width-CurBullet->SurfacePointer[ PhaseOfBullet ]->h/2;

  SDL_BlitSurface( tmp , NULL, Screen , &TargetRectangle );
  SDL_FreeSurface( tmp );
#endif

  DebugPrintf ( 1 , "\nvoid PutBullet(int BulletNummer): end of function reched.\n");

}; // void PutBullet (int Bulletnumber )

/* ----------------------------------------------------------------------
 * This function draws an item into the combat window.
 * The only given parameter is the number of the item within
 * the AllItems array.
 * ---------------------------------------------------------------------- */
void
PutItem( int ItemNumber )
{
  Item CurItem = &CurLevel->ItemList[ ItemNumber ];
  SDL_Rect TargetRectangle;
  
  if ( CurItem->type == ( -1 ) ) return;

  TargetRectangle.x=User_Rect_Center_x - (Me.pos.x - CurItem->pos.x)*Block_Width  -Block_Width/2;
  TargetRectangle.y=User_Rect_Center_y - (Me.pos.y - CurItem->pos.y)*Block_Height -Block_Height/2;

  SDL_BlitSurface( ItemSurfaceList[ ItemMap[ CurItem->type ].picture_number ] , NULL , Screen , &TargetRectangle);
}; // void PutItem( int ItemNumber );

/* ----------------------------------------------------------------------
 * This function draws a blast into the combat window.
 * The only given parameter is the number of the blast within
 * the AllBlasts array.
 * ---------------------------------------------------------------------- */
void
PutBlast (int BlastNummer)
{
  Blast CurBlast = &AllBlasts[BlastNummer];
  SDL_Rect TargetRectangle;

  // If the blast is already long dead, we need not do anything else here
  if (CurBlast->type == OUT)
    return;

  
  TargetRectangle.x=User_Rect_Center_x - (Me.pos.x - CurBlast->PX)*Block_Width  -Block_Width/2;
  TargetRectangle.y=User_Rect_Center_y - (Me.pos.y - CurBlast->PY)*Block_Height -Block_Height/2;
  // Blastmap[CurBlast->type].block + ((int) floorf(CurBlast->phase)), Screen , &TargetRectangle);
  SDL_BlitSurface( Blastmap[CurBlast->type].SurfacePointer[ (int)floorf(CurBlast->phase) ] , NULL , Screen , &TargetRectangle);

}  // void PutBlast(int BlastNummer)

/* ----------------------------------------------------------------------
 * This function fills the combat window with one single color, given as
 * the only parameter to the function.
 * ---------------------------------------------------------------------- */
void
FlashWindow (SDL_Color Flashcolor)
{
  Fill_Rect( User_Rect, Flashcolor);
}; // void FlashWindow(int Flashcolor)


/* -----------------------------------------------------------------
 * Fill given rectangle with given RBG color
 * ----------------------------------------------------------------- */
void
Fill_Rect (SDL_Rect rect, SDL_Color color)
{
  Uint32 pixcolor;
  SDL_Rect tmp;

  Set_Rect (tmp, rect.x, rect.y, rect.w, rect.h);

  pixcolor = SDL_MapRGB (Screen->format, color.r, color.g, color.b);

  SDL_FillRect (Screen, &tmp, pixcolor);
  
  return;
}; // void Fill_Rect (SDL_Rect rect, SDL_Color color)

/* ----------------------------------------------------------------------
 * This function displays a robot picture.  This does NOT mean a
 * robot picture like in combat but this means a finely renderd
 * artwork by bastian, that is displayed in the console if info
 * about a robot is requested.  The only parameters to this 
 * function are the position on the screen where to blit the 
 * picture and the number of the robot in the Druidmap *NOT*
 * in AllEnemys!!
 *
 * ---------------------------------------------------------------------- */
void
ShowRobotPicture (int PosX, int PosY, int Number )
{
  SDL_Surface *tmp;
  SDL_Rect SourceRectangle;
  SDL_Rect TargetRectangle;
  char *fpath;
  char fname[500];

  DebugPrintf (2, "\nvoid ShowRobotPicture(...): Function call confirmed.");

  strcpy( fname, Druidmap[Number].druidname );
  strcat( fname , ".png" );

  fpath = find_file (fname, GRAPHICS_DIR, FALSE);

  if ( (tmp=IMG_Load (fpath)) == NULL )
    {
      fprintf (stderr,
	     "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
The image file named %s could not be read by SDL.\n\
\n\
The error reason as specified by SDL is : %s.\n\
\n\
Please check that the file is present and not corrupted\n\
in your distribution of Freedroid.\n\
\n\
Freedroid will terminate now to point at the error.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , fpath , SDL_GetError() );
      Terminate (ERR);
    }
  

  SourceRectangle.x=0;
  SourceRectangle.y=0;
  SourceRectangle.w=USERFENSTERBREITE;
  if ( tmp->w > 200 ) 
    {
      SDL_SetClipRect( Screen , NULL );
      TargetRectangle.x=0;
      TargetRectangle.y=BANNER_HEIGHT;
      SourceRectangle.h=SCREENHEIGHT-BANNER_HEIGHT;
    }
  else 
    {
      TargetRectangle.x=User_Rect.x;
      TargetRectangle.y=User_Rect.y + TEXT_STRETCH * FontHeight(Menu_BFont) ;
      SourceRectangle.h=USERFENSTERHOEHE;
    }

  SDL_BlitSurface( tmp , &SourceRectangle, Screen , &TargetRectangle );
  
  SDL_FreeSurface(tmp);

  DebugPrintf (2, "\nvoid ShowRobotPicture(...): Usual end of function reached.");
}				// void ShowRobotPicture(...)

#undef _view_c

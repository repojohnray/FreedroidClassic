/*----------------------------------------------------------------------
 *
 * Desc: Graphics primitived, such as functions to load LBM or PCX images,
 * 	 to change the vga color table, to activate or deachtivate monitor
 *	 signal, to set video modes etc.
 *
 *----------------------------------------------------------------------*/

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
#define _graphics_c

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"
#include "map.h"
#include "text.h"
#include "colodefs.h"
#include "SDL_rotozoom.h"

/*
----------------------------------------------------------------------
----------------------------------------------------------------------
*/

void 
DrawLineBetweenTiles( float x1 , float y1 , float x2 , float y2 , int Color )
{
  int i;
  int pixx;
  int pixy;
  float tmp;
  float slope;

  if ( (x1 == x2) && (y1 == y2) ) return; // nothing is to be done here


  if (x1 == x2) // infinite slope!! special case, that must be caught!
    {

      if (y1 > y2) // in this case, just interchange 1 and 2
	{
	  tmp = y1;
	  y1=y2;
	  y2=tmp;
	}

      for ( i=0 ; i < (y2 - y1) * Block_Width ; i++ )
	{
	  pixx=User_Rect.x + User_Rect.w/2 - Block_Width * (Me.pos.x - x1 );
	  pixy=User_Rect.y + User_Rect.h/2 - Block_Height * (Me.pos.y - y1 ) + i ;
	  if ( (pixx <= User_Rect.x) || 
	       (pixx >= User_Rect.x + User_Rect.w -1) || 
	       (pixy <= User_Rect.y ) || 
	       (pixy >= User_Rect.y + User_Rect.h -1) ) continue; 
	  putpixel( ne_screen , pixx , pixy , Color );
	  putpixel( ne_screen , pixx-1 , pixy , Color );
	}
      return; 
    }

  if (x1 > x2) // in this case, just interchange 1 and 2
    {
      tmp = x1;
      x1=x2;
      x2=tmp;
      tmp = y1;
      y1=y2;
      y2=tmp;
    }

  //--------------------
  // Now we start the drawing process
  //
  // SDL_LockSurface( ne_screen );

  slope = ( y2 - y1 ) / (x2 - x1) ;
  for ( i=0 ; i<(x2-x1)*Block_Width ; i++ )
    {
      pixx=User_Rect.x + User_Rect.w/2 - Block_Width * (Me.pos.x - x1 ) + i;
      pixy=User_Rect.y + User_Rect.h/2 - Block_Height * (Me.pos.y - y1 ) + i * slope ;
      if ( (pixx <= User_Rect.x) || 
	   (pixx >= User_Rect.x + User_Rect.w -1) || 
	   (pixy <= User_Rect.y ) || 
	   (pixy >= User_Rect.y + User_Rect.h -1) ) continue; 
      putpixel( ne_screen , pixx , pixy , Color );
      putpixel( ne_screen , pixx , pixy -1 , Color );
    }

  // SDL_UnlockSurface( ne_screen );

} // void DrawLineBetweenTiles


/*-----------------------------------------------------------------
 * This function saves a screenshot to disk.
 * The screenshots are names "Screenshot_XX.bmp" where XX is a
 * running number.  
 *
 * NOTE:  This function does NOT check for existing screenshots,
 *        but will silently overwrite them.  No problem in most
 *        cases I think.
 *
 *-----------------------------------------------------------------*/
void
TakeScreenshot(void)
{
  static int Number_Of_Screenshot=0;
  char *Screenshoot_Filename;

  Screenshoot_Filename=malloc(100);
  DebugPrintf (1, "\n\nScreenshoot function called.\n\n");
  sprintf( Screenshoot_Filename , "Screenshot_%d.bmp", Number_Of_Screenshot );
  DebugPrintf(1, "\n\nScreenshoot function: The Filename is: %s.\n\n" , Screenshoot_Filename );
  SDL_SaveBMP( ne_screen , Screenshoot_Filename );
  Number_Of_Screenshot++;
  free(Screenshoot_Filename);

} // void TakeScreenshot(void)

/*
----------------------------------------------------------------------
@Desc: This function draws a "grid" on the screen, that means every
       "second" pixel is blacked out, thereby generation a fading 
       effect.  This function was created to fade the background of the 
       Escape menu and its submenus.

@Ret: none
----------------------------------------------------------------------
*/
void 
MakeGridOnScreen(void)
{
  int x,y;

  DebugPrintf (2, "\nvoid MakeGridOnScreen(...): real function call confirmed.");
  SDL_LockSurface( ne_screen );
  for (y=0; y<SCREENHEIGHT; y++) 
    {
      for (x=0; x<SCREENLEN; x++) 
	{
	  if ((x+y)%2 == 0) 
	    {
	      putpixel( ne_screen, x, y, 0 );
	    }
	}
    }
  
  SDL_UnlockSurface( ne_screen );
  DebugPrintf (2, "\nvoid MakeGridOnScreen(...): end of function reached.");
} // void MakeGridOnSchreen(void)


/*----------------------------------------------------------------------
 * This function load an image and displays it directly to the ne_screen
 * but without updating it.
 * This might be very handy, especially in the Title() function to 
 * display the title image and perhaps also for displaying the ship
 * and that.
 *
 ----------------------------------------------------------------------*/
void DisplayImage(char *datafile)
{
  SDL_Surface *image;
  
  image = IMG_Load(datafile);
  if ( image == NULL ) {
    fprintf(stderr, "Couldn't load image %s: %s\n",
	    datafile, IMG_GetError());
    Terminate(ERR);
  }

  SDL_BlitSurface(image, NULL, ne_screen, NULL);

  SDL_FreeSurface(image);
}


/*
 * replace every occurance of color src by dst in Surface surf
 */
void replace_color (SDL_Surface *surf, SDL_Color src, SDL_Color dst)
{
  int i, j;
    
  for (i=0; i < surf->w; i++)
    for (j=0; j < surf->h; i++)
      ; /* ok, I'll do that later ; */

  return;
}

/*
----------------------------------------------------------------------
@Desc: This function initializes ALL the graphics again, propably after 
they have been destroyed by resizing operations.
This is done via freeing the old structures and starting the classical
allocations routine again.

@Ret: TRUE/FALSE, same as InitPictures() returns

----------------------------------------------------------------------
*/
int
ReInitPictures (void)
{
  SDL_FreeSurface( ne_blocks );
  SDL_FreeSurface( ne_static );

  return (InitPictures());
} // int ReInitPictures(void)


/*----------------------------------------------------------------------
 * This function resizes all blocks and structures involved in assembling
 * the combat picture to a new scale.  The new scale is relative to the
 * standard scale with means 64x64 tile size.
 *
 ----------------------------------------------------------------------*/
void 
SetCombatScaleTo(float ResizeFactor)
{
  int i, j;
  SDL_Surface *tmp;

  return;

  CenteredPutString   ( ne_screen ,  User_Rect.y+User_Rect.h-FontHeight(Menu_BFont), "Rescaling...");

  // just to be sure, reset the size of the graphics
  ReInitPictures();

  //--------------------
  // now to the resizing of all combat elements
  // and the corresponding rectangle entries...

  // SDL_SetColorKey( ne_blocks , 0 , 0 );

  if (SDL_SetColorKey(ne_blocks, SDL_SRCCOLORKEY, 0x00000100 ) == -1 )
    {
      fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
	       SDL_GetError());
      Terminate(ERR);
    }

  tmp=zoomSurface( ne_blocks , ResizeFactor , ResizeFactor , 0 );
  SDL_FreeSurface( ne_blocks );
  ne_blocks = SDL_DisplayFormat(tmp);  /* the surface is copied !*/
  if (ne_blocks == NULL) 
    {
      DebugPrintf (1, "\nSDL_DisplayFormat() has failed: %s\n", SDL_GetError());
      Terminate(ERR);
    }
  SDL_FreeSurface( tmp );
  /* set the transparent color */
  if (SDL_SetColorKey(ne_blocks, SDL_SRCCOLORKEY, ne_transp_key) == -1 )
    {
      fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
	       SDL_GetError());
      Terminate(ERR);
    }

  for (i=0; i< NUM_MAP_BLOCKS ; i++)
    {
      ne_map_block[i].x *= ResizeFactor;
      ne_map_block[i].y *= ResizeFactor;
      ne_map_block[i].w *= ResizeFactor;
      ne_map_block[i].h *= ResizeFactor;
    }

  for (i=0; i< DROID_PHASES ; i++)
    {
      ne_influ_block[i].x *= ResizeFactor;
      ne_influ_block[i].y *= ResizeFactor;
      ne_influ_block[i].w *= ResizeFactor;
      ne_influ_block[i].h *= ResizeFactor;
    }

  for (i=0; i< DROID_PHASES ; i++)
    {
      ne_droid_block[i].x *= ResizeFactor;
      ne_droid_block[i].y *= ResizeFactor;
      ne_droid_block[i].w *= ResizeFactor;
      ne_droid_block[i].h *= ResizeFactor;
    }

  /*
  for (i=0; i < Number_Of_Bullet_Types ; i++)
    for (j=0; j < Bulletmap[i].phases; j++)
      {
	// Bulletmap[i].block[j].x *= ResizeFactor;
	// Bulletmap[i].block[j].y *= ResizeFactor;
	// Bulletmap[i].block[j].w *= ResizeFactor; 
	// Bulletmap[i].block[j].h *= ResizeFactor;
      }
  */

  for (i=0; i < ALLBLASTTYPES; i++)
    for (j=0; j < Blastmap[i].phases; j++)
      {
	Blastmap[i].block[j].x *= ResizeFactor;
	Blastmap[i].block[j].y *= ResizeFactor;
	Blastmap[i].block[j].w *= ResizeFactor; 
	Blastmap[i].block[j].h *= ResizeFactor;
      }

  for (i=0; i< DIGITNUMBER ; i++)
    {
      ne_digit_block[i].x *= ResizeFactor;
      ne_digit_block[i].y *= ResizeFactor;
      ne_digit_block[i].w *= ResizeFactor;
      ne_digit_block[i].h *= ResizeFactor;
    }

  Block_Width *= ResizeFactor;
  Block_Height *= ResizeFactor;

  Digit_Length *= ResizeFactor;
  Digit_Height *= ResizeFactor;

  First_Digit_Pos_X *= ResizeFactor;
  First_Digit_Pos_Y *= ResizeFactor;
  Second_Digit_Pos_X *= ResizeFactor;
  Second_Digit_Pos_Y *= ResizeFactor;
  Third_Digit_Pos_X *= ResizeFactor;
  Third_Digit_Pos_Y *= ResizeFactor;

  // SDL_SaveBMP ( tmp, "../graphics/debugSmall.bmp");

} // void SetCombatScaleTo(float new_scale);

/*
----------------------------------------------------------------------
----------------------------------------------------------------------
*/
void 
LoadThemeConfigurationFile(void)
{
  char *Data;
  char *ReadPointer;
  char *fpath;
  char *EndOfThemesBulletData;
  char *EndOfThemesBlastData;
  char *EndOfThemesDigitData;
  int BulletIndex;
  
#define END_OF_THEME_DATA_STRING "**** End of theme data section ****"
#define END_OF_THEME_BLAST_DATA_STRING "*** End of themes blast data section ***" 
#define END_OF_THEME_BULLET_DATA_STRING "*** End of themes bullet data section ***" 
#define END_OF_THEME_DIGIT_DATA_STRING "*** End of themes digit data section ***" 

  fpath = find_file ("config.theme", GRAPHICS_DIR, TRUE);

  Data = ReadAndMallocAndTerminateFile( fpath , END_OF_THEME_DATA_STRING ) ;

  EndOfThemesBulletData = LocateStringInData ( Data , END_OF_THEME_BULLET_DATA_STRING );
  EndOfThemesBlastData  = LocateStringInData ( Data , END_OF_THEME_BLAST_DATA_STRING  );
  EndOfThemesDigitData  = LocateStringInData ( Data , END_OF_THEME_DIGIT_DATA_STRING  );

  //--------------------
  // Now the file is read in entirely and
  // we can start to analyze its content, 
  //
#define BLAST_ONE_NUMBER_OF_PHASES_STRING "How many phases in Blast one :"
#define BLAST_TWO_NUMBER_OF_PHASES_STRING "How many phases in Blast two :"

  ReadValueFromString( Data , BLAST_ONE_NUMBER_OF_PHASES_STRING , "%d" , 
		       &Blastmap[0].phases , EndOfThemesBlastData );

  ReadValueFromString( Data , BLAST_TWO_NUMBER_OF_PHASES_STRING , "%d" , 
		       &Blastmap[1].phases , EndOfThemesBlastData );

  //--------------------
  // Now we read in the total time amount for each animation
  //
#define BLAST_ONE_TOTAL_AMOUNT_OF_TIME_STRING "Time in seconds for the hole animation of blast one :"
#define BLAST_TWO_TOTAL_AMOUNT_OF_TIME_STRING "Time in seconds for the hole animation of blast two :"

  ReadValueFromString( Data , BLAST_ONE_TOTAL_AMOUNT_OF_TIME_STRING , "%lf" , 
		       &Blastmap[0].total_animation_time , EndOfThemesBlastData );

  ReadValueFromString( Data , BLAST_TWO_TOTAL_AMOUNT_OF_TIME_STRING , "%lf" , 
		       &Blastmap[1].total_animation_time , EndOfThemesBlastData );

  //--------------------
  // Next we read in the number of phases that are to be used for each bullet type
  ReadPointer = Data ;
  while ( ( ReadPointer = strstr ( ReadPointer , "For Bullettype Nr.=" ) ) != NULL )
    {
      ReadValueFromString( ReadPointer , "For Bullettype Nr.=" , "%d" , &BulletIndex , EndOfThemesBulletData );
      if ( BulletIndex >= Number_Of_Bullet_Types )
	{
	  fprintf(stderr, "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
In function 'char* LoadThemeConfigurationFile ( ... ):\n\
\n\
There was a specification for the number of phases in a bullet type\n\
that does not at all exist in the ruleset.\n\
\n\
This might indicate that either the ruleset file is corrupt or the \n\
theme.config configuration file is corrupt or (less likely) that there\n\
is a severe bug in the reading function.\n\
\n\
Please check that your theme and ruleset files are properly set up.\n\
\n\
Please also don't forget, that you might have to run 'make install'\n\
again after you've made modifications to the data files in the source tree.\n\
\n\
Freedroid will terminate now to draw attention to the data problem it could\n\
not resolve.... Sorry, if that interrupts a major game of yours.....\n\
----------------------------------------------------------------------\n\
\n" );
	  Terminate(ERR);
	}
      ReadValueFromString( ReadPointer , "we will use number of phases=" , "%d" , 
			   &Bulletmap[BulletIndex].phases , EndOfThemesBulletData );
      ReadValueFromString( ReadPointer , "and number of phase changes per second=" , "%lf" , 
			   &Bulletmap[BulletIndex].phase_changes_per_second , EndOfThemesBulletData );
      ReadPointer++;
    }
  
  // --------------------
  // Also decidable from the theme is where in the robot to
  // display the digits.  This must also be read from the configuration
  // file of the theme
  //
#define DIGIT_ONE_POSITION_X_STRING "First digit x :"
#define DIGIT_ONE_POSITION_Y_STRING "First digit y :"
#define DIGIT_TWO_POSITION_X_STRING "Second digit x :"
#define DIGIT_TWO_POSITION_Y_STRING "Second digit y :"
#define DIGIT_THREE_POSITION_X_STRING "Third digit x :"
#define DIGIT_THREE_POSITION_Y_STRING "Third digit y :"

  ReadValueFromString( Data , DIGIT_ONE_POSITION_X_STRING , "%d" , 
		       &First_Digit_Pos_X , EndOfThemesDigitData );
  ReadValueFromString( Data , DIGIT_ONE_POSITION_Y_STRING , "%d" , 
		       &First_Digit_Pos_Y , EndOfThemesDigitData );

  ReadValueFromString( Data , DIGIT_TWO_POSITION_X_STRING , "%d" , 
		       &Second_Digit_Pos_X , EndOfThemesDigitData );
  ReadValueFromString( Data , DIGIT_TWO_POSITION_Y_STRING , "%d" , 
		       &Second_Digit_Pos_Y , EndOfThemesDigitData );

  ReadValueFromString( Data , DIGIT_THREE_POSITION_X_STRING , "%d" , 
		       &Third_Digit_Pos_X , EndOfThemesDigitData );
  ReadValueFromString( Data , DIGIT_THREE_POSITION_Y_STRING , "%d" , 
		       &Third_Digit_Pos_Y , EndOfThemesDigitData );

}; // void LoadThemeConfigurationFile ( void )

/*
----------------------------------------------------------------------
This function loads the Blast image and decodes it into the multiple
small Blast surfaces.
----------------------------------------------------------------------
*/
void 
Load_Blast_Surfaces( void )
{
  SDL_Surface* Whole_Image;
  SDL_Surface* tmp_surf;
  SDL_Rect Source;
  SDL_Rect Target;
  int i;
  int j;
  char *fpath;

  fpath = find_file (NE_BLAST_BLOCK_FILE, GRAPHICS_DIR, TRUE);

  Whole_Image = IMG_Load( fpath ); // This is a surface with alpha channel, since the picture is one of this type
  SDL_SetAlpha( Whole_Image , 0 , SDL_ALPHA_OPAQUE );

  for ( i=0 ; i < ALLBLASTTYPES ; i++ )
    {
      for ( j=0 ; j < Blastmap[i].phases ; j++ )
	{
	  tmp_surf = SDL_CreateRGBSurface( 0 , Block_Width, Block_Height, ne_bpp, 0, 0, 0, 0);
	  SDL_SetColorKey( tmp_surf , 0 , 0 ); // this should clear any color key in the source surface
	  Blastmap[i].SurfacePointer[j] = SDL_DisplayFormatAlpha( tmp_surf ); // now we have an alpha-surf of right size
	  SDL_SetColorKey( Blastmap[i].SurfacePointer[j] , 0 , 0 ); // this should clear any color key in the dest surface
	  // Now we can copy the image Information
	  Source.x=j*(Block_Height+2);
	  Source.y=i*(Block_Width+2);
	  Source.w=Block_Width;
	  Source.h=Block_Height;
	  Target.x=0;
	  Target.y=0;
	  Target.w=Block_Width;
	  Target.h=Block_Height;
	  SDL_BlitSurface ( Whole_Image , &Source , Blastmap[i].SurfacePointer[j] , &Target );
	  SDL_SetAlpha( Blastmap[i].SurfacePointer[j] , SDL_SRCALPHA , SDL_ALPHA_OPAQUE );
	}
    }

  SDL_FreeSurface( tmp_surf );

}; // void Load_Blast_Surfaces( void )


/*
----------------------------------------------------------------------
This function loads the Blast image and decodes it into the multiple
small Blast surfaces.
----------------------------------------------------------------------
*/
void 
Load_Bullet_Surfaces( void )
{
  SDL_Surface* Whole_Image;
  SDL_Surface* tmp_surf;
  SDL_Rect Source;
  SDL_Rect Target;
  int i;
  int j;
  char *fpath;

  fpath = find_file (NE_BULLET_BLOCK_FILE, GRAPHICS_DIR, TRUE);

  Whole_Image = IMG_Load( fpath ); // This is a surface with alpha channel, since the picture is one of this type
  SDL_SetAlpha( Whole_Image , 0 , SDL_ALPHA_OPAQUE );

  for ( i=0 ; i < Number_Of_Bullet_Types ; i++ )
    {
      for ( j=0 ; j < Bulletmap[i].phases ; j++ )
	{
	  tmp_surf = SDL_CreateRGBSurface( 0 , Block_Width, Block_Height, ne_bpp, 0, 0, 0, 0);
	  SDL_SetColorKey( tmp_surf , 0 , 0 ); // this should clear any color key in the source surface
	  Bulletmap[i].SurfacePointer[j] = SDL_DisplayFormatAlpha( tmp_surf ); // now we have an alpha-surf of right size
	  SDL_SetColorKey( Bulletmap[i].SurfacePointer[j] , 0 , 0 ); // this should clear any color key in the dest surface
	  // Now we can copy the image Information
	  Source.x=j*(Block_Height+2);
	  Source.y=i*(Block_Width+2);
	  Source.w=Block_Width;
	  Source.h=Block_Height;
	  Target.x=0;
	  Target.y=0;
	  Target.w=Block_Width;
	  Target.h=Block_Height;
	  SDL_BlitSurface ( Whole_Image , &Source , Bulletmap[i].SurfacePointer[j] , &Target );
	  SDL_SetAlpha( Bulletmap[i].SurfacePointer[j] , SDL_SRCALPHA , SDL_ALPHA_OPAQUE );
	}
    }

  SDL_FreeSurface( tmp_surf );

}; // void Load_Bullet_Surfaces( void )


/* 
----------------------------------------------------------------------
----------------------------------------------------------------------
*/
void 
Load_Enemy_Surfaces( void )
{
  SDL_Surface* Whole_Image;
  SDL_Surface* tmp_surf;
  SDL_Rect Source;
  SDL_Rect Target;
  int i;
  char *fpath;

  fpath = find_file ( NE_DROID_BLOCK_FILE , GRAPHICS_DIR, TRUE);

  Whole_Image = IMG_Load( fpath ); // This is a surface with alpha channel, since the picture is one of this type
  SDL_SetAlpha( Whole_Image , 0 , SDL_ALPHA_OPAQUE );

  for ( i=0 ; i < DROID_PHASES ; i++ )
    {
      tmp_surf = SDL_CreateRGBSurface( 0 , Block_Width, Block_Height, ne_bpp, 0, 0, 0, 0);
      SDL_SetColorKey( tmp_surf , 0 , 0 ); // this should clear any color key in the source surface
      EnemySurfacePointer[i] = SDL_DisplayFormatAlpha( tmp_surf ); // now we have an alpha-surf of right size
      SDL_SetColorKey( EnemySurfacePointer[i] , 0 , 0 ); // this should clear any color key in the dest surface
      // Now we can copy the image Information
      Source.x=i*(Block_Height+2);
      Source.y=1*(Block_Width+2);
      Source.w=Block_Width;
      Source.h=Block_Height;
      Target.x=0;
      Target.y=0;
      Target.w=Block_Width;
      Target.h=Block_Height;
      SDL_BlitSurface ( Whole_Image , &Source , EnemySurfacePointer[i] , &Target );
      SDL_SetAlpha( EnemySurfacePointer[i] , SDL_SRCALPHA , SDL_ALPHA_OPAQUE );
    }

  SDL_FreeSurface( tmp_surf );

}; // void LoadEnemySurfaces( void )


/* 
----------------------------------------------------------------------
----------------------------------------------------------------------
*/
void 
Load_Influencer_Surfaces( void )
{
  SDL_Surface* Whole_Image;
  SDL_Surface* tmp_surf;
  SDL_Rect Source;
  SDL_Rect Target;
  int i;
  char *fpath;

  fpath = find_file ( NE_DROID_BLOCK_FILE , GRAPHICS_DIR, TRUE);

  Whole_Image = IMG_Load( fpath ); // This is a surface with alpha channel, since the picture is one of this type
  SDL_SetAlpha( Whole_Image , 0 , SDL_ALPHA_OPAQUE );

  for ( i=0 ; i < DROID_PHASES ; i++ )
    {
      tmp_surf = SDL_CreateRGBSurface( 0 , Block_Width, Block_Height, ne_bpp, 0, 0, 0, 0);
      SDL_SetColorKey( tmp_surf , 0 , 0 ); // this should clear any color key in the source surface
      InfluencerSurfacePointer[i] = SDL_DisplayFormatAlpha( tmp_surf ); // now we have an alpha-surf of right size
      SDL_SetColorKey( InfluencerSurfacePointer[i] , 0 , 0 ); // this should clear any color key in the dest surface
      // Now we can copy the image Information
      Source.x=i*(Block_Height+2);
      Source.y=0*(Block_Width+2);
      Source.w=Block_Width;
      Source.h=Block_Height;
      Target.x=0;
      Target.y=0;
      Target.w=Block_Width;
      Target.h=Block_Height;
      SDL_BlitSurface ( Whole_Image , &Source , InfluencerSurfacePointer[i] , &Target );
      SDL_SetAlpha( InfluencerSurfacePointer[i] , SDL_SRCALPHA , SDL_ALPHA_OPAQUE );
    }

  SDL_FreeSurface( tmp_surf );

}; // void Load_Influencer_Surfaces( void )

/* 
----------------------------------------------------------------------
----------------------------------------------------------------------
*/
void 
Load_Digit_Surfaces( void )
{
  SDL_Surface* Whole_Image;
  SDL_Surface* tmp_surf;
  SDL_Rect Source;
  SDL_Rect Target;
  int i;
  char *fpath;

  fpath = find_file ( NE_DIGIT_BLOCK_FILE , GRAPHICS_DIR, TRUE);

  Whole_Image = IMG_Load( fpath ); // This is a surface with alpha channel, since the picture is one of this type
  SDL_SetAlpha( Whole_Image , 0 , SDL_ALPHA_OPAQUE );

  for ( i=0 ; i < DIGITNUMBER ; i++ )
    {
      tmp_surf = SDL_CreateRGBSurface( 0 , INITIAL_DIGIT_LENGTH , INITIAL_DIGIT_HEIGHT, ne_bpp, 0, 0, 0, 0);
      SDL_SetColorKey( tmp_surf , 0 , 0 ); // this should clear any color key in the source surface
      InfluDigitSurfacePointer[i] = SDL_DisplayFormat( tmp_surf ); // now we have an alpha-surf of right size
      // SDL_SetColorKey( InfluDigitSurfacePointer[i] , 0 , 0 ); // this should clear any color key in the dest surface
      // Now we can copy the image Information
      Source.x=i*( INITIAL_DIGIT_LENGTH + 2 );
      Source.y=0*( INITIAL_DIGIT_HEIGHT + 2);
      Source.w=INITIAL_DIGIT_LENGTH;
      Source.h=INITIAL_DIGIT_HEIGHT;
      Target.x=0;
      Target.y=0;
      Target.w=INITIAL_DIGIT_LENGTH;
      Target.h=INITIAL_DIGIT_HEIGHT;
      SDL_BlitSurface ( Whole_Image , &Source , InfluDigitSurfacePointer[i] , &Target );
      SDL_SetAlpha( InfluDigitSurfacePointer[i] , 0 , SDL_ALPHA_OPAQUE );
      if ( SDL_SetColorKey( InfluDigitSurfacePointer[i] , SDL_SRCCOLORKEY, ne_transp_key ) == -1 )
	{
	  fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
		   SDL_GetError());
	  Terminate( ERR );
	}
    }
  SDL_FreeSurface( tmp_surf );

  for ( i=0 ; i < DIGITNUMBER ; i++ )
    {
      tmp_surf = SDL_CreateRGBSurface( 0 , INITIAL_DIGIT_LENGTH , INITIAL_DIGIT_HEIGHT, ne_bpp, 0, 0, 0, 0);
      SDL_SetColorKey( tmp_surf , 0 , 0 ); // this should clear any color key in the source surface
      EnemyDigitSurfacePointer[i] = SDL_DisplayFormat( tmp_surf ); // now we have an alpha-surf of right size
      // SDL_SetColorKey( EnemyDigitSurfacePointer[i] , 0 , 0 ); // this should clear any color key in the dest surface
      // Now we can copy the image Information
      Source.x=(i+10)*( INITIAL_DIGIT_LENGTH + 2 );
      Source.y=0*( INITIAL_DIGIT_HEIGHT + 2);
      Source.w=INITIAL_DIGIT_LENGTH;
      Source.h=INITIAL_DIGIT_HEIGHT;
      Target.x=0;
      Target.y=0;
      Target.w=INITIAL_DIGIT_LENGTH;
      Target.h=INITIAL_DIGIT_HEIGHT;
      SDL_BlitSurface ( Whole_Image , &Source , EnemyDigitSurfacePointer[i] , &Target );
      SDL_SetAlpha( EnemyDigitSurfacePointer[i] , 0 , SDL_ALPHA_OPAQUE );
      if ( SDL_SetColorKey( EnemyDigitSurfacePointer[i] , SDL_SRCCOLORKEY, ne_transp_key ) == -1 )
	{
	  fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
		   SDL_GetError());
	  Terminate( ERR );
	}
    }
  SDL_FreeSurface( tmp_surf );

}; // void Load_Digit_Surfaces( void )

/*-----------------------------------------------------------------
 * @Desc: get the pics for: druids, bullets, blasts
 * 				
 * 	reads all blocks and puts the right pointers into
 * 	the various structs
 *
 * @Ret: TRUE/FALSE
 *
 *-----------------------------------------------------------------*/
int
InitPictures (void)
{
  SDL_Surface *tmp;
  SDL_Surface *tmp2;
  int block_line = 0;   /* keep track of line in ne_blocks we're writing */
  char *fpath;

  Block_Width=INITIAL_BLOCK_WIDTH;
  Block_Height=INITIAL_BLOCK_HEIGHT;
  
  // Loading all these pictures might take a while...
  // and we do not want do deal with huge frametimes, which
  // could box the influencer out of the ship....
  Activate_Conservative_Frame_Computation();

  // In the following we will be reading in image information.  But the number
  // of images to read in and the way they are displayed might be strongly dependant
  // on the theme.  That is not at all a problem.  We just got to read in the
  // theme configuration file again.  After that is done, the following reading
  // commands will do the right thing...
  LoadThemeConfigurationFile();

  /* 
     create the internal storage for all our blocks 
  */
  tmp = SDL_CreateRGBSurface( 0 , NUM_MAP_BLOCKS*Block_Width,
			      18*Block_Height, ne_bpp, 0, 0, 0, 0);
  tmp2 = SDL_CreateRGBSurface(0, SCREENLEN, SCREENHEIGHT, ne_bpp, 0, 0, 0, 0);
  if ( (tmp == NULL) || (tmp2 == NULL) )
    {
      DebugPrintf (1, "\nCould not create ne_blocks surface: %s\n", SDL_GetError());
      return (FALSE);
    }

  /* 
   * convert this to display format for fast blitting 
   */
  ne_blocks = SDL_DisplayFormat(tmp);  /* the surface is copied !*/
  if (ne_blocks == NULL) 
    {
      DebugPrintf (1, "\nSDL_DisplayFormat() has failed: %s\n", SDL_GetError());
      return (FALSE);
    }

  ne_static = SDL_DisplayFormat(tmp2);  /* the second surface is copied !*/
  if (ne_static == NULL) 
    {
      DebugPrintf (1, "\nSDL_DisplayFormat() has failed: %s\n", SDL_GetError());
      return (FALSE);
    }
  SDL_FreeSurface (tmp); /* and free the old one */

  /* set the transparent color */
  /*
  if (SDL_SetColorKey(ne_blocks, SDL_SRCCOLORKEY, ne_transp_key) == -1 )
    {
      fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
	       SDL_GetError());
      return (FALSE);
    }
  */
  if (SDL_SetColorKey(ne_blocks, 0 , 0 ) == -1 )
    {
      fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
	       SDL_GetError());
      return (FALSE);
    }


  if (SDL_SetColorKey(ne_static, SDL_SRCCOLORKEY, ne_transp_key) == -1 )
    {
      fprintf (stderr, "Transp setting by SDL_SetColorKey() failed: %s \n",
	       SDL_GetError());
      return (FALSE);
    }

  /* 
   * and now read in the blocks from various files into ne_blocks
   * and initialise the block-coordinates 
   */

  /*
  fpath =  find_file (NE_MAP_BLOCK_FILE, GRAPHICS_DIR, TRUE);
  ne_map_block =
    ne_get_blocks (fpath , NUM_MAP_BLOCKS, 9, 0, block_line++);
  */

  if ( CurLevel == NULL )
    {
      fpath =  find_file (NE_MAP_BLOCK_FILE, GRAPHICS_DIR, TRUE);
      ne_map_block =
	ne_get_blocks (fpath , NUM_MAP_BLOCKS, 9, 0, block_line++);
    }
  else 
    {
      SetLevelColor ( CurLevel->color );
      block_line++;
    }

  DebugPrintf( 2 , "\nvoid InitPictures(void): preparing to load droids." );

  Load_Influencer_Surfaces();
  Load_Enemy_Surfaces();

  DebugPrintf( 2 , "\nvoid InitPictures(void): preparing to load bullet file." );
  DebugPrintf( 1 , "\nvoid InitPictures(void): Number_Of_Bullet_Types : %d." , Number_Of_Bullet_Types );

  Load_Bullet_Surfaces();

  DebugPrintf( 2 , "\nvoid InitPictures(void): preparing to load blast image file." );

  Load_Blast_Surfaces();

  DebugPrintf( 2 , "\nvoid InitPictures(void): preparing to load digits image file." );

  fpath = find_file (NE_DIGIT_BLOCK_FILE, GRAPHICS_DIR, TRUE);

  Load_Digit_Surfaces();

  //   ne_digit_block =    ne_get_digit_blocks ( fpath , DIGITNUMBER, DIGITNUMBER, 0, block_line++);

  fpath = find_file (NE_BANNER_BLOCK_FILE, GRAPHICS_DIR, FALSE);
  ne_rahmen_block = ne_get_rahmen_block (fpath);
  
  // console picture need not be rendered fast or something.  This
  // really has time, so we load it as a surface and do not take the
  // elements apart (they dont have typical block format either)
  fpath = find_file (NE_CONSOLEN_PIC_FILE, GRAPHICS_DIR, FALSE);
  ne_console_surface= IMG_Load (fpath); 

  // ne_blocks = SDL_DisplayFormat( ne_blocks );  /* the surface is copied !*/
  
  return (TRUE);
}  // InitPictures

void PlusDrawEnergyBar (void);

/*@Function============================================================
@Desc: 

@Ret: 
@Int:
* $Function----------------------------------------------------------*/
void
SetColors (int FirstCol, int PalAnz, char *PalPtr)
{
  char *MyPalPtr;
  int i;

  MyPalPtr = PalPtr;

  for (i = FirstCol; i < FirstCol + PalAnz; i++)
    {
      SetPalCol(i, MyPalPtr[0], MyPalPtr[1], MyPalPtr[2]);
      MyPalPtr += 3;
    }
}				// void SetColors(...)

/*@Function============================================================
@Desc: 

 @Ret: 
@Int:
* $Function----------------------------------------------------------*/
void
SetLevelColor (int ColorEntry)
{
  char *fpath;

  char *ColoredBlockFiles[] = {
    "ne_block_red.gif",
    "ne_block_yellow.gif",
    "ne_block_green.gif",
    "ne_block_gray.gif",
    "ne_block_blue.gif",
    "ne_block_turquoise.gif",
    "ne_block_dark.gif",
    NULL
  };

  fpath = find_file (ColoredBlockFiles[ColorEntry], GRAPHICS_DIR, TRUE);
  ne_map_block = ne_get_blocks (fpath, NUM_MAP_BLOCKS, 9, 0, 0);

  
} // void SetLevelColor(int ColorEntry)


/*-----------------------------------------------------------------
 * Initialise the Video display and graphics engine
 *
 *
 *-----------------------------------------------------------------*/
void
Init_Video (void)
{
  const SDL_VideoInfo *vid_info;
  SDL_Rect **vid_modes;
  char vid_driver[81];
  Uint32 flags;  /* flags for SDL video mode */
  char *fpath;

  /* Initialize the SDL library */
  // if ( SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1 ) 

  if ( SDL_Init (SDL_INIT_VIDEO) == -1 ) 
    {
      fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
      Terminate(ERR);
    } else
      DebugPrintf(1, "\nSDL Video initialisation successful.\n");

  // Now SDL_TIMER is initialized here:

  if ( SDL_InitSubSystem ( SDL_INIT_TIMER ) == -1 ) 
    {
      fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
      Terminate(ERR);
    } else
      DebugPrintf(1, "\nSDL Timer initialisation successful.\n");

  /* clean up on exit */
  atexit (SDL_Quit);

  //--------------------
  // Now we initialize the fonts needed by BFont functions
  fpath = find_file (MENU_FONT_FILE, GRAPHICS_DIR, FALSE);
  if ( ( Menu_BFont = LoadFont (fpath) ) == NULL )
      {
      fprintf (stderr,
	     "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
A font file named %s it wanted to load was not found.\n\
\n\
Please check that the file is present and not corrupted\n\
in your distribution of Freedroid.\n\
\n\
Freedroid will terminate now to point at the error.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , MENU_FONT_FILE );
        Terminate(ERR);
  } else
  DebugPrintf(1, "\nSDL Menu Font initialisation successful.\n");
  
  fpath = find_file (PARA_FONT_FILE, GRAPHICS_DIR, FALSE);
  if ( ( Para_BFont = LoadFont (fpath) ) == NULL )
    {
      fprintf (stderr,
	     "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
A font file named %s it wanted to load was not found.\n\
\n\
Please check that the file is present and not corrupted\n\
in your distribution of Freedroid.\n\
\n\
Freedroid will terminate now to point at the error.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , PARA_FONT_FILE );
      Terminate(ERR);
    } else
      DebugPrintf(1, "\nSDL Para Font initialisation successful.\n");

  fpath = find_file (FPS_FONT_FILE, GRAPHICS_DIR, FALSE);
  if ( ( FPS_Display_BFont = LoadFont (fpath) ) == NULL )
    {
      fprintf (stderr,
	     "\n\
\n\
----------------------------------------------------------------------\n\
Freedroid has encountered a problem:\n\
A font file named %s it wanted to load was not found.\n\
\n\
Please check that the file is present and not corrupted\n\
in your distribution of Freedroid.\n\
\n\
Freedroid will terminate now to point at the error.\n\
Sorry...\n\
----------------------------------------------------------------------\n\
\n" , FPS_FONT_FILE );
      Terminate(ERR);
    } else
      DebugPrintf(1, "\nSDL FPS Display Font initialisation successful.\n");

  //  SetCurrentFont(Menu_BFont);

  vid_info = SDL_GetVideoInfo (); /* just curious */
  SDL_VideoDriverName (vid_driver, 80);
  
  flags = SDL_SWSURFACE | SDL_HWPALETTE ;
  if (fullscreen_on) flags |= SDL_FULLSCREEN;

  vid_modes = SDL_ListModes (NULL, SDL_SWSURFACE);

  if (vid_info->wm_available)  /* if there's a window-manager */
    {
      SDL_WM_SetCaption ("Freedroid", "");
      fpath = find_file (ICON_FILE, GRAPHICS_DIR, FALSE);
      SDL_WM_SetIcon( IMG_Load (fpath), NULL);
    }


  /* 
   * currently only the simple 320x200 mode is supported for 
   * simplicity, as all our graphics are in this format
   * once this is up and running, we'll provide others modes
   * as well.
   */
  ne_bpp = 16; /* start with the simplest */

  #define SCALE_FACTOR 2

  if( !(ne_screen = SDL_SetVideoMode ( SCREENLEN, SCREENHEIGHT , 0 , flags)) )
    {
      fprintf(stderr, "Couldn't set (2*) 320x240*SCALE_FACTOR video mode: %s\n",
	      SDL_GetError()); 
      exit(-1);
    }

  if (!mouse_control)  /* hide mouse pointer if not needed */
    SDL_ShowCursor (SDL_DISABLE);

  ne_vid_info = SDL_GetVideoInfo (); /* info about current video mode */
  /* RGB of transparent color in our pics */
  ne_transp_rgb.rot   = 199; 
  ne_transp_rgb.gruen =  43; 
  ne_transp_rgb.blau  =  43; 
  /* and corresponding key: */
  ne_transp_key = SDL_MapRGB(ne_screen->format, ne_transp_rgb.rot,
			     ne_transp_rgb.gruen, ne_transp_rgb.blau);

  SDL_SetGamma( 1 , 1 , 1 );
  GameConfig.Current_Gamma_Correction=1;

  return;

} /* InitVideo () */

/*@Function============================================================
@Desc: 

@Ret: 
@Int:
* $Function----------------------------------------------------------*/
void
UnfadeLevel (void)
{

  SetLevelColor( CurLevel->color );
}				// void UnfadeLevel(void)

/* *********************************************************************** */

void
LadeZeichensatz (char *Zeichensatzname)
{
  char *fpath;
/*
  Diese Prozedur laedt einen Zeichensatz in den Standardzeichensatzbereich
  und verwendet dazu das BIOS.

  Parameter sind ein Zeiger auf den Zeichensatznamen
  Returnwert: keiner
*/

  /* lokale Variablen der Funktion */
  unsigned char *Zeichensatzpointer;
  FILE *CharDateiHandle;
  int i, j, k;

  DebugPrintf (1, "\nLadeZeichensatz() called... is that not obsolete?\n");

  /* Speicher fuer die zu ladende Datei reservieren */
  Zeichensatzpointer = MyMalloc (256 * 8 + 10);

  /* Datei in den Speicher laden */
  fpath = find_file (Zeichensatzname, GRAPHICS_DIR, FALSE);
  if ((CharDateiHandle = fopen (fpath, "rb")) == NULL)
    {
      DebugPrintf (1, "\nvoid LadeZeichensatz(char* Zeichensatzname):  Konnte die Datei %s nicht oeffnen !\n",
	 Zeichensatzname);
      getchar ();
      Terminate (-1);
    }
  fread (Zeichensatzpointer, 1, 30000, CharDateiHandle);
  if (fclose (CharDateiHandle) == EOF)
    {
      DebugPrintf (1, "\nvoid LadeZeichensatz(char* Zeichensatzname): Konnte die Datei %s nicht schlie3en !\n",
	 Zeichensatzname);
      getchar ();
      Terminate (-1);
    }

  /* Eventuell Report erstatten das der Zeichensatz installiert ist */
#ifdef REPORTDEBUG
  DebugPrintf
    ("\nvoid LadeZeichensatz(char* Zeichensatzname): Der Zeichensatz ist installiert ! ");
#endif

  if (Data70Pointer)
    {
      DebugPrintf (2, " Der Zeichensatz war schon installiert !.\n");
      getchar ();
      Terminate (-1);
    }
  Data70Pointer = malloc (256 * 8 * 8 + 10);
  for (i = 0; i < 256; i++)
    {
      for (j = 0; j < 8; j++)
	{
	  for (k = 0; k < 8; k++)
	    {
	      if (((int) Zeichensatzpointer[i * 8 + j]) & (1 << (7 - k)))
		Data70Pointer[i * 8 * 8 + j * 8 + k] = DATA70FONTCOLOR;
	      else
		Data70Pointer[i * 8 * 8 + j * 8 + k] = DATA70BGCOLOR;
	    }
	}
    }
}  // void LadeZeichensatz(char* Zeichensatzname)

/*@Function============================================================
@Desc: 

@Ret: 
@Int:
* $Function----------------------------------------------------------*/
void
LevelGrauFaerben (void)
{
  SetLevelColor (PD_DARK);
  Activate_Conservative_Frame_Computation();  // to prevent a "jump" when the level turns gray
                                               // observed and reported by rp.
}


/*@Function============================================================
@Desc: 

@Ret: 
@Int:
* $Function----------------------------------------------------------*/
void
ClearGraphMem ( void )
{
  // One this function is done, the rahmen at the
  // top of the screen surely is destroyed.  We inform the
  // DisplayBanner function of the matter...
  BannerIsDestroyed=TRUE;

  // 
  SDL_SetClipRect( ne_screen, NULL );

  // Now we fill the screen with black color...
  SDL_FillRect( ne_screen , NULL , 0 );
} // ClearGraphMem( void )


/*@Function============================================================
@Desc: This function sets a selected colorregister to a specified
		RGB value

@Ret: 
@Int:
* $Function----------------------------------------------------------*/
void
SetPalCol (unsigned int palpos, unsigned char rot, unsigned char gruen,
	   unsigned char blau)
{
  SDL_Color ThisOneColor;

  ThisOneColor.r=rot;
  ThisOneColor.g=gruen;
  ThisOneColor.b=blau;
  ThisOneColor.unused=0;

  return;

  // DebugPrintf (2, "\nvoid SetPalCol(...): Real function called.");
  // vga_setpalette (palpos, rot, gruen, blau);

  SDL_SetColors( ne_screen , &ThisOneColor, palpos, 1 );
  // SDL_SetColors( ne_blocks , &ThisOneColor, palpos, 1 );

  // SDL_SetColors( screen , &ThisOneColor, palpos, 1 );
  // DebugPrintf (2, "\nvoid SetPalCol(...): Usual end of function reached.");
}				// void SetPalCol(...)


/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
Uint32 
getpixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
  
  switch(bpp) 
    {
    case 1:
      return *p;
      
    case 2:
      return *(Uint16 *)p;
      
    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
	return p[0] << 16 | p[1] << 8 | p[2];
      else
	return p[0] | p[1] << 8 | p[2] << 16;
      
    case 4:
      return *(Uint32 *)p;
      
    default:
      return 0;       /* shouldn't happen, but avoids warnings */
    }

} // Uint32 getpixel(...)


/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
} // void putpixel(...)


#undef _graphics_c

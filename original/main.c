/*
Tank Ravagers by Loirak Development

Programming: Ben Rhoades (benr@loirak.com)

Compiler Setup 30 Nov 2001
Programming 1 Dec 2001 (from 9pm-2:30am)
Artwork by Joe McCombs 1996-ish, but hey its a game!

Thanks go out to:
Dovoto : The Pern Project
gbadev.org
devrs.com
Visoly/Lik-Sang!

*/
#include "gba.h"
#include "screenmode.h"
#include "keypad.h"
#include "notedata.h"
#include "saving.h" 
#include "gfx.h"
#include "gfx\logo.h" //header file created by pcx2gba
#include "gfx\tank.h"
#include "gfx\text.h"
#include "gfx\oobjtank.h"
#include "gfx\bobjbomb.h"
#include "gfx\ggrid.h"
#include "gfx\ggrid_syn2.h"
#include "gfx\ggrid_syn3.h"
#include "gfx\ggrid_mixer.h"
#include "gfx\playback_cursor.h"
#include "gfx\palette.h"
#include "gfx\ppiano.h"
#include "gfx\right_scroll_thumb.h"
#include "text.h"
#include "positions.h"
#include "gfx\A-Z.h"
#include "gfx\aaz_white.h"
#include "gfx\aaz_red.h"
#include "gfx\AAZ_terminal_8.h"
#include "sound.h"

//GENERAL
#define NUM_CYCLES_PER_STEP	1
#define CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_KEYPRESSES	       1
#define CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_NON_ACTION_KEYPRESSES	       4 

#define NUM_SEQUENCE_STEPS		16
#define MAX_NUM_SEQUENCE_STEPS	32

#define COLUMN_WIDTH		12
#define ROW_HEIGHT		8

#define GRID_LEFT_OFFSET	32
#define GRID_TOP_OFFSET		28
#define GRID_BOTTOM_OFFSET      7
#define GRID_NUMBER_DOWN	15
#define GRID_NUMBER_ACROSS	16
#define PATTERN_GRID_LEFT 5
#define PATTERN_GRID_TOP 7
#define PATTERN_THUMB_WIDTH 6
#define PATTERN_THUMB_HEIGHT 8
#define MAX_PATTERN_NUMBER 15
#define MAX_PATTERN_LENGTH 64

#define KEYBOARD_START_X	6
#define KEYBOARD_START_Y	27

#define MAX_MIDI_NOTE		100
#define MIN_MIDI_NOTE		30

//RIGHT SCROLLBAR
#define	RIGHT_SCROLL_TOP_Y      36
#define RIGHT_SCROLL_BOTTOM_Y   135
#define RIGHT_SCROLL_STARTS_X	234

//BOTTOM SCROLLBAR
#define	BOTTOM_SCROLL_LEFT_X      22
#define BOTTOM_SCROLL_RIGHT_X   106 
#define BOTTOM_SCROLL_STARTS_Y	150

//PATTERN LENGTH BOX
#define PATTERN_LENGTH_BOX_X	70
#define PATTERN_LENGTH_BOX_Y	12

//SEQUENCE SCREEN POSITIONS
#define TEMPO_X	50
#define TEMPO_Y	10


#define RGB16(r,g,b)  ((r)+(g<<5)+(b<<10))  //this converts a color value to 15 bit BGR value used by GBA		
#define DOUBLE_COLOR(c)	((c)+256*(c))

// some memory location defines
u16* FrontBuffer = (u16*)0x6000000;
u16* BackBuffer = (u16*)0x600A000;
u16* videoBuffer;
volatile u16* ScanlineCounter = (volatile u16*)0x4000006;

u16* paletteMem = (u16*)0x5000000;	//PalMemory is 256 16 bit BGR values starting at 0x5000000

//////////////////Globals/////////////////////////
//extern u16 letterTiles;		//The background tiles (included with data.asm)
//extern u16 letterPalette;	//The background palette (included with data.asm)

//u16 *bgPal = &letterPalette;
//u16 *bgData = &letterTiles;

//function prototypes

void EraseScreen(void);		// erases the screen in mode4
void DoIntro(void);			// display the loirak screen, and sleep for a second or two
void DrawIntroScreenImage(void);		// the titlescreen/menu
void WaitForStart(void);	// waits for the start key to be pressed
void Flip(void);			// flips between the back/front buffer
void WaitForVblank(void);	// waits for the drawer to get to end before flip
void DrawSequenceNote(int notex, int notey);
void DrawBackground(u16 *screenData);
void DrawBackgroundRegion(u16 *screenData,int x1,int y1, int x2, int y2);
void ChangeBackPal(void);							// changes to the star palette
int SaveSong(int nSlot,const int * const (* syn_sequence[MAX_PATTERN_NUMBER][NUM_SEQUENCE_STEPS]));
void DrawCursor(int tankx, int tanky);	// draws the tank on the screen
void DrawSequenceData(int seq[NUM_SEQUENCE_STEPS], int grid_top_offset);
void PlayStepVoice1(int note);
void PlayStepVoice2(int note);
void PlayStepVoice3(int note);
void DrawPalette(void);
void PlayStepVoice4(int note);
void DrawPlaybackHead(int tankx, int tanky);
void DrawCurrentFieldBox(int screen,int field);
void DrawKeyboard(int grid_top_offset);
void DrawStatusBox(char *message);
void DrawRightScrollThumb(int tanky);	
void DrawBottomScrollThumb(int tankx,int sequence_length, int screen_pan_value);	
void DrawPatternBoundingBraces(int position_x,int sequence_length);
void InitSound(void);		// draws you died on bg, wait for start
void DrawText(int topleft_x, int topleft_y, char *message,char *color,int charHeight, int charWidth);
void DrawTextCentered(int stringY,char *message,char *font,int char_height,int char_width);
void DrawFilledRect(int x1,int y1, int x2, int y2, int color);
void Line(int x1, int y1, int x2, int y2, u16 color);
void LoadSong(int *syn1_sequence, int *syn2_sequence, int *syn3_sequence);
void HorizontalBox(int x1, int y1, int x2, int y2, u16 color);
void PlotPixelTransparent(int x,int y, unsigned short int c);	
void PlotPixel(int x,int y, unsigned short int c);	
void InterruptProcess(void) __attribute__ ((section(".iwram"))); //the interrupt handler from crt0.s

int main(void)
{

	SetMode(MODE_4 | BG2_ENABLE ); //set mode 4

        //turn on sound circuit
	REG_SOUNDCNT_X = 0x80;
	REG_SOUNDCNT_H=0x0b0F; //enable DS A&B + fifo reset + use timer0 + max volume to L and R

        InitSound();


		char nStatusText[100];
		char nPatternLengthText[2];
		char nTempoText[3];
		int rflags[20] = {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};		//redraw flags
	    int loop,loop2,loop3;
		int playback_head = -1; // -1 means not play, 0 to 15 means playback position
		int step_cycle_counter = 0; //counts to a certain number of cyles before moving to the next step
       		int screen = 1, field = 1;  //determines the mode of the entire program
       		    	     	      	    //screen = 1, synth 1 screen
       		    	     	      	    //	     field = 1, pattern editor
       		    	     	      	    //	     field = 2, pattern selector
       		    	     	      	    //screen = 2, synth 2 screen
       		    	     	      	    //	     fields, same as screen = 1
       		    	     	      	    //screen = 3, drum kit screen
       		    	     	      	    //	     field = 1, pattern editor
       		    	     	      	    //	     field = 2, pattern selector
       		    	     	      	    //screen = 4, sequence editor/mixer


		//Instrument Specific Variables
       		int syn_sequence[3][MAX_PATTERN_NUMBER][MAX_NUM_SEQUENCE_STEPS]; // status of the sequence 1 on, 0 off
			int sequence_lengths[3][MAX_PATTERN_NUMBER];
			int syn_cursor_x[3] = {0,0,0}, syn_cursor_y[3] = {71,71,71};
			int syn_curpattern[3] = {0,0,0};		//index is voice number
			int syn_grid_top_row_note[3] ={71,71,71};
			int syn_grid_left_position[3] ={0,0,0};
			int nTempo = 120;



		//"Last Cycle" button trackers
		int cycles_since_button_a_was_pressed =0; //we use this to prevent accidental double clicks
		int cycles_since_button_b_was_pressed =0; //we use this to prevent accidental double clicks
		int cycles_since_left_was_pressed=0;
		int cycles_since_right_was_pressed=0;
		int cycles_since_up_was_pressed=0;
		int cycles_since_down_was_pressed=0;
		int cycles_since_start_button_was_pressed=0;
		int num_cycles_until_status_screen_gets_turned_off = 0;

//u16* M4CharMem = (u16*)0x6014000;		//Memory for character graphics in Mode 4 (OAMData part1)

		sprintf(nStatusText,"Welcome");
		num_cycles_until_status_screen_gets_turned_off = 20;
	
		EraseScreen();
		DrawIntroScreenImage();
		// they pressed start so lets play the game, and when they die it will erase
		// the screen and do the menu again

		ChangeBackPal(); // switch to the star palette

		for (loop = 0; loop < MAX_NUM_SEQUENCE_STEPS; loop++)
		{
			syn_sequence[0][syn_curpattern[0]][loop] = -1; // set the status of the sequence_syn1 to off
			syn_sequence[1][syn_curpattern[1]][loop] = -1; // set the status of the sequence_syn1 to off
			syn_sequence[2][syn_curpattern[2]][loop] = -1; // set the status of the sequence_syn1 to off
		}

		for (loop = 0; loop < 3; loop++)
			for(loop2 = 0; loop2 < 16;loop2++)
		{
			sequence_lengths[loop][loop2] = NUM_SEQUENCE_STEPS; // set the status of the sequence_syn1 to off
		}

		syn_sequence[0][syn_curpattern[0]][0] = 71;
		syn_sequence[0][syn_curpattern[0]][0] = 71;
		syn_sequence[0][syn_curpattern[0]][5] = 63;
		syn_sequence[0][syn_curpattern[0]][6] = 63;
		
		

		while (1)
		{
		      	//get the drawing done
		      	switch(screen)
		      	{
	      		 case 1:

//		for (loop = 0; loop < 20; loop++)
//		{
//			if(rflags[loop]>0)
//			{
					DrawBackground(gridData);
//				DrawBackgroundRegion(gridData,
//					background_regions[screen-1][loop][0],
//					background_regions[screen-1][loop][1],
//					background_regions[screen-1][loop][2],
//					background_regions[screen-1][loop][3]);
//				rflags[loop]--;
//			}
//		}
//				 if(rflags[1]>0)
					DrawSequenceData(syn_sequence[0][syn_curpattern[0]],syn_grid_top_row_note[0]);
//				if(rflags[2]>0)
					if(((syn_grid_top_row_note[0]-syn_cursor_y[0])*ROW_HEIGHT)+GRID_TOP_OFFSET > 0 )
				 		DrawCursor((GRID_LEFT_OFFSET+syn_cursor_x[0]*COLUMN_WIDTH)/2, ((syn_grid_top_row_note[0]-syn_cursor_y[0])*ROW_HEIGHT)+GRID_TOP_OFFSET);
//				if(rflags[3]>0)
					DrawKeyboard(syn_grid_top_row_note[0]);
//				if(rflags[4]>0)
					DrawRightScrollThumb(syn_cursor_y[0]);
					DrawBottomScrollThumb(syn_cursor_x[screen-1], sequence_lengths[screen-1][syn_curpattern[screen-1]],syn_grid_left_position[screen-1]);
					DrawPatternBoundingBraces(syn_grid_left_position[screen-1],sequence_lengths[screen-1][syn_curpattern[screen-1]]);
//				if(rflags[5]>0)
					DrawCurrentFieldBox(screen,field);
//				if(rflags[6]>0)
					DrawCursor(((syn_curpattern[0])%8)*PATTERN_THUMB_WIDTH+PATTERN_GRID_LEFT,((syn_curpattern[0])/8)*PATTERN_THUMB_HEIGHT+PATTERN_GRID_TOP);
					sprintf(nPatternLengthText,"%d",sequence_lengths[0][syn_curpattern[0]]);

					DrawText(PATTERN_LENGTH_BOX_X,PATTERN_LENGTH_BOX_Y, nPatternLengthText,  "8",8,7);
//				DrawText(64,34, "ABCabc123.;'", "8White",8,7);

				 break;
			 case 2:
				      DrawBackground(grid_syn2Data);
				      DrawSequenceData(syn_sequence[1][syn_curpattern[1]],syn_grid_top_row_note[1]);
				      if(((syn_grid_top_row_note[1]-syn_cursor_y[1])*ROW_HEIGHT)+GRID_TOP_OFFSET > 0 )
				      	DrawCursor((GRID_LEFT_OFFSET+syn_cursor_x[1]*COLUMN_WIDTH)/2, ((syn_grid_top_row_note[1]-syn_cursor_y[1])*ROW_HEIGHT)+GRID_TOP_OFFSET);
       				     DrawKeyboard(syn_grid_top_row_note[1]);
   				     DrawRightScrollThumb(syn_cursor_y[1]);
					 			DrawCurrentFieldBox(screen,field);
				DrawCursor(((syn_curpattern[0])%8)*PATTERN_THUMB_WIDTH+PATTERN_GRID_LEFT,((syn_curpattern[0])/8)*PATTERN_THUMB_HEIGHT+PATTERN_GRID_TOP);
				  break;
			 case 3:
				      DrawBackground(grid_syn3Data);
				      DrawSequenceData(syn_sequence[2][syn_curpattern[2]],syn_grid_top_row_note[2]);
				      if(((syn_grid_top_row_note[2]-syn_cursor_y[2])*ROW_HEIGHT)+GRID_TOP_OFFSET > 0 )
				      	DrawCursor((GRID_LEFT_OFFSET+syn_cursor_x[2]*COLUMN_WIDTH)/2, ((syn_grid_top_row_note[2]-syn_cursor_y[2])*ROW_HEIGHT)+GRID_TOP_OFFSET);
       				     DrawKeyboard(syn_grid_top_row_note[2]);
   				     DrawRightScrollThumb(syn_cursor_y[2]);
			DrawCurrentFieldBox(screen,field);
							DrawCursor(((syn_curpattern[0])%8)*PATTERN_THUMB_WIDTH+PATTERN_GRID_LEFT,((syn_curpattern[0])/8)*PATTERN_THUMB_HEIGHT+PATTERN_GRID_TOP);
				  break;
				  case 4:
					      DrawBackground(grid_mixerData);
		  					sprintf(nTempoText,"%d",nTempo);
			DrawCurrentFieldBox(screen,field);
					DrawText(TEMPO_X,TEMPO_Y, nTempoText,  "8",8,7);

					  break;

			case 5:		//The overall settings/saving screen
				//DrawPalette();
//				 DrawBackground(gridData);
//				 DrawSequenceData(syn_sequence[0][syn_curpattern[0]],syn_grid_top_row_note[0]);
//				 if(((syn_grid_top_row_note[0]-syn_cursor_y[0])*ROW_HEIGHT)+GRID_TOP_OFFSET > 0 )
//				 	DrawCursor((GRID_LEFT_OFFSET+syn_cursor_x[0]*COLUMN_WIDTH)/2, ((syn_grid_top_row_note[0]-syn_cursor_y[0])*ROW_HEIGHT)+GRID_TOP_OFFSET);
//				 DrawKeyboard(syn_grid_top_row_note[0]);
//	                         DrawRightScrollThumb(syn_cursor_y[0]);
//				DrawText(50,16, "Settings", "8White",8,7);
				DrawTextCentered(16, "Settings", "8White",8,7);
//				DrawFilledRect(0,0,120,240,0x3093);
				DrawFilledRect(field_positions[screen-1][0][0],field_positions[screen-1][0][1],field_positions[screen-1][0][2]+2,field_positions[screen-1][0][3]+2,255+256*255);
				DrawFilledRect(field_positions[screen-1][1][0],field_positions[screen-1][1][1],field_positions[screen-1][1][2]+2,field_positions[screen-1][1][3]+2,255+256*255);
				DrawText(16,34, "Save", "8White",8,7);
				DrawText(16,44, "Load", "8White",8,7);
				DrawCurrentFieldBox(screen,field);


				 break;					
			}

			//Output the string

			//Universal While Playing Drawing and Playback Routines
			if(playback_head != -1)
			{
//				if(rflags[7])
					DrawPlaybackHead((GRID_LEFT_OFFSET+playback_head*COLUMN_WIDTH)/2-1, GRID_TOP_OFFSET);
				if((syn_sequence[0][syn_curpattern[0]][playback_head] != -1) && (step_cycle_counter == 0)) PlayStepVoice1(syn_sequence[0][syn_curpattern[0]][playback_head]);
				if((syn_sequence[1][syn_curpattern[1]][playback_head] != -1) && (step_cycle_counter == 0)) PlayStepVoice2(syn_sequence[1][syn_curpattern[1]][playback_head]);
				if((syn_sequence[2][syn_curpattern[2]][playback_head] != -1) && (step_cycle_counter == 0)) PlayStepVoice3(syn_sequence[2][syn_curpattern[2]][playback_head]);
			}

			if(num_cycles_until_status_screen_gets_turned_off > 0)
			{
				DrawStatusBox(nStatusText);
				num_cycles_until_status_screen_gets_turned_off--;

				//The only way num_cycles could be zero at this point is
				//if it was previously 1 and we just set it to 0. Thus,
				//the status message is going to be turned off and the 
				//background needs redrawing.
				if(num_cycles_until_status_screen_gets_turned_off == 0)
					rflags[0] = 2;

			}
			WaitForVblank();
			Flip();


			// cursor movement
			if ((! ((*KEYS) & KEY_RIGHT) ) && (cycles_since_right_was_pressed > CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_KEYPRESSES))
			{
				switch(screen)
				{
					case 1:
           			case 2:
           			case 3:

//								rflags[1] = 2;
//								rflags[2] = 2;
						switch(field)
						{
	               			case 1:
								rflags[syn_cursor_x[screen-1]+2]=2;
								syn_cursor_x[screen-1]+=1;
								rflags[syn_cursor_x[screen-1]+2]=2;
									 rflags[18]=2;
								break;
							case 2:
								if(syn_curpattern[screen-1]%8 != 7)
								syn_curpattern[screen-1] +=1;
								break;
						}
				        break;

				}
				cycles_since_right_was_pressed=0;
			}

			if ((! ((*KEYS) & KEY_LEFT) )  && (cycles_since_left_was_pressed > CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_KEYPRESSES))
				{
				switch(screen)
				{
					case 1:
					case 2:
					case 3:
				
//								rflags[1] = 2;
//								rflags[2] = 2;
	       		         switch(field)
			                 {
	              		         case 1:

									 rflags[syn_cursor_x[screen-1]+2]=2;
									 syn_cursor_x[screen-1]-=1;
									rflags[18]=2;
										break;
										case 2:
											if(syn_curpattern[screen-1]%8 != 0)
										 syn_curpattern[screen-1] -=1;
										
									 rflags[syn_cursor_x[screen-1]+2]=2;

					              break;

                              }
				                break;
                   	   }
                              cycles_since_left_was_pressed=0;				         
				}
			if ((! ((*KEYS) & KEY_DOWN) )  && (cycles_since_down_was_pressed > CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_KEYPRESSES))
				{			 switch(screen){
			                case 1:
			                case 2:
			                case 3:

//								rflags[1] = 2;
//								rflags[2] = 2;
	       		                        switch(field)
                              {
	                       		         case 1:
//								rflags[0] = 2;
//								rflags[1] = 2;
//								rflags[2] = 2;
					              syn_cursor_y[screen-1]-=1;
								 rflags[syn_cursor_x[screen-1]+2]=2;
								 rflags[19]=2;
					              break;
								  case 2:
										if(syn_curpattern[screen-1]/8 != 1)
										 syn_curpattern[screen-1] +=8;
					              break;
								  	case 3:
										if(sequence_lengths[screen-1][syn_curpattern[screen-1]] >0)
								  {
											sequence_lengths[screen-1][syn_curpattern[screen-1]]--;
											if(sequence_lengths[screen-1][syn_curpattern[screen-1]] <= syn_cursor_x[screen-1])
												syn_cursor_x[screen - 1] = sequence_lengths[screen-1][syn_curpattern[screen-1]] -1;
								  }
					              break;

                              }
				                break;
							case 5:
								if(field <= 1) field++;
								break;
                   	   }  cycles_since_down_was_pressed=0;}
			if ((! ((*KEYS) & KEY_UP) ) && (cycles_since_up_was_pressed > CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_KEYPRESSES))
				{			 switch(screen){
			                case 1:
			                case 2:
			                case 3:
							switch(field)
								{
	                       			case 1:
										syn_cursor_y[screen-1]+=1;
										rflags[syn_cursor_x[screen-1]+2]=2;
										rflags[19]=2;
										break;
									case 2:
										if(syn_curpattern[screen-1]/8 != 0)
										syn_curpattern[screen-1] -=8;
										break;
									case 3:
										if(sequence_lengths[screen-1][syn_curpattern[screen-1]] < MAX_PATTERN_LENGTH)
											sequence_lengths[screen-1][syn_curpattern[screen-1]]++;
					              break;
                              }
				                break;
							case 5:
								if(field >= 2) field--;
								break;
                   	   }cycles_since_up_was_pressed=0;}

			cycles_since_left_was_pressed++;
			cycles_since_right_was_pressed++;
			cycles_since_up_was_pressed++;
			cycles_since_down_was_pressed++;

			// boundary checking for tank
			switch(screen)
			{
			case 1:
				if (syn_cursor_x[0] < 0) 
					{
						syn_cursor_x[0] = 0;
					}
				if (syn_cursor_x[0] > sequence_lengths[screen-1][syn_curpattern[screen-1]]-1) 
					{
						syn_cursor_x[0] = sequence_lengths[screen-1][syn_curpattern[screen-1]]-1;
					}
				if (syn_cursor_y[0] > syn_grid_top_row_note[0]) 
					{
						syn_grid_top_row_note[0] = syn_cursor_y[0] + GRID_NUMBER_DOWN/2;
						rflags[0]=2;			
					}
				if (syn_cursor_y[0] < MIN_MIDI_NOTE) 
					{
						syn_cursor_y[0] = MIN_MIDI_NOTE;
					}
				if (syn_cursor_y[0] < syn_grid_top_row_note[0]-GRID_NUMBER_DOWN+1) 
					{
						syn_grid_top_row_note[0] = syn_cursor_y[0] - GRID_NUMBER_DOWN/2;				
						rflags[0]=2;
					}
				if (syn_cursor_y[0] >MAX_MIDI_NOTE) 
					{
						syn_cursor_y[0] = MAX_MIDI_NOTE;
					}
				if (syn_curpattern[screen-1] > MAX_PATTERN_NUMBER) 
					syn_curpattern[screen-1] = MAX_PATTERN_NUMBER;
			if (syn_curpattern[screen-1] < 0) syn_curpattern[screen-1] =0;
			{
			}
			{
			}
			break;
			case 2:
                        if (syn_cursor_x[1] < 0) {syn_cursor_x[1] = 0;}
			if (syn_cursor_x[1]>GRID_NUMBER_ACROSS-1) {syn_cursor_x[1] = GRID_NUMBER_ACROSS-1;}
                        if (syn_cursor_y[1] > syn_grid_top_row_note[1]) {syn_grid_top_row_note[1]=syn_cursor_y[1] + GRID_NUMBER_DOWN/2;}
                        if (syn_cursor_y[1] < MIN_MIDI_NOTE) {syn_cursor_y[1] = MIN_MIDI_NOTE;}
			if (syn_cursor_y[1] < syn_grid_top_row_note[1]-GRID_NUMBER_DOWN+1) {syn_grid_top_row_note[1] = syn_cursor_y[1] - GRID_NUMBER_DOWN/2;}
			if (syn_cursor_y[1] >MAX_MIDI_NOTE) {syn_cursor_y[1] = MAX_MIDI_NOTE;}
			break;
			case 3:
                        if (syn_cursor_x[2] < 0) {syn_cursor_x[2] = 0;}
			if (syn_cursor_x[2]>GRID_NUMBER_ACROSS-1) {syn_cursor_x[2] = GRID_NUMBER_ACROSS-1;}
                        if (syn_cursor_y[2] > syn_grid_top_row_note[2]) {syn_grid_top_row_note[2]=syn_cursor_y[2] + GRID_NUMBER_DOWN/2;}
                        if (syn_cursor_y[2] < MIN_MIDI_NOTE) {syn_cursor_y[2] = MIN_MIDI_NOTE;}
			if (syn_cursor_y[2] < syn_grid_top_row_note[2]-GRID_NUMBER_DOWN+1) {syn_grid_top_row_note[2] = syn_cursor_y[2] - GRID_NUMBER_DOWN/2;}
			if (syn_cursor_y[2] >MAX_MIDI_NOTE) {syn_cursor_y[2] = MAX_MIDI_NOTE;}
			break;
			}

			//Switching instruments. If START is pressed:
			if (! ((*KEYS) & KEY_START) )
			{


				 if(cycles_since_start_button_was_pressed > CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_NON_ACTION_KEYPRESSES)
			   {
			      if(screen != 5)
			      	  screen = 5;
			      else
			      	  screen = 1;

			      field = 1;
			   }

   			      //this is strategically place outside this last if statement, in other words, 
				  //it gets set to zero regardless of whether or not we handle a start key press.
				  //this means you have ot not be pressing start for at least 2 cycles in order for
				  //a new one to be accepted as meaningful input
			      cycles_since_start_button_was_pressed = 0;

  		    	}

			 cycles_since_start_button_was_pressed++;

			//Switching instruments. If SELECT is pressed:
			if (! ((*KEYS) & KEY_SELECT) )
			{
			   if(screen == 1)
			   	screen = 2;
			   	else
				    if(screen == 2)
				     	screen = 3;
				     	else
				     	    if(screen == 3)
								screen = 4;
								else
									if (screen==4)
				     					screen = 1;
			   field=1;
			}



			if (! ((*KEYS) & KEY_R) )
			{
	//			LoadSong(syn_sequence[0][syn_curpattern[0]], syn_sequence[1][syn_curpattern[1]], syn_sequence[2][syn_curpattern[2]]);
/*					for(loop=0;loop<NUM_SEQUENCE_STEPS;loop++)
				{
					syn_sequence[0][syn_curpattern[0]][loop] = LoadInt(loop*7+1);	
					syn_sequence[1][syn_curpattern[1]][loop] = LoadInt(loop*7+(1+NUM_SEQUENCE_STEPS*7));	
					syn_sequence[2][syn_curpattern[2]][loop] = LoadInt(loop*7+(1+2*NUM_SEQUENCE_STEPS*7));	
				} */
			}

			if (! ((*KEYS) & KEY_L) )
			{
				if (screen==1 || screen == 2 || screen ==3)
				{
					if(field == 1)
						field = 2;
					else if (field == 2)
						field = 3;
					else if (field == 3)
						field = 1;

				}
				else if(screen==4)
				{
					if(field == 1)
						field = 2;
					else if (field == 2)
						field = 1;
				}
				rflags[0]=2;
			}

			//The actual editing code
			if (! ((*KEYS) & KEY_A) )
			{
 			 if(screen ==1)
			 {
			  if(cycles_since_button_a_was_pressed != syn_cursor_x[0])
			  {
				if(syn_sequence[0][syn_curpattern[0]][syn_cursor_x[0]] != syn_cursor_y[0])
		      		syn_sequence[0][syn_curpattern[0]][syn_cursor_x[0]] = syn_cursor_y[0];
	      		else
		      	    syn_sequence[0][syn_curpattern[0]][syn_cursor_x[0]] = -1;
                
				cycles_since_button_a_was_pressed = syn_cursor_x[0];
				rflags[syn_cursor_x[screen-1]+2]=2;
			  }
                                        }
			   	else

			    if(screen == 2)
				    	{
   					 if(cycles_since_button_a_was_pressed != syn_cursor_x[1])
                                            {
				          if(syn_sequence[1][syn_curpattern[1]][syn_cursor_x[1]] != syn_cursor_y[1])
		      			 syn_sequence[1][syn_curpattern[1]][syn_cursor_x[1]] = syn_cursor_y[1];
					else
		      			syn_sequence[1][syn_curpattern[1]][syn_cursor_x[1]] = -1;
                                            cycles_since_button_a_was_pressed = syn_cursor_x[1];
				rflags[syn_cursor_x[screen-1]+2]=2;
					}



		  }
		  else
		  			    if(screen == 3)
				    	{
   					 if(cycles_since_button_a_was_pressed != syn_cursor_x[2])
                                            {
				          if(syn_sequence[2][syn_curpattern[2]][syn_cursor_x[2]] != syn_cursor_y[2])
		      			 syn_sequence[2][syn_curpattern[2]][syn_cursor_x[2]] = syn_cursor_y[2];
					else
		      			syn_sequence[2][syn_curpattern[2]][syn_cursor_x[2]] = -1;
                                            cycles_since_button_a_was_pressed = syn_cursor_x[2];
				rflags[syn_cursor_x[screen-1]+2]=2;
					}
						}
  			else    if(screen == 5)
				    	{
							switch(field){
								case 1:
							//	SaveSong(1,(const int * const (*)[MAX_PATTERN_NUMBER][NUM_SEQUENCE_STEPS])syn_sequence);
								
								sprintf(nStatusText,"Song Saved...");
								num_cycles_until_status_screen_gets_turned_off = 20;
								break;
								case 2:
								sprintf(nStatusText,"Song Loaded...");
								num_cycles_until_status_screen_gets_turned_off = 20;
//								LoadSong(syn_sequence[0][syn_curpattern[0]], syn_sequence[1][0], syn_sequence[2][0]);
								break;
							}
						
					}



		  }

			
			else
		    	    cycles_since_button_a_was_pressed = -1;

			//if button B is pressed
  		    	if (! ((*KEYS) & KEY_B) ){
			   //as long a button b wasn't pressed last time
			   if((cycles_since_button_b_was_pressed > CYCLES_REQUIRED_BETWEEN_TWO_DISTINCT_KEYPRESSES))
			   {
			      if(playback_head != -1)
			      	  playback_head = -1;
			      else
			      	  playback_head = 0;

			      //let the folks know that we acted on a pressing of botton b
			      cycles_since_button_b_was_pressed = 0;
			      
			   }
  		    	}

			 cycles_since_button_b_was_pressed++;
		       //Move player thing forward
		       if (playback_head != -1)
		       {
		       	//if we have not gotten to the requisite number of steps
		       	//then just increment the cycle counter
		       	if(step_cycle_counter < NUM_CYCLES_PER_STEP)
		       	{step_cycle_counter++;}
		       	//otherwise actually move to the next step
		       	else
		       	{
		       		step_cycle_counter = 0;
		       		//move the playback head forward or back to 0
									rflags[playback_head+2]=2;
		       		if(playback_head < NUM_SEQUENCE_STEPS-1)
		       			 playback_head++;
		       		else
		               	 playback_head = 0;
									rflags[playback_head+2]=2;

			} // end else
		       } //end if playback_head != -1
		} // end while life > 0
	
	return 0;
} // end Agbmain

void DrawPalette(void)
{
	int i,b,x,y,x2,y2,xt,yt;
	for(i=0;i<16;i++)
	for(b=0;b<16;b++)
	{
		x = i*(10);
		y = b*(7);
		x2 = (i+1)*(10);
		y2 =(b+1)*(7);

	//		DrawFilledRect(x,y,x2,y2,b*16+i);
	for(xt=x;xt < x2;xt++)
		for(yt=y;yt < y2;yt++)
			PlotPixel(xt/2,yt,(b*16+i)+(b*16+i)*256);
	
	}
}
void EraseScreen(void)
{
	int x,y;
	for(y = 0; y < 160; y++)
	{
		for(x = 0; x < 120; x++)

		{
			PlotPixel(x,y,0xFFFF);//logoData contains the color values of your pict

		}
	}

}

void DrawStatusBox(char *message)
{
	int stringLength = 0; int index = 0; int stringWidth; int stringX,stringY,stringHeight;
	for(index=0;message[index] != '\0';index++);

	stringWidth = 7*index;
	stringHeight = 8;
	stringX = 160 /2 - stringWidth / 2;
	stringY = 120 /2 - stringHeight /2;
	DrawFilledRect(2*(stringX-2),stringY-2,(stringX+stringWidth+2),stringY+8,0xFFFF);
	DrawText(stringX,stringY, message, "8White",8,7);
}

void DrawTextCentered(int stringY,char *message,char *font,int char_height,int char_width)
{
	int stringLength = 0; int index = 0; int stringWidth; int stringX,stringHeight;
	for(index=0;message[index] != '\0';index++);

	stringWidth = char_width*index;
	stringX = 160 /2 - stringWidth / 2 - 4;
	DrawText(stringX,stringY, message, font,char_height,char_width);



}

void DrawFilledRect(int x1,int y1, int x2, int y2, int color)
{
	int x,y;
	for(y = y1; y < y2; y++)
	{
		for(x = x1/2; x < x2/2; x++)

		{
			PlotPixel(x,y,color);//logoData contains the color values of your pict

		}
	}

}

void LoadSong(int *syn1_sequence, int *syn2_sequence, int *syn3_sequence)
{
	int loop;
					for(loop=0;loop<NUM_SEQUENCE_STEPS;loop++)
				{

					syn1_sequence[loop] = LoadInt(loop*7+1);	
					if(syn1_sequence[loop] == 256)
					syn1_sequence[loop] = -1;

					syn3_sequence[loop] = LoadInt(loop*7+(1+NUM_SEQUENCE_STEPS*7));	
					if(syn3_sequence[loop] == 256)
					syn3_sequence[loop] = -1;

					syn3_sequence[loop] = LoadInt(loop*7+(1+2*NUM_SEQUENCE_STEPS*7));	
					if(syn3_sequence[loop] == 256)
					syn3_sequence[loop] = -1;

				
				} 
}

void PlayStepVoice1(int note)
{
 int midi_note =  71 - note;
/*  if (note < 0)
 return;
 if (note > 4)
 return; */

 //full volume, enable sound 1 to left and right
 REG_SOUNDCNT_L=0x7777;
 // Overall output ratio - Full
 REG_SOUNDCNT_H = 2;
 REG_SOUND1CNT_L=0x0000; //sweep shifts=6, increment, sweep time=39.1ms
 REG_SOUND1CNT_H=0xf780; //duty=50%,envelope decrement
 REG_SOUND1CNT_X=0x8000 + noteData[note];
}

void PlayStepVoice2(int note)
{
 int midi_note =  71 - note;
 //full volume, enable sound 2 to left and right
 REG_SOUNDCNT_L=0x7777;
 // Overall output ratio - Full
 REG_SOUNDCNT_H = 2;
 REG_SOUND2CNT_L=0xf780; //duty=50%,envelope decrement
 REG_SOUND2CNT_H=0x8000 + noteData[note];
}

void PlayStepVoice3(int note)
{
 int midi_note =  71 - note;
 // Overall output ratio - Full
// REG_SOUNDCNT_H = 2;
// REG_SOUND2CNT_L=0xf780; //duty=50%,envelope decrement
// REG_SOUND2CNT_H=0x8000 + noteData[note];

// Overall output ratio - Full
//REG_SOUNDCNT_H = SOUND3OUTPUT1;

//select bank 0 for playing
REG_SOUND3CNT_L = SOUND3BANK32 | SOUND3SETBANK0;

REG_SOUND3CNT_L |= SOUND3PLAY;
REG_SOUND3CNT_H = SOUND3OUTPUT1;
REG_SOUND3CNT_X=SOUND3INIT|SOUND3PLAYLOOP| noteData[note] ; //play a C-4 in loop mode

}
void InterruptProcess(void){
//sample finished!,stop Direct sound
REG_TM0CNT_H=0; //disable timer 0
REG_DMA1CNT_H=0; //stop DMA

//clear the interrupt(s)
REG_IF |= REG_IF;
}

void PlayStepVoice4(int note)
{
 int midi_note =  71 - note;
 // Overall output ratio - Full


if(midi_note==0)
	{

			REG_TM0CNT_H=0;	//disable timer 0
//clear the interrupt(s)
//		REG_IF |= REG_IF;
//		REG_DMA1SAD=(unsigned long)_drumbox_snare1.pName; //dma1 source
//		REG_DMA1DAD=REG_SGFIFOA_L; //write to FIFO A address
//		REG_DMA1CNT_H=0xb600; //dma control: DMA enabled+ start on FIFO+32bit+repeat+increment source&dest

//		REG_TM1CNT_L=0x7098; //0xffff-the number of samples to play
//		REG_TM1CNT_L=0xFFFF-_drumbox_snare1.nLength; //0xffff-the number of samples to play
//		REG_TM1CNT_H=0xC4; //enable timer1 + irq and cascade from timer 0

//		REG_IE=0x10; //enable irq for timer 1
//		REG_IME=1; //master enable interrupts

		//Formula for playback frequency is: 0xFFFF-round(cpuFreq/playbackFreq)
//		REG_TM0CNT_L=0xFBE8; //16khz playback freq
//		REG_TM0CNT_H=0x0080; //enable timer0

	REG_SOUNDCNT_L=0;
	REG_SOUNDCNT_H=0x0b0F;  //DS A&B + fifo reset + timer0 + max volume to L and R
	REG_SOUNDCNT_X=0x0080;  //turn sound chip on
	
	REG_DMA1SAD=(unsigned long)_drumbox_bd1.pName;	//dma1 source
	REG_DMA1DAD=0x040000a0; //write to FIFO A address
	REG_DMA1CNT_H=0xb600;	//dma control: DMA enabled+ start on FIFO+32bit+repeat
	
	REG_TM1CNT_L=0x9F9B;	//0xffff-the number of samples to play
	REG_TM1CNT_H=0xC4;		//enable timer1 + irq and cascade from timer 0

	REG_IE=0x10;	  	//enable irq for timer 1 overflow
	REG_IME=1;				//enable interrupt
	
	//Formula for playback frequency is: 0xFFFF-round(cpuFreq/playbackFreq)
	REG_TM0CNT_L=0xFBE8;	//16khz playback freq
	REG_TM0CNT_H=0x0080; 	//enable timer at CPU freq 	
	
	}

}


int SaveSong(int nSlot,const int * const (* syn_sequence[MAX_PATTERN_NUMBER][NUM_SEQUENCE_STEPS]))
{
	int loop;
	int cp = 0; //cp = cursor position
	SaveInt(cp,1); cp++;	
	for(loop=0;loop<NUM_SEQUENCE_STEPS;loop++)
	{
		SaveInt(cp,syn_sequence[0][0][loop]);	
		cp+=7;
	}
	for(loop=0;loop<NUM_SEQUENCE_STEPS;loop++)
	{
		SaveInt(cp,syn_sequence[1][0][loop]);	
		cp+=7;
	}
	for(loop=0;loop<NUM_SEQUENCE_STEPS;loop++)
	{
		SaveInt(cp,syn_sequence[2][0][loop]);	
		cp+=7;
	}

	return 1; // 1 means success
}
 
 void InitSound(void)
{
 REG_SOUNDCNT_L=0x7777;

 //select bank 0 for writing (bank 1 playing)
REG_SOUND3CNT_L = SOUND3BANK32 | SOUND3SETBANK1;
//load the wave ram bank 0
REG_WAVE_RAM0=0x10325476;
REG_WAVE_RAM1=0x98badcfe;
REG_WAVE_RAM2=0x10325476;
REG_WAVE_RAM3=0x98badcfe;

}
void DoIntro(void)
{
	int x, y;
	for(x = 0; x < 256; x++)
		paletteMem[x] = logoPalette[x];	//loop through and store the palette from your pict
									//palette into video palette mem
	for(y = 0; y < 160; y++)
	{
		for(x = 0; x < 120; x++)

		{
			PlotPixel(x,y,logoData[y*120+x]);//logoData contains the color values of your pict

		}
	}
	WaitForVblank();
	Flip();
//	Sleep(125);
}

void DrawText(int topleft_x, int topleft_y, char *message, char *color, int charHeight, int charWidth)
{
	int single_letter_x, overall_x=0, y,y_letter_offset, letter_index = 0;
	
	for(letter_index = 0; message[letter_index] != '\0';letter_index++)
	{
		if(message[letter_index] >= 'A' && message[letter_index] <= 'Z')
		{
			y_letter_offset = charHeight + charHeight*(message[letter_index]-0x41);
		}
		else if(message[letter_index] >= 'a' && message[letter_index] <= 'z')
		{
			y_letter_offset = charHeight + charHeight*(message[letter_index]-0x47);
		}
		else if(message[letter_index] >= '0' && message[letter_index] <= '9')
		{
			y_letter_offset = charHeight + charHeight*(message[letter_index]+4);
		}
		else y_letter_offset = 0;
		// draw the tank
		for(y = 0; y < charHeight; y++)
			for (single_letter_x = 0; single_letter_x < (charWidth/2);single_letter_x++)
		{
			if(color[0] == 'W')
				PlotPixel(topleft_x+overall_x+single_letter_x, topleft_y+y, az_whiteData[(y_letter_offset+y)*(charHeight/2)+single_letter_x]);
			if(color[0] == '8')
				PlotPixel(topleft_x+overall_x+single_letter_x, topleft_y+y, AZ_terminal_8Data[(y_letter_offset+y)*(charHeight/2)+single_letter_x]);
			else
				PlotPixel(topleft_x+overall_x+single_letter_x, topleft_y+y, azData[(y_letter_offset+y)*(charHeight/2)+single_letter_x]);
		}
		overall_x+=charWidth/2;
	}
}

void DrawIntroScreenImage(void)
{
	int x, y;

	// set the palette
	for(x = 0; x < 256; x++)
		paletteMem[x] = tankPalette[x];	//loop through and store the palette from your pict
									//palette into video palette mem

	// draw the picture
	for(y = 0; y < 160; y++)
	{
		for(x = 0; x < 120; x++)

		{
			PlotPixel(x,y,tankData[y*120+x]);//logoData contains the color values of your pict

		}
	}
	WaitForVblank();
	Flip();
	Sleep(50);

	// put the start text on top of it

	// draw the picture
	for(y = 0; y < 160; y++)
		for(x = 0; x < 120; x++)
			PlotPixel(x,y,tankData[y*120+x]);//logoData contains the color values of your pict

	// draw the text
	for(y = 0; y < txstart_HEIGHT; y++)
		for (x = 0; x < (txstart_WIDTH/2); x++)
			PlotPixel(x+58, y+10, txstartData[y*(txstart_WIDTH/2)+x]);
	WaitForVblank();
	Flip();

	WaitForStart();

}

void WaitForStart(void)
{
	while (1) // loops infinitely till they press start
		if (! ((*KEYS) & KEY_START) )
			return;
}

void Flip(void)			// flips between the back/front buffer
{
	if(REG_DISPCNT & BACKBUFFER) //back buffer is the current buffer so we need to switch it to the font buffer
	{
		REG_DISPCNT &= ~BACKBUFFER; //flip active buffer to front buffer by clearing back buffer bit
		videoBuffer = BackBuffer; //now we point our drawing buffer to the back buffer
    }
    else //front buffer is active so switch it to backbuffer
    {
		REG_DISPCNT |= BACKBUFFER; //flip active buffer to back buffer by setting back buffer bit
		videoBuffer = FrontBuffer; //now we point our drawing buffer to the front buffer
	}
}

void WaitForVblank(void)	// waits for the drawer to get to end before flip
{
	while(*ScanlineCounter < 160) {	// do nothing
	}//now we are in the vblank period
}

void ChangeBackPal(void)							// changes to the star palette
{
	int x;
	// set the palette
	for(x = 0; x < 256; x++)
		paletteMem[x] = genericPalette[x];	//loop through and store the palette from your pict
									//palette into video palette mem
}


void DrawBackground(u16 *screenData)								// draws the starish background
{
	int x, y;
	// draw the picture
	for(y = 0; y < 160; y++)
	{
		for(x = 0; x < 120; x++)
		{
			PlotPixel(x,y,screenData[y*120+x]);//logoData contains the color values of your pict
		}
	}
}

void DrawBackgroundRegion(u16 *screenData,int x1,int y1, int x2, int y2)								// draws the starish background
{
	int x, y;
	// draw the picture
	for(y = y1; y < y2; y++)
	{
		for(x = (x1/2); x < (x2/2); x++)
		{
			PlotPixel(x,y,screenData[y*120+x]);//logoData contains the color values of your pict
		}
	}
}

void DrawPlaybackHead(int tankx, int tanky)	// draws a vertical line on the screen
{
	int x, y;
	x = tankx;
		for (y = tanky; y < tanky+119; y++)
			PlotPixel(x, y, 253); // green pal[2]
}

void DrawKeyboard(int grid_top_offset)
{
     int x,y,y_internal;
   	int how_many_notes_into_keyboard_one_do_we_begin = (12 -( (grid_top_offset+1) % 12))%12;
	y_internal=ROW_HEIGHT*how_many_notes_into_keyboard_one_do_we_begin+1;

 	for(y = 1; y < GRID_NUMBER_DOWN*ROW_HEIGHT-1; y++)
 	{
 		if(y_internal >= piano_HEIGHT) {y_internal=0;}
		for (x = 0; x < (piano_WIDTH/2); x++)
			PlotPixelTransparent(x+(KEYBOARD_START_X)/2, KEYBOARD_START_Y+y, pianoData[y_internal*(piano_WIDTH/2)+x]);
		y_internal++;
	}
}

void DrawCursor(int tankx, int tanky)	// draws the tank on the screen
{
	int x, y;
	// draw the tank
	for(y = 0; y < objtank_HEIGHT; y++)
		for (x = 0; x < (objtank_WIDTH/2); x++)
			PlotPixelTransparent(x+tankx, y+tanky, objtankData[y*(objtank_WIDTH/2)+x]);
}

void DrawRightScrollThumb(int tanky)	// draws the tank on the screen
{
	int x, y; 
	float percentage = ((float)tanky - MIN_MIDI_NOTE)/(MAX_MIDI_NOTE-MIN_MIDI_NOTE);
	float position_y = RIGHT_SCROLL_TOP_Y + (RIGHT_SCROLL_BOTTOM_Y - RIGHT_SCROLL_TOP_Y)*(1.0-percentage);

	for(y = 0; y < ight_scroll_thumb_HEIGHT; y++)
		for (x = 0; x < (ight_scroll_thumb_WIDTH/2); x++)
			PlotPixel(x+RIGHT_SCROLL_STARTS_X, y+position_y, ight_scroll_thumbData[y*(objtank_WIDTH/2)+x]);
}


void DrawPatternBoundingBraces(int pan_x,int sequence_length)	// draws the tank on the screen
{
	int x, y;
	int tankx = 0 + pan_x;
	float percentage = ((float)tankx)/((float)(sequence_length-1.0));
	float position_x = (BOTTOM_SCROLL_LEFT_X-2) + ((BOTTOM_SCROLL_RIGHT_X+2) - (BOTTOM_SCROLL_LEFT_X-2))*percentage;

//	for(y = 0; y < ight_scroll_thumb_HEIGHT; y++)
//		for (x = 0; x < (ight_scroll_thumb_WIDTH/2); x++)
//			PlotPixel(x+position_x, y+BOTTOM_SCROLL_STARTS_Y, ight_scroll_thumbData[y*(objtank_WIDTH/2)+x]);

  x =0;
	y=0;

		PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y-1, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y+1, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y+2, 1);

	if(sequence_length < 16) return;

	tankx = 15 + pan_x;
	percentage = ((float)tankx)/((float)(sequence_length-1.0));
	position_x = (BOTTOM_SCROLL_LEFT_X-2) + ((BOTTOM_SCROLL_RIGHT_X+2) - (BOTTOM_SCROLL_LEFT_X-2))*percentage;

//	for(y = 0; y < ight_scroll_thumb_HEIGHT; y++)
//		for (x = 0; x < (ight_scroll_thumb_WIDTH/2); x++)
//			PlotPixel(x+position_x, y+BOTTOM_SCROLL_STARTS_Y, ight_scroll_thumbData[y*(objtank_WIDTH/2)+x]);
    x =0;
	y=0;
	
//	PlotPixel(x+position_x-1, BOTTOM_SCROLL_STARTS_Y-1, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y-1, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y+1, 1);
	PlotPixel(x+position_x, BOTTOM_SCROLL_STARTS_Y+2, 1);
//	PlotPixel(x+position_x-1, BOTTOM_SCROLL_STARTS_Y+2, 1);
	
	}

void DrawBottomScrollThumb(int tankx,int sequence_length,int screen_pan_value)	// draws the tank on the screen
{
	int x, y;
	float percentage = ((float)tankx)/((float)(sequence_length-1.0));
	float position_x = BOTTOM_SCROLL_LEFT_X + (BOTTOM_SCROLL_RIGHT_X - BOTTOM_SCROLL_LEFT_X)*percentage;

	for(y = 0; y < ight_scroll_thumb_HEIGHT; y++)
		for (x = 0; x < (ight_scroll_thumb_WIDTH/2); x++)
			PlotPixel(x+position_x, y+BOTTOM_SCROLL_STARTS_Y, ight_scroll_thumbData[y*(objtank_WIDTH/2)+x]);
}

void DrawSequenceData(int seq[NUM_SEQUENCE_STEPS], int grid_top_offset)
{
	int loop;
	for(loop=0; loop < NUM_SEQUENCE_STEPS; loop++)
		    if(seq[loop] != -1)
		    {
		    	if((seq[loop] <= grid_top_offset) && (seq[loop] >= grid_top_offset-GRID_NUMBER_DOWN+1))
		    		DrawSequenceNote((GRID_LEFT_OFFSET+loop*COLUMN_WIDTH)/2, (grid_top_offset-seq[loop])*ROW_HEIGHT+GRID_TOP_OFFSET);
		    }
}

/*NOTE: These note values are in actual screen pixels, not grid coordinates*/
void DrawSequenceNote(int notex, int notey)
{
	int x, y;
	// draw the tank
	for(y = 0; y < objbomb_HEIGHT; y++)
		for (x = 0; x < (objbomb_WIDTH/2); x++)
			PlotPixel(x+notex, y+notey, objbombData[y*(objbomb_WIDTH/2)+x]);
}



void DrawCurrentFieldBox(int screen,int field)
{
	HorizontalBox(field_positions[screen-1][field-1][0],field_positions[screen-1][field-1][1],field_positions[screen-1][field-1][2],field_positions[screen-1][field-1][3],559);
}

// Mode 4 is 240(120)x160 by 8bit
void PlotPixel(int x,int y, unsigned short int c)	
{ 
	videoBuffer[(y) *120 + (x)] = (c); 
}

// Mode 4 is 240(120)x160 by 8bit
void PlotPixelTransparent(int x,int y, unsigned short int c)	
{ 
	unsigned short int temp;
	if ((c & 0x00FF) == 0) // bottom is transparent
	{
		if ((c & 0xFF00) == 0) // top is also transparent
			return;
		// so bottom transparent, top is not so....
		temp = ((videoBuffer[(y) * 120 + (x)]) & 0x00FF);
		temp |= c; // or it with c (to get the highpart of c in
		videoBuffer[(y) *120 + (x)] = (temp);
	}
	else
	if 	((c & 0xFF00) == 0) // only the top is transprent
	{
		temp = ((videoBuffer[(y) * 120 + (x)]) & 0xFF00);
		temp |= c;
		videoBuffer[(y) *120 + (x)] = (temp);
	}
	else // no part is transparent
		videoBuffer[(y) *120 + (x)] = (c); 
}
void HorizontalBox(int x1, int y1, int x2, int y2, u16 color)
{
	int width = x2 - x1;
	int height = y2-y1;
	Line(x1/2,y1,(x1+width)/2,y1,color);
	Line(x1/2,y1,x1/2,y1+height, color);
	Line((x1+width)/2, y1, (x1+width)/2, y1+height, color);
	Line(x1/2,y1+height, (x1+width)/2, y1+height, color);

}


/////////////////////Line//////////////////////////
//An implementation of the Bresenham Line algorithm
void Line(int x1, int y1, int x2, int y2, u16 color)
{
	//Variables
	int i, deltax, deltay, numpixels;
	int d, dinc1, dinc2;
	int x, xinc1, xinc2;
	int y, yinc1, yinc2;

	//Calculate deltaX and deltaY
	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	//Init vars
	if(deltax >= deltay)
	{
		//If x is independent variable
		numpixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		//If y is independant variable
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	//Move the right direction
	if(x1 > x2)
	{
		xinc1 = -xinc1;
		xinc2 = -xinc2;
	}
	if(y1 > y2)
	{
		yinc1 = -yinc1;
		yinc2 = -yinc2;
	}

	x = x1;
	y = y1;

	//Draw the pixels
	for(i = 1; i < numpixels; i++)
	{
		PlotPixel(x, y, color);

		if(d < 0)
		{
			d = d + dinc1;
			x = x + xinc1;
			y = y + yinc1;
		}
		else
		{
			d = d + dinc2;
			x = x + xinc2;
			y = y + yinc2;
		}
	}
}

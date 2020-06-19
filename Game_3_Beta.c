
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"



#define NES_MIRRORING 1


///// METASPRITES

#define TILE 0xd8
#define ATTR 0
#define CAR  0x81
#define RED  0x00  // - To ensure enemy cars are RED (traditional enemy color in games).
#define BRICK 0xf4


// define a 2x2 metasprite
const unsigned char metasprite[]={
        0,      0,      TILE+0,   ATTR, 
        0,      8,      TILE+1,   ATTR, 
        8,      0,      TILE+2,   ATTR, 
        8,      8,      TILE+3,   ATTR, 
        128};



//Sets up for Enemy Cars to avoid. 
const unsigned char enemyCar[]={
        0,      0,      CAR+0,   RED, 
        0,      8,      CAR+1,   RED, 
        8,      0,      CAR+2,   RED, 
        8,      8,      CAR+3,   RED, 
        128};


//This sets up the wall tile. 
const unsigned char wall[]={
        0,      0,      BRICK+0,   ATTR, 
        0,      8,      BRICK+1,   ATTR, 
        8,      0,      BRICK+2,   ATTR, 
        8,      8,      BRICK+3,   ATTR, 
        128};


/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0x0F,			// screen color

  0x01,0x30,0x27,0x00,	// background palette 0
  0x1C,0x20,0x2C,0x00,	// background palette 1
  0x00,0x10,0x20,0x00,	// background palette 2
  0x06,0x16,0x26,0x00,   // background palette 3

  0x16,0x35,0x24,0x00,	// sprite palette 0
  0x00,0x37,0x25,0x00,	// sprite palette 1
  0x0D,0x2D,0x3A,0x00,	// sprite palette 2
  0x0D,0x27,0x2A	// sprite palette 3
};



// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_clear();
  // set palette colors
  pal_all(PALETTE);
}


// number of actors (4 h/w sprites each)
#define NUM_ACTORS 4 //If only there is a way to check 


// actor x/y positions
byte human_actor_x;
byte human_actor_y;

byte car_actor_x[NUM_ACTORS];
byte car_actor_y[NUM_ACTORS];

//Sprite Stuff:
byte sp_actor_x[NUM_ACTORS];
byte sp_actor_y[NUM_ACTORS];



// actor x/y deltas per frame (signed)
sbyte human_actor_dx;
sbyte human_actor_dy;

sbyte car_actor_dx[NUM_ACTORS];
sbyte car_actor_dy[NUM_ACTORS];

sbyte sp_actor_dx[NUM_ACTORS];
sbyte sp_actor_dy[NUM_ACTORS];




//////////////////////////////////////
/// Function Listing:
//


//Scroll EX:
void scroll_demo() {
  int x = 0;   // x scroll position
  int y = 0;   // y scroll position
  int dy = 1;  // y scroll direction
  int dx = 1;
  // infinite loop
  while (1) {
    // wait for next frame
    ppu_wait_frame();
    // update y variable
    x += dx;
    // change direction when hitting either edge of scroll area
    if (x >= 479) dx = -1;
    if (x == 0) dx = 1;
    // set scroll register
    scroll(x, y);
  }
}


//Checks if there's a Car Crash
// 
//Requires: Car.X Cordinate, Car.Y Coordinate, Human X-Coordinate, 
//          Human Y-Coordinate, and the noCrash Bool value.
//
bool check_Crash(byte *cX, byte *cY, byte *hX, byte *hY, bool *impact)
{
  
  int carMin, carMax, yMin, yMax;
  // bool test;
  
  
  //Find the Min and Max range for the Crash:
  //- Both Min and Max ranges are within 13 from the car's current spot.
  //- If the min and max ranges are greater than the limits, they're rounded 
  //  to the nearest respective value. 
  
  //We start with the X-Cordinates
  carMin = *cX - 13;
  
   if (carMin < 0)
   {
     carMin = 0;
   }
  
  carMax = *cX + 13;
    if (carMax > 230)
    {
      carMax = 200;
    }
  
  
  //Now we assign the Y-Cordinates
  yMin = *cY - 13;
     if (yMin < 11)
     {
       yMin = 11;
     }
  
  yMax = *cY +13;
    if (yMax > 30)
    {
      
    }
 
  
  
  //Now we need to see if they're within the same Y-Coordinate:
  if (*hY > yMin && *hY < yMax)
  {
    //If they're on the same Y-Coordinate, then we check if they're within the ranges
    
      if(*hX > carMin && *hX < carMax)
      {
        *impact = false; //Set impact to false
      }
    
  }
  
  //If they're not within the same Y-Coordinate, then don't check for crash.
  else
  {
    *impact = true; //set impact to true
  }
 
  
  
  //Just to make sure it works;
 
    
  // test = true;
  
  return *impact; //Pass that value to noCrash to confirm the impact.
  
}

//The Main Function:
void main(void)
{
   char i;	 // actor index
   char oam_id;	 // sprite ID
   char pad;     // Controller Index
   char str[32]; //This is for checking elements. 
  
   bool isClear; //A bool to confirm wether or not the player reached the goal.
   bool noCrash; //A bool to confirm wether there was car crash or not
   bool collision; //A bool to confirm there was a collision.
  
  
  
   int x,y,dy, level, stop;
    x = 0;
    y = 220;
    dy = -2;
    level = 1;
    stop = 120;
  
  setup_graphics();
    // draw message  
    // vram_adr(NTADR_A(2,2));
    // vram_write("HELLO, WORLD!", 12);
   
  
  isClear = false;
  noCrash = true;
  collision = false;
  
  //Assigning Cordinates;
  // - This sets where the metasprites will spawn in. 
 
  for (i = 0; i < NUM_ACTORS; i++)
  {
    //For the person:
    if(i == 0)
    {
      //Place the person somewhere (ideally not outside the borders).
       human_actor_x = rand();
       human_actor_y = 200;
       human_actor_dx = 5;
       human_actor_dy = 5;
    }
    
    //Now, let's put the cars.
    else 
    {  car_actor_x[i] = 30;
       car_actor_y[i] = 40 * i;
       
       car_actor_dx[i] = 3;
       car_actor_dy[i] = 0;
    }
    
    
   
  
  }
  
 
 
  
  // enable rendering
  ppu_on_all();
  
  
  
  
  
  
  //Level Loop:
  
  //The main game loop:
  // - The game continues until the player makes it to the top
  //   or if a car crashes into the player.
  
  while(level < 4 && collision == false)
  {
    
    // start with OAMid/sprite 0
    oam_id = 0;
    
    
    //Controller Inputs:
    pad = pad_poll(0); // We only want to read from player 1
    			// - Arrowkeys.
    
      //Moves the Human provided they don't pass certain points.
      if (pad&PAD_LEFT && human_actor_x>10) human_actor_dx=-1;
      else if (pad&PAD_RIGHT && human_actor_x<230) human_actor_dx=1;
      else human_actor_dx=0;
      // move actor[i] up/down
      if (pad&PAD_UP && human_actor_y>10) human_actor_dy=-1;
      else if (pad&PAD_DOWN && human_actor_y<202) human_actor_dy=1;
      else human_actor_dy=0;
    
    
      
      //Drawing the Metasprites:
      for (i=0; i<NUM_ACTORS; i++) 
      {
        
        
        
        //If all is programed correctly, you can move the human.
        if(i == 0)
        {
          oam_id = oam_meta_spr(human_actor_x, human_actor_y, oam_id, metasprite);
          human_actor_x += human_actor_dx;
          human_actor_y += human_actor_dy;
         
           
          
        }
        
        
        // The cars should move on their own here.
        else
        {
        
          //Currently, they're set to go in a straight line and warp around the screen.
          oam_id = oam_meta_spr(car_actor_x[i], car_actor_y[i], oam_id, enemyCar);
          car_actor_x[i] += car_actor_dx[i];
          car_actor_y[i] += car_actor_dy[i];
          
          
          
           // "Collision Check":
           // - Calls a function to check if the human is in a certain range within
           //   the car.
        
      
          //Gets the results of the check.
           noCrash = check_Crash(&car_actor_x[i], &car_actor_y[i], &human_actor_x, &human_actor_y, &noCrash);
          
          if (noCrash == false) // If there was a crash, update the bools.
          {
             collision = true; //There was a crash.
           
           
          }
          
            
          else //As long as there wasn't a crash, show them the goal.
          {
             // Show the other message if we don't crash.
              memset(str, 0, sizeof(str)); 
              sprintf(str,"Level %d",level);
              vrambuf_put(NTADR_A(2,20), str, 32);
            
          }
          
        
          
          
        }
        
        
         //Also for testing. Keeps track of the counter. Turn on
         // to see human's corrdiantes in real time.
           // memset(str, 0, sizeof(str)); 
           //sprintf(str,"X-Axis: %d",actor_x);
           // vrambuf_put(NTADR_A(2,4), str, 32);
            
        
           // //memset(str, 0, sizeof(str)); 
           //sprintf(str,"Y-Axis: %d",actor_y);
           //  vrambuf_put(NTADR_A(2,8), str, 32);
        
        
        
           // GOAL CHECK:
        
           // Important check. If we can get this work, we have a goal point.
            if (human_actor_y <= 10)
           {
              // isClear = true; // The human made it to the goal.
              
              //Test Check:
              //This only shows up if we reach the Goal Point
             // memset(str, 0, sizeof(str)); 
              //sprintf(str,"Reached Goal");
              // vrambuf_put(NTADR_A(2,16), str, 32);
              
              
              level ++;
              while (y > stop)
              {          
                 ppu_wait_frame(); // Remove this if you want to see the human to go 
                      // at lightspeed (or at least till the borders
                 y+=dy;
                 scroll(x,y);
              }
              
              stop = stop - 100;
              
                for (i = 0; i < NUM_ACTORS; i++)
  		{
   	            //For the person:
   		   if(i == 0)
    		   {
    		     //Place the person somewhere (ideally not outside the borders.
      		       human_actor_x = rand();
    		       human_actor_y = 200;
     		       human_actor_dx = 2;
  		       human_actor_dy = 2;
  		    }
    
  		    //Now, let's put the cars.
    		    else 
   		    { 
                      
                       if(level == 2)
                       {
                         car_actor_x[i] = 30;
     		         car_actor_y[i] = 50 * i;
        
      		         car_actor_dx[i] = 3;
     		         car_actor_dy[i] = 0;
                       }
                      
                       if (level == 3)
                       {
                         car_actor_x[i] = 30;
     		         car_actor_y[i] = 40 * i;
        
      		         car_actor_dx[i] = 3;
     		         car_actor_dy[i] = 1;                         
                         
                       }
                        
                     
                     
                     }
    
    
   
  
                }
              
              
            
          }
         

        
        
      } //End of while loop

    
    
     // hide rest of sprites
    // if we haven't wrapped oam_id around to 0
    if (oam_id!=0) oam_hide_rest(oam_id);
    // wait for next frame
    ppu_wait_frame(); // Remove this if you want to see the human to go 
                      // at lightspeed (or at least till the borders).
    
   
    
      //This is needed to make sure loading to the vram buffer works.
      vrambuf_clear();
      set_vram_update(updbuf);
    
  
        
  }
  
  //End Screen for the game:
  while(1)
  {
    //Did they make it on time? Congradulate the player.
    if(collision == false)
    {
       memset(str, 0, sizeof(str)); 
       sprintf(str,"You Win!");
       vrambuf_put(NTADR_A(2,20), str, 32);
    }
    
    //Or did they crash? Then tell them the truth:
    else
    {
       //Test Check:
              // This only shows up if we crash
            memset(str, 0, sizeof(str)); 
            sprintf(str,"Crashed...");
            vrambuf_put(NTADR_A(2,22), str, 32);
    }
          
  }
  
   
  
  
}

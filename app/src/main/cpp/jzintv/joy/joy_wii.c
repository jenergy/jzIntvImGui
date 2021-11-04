/*------------------------------------------------------------------------------------
* Hi, this file converts direct wii controller inputs to SDL events,
* so it's possible to use and configure easily controllers, with sticks included.
*
* Thanks to:
*
*        http://www.devsgen.com/wiki/doku.php/tutoriels:wii:manette_manette_virtuelle
*        http://www.lazyfoo.net/SDL_tutorials/index.php
*        and obviously Jow Zbiciak and jzintv
*
* Enjoy!
*
* Daniele Moglia (daniele.moglia@gmail.com)
*------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <ogc/pad.h>
#include "SDL/SDL.h"

#define JOY_BTN_00 0
#define JOY_BTN_01 1
#define JOY_BTN_02 2
#define JOY_BTN_03 3
#define JOY_BTN_04 4
#define JOY_BTN_05 5
#define JOY_BTN_06 6
#define JOY_BTN_07 7
#define JOY_BTN_08 8
#define JOY_BTN_09 9
#define JOY_BTN_10 10
#define JOY_BTN_11 11
#define JOY_BTN_12 12
#define JOY_BTN_13 13

#define MAX_EVENTS_PER_CYCLE 50

//************************************************
// NUNCHUK defines
//************************************************

#define NUNCHUK_X_TOLERANCE 10
#define NUNCHUK_Y_TOLERANCE 10

//************************************************
// Classic controller defines
//************************************************

//Left pad defines

#define CLASSIC_LEFT_X_TOLERANCE 10
#define CLASSIC_LEFT_Y_TOLERANCE 10
#define DEFAULT_CLASSIC_LEFT_X_CENTER 30
#define DEFAULT_CLASSIC_LEFT_Y_CENTER 30

//Right pad defines

#define CLASSIC_RIGHT_X_TOLERANCE 5
#define CLASSIC_RIGHT_Y_TOLERANCE 5
#define DEFAULT_CLASSIC_RIGHT_X_CENTER 15
#define DEFAULT_CLASSIC_RIGHT_Y_CENTER 15

//************************************************
// Gamecube defines
//************************************************

//Left pad defines

#define DEFAULT_GC_LEFT_X_CENTER 0
#define DEFAULT_GC_LEFT_Y_CENTER 0
#define GC_LEFT_X_TOLERANCE 10
#define GC_LEFT_Y_TOLERANCE 10

//Right pad defines

#define DEFAULT_GC_RIGHT_X_CENTER 0
#define DEFAULT_GC_RIGHT_Y_CENTER 0
#define GC_RIGHT_X_TOLERANCE 4
#define GC_RIGHT_Y_TOLERANCE 4

int lastHatSent           [2] = {SDL_HAT_CENTERED, SDL_HAT_CENTERED};
int joyNunchukUsed        [2] = { 0, 1 };
int joyClassicUsed        [2] = { 1, 0 };
int joyGcUsed             [2] = { 0, 0 };
int joyClassicXLeftCenter [2] = { DEFAULT_CLASSIC_LEFT_X_CENTER, DEFAULT_CLASSIC_LEFT_X_CENTER };
int joyClassicYLeftCenter [2] = { DEFAULT_CLASSIC_LEFT_Y_CENTER, DEFAULT_CLASSIC_LEFT_Y_CENTER };
int joyClassicXRightCenter[2] = { DEFAULT_CLASSIC_RIGHT_X_CENTER, DEFAULT_CLASSIC_RIGHT_X_CENTER };
int joyClassicYRightCenter[2] = { DEFAULT_CLASSIC_RIGHT_Y_CENTER, DEFAULT_CLASSIC_RIGHT_Y_CENTER };
int joyGcXLeftCenter      [2] = { DEFAULT_GC_LEFT_X_CENTER, DEFAULT_GC_LEFT_X_CENTER };
int joyGcYLeftCenter      [2] = { DEFAULT_GC_LEFT_Y_CENTER, DEFAULT_GC_LEFT_Y_CENTER };
int joyGcXRightCenter     [2] = { DEFAULT_GC_RIGHT_X_CENTER, DEFAULT_GC_RIGHT_X_CENTER };
int joyGcYRightCenter     [2] = { DEFAULT_GC_RIGHT_Y_CENTER, DEFAULT_GC_RIGHT_Y_CENTER };
u32 buttonsDown           [2];
u32 buttonsHeld           [2];
u32 buttonsUp             [2];

int getAngleIndex(int x, int y)
{
  if (
       (x >= 0)&&
       (y >= 0)
      )
  {
     if               (4*y <   x) return 0;
     if (4*y >=   x && 2*y <   x) return 1;
     if (2*y >=   x && 4*y < 3*x) return 2;
     if (4*y >= 3*x &&   y <   x) return 3;
     if (  y >=   x && 3*y < 4*x) return 4;
     if (3*y >= 4*x &&   y < 2*x) return 5;
     if (  y >= 2*x &&   y < 4*x) return 6;
     if (  y >= 4*x)              return 7;
  }
  else if (
       (x < 0)&&
       (y >= 0)
      )
  {
     if                  (y >-4*x) return 8;
     if (  y <= -4*x &&   y >-2*x) return 9;
     if (  y <= -2*x && 3*y >-4*x) return 10;
     if (3*y <= -4*x &&   y >  -x) return 11;
     if (  y <=   -x && 4*y >-3*x) return 12;
     if (4*y <= -3*x && 2*y >  -x) return 13;
     if (2*y <=   -x && 4*y >  -x) return 14;
     if (4*y <=   -x)              return 15;
  }
    else if (
       (x < 0)&&
       (y < 0)
      )
  {
     if                (4*y >   x) return 16;
     if ( 4*y <=   x && 2*y >   x) return 17;
     if ( 2*y <=   x && 4*y > 3*x) return 18;
     if ( 4*y <= 3*x &&   y >   x) return 19;
     if (   y <=   x && 3*y > 4*x) return 20;
     if ( 3*y <= 4*x &&   y > 2*x) return 21;
     if (   y <= 2*x &&   y > 4*x) return 22;
     if (   y <= 4*x)              return 23;
  }
    else if (
       (x >= 0)&&
       (y < 0)
      )
  {
     if                   (y <-4*x) return 24;
     if (   y >=-4*x &&    y <-2*x) return 25;
     if (   y >=-2*x &&  3*y <-4*x) return 26;
     if ( 3*y >=-4*x &&    y <  -x) return 27;
     if (   y >=  -x &&  4*y <-3*x) return 28;
     if ( 4*y >=-3*x &&  2*y <   x) return 29;
     if ( 2*y >=   x &&  4*y <   x) return 30;
     if ( 4*y >=   x)               return 31;
  }
  return -1;
}

void getHatEvent(SDL_Event* hatEvent, int angleIndex)
{
  if      ((angleIndex>=2)  && (angleIndex<6))  hatEvent->jhat.value = SDL_HAT_RIGHTUP;
  else if ((angleIndex>=6)  && (angleIndex<10)) hatEvent->jhat.value = SDL_HAT_UP;
  else if ((angleIndex>=10) && (angleIndex<14)) hatEvent->jhat.value = SDL_HAT_LEFTUP;
  else if ((angleIndex>=14) && (angleIndex<18)) hatEvent->jhat.value = SDL_HAT_LEFT;
  else if ((angleIndex>=18) && (angleIndex<22)) hatEvent->jhat.value = SDL_HAT_LEFTDOWN;
  else if ((angleIndex>=22) && (angleIndex<26)) hatEvent->jhat.value = SDL_HAT_DOWN;
  else if ((angleIndex>=26) && (angleIndex<30)) hatEvent->jhat.value = SDL_HAT_RIGHTDOWN;
  else hatEvent->jhat.value = SDL_HAT_RIGHT;
}

void getSDLJoyAxisEvent(SDL_Event* xevent, SDL_Event* yevent, int angleIndex)
{
  if ((angleIndex==31) || (angleIndex==0))
  {
     // Right
     xevent->jaxis.value = 10000;
     yevent->jaxis.value = 0;
  }
  else if ((angleIndex==1) || (angleIndex==2))
  {
     // Right Up Right
     xevent->jaxis.value = 10000;
     yevent->jaxis.value = -5000;
  }
  else if ((angleIndex==3) || (angleIndex==4))
  {
     // Right Up
     xevent->jaxis.value = 10000;
     yevent->jaxis.value = -10000;
  }
  else if ((angleIndex==5) || (angleIndex==6))
  {
     // Right Up Up
     xevent->jaxis.value = 5000;
     yevent->jaxis.value = -10000;
  }
  else if ((angleIndex==7) || (angleIndex==8))
  {
     // Up
     xevent->jaxis.value = 0;
     yevent->jaxis.value = -10000;
  }
  else if ((angleIndex==9) || (angleIndex==10))
  {
     // Left Up Up
     xevent->jaxis.value = -5000;
     yevent->jaxis.value = -10000;
  }
  else if ((angleIndex==11) || (angleIndex==12))
  {
     // Left Up
     xevent->jaxis.value = -10000;
     yevent->jaxis.value = -10000;
  }
  else if ((angleIndex==13) || (angleIndex==14))
  {
     // Left Up Left
     xevent->jaxis.value = -10000;
     yevent->jaxis.value = -5000;
  }
  else if ((angleIndex==15) || (angleIndex==16))
  {
     // Left
     xevent->jaxis.value = -10000;
     yevent->jaxis.value = 0;
  }
  else if ((angleIndex==17) || (angleIndex==18))
  {
     // Left Down Left
     xevent->jaxis.value = -10000;
     yevent->jaxis.value = 5000;
  }
  else if ((angleIndex==19) || (angleIndex==20))
  {
     // Left Down
     xevent->jaxis.value = -10000;
     yevent->jaxis.value = 10000;
  }
  else if ((angleIndex==21) || (angleIndex==22))
  {
     // Left Down Down
     xevent->jaxis.value = -5000;
     yevent->jaxis.value = 10000;
  }
  else if ((angleIndex==23) || (angleIndex==24))
  {
     // Down
     xevent->jaxis.value = 0;
     yevent->jaxis.value = 10000;
  }
  else if ((angleIndex==25) || (angleIndex==26))
  {
     // Right Down Down
     xevent->jaxis.value = 5000;
     yevent->jaxis.value = 10000;
  }
  else if ((angleIndex==27) || (angleIndex==28))
  {
     // Right Down
     xevent->jaxis.value = 10000;
     yevent->jaxis.value = 10000;
  }
  else if ((angleIndex==29) || (angleIndex==30))
  {
     // Right Down Right
     xevent->jaxis.value = 10000;
     yevent->jaxis.value = 5000;
  }
}

void getWiimoteAndNunchukPadInput()
{
   u32 pressed;
   int virtualJoyIndex=0;
   int physicalJoyIndex=0;
   SDL_Event keyDownEvent[MAX_EVENTS_PER_CYCLE];

   //************************************************
   // Buttons PRESSED
   //************************************************
   for (virtualJoyIndex=0; virtualJoyIndex<2; virtualJoyIndex++)
   {
      if (joyNunchukUsed[virtualJoyIndex]==0) continue;
      physicalJoyIndex = virtualJoyIndex;
      if (joyGcUsed[0]==1)
      {
         physicalJoyIndex=0;
      }
      pressed = buttonsDown[physicalJoyIndex];
      int keyDownIndex=0;

      if (pressed & WPAD_BUTTON_2)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_09;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_BUTTON_1)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_10;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_BUTTON_A)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_11;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_BUTTON_PLUS)
      {
         keyDownEvent[keyDownIndex].key.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].key.type=SDL_KEYDOWN;
         keyDownEvent[keyDownIndex].key.keysym.sym=SDLK_F12;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_BUTTON_HOME)
      {
         keyDownEvent[keyDownIndex].key.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].key.type=SDL_KEYDOWN;
         keyDownEvent[keyDownIndex].key.keysym.sym=SDLK_F1;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_BUTTON_MINUS)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_13;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_BUTTON_B)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_02;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_NUNCHUK_BUTTON_C)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_NUNCHUK_BUTTON_Z)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_01;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      //************************************************
      // Buttons PRESSED --> D-Pads
      //************************************************
      /*
      int x=0;
      int y=0;
      int angleIndex=0;

      if (pressed & WPAD_BUTTON_RIGHT)y=1000;
      if (pressed & WPAD_BUTTON_DOWN) x=1000;
      if (pressed & WPAD_BUTTON_LEFT) y=-1000;
      if (pressed & WPAD_BUTTON_UP)   x=-1000;

      angleIndex = getAngleIndex(x,y);

      SDL_Event hatEvent;
      hatEvent.type = SDL_JOYHATMOTION;
      hatEvent.jhat.which=virtualJoyIndex;
      hatEvent.jhat.hat = 0;
      hatEvent.jhat.value = SDL_HAT_CENTERED;

      if ((x!=0) || (y!=0))
      {
          getHatEvent(&hatEvent, angleIndex);
      }
      if (lastHatSent[virtualJoyIndex] != hatEvent.jhat.value)
      {
        SDL_PushEvent(&hatEvent);
        lastHatSent[virtualJoyIndex] = hatEvent.jhat.value;
      }
      */
      //************************************************
      // Buttons HELD --> D-Pads
      //************************************************

      pressed = buttonsHeld[physicalJoyIndex];
      int x=0;
      int y=0;
      int angleIndex=0;

      if (pressed & WPAD_BUTTON_RIGHT)y=1000;
      if (pressed & WPAD_BUTTON_DOWN) x=1000;
      if (pressed & WPAD_BUTTON_LEFT) y=-1000;
      if (pressed & WPAD_BUTTON_UP)   x=-1000;

      angleIndex = getAngleIndex(x,y);

      SDL_Event hatEvent;
      hatEvent.type = SDL_JOYHATMOTION;
      hatEvent.jhat.which=virtualJoyIndex;
      hatEvent.jhat.hat = 0;
      hatEvent.jhat.value = SDL_HAT_CENTERED;

      if ((x!=0) || (y!=0))
      {
          getHatEvent(&hatEvent, angleIndex);
      }
      if (lastHatSent[virtualJoyIndex] != hatEvent.jhat.value)
      {
        SDL_PushEvent(&hatEvent);
        lastHatSent[virtualJoyIndex] = hatEvent.jhat.value;
      }

      //************************************************
      // Buttons RELEASED
      //************************************************

      pressed = buttonsUp[physicalJoyIndex];
      int keyUpIndex=0;
      SDL_Event keyUpEvent[MAX_EVENTS_PER_CYCLE];

      if (pressed & WPAD_BUTTON_2)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_09;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_BUTTON_1)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_10;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_BUTTON_A)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_11;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_BUTTON_PLUS)
      {
         keyUpEvent[keyUpIndex].key.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].key.type=SDL_KEYUP;
         keyUpEvent[keyUpIndex].key.keysym.sym=SDLK_F12;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_BUTTON_HOME)
      {
         keyUpEvent[keyUpIndex].key.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].key.type=SDL_KEYUP;
         keyUpEvent[keyUpIndex].key.keysym.sym=SDLK_F1;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_BUTTON_MINUS)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_13;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_BUTTON_B)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_02;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_NUNCHUK_BUTTON_C )
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_NUNCHUK_BUTTON_Z)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_01;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      //************************************************
      // Stick --> JoyAXIS
      //************************************************

      struct expansion_t ext;
      WPAD_Expansion(physicalJoyIndex, &ext);

      x = ext.nunchuk.js.pos.x-ext.nunchuk.js.center.x;
      y = ext.nunchuk.js.pos.y-ext.nunchuk.js.center.y;
      angleIndex = getAngleIndex(x,y);

      SDL_Event xevent;
      xevent.type = SDL_JOYAXISMOTION;
      xevent.jaxis.which=virtualJoyIndex;
      xevent.jaxis.axis=0;

      SDL_Event yevent;
      yevent.type = SDL_JOYAXISMOTION;
      yevent.jaxis.which=virtualJoyIndex;
      yevent.jaxis.axis = 1;

      getSDLJoyAxisEvent(&xevent, &yevent, angleIndex);

      if (
          (x < NUNCHUK_X_TOLERANCE) &&
          (x > -NUNCHUK_X_TOLERANCE)
         )
      {
         xevent.jaxis.value = 0;
      }

      if (
          (y < NUNCHUK_Y_TOLERANCE) &&
          (y > -NUNCHUK_Y_TOLERANCE)
         )
      {
         yevent.jaxis.value = 0;
      }
      if (ext.type & WPAD_EXP_NUNCHUK)
      {
        SDL_PushEvent(&xevent);
        SDL_PushEvent(&yevent);
      }
   }
}

void getClassicControllerPadInput()
{
   u32 pressed;
   int dpadPressed=0;
   int virtualJoyIndex=0;
   int physicalJoyIndex=0;
   SDL_Event keyDownEvent[MAX_EVENTS_PER_CYCLE];

   //************************************************
   // Buttons PRESSED
   //************************************************
   for (virtualJoyIndex=0; virtualJoyIndex<2; virtualJoyIndex++)
   {
      if (joyClassicUsed[virtualJoyIndex]==0) continue;
      physicalJoyIndex = virtualJoyIndex;
      if (joyGcUsed[0]==1)
      {
         physicalJoyIndex=0;
      }
      pressed = buttonsDown[physicalJoyIndex];
      int keyDownIndex=0;

      if (pressed & WPAD_CLASSIC_BUTTON_A)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_09;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_Y)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_10;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_X)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_11;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_B)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_12;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_PLUS)
      {
         keyDownEvent[keyDownIndex].key.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].key.type=SDL_KEYDOWN;
         keyDownEvent[keyDownIndex].key.keysym.sym=SDLK_F12;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_HOME)
      {
         keyDownEvent[keyDownIndex].key.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].key.type=SDL_KEYDOWN;
         keyDownEvent[keyDownIndex].key.keysym.sym=SDLK_F1;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_MINUS)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_13;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_ZR)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_02;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_FULL_R)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_ZL)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_01;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_FULL_L)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      //************************************************
      // Buttons PRESSED --> D-Pads
      //************************************************

      if (pressed & WPAD_CLASSIC_BUTTON_RIGHT)
      {
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 0;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_DOWN)
      {
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 1;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_LEFT)
      {
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 0;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_UP)
      {
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 1;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      //************************************************
      // Buttons Held --> D-Pads
      //************************************************

      pressed = buttonsHeld[physicalJoyIndex];
      int keyHeldIndex=0;
      SDL_Event keyHeldEvent[MAX_EVENTS_PER_CYCLE];

      if (pressed & WPAD_CLASSIC_BUTTON_RIGHT)
      {
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 0;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_DOWN)
      {
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 1;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_LEFT)
      {
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 0;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_UP)
      {
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 1;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      //************************************************
      // Buttons RELEASED
      //************************************************

      pressed = buttonsUp[physicalJoyIndex];
      int keyUpIndex=0;
      SDL_Event keyUpEvent[MAX_EVENTS_PER_CYCLE];

      if (pressed & WPAD_CLASSIC_BUTTON_A)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_09;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_Y)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_10;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_X)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_11;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_B)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_12;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_PLUS)
      {
         keyUpEvent[keyUpIndex].key.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].key.type=SDL_KEYUP;
         keyUpEvent[keyUpIndex].key.keysym.sym=SDLK_F12;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_HOME)
      {
         keyUpEvent[keyUpIndex].key.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].key.type=SDL_KEYUP;
         keyUpEvent[keyUpIndex].key.keysym.sym=SDLK_F1;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_MINUS)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_13;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_ZR)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_02;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_FULL_R )
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_ZL)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_01;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & WPAD_CLASSIC_BUTTON_FULL_L)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      //************************************************
      // Buttons RELEASED --> D-Pads
      //************************************************

      if (pressed & WPAD_CLASSIC_BUTTON_RIGHT)
      {
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 0;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_DOWN)
      {
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 1;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_LEFT)
      {
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 0;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      if (pressed & WPAD_CLASSIC_BUTTON_UP)
      {
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 1;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      //************************************************
      // Left stick --> JoyAXIS
      //************************************************
      int x=0;
      int y=0;
      struct expansion_t ext;
      int angleIndex = 0;
      WPAD_Expansion(physicalJoyIndex, &ext);

      if (!dpadPressed)
      {
         x = ext.classic.ljs.pos.x-DEFAULT_CLASSIC_LEFT_X_CENTER;
         y = ext.classic.ljs.pos.y-DEFAULT_CLASSIC_LEFT_Y_CENTER;
         angleIndex = getAngleIndex(x,y);

         SDL_Event xevent;
         xevent.type = SDL_JOYAXISMOTION;
         xevent.jaxis.which=virtualJoyIndex;
         xevent.jaxis.axis = 0;

         SDL_Event yevent;
         yevent.type = SDL_JOYAXISMOTION;
         yevent.jaxis.which=virtualJoyIndex;
         yevent.jaxis.axis = 1;

         getSDLJoyAxisEvent(&xevent, &yevent, angleIndex);

         if (
             (x < CLASSIC_LEFT_X_TOLERANCE) &&
             (x > -CLASSIC_LEFT_X_TOLERANCE)
             )
         {
            xevent.jaxis.value = 0;
         }

         if (
             (y < CLASSIC_LEFT_Y_TOLERANCE) &&
             (y > -CLASSIC_LEFT_Y_TOLERANCE)
            )
         {
            yevent.jaxis.value = 0;
         }
         if (ext.type & WPAD_EXP_CLASSIC)
         {
            SDL_PushEvent(&xevent);
            SDL_PushEvent(&yevent);
         }
      }


      //************************************************
      // Right stick --> HAT
      //************************************************

      x = ext.classic.rjs.pos.x-DEFAULT_CLASSIC_RIGHT_X_CENTER;
      y = ext.classic.rjs.pos.y-DEFAULT_CLASSIC_RIGHT_Y_CENTER;
      angleIndex = getAngleIndex(x,y);

      SDL_Event hatEvent;
      hatEvent.type = SDL_JOYHATMOTION;
      hatEvent.jhat.which=virtualJoyIndex;
      hatEvent.jhat.hat = 0;
      hatEvent.jhat.value = SDL_HAT_CENTERED;

      if (
          (x > CLASSIC_RIGHT_X_TOLERANCE) ||
          (x < -CLASSIC_RIGHT_X_TOLERANCE)||
          (y > CLASSIC_RIGHT_Y_TOLERANCE) ||
          (y < -CLASSIC_RIGHT_Y_TOLERANCE)
         )
      {
          getHatEvent(&hatEvent, angleIndex);
      }
      if (lastHatSent[hatEvent.jhat.which] != hatEvent.jhat.value)
      {
        SDL_PushEvent(&hatEvent);
        lastHatSent[hatEvent.jhat.which] = hatEvent.jhat.value;
      }
   }
}

void getGamecubePadInput()
{
   u32 pressed;
   int dpadPressed = 0;
   int virtualJoyIndex=0;
   int physicalJoyIndex=0;
   SDL_Event keyDownEvent[MAX_EVENTS_PER_CYCLE];

   //************************************************
   // Buttons PRESSED
   //************************************************
   for (virtualJoyIndex=0; virtualJoyIndex<2; virtualJoyIndex++)
   {
      if (joyGcUsed[virtualJoyIndex]==0) continue;
      physicalJoyIndex = virtualJoyIndex;
      if (joyGcUsed[0]==0)
      {
         physicalJoyIndex=0;
      }
      pressed = PAD_ButtonsDown(physicalJoyIndex);
      int keyDownIndex=0;

      if (pressed & PAD_BUTTON_A)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_09;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_BUTTON_Y)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_10;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_BUTTON_X)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_11;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_BUTTON_B)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_12;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_BUTTON_START)
      {
         keyDownEvent[keyDownIndex].key.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].key.type=SDL_KEYDOWN;
         keyDownEvent[keyDownIndex].key.keysym.sym=SDLK_F1;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_TRIGGER_R)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_02;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_TRIGGER_Z)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      if (pressed & PAD_TRIGGER_L)
      {
         keyDownEvent[keyDownIndex].jbutton.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jbutton.type=SDL_JOYBUTTONDOWN;
         keyDownEvent[keyDownIndex].jbutton.button=JOY_BTN_01;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
      }

      //************************************************
      // Buttons PRESSED --> D-Pads
      //************************************************

       if (
          (pressed & PAD_BUTTON_RIGHT)
         )
      {
         //x
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 0;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_DOWN)
         )
      {
         //y
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 1;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_LEFT)
         )
      {
         //x
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 0;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_UP)
         )
      {
         //x
         keyDownEvent[keyDownIndex].type = SDL_JOYAXISMOTION;
         keyDownEvent[keyDownIndex].jaxis.axis = 1;
         keyDownEvent[keyDownIndex].jaxis.which=virtualJoyIndex;
         keyDownEvent[keyDownIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyDownEvent[keyDownIndex++]);
         dpadPressed=1;
      }

      //************************************************
      // Buttons Held --> D-Pads
      //************************************************


      pressed = PAD_ButtonsHeld(physicalJoyIndex);
      int keyHeldIndex=0;
      SDL_Event keyHeldEvent[MAX_EVENTS_PER_CYCLE];

      if (
          (pressed & PAD_BUTTON_RIGHT)
         )
      {
         //x
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 0;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_DOWN)
         )
      {
         //y
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 1;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_LEFT)
         )
      {
         //x
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 0;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_UP)
         )
      {
         //x
         keyHeldEvent[keyHeldIndex].type = SDL_JOYAXISMOTION;
         keyHeldEvent[keyHeldIndex].jaxis.axis = 1;
         keyHeldEvent[keyHeldIndex].jaxis.which=virtualJoyIndex;
         keyHeldEvent[keyHeldIndex].jaxis.value=-10000;
         SDL_PushEvent(&keyHeldEvent[keyHeldIndex++]);
         dpadPressed=1;
      }

      //************************************************
      // Buttons RELEASED
      //************************************************

      pressed = PAD_ButtonsUp(physicalJoyIndex);
      int keyUpIndex=0;
      SDL_Event keyUpEvent[MAX_EVENTS_PER_CYCLE];

      if (pressed & PAD_BUTTON_A)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_09;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & PAD_BUTTON_Y)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_10;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & PAD_BUTTON_X)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_11;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & PAD_BUTTON_B)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_12;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & PAD_TRIGGER_R)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_02;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & PAD_TRIGGER_Z)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_00;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      if (pressed & PAD_TRIGGER_L)
      {
         keyUpEvent[keyUpIndex].jbutton.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jbutton.type=SDL_JOYBUTTONUP;
         keyUpEvent[keyUpIndex].jbutton.button=JOY_BTN_01;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
      }

      //************************************************
      // Buttons RELEASED --> D-Pads
      //************************************************

      if (
          (pressed & PAD_BUTTON_RIGHT)
         )
      {
         //x
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 0;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_DOWN)
         )
      {
         //y
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 1;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_LEFT)
         )
      {
         //x
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 0;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      if (
          (pressed & PAD_BUTTON_UP)
         )
      {
         //x
         keyUpEvent[keyUpIndex].type = SDL_JOYAXISMOTION;
         keyUpEvent[keyUpIndex].jaxis.axis = 1;
         keyUpEvent[keyUpIndex].jaxis.which=virtualJoyIndex;
         keyUpEvent[keyUpIndex].jaxis.value=0;
         SDL_PushEvent(&keyUpEvent[keyUpIndex++]);
         dpadPressed=1;
      }

      //************************************************
      // Left stick --> JoyAXIS
      //************************************************
      int x = 0;
      int y = 0;
      int angleIndex = 0;
      s8 X1=0, Y1=0, X2=0, Y2=0;
      X1 = PAD_StickX(physicalJoyIndex);
      Y1 = PAD_StickY(physicalJoyIndex);
      X2 = PAD_SubStickX(physicalJoyIndex);
      Y2 = PAD_SubStickY(physicalJoyIndex);

      if (!dpadPressed)
      {
         x = X1-DEFAULT_GC_LEFT_X_CENTER;
         y = Y1-DEFAULT_GC_LEFT_Y_CENTER;
         angleIndex = getAngleIndex(x,y);

         SDL_Event xevent;
         xevent.type = SDL_JOYAXISMOTION;
         xevent.jaxis.which=virtualJoyIndex;
         xevent.jaxis.axis = 0;

         SDL_Event yevent;
         yevent.type = SDL_JOYAXISMOTION;
         yevent.jaxis.which=virtualJoyIndex;
         yevent.jaxis.axis = 1;

         getSDLJoyAxisEvent(&xevent, &yevent, angleIndex);

         if (
             (x < GC_LEFT_X_TOLERANCE) &&
             (x > -GC_LEFT_X_TOLERANCE)
             )
         {
            xevent.jaxis.value = 0;
         }

         if (
             (y < GC_LEFT_Y_TOLERANCE) &&
             (y > -GC_LEFT_Y_TOLERANCE)
            )
         {
            yevent.jaxis.value = 0;
         }

         SDL_PushEvent(&xevent);
         SDL_PushEvent(&yevent);
      }

      //************************************************
      // Right stick --> HAT
      //************************************************

      x = X2-DEFAULT_GC_RIGHT_X_CENTER;
      y = Y2-DEFAULT_GC_RIGHT_Y_CENTER;
      angleIndex = getAngleIndex(x,y);

      SDL_Event hatEvent;
      hatEvent.type = SDL_JOYHATMOTION;
      hatEvent.jhat.which=virtualJoyIndex;
      hatEvent.jhat.hat = 0;
      hatEvent.jhat.value = SDL_HAT_CENTERED;

      if (
          (x > GC_RIGHT_X_TOLERANCE) ||
          (x < -GC_RIGHT_X_TOLERANCE)||
          (y > GC_RIGHT_Y_TOLERANCE) ||
          (y < -GC_RIGHT_Y_TOLERANCE)
         )
      {
         getHatEvent(&hatEvent, angleIndex);
      }
      if (lastHatSent[hatEvent.jhat.which] != hatEvent.jhat.value)
      {
        SDL_PushEvent(&hatEvent);
        lastHatSent[hatEvent.jhat.which] = hatEvent.jhat.value;
      }
   }
}

void getWiiJoyEvents()
{
   WPAD_ScanPads();
   PAD_ScanPads();
   buttonsDown[0] = WPAD_ButtonsDown(WPAD_CHAN_0);
   buttonsDown[1] = WPAD_ButtonsDown(WPAD_CHAN_1);
   buttonsHeld[0] = WPAD_ButtonsHeld(WPAD_CHAN_0);
   buttonsHeld[1] = WPAD_ButtonsHeld(WPAD_CHAN_1);
   buttonsUp[0]   = WPAD_ButtonsUp(WPAD_CHAN_0);
   buttonsUp[1]   = WPAD_ButtonsUp(WPAD_CHAN_1);
   getGamecubePadInput();
   getWiimoteAndNunchukPadInput();
   getClassicControllerPadInput();
}

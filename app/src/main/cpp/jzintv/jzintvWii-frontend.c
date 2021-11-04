#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/pad.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <dirent.h>
#include "sdl.h"
#include <ogc/usb.h>
#include "config.h"

#define FILES_PER_PAGE 20

static u32 *xfb;
static GXRModeObj *rmode;
int mainBioses=0;
int ecsBios=0;

extern int joyNunchukUsed[2];
extern int joyClassicUsed[2];
extern int joyGcUsed     [2];

void reset_print_cursor() {
   printf("\x1b[0;0H");
}

void print_disclaimer()
{
   reset_print_cursor();
   printf("\n                         Welcome to jzintvWii 1.0.0 !!");
   printf("\n\nThis is a Wii port of the terrific multi-platform jzintv intellivision emulator created by Joe Zbiciak.");
   printf("\n\nPlease visit:");
   printf("\n    http://spatula-city.org/~im14u2c/intv/");
   printf("\n\nPlease do NOT contact Joe Zbiciak for questions about jzintvWii, post them on");
   printf("\nthe wiibrew page instead:");
   printf("\n    http://wiibrew.org/wiki/jzintvWii");
   printf("\n\nDISCLAIMER: ");
   printf("\nUse of jzintvWii emulator with copyrighted ROMs which you don't own is");
   printf("\nILLEGAL. You will take full responsibilities if you choose to do so.");
   printf("\nUse of this software is entirely at your own risk. The author will not be held");
   printf("\nresponsible for any consequences of installing, using or removing this software,");
   printf("including but not limited to loss of data and earnings.");
   printf("\n\nPress A key to continue\n\n\n                            jenergy");

   while(1) {
        WPAD_ScanPads();
        PAD_ScanPads();
        u32 buttonsDown   = WPAD_ButtonsDown(WPAD_CHAN_0);
        u32 gcButtonsDown = PAD_ButtonsDown (PAD_CHAN0);

        if(
           ( buttonsDown & WPAD_BUTTON_A ) ||
           ( buttonsDown & WPAD_CLASSIC_BUTTON_A ) ||
           ( gcButtonsDown & PAD_BUTTON_A)
          )
        {
            break;
        }
   }
   VIDEO_ClearFrameBuffer (rmode, xfb, COLOR_BLACK);
}

void Initialise() {
    VIDEO_Init();
    WPAD_Init();
    PAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_ClearFrameBuffer (rmode, xfb, COLOR_BLACK);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}

int print_selection(char** files, int arrow_index, int file_index, int how_much_files)
{
   int i=0;
   int j=0;
   int items_showed=0;
   char* file_without_extension;

   reset_print_cursor();
   printf("\n                         Welcome to jzintvWii 1.0.0 !!");
   printf("\n\nUSAGE:                                                                       ");
   printf("\nPress UP or DOWN key on wii-mote to choose next/previous rom                   ");
   printf("\nPress LEFT or RIGHT key on wii-mote to jump forward/backward %d roms           ",FILES_PER_PAGE);
   printf("\nPress A key to launch rom                                                      ");
   printf("\nPress + to configure controllers                                               ");
   printf("\nPress HOME key to exit                                                         ");
   printf("\n                         Please choose the rom to launch:                      ");
   printf("\n                                                                               ");

   for (i=0; i<FILES_PER_PAGE; i++)
   {
      if (i == arrow_index) printf("\n-->");
      else printf("\n   ");

      if (file_index-arrow_index+i<how_much_files)
      {
        file_without_extension = strdup(files[file_index-arrow_index+i]);
        //file_without_extension[strlen(file_without_extension)-4]=0;
        printf("%d-%s",(file_index-arrow_index+i+1),file_without_extension);
        for (j=strlen(file_without_extension);j<72;j++)
        {
           printf(" ");
        }
        items_showed++;
      }
      else
      {
         for (j=0;j<72;j++)
         {
           printf(" ");
         }
      }
   }
   return items_showed;
}

void please_exit() {
  reset_print_cursor();
  printf("\n                         Welcome to jzintvWii 1.0.0 !!");
  printf("\n\n\n\nNo bioses (exec.bin AND grom.bin) or no .int .rom .bin .itv found in %s", DEFAULT_ROM_PATH);
  printf("\n\nPlease press HOME to exit");

  while(1) {
        WPAD_ScanPads();
        PAD_ScanPads();
        u32 buttonsDown   = WPAD_ButtonsDown(WPAD_CHAN_0);
        u32 gcButtonsDown = PAD_ButtonsDown (PAD_CHAN0);

        if(
           ( buttonsDown   & WPAD_BUTTON_HOME) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_HOME ) ||
           ( gcButtonsDown & PAD_BUTTON_START)
          )
        {
            reset_print_cursor();
            printf("\nExit...");
            write_config();
            exit(0);
        }
  }
}

void print_choose_controllers()
{
   reset_print_cursor();
   printf("\n                     Please choose controllers to use:");
   printf("\n\nPress button 1 to change first controller");
   printf("\nPress button 2 to change second controller");
   printf("\nPress - to return to previous menu");
   printf("\n\n\n           Controller 1                         Controller 2\n\n");
   if (joyNunchukUsed[0]==1) printf("       *");else  printf("        ");
   printf(               "Nunchuk/Wiimote                     ");
   if (joyNunchukUsed[1]==1)
   {
      printf("*");
   }
   else
   {
      printf(" ");
   }
   printf("Nunchuk/Wiimote\n       ");
   if (joyClassicUsed[0]==1)
   {
      printf("*");
   }
   else
   {
      printf(" ");
   }
   printf("Classic controller                  ");
   if (joyClassicUsed[1]==1)
   {
      printf("*");
   }
   else
   {
      printf(" ");
   }
   printf("Classic controller\n       ");

   if (joyGcUsed[0]==1)
   {
      printf("*");
   }
   else
   {
      printf(" ");
   }
   printf("Gamecube controller                 ");
   if (joyGcUsed[1]==1)
   {
      printf("*");
   }
   else
   {
      printf(" ");
   }
   printf("Gamecube controller\n       ");
}

void choose_controllers()
{
   int button_pressed=0;
   print_choose_controllers();
   while(1) {
      WPAD_ScanPads();

      u32 buttonsDown   = WPAD_ButtonsDown(WPAD_CHAN_0);

      if(
         ( buttonsDown & WPAD_BUTTON_1 )
        )
      {
          button_pressed=1;
          if (joyNunchukUsed[0]==1)
          {
            joyNunchukUsed[0]=0;
            joyClassicUsed[0]=1;
            joyGcUsed[0]=0;
          }
          else if (joyClassicUsed[0]==1)
          {
            joyNunchukUsed[0]=0;
            joyClassicUsed[0]=0;
            joyGcUsed[0]=1;
          }
          else if (joyGcUsed[0]==1)
          {
            joyNunchukUsed[0]=1;
            joyClassicUsed[0]=0;
            joyGcUsed[0]=0;
          }
      }

      if(
         ( buttonsDown & WPAD_BUTTON_2 )
        )
      {
          button_pressed=1;
          if (joyNunchukUsed[1]==1)
          {
            joyNunchukUsed[1]=0;
            joyClassicUsed[1]=1;
            joyGcUsed[1]=0;
          }
          else if (joyClassicUsed[1]==1)
          {
            joyNunchukUsed[1]=0;
            joyClassicUsed[1]=0;
            joyGcUsed[1]=1;
          }
          else if (joyGcUsed[1]==1)
          {
            joyNunchukUsed[1]=1;
            joyClassicUsed[1]=0;
            joyGcUsed[1]=0;
          }
      }

      if (( buttonsDown & WPAD_BUTTON_MINUS ) ||
         ( buttonsDown & WPAD_CLASSIC_BUTTON_MINUS ))
      break;

      if (button_pressed)
         print_choose_controllers();
   }
   VIDEO_ClearFrameBuffer (rmode, xfb, COLOR_BLACK);
}

void fixVideoMode()
{
    VIDEO_Configure(&TVNtsc480IntDf);
}


void write_config()
{
   FILE *fp;
   char s2[80];
   int i;
   char complete_file_name[80];

   sprintf(complete_file_name,"%s%s",DEFAULT_CFG_PATH,"/cfg.txt");

   if((fp=fopen(complete_file_name, "w")) != NULL) {
      for (i=0; i<2; i++)
      {
         sprintf(s2,"joy%d = ",i);
         fprintf(fp, s2);

         if (joyNunchukUsed[i] == 1) fprintf(fp,"Nunchuk");
         else if (joyClassicUsed[i] == 1) fprintf(fp,"Classic");
         else if (joyGcUsed[i] == 1) fprintf(fp,"Gamecube");

         fprintf(fp, "%s","\r\n");
      }
      fclose(fp);
   }
}

void mkdirCfg()
{
  mkdir(DEFAULT_CFG_PATH,777);
}

void read_config()
{
   FILE *fp;
   char s[80];
   char s1[80];
   char s2[80];
   int i;
   char complete_file_name[80];

   mkdirCfg();
   sprintf(complete_file_name,"%s%s",DEFAULT_CFG_PATH,"/cfg.txt");

   if((fp=fopen(complete_file_name, "r")) != NULL) {
      for (i=0; i<2; i++)
      {
         fscanf(fp, "%s", s);
         s[4]=' ';
         fscanf(fp, "%s", s+5);
         s[6]=' ';
         fscanf(fp, "%s", s+7);
         strcpy(s1,s);
         s1[7]=0;
         sprintf(s2,"joy%d = ",i);
         if (stricmp(s1,s2)) return;
         char* p = s+7;
         if (!stricmp(p, "Nunchuk"))
         {
            joyNunchukUsed[i] = 1;
            joyClassicUsed[i] = 0;
            joyGcUsed[i] = 0;
         }
         else if (!stricmp(p, "Classic"))
         {
            joyNunchukUsed[i] = 0;
            joyClassicUsed[i] = 1;
            joyGcUsed[i] = 0;
         }
         else if (!stricmp(p, "Gamecube"))
         {
            joyNunchukUsed[i] = 0;
            joyClassicUsed[i] = 0;
            joyGcUsed[i] = 1;
         }
         else return;
         fscanf(fp, "%c", &s[0]); // \r
         fscanf(fp, "%c", &s[0]); // \n
      }
      fclose(fp);
   }
}

void choose_rom(char** files, int how_much_files) {
    int button_pressed=0;
    int arrow_index=0;
    int file_index=0;
    char** argv;
    int item_showed = print_selection(files,arrow_index, file_index, how_much_files);

    while(1) {
        button_pressed=0;
        WPAD_ScanPads();
        PAD_ScanPads();

        u32 buttonsDown   = WPAD_ButtonsDown(WPAD_CHAN_0);
        u32 gcButtonsDown = PAD_ButtonsDown (PAD_CHAN0);

        if(
           ( buttonsDown   & WPAD_BUTTON_PLUS ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_PLUS )
          )
        {
            button_pressed=1;
            VIDEO_ClearFrameBuffer (rmode, xfb, COLOR_BLACK);
              choose_controllers();
        }

        if(
           ( buttonsDown   & WPAD_BUTTON_A ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_A )  ||
           ( gcButtonsDown & PAD_BUTTON_A )
          )
        {
            button_pressed=1;
            argv = malloc(10*sizeof(char*));
            int offset=0;
            argv[offset++]=strdup("jzintvWii!!");
            if (ecsBios!=1)
            {
               argv[offset++]=strdup("-s0");  //Disable Ecs
            }
            argv[offset++]=strdup(files[file_index]);

            reset_print_cursor();
            printf("Loading...");

            fixVideoMode();
            //Start emulation
            write_config();
            jzintv_entry_point(offset,argv);

            SDL_Quit();
            Initialise();
        }

        if(
           ( buttonsDown   & WPAD_BUTTON_DOWN ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_DOWN ) ||
           ( gcButtonsDown & PAD_BUTTON_DOWN)
          )
        {
           button_pressed=1;
           if (file_index<how_much_files-1)
           {
              file_index++;
               if(arrow_index<FILES_PER_PAGE-1)
              {
                 arrow_index++;
              }
           }
        }

        if(
           ( buttonsDown   & WPAD_BUTTON_UP ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_UP ) ||
           ( gcButtonsDown & PAD_BUTTON_UP)
          )
        {
            button_pressed=1;
            if(arrow_index>0)
            {
                arrow_index--;
            }
            if (file_index>0)
               file_index--;
        }

        if(
           ( buttonsDown   & WPAD_BUTTON_LEFT ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_LEFT ) ||
           ( gcButtonsDown & PAD_BUTTON_LEFT)
          )
        {
            button_pressed=1;

            file_index-=FILES_PER_PAGE;
            if (file_index<0)
               file_index=0;

            if (arrow_index > file_index)
               arrow_index=file_index;
        }

        if(
           ( buttonsDown   & WPAD_BUTTON_RIGHT ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_RIGHT ) ||
           ( gcButtonsDown & PAD_BUTTON_RIGHT)
          )
        {
            int fix_arrow_index_offset=0;
            button_pressed=1;
            file_index+=FILES_PER_PAGE;
            if (file_index>how_much_files-1)
            {
               fix_arrow_index_offset = file_index - how_much_files +1;
               file_index=how_much_files-1;
            }

            if (file_index==how_much_files-1)
            {
               if (how_much_files < FILES_PER_PAGE)
                  arrow_index=how_much_files-1;
               else
               {
                  arrow_index-=fix_arrow_index_offset;

                  //Already in the last page
                  if (arrow_index<0) arrow_index = item_showed-1;
               }
            }
        }

        if(
           ( buttonsDown   & WPAD_BUTTON_HOME ) ||
           ( buttonsDown   & WPAD_CLASSIC_BUTTON_HOME ) ||
           ( gcButtonsDown & PAD_BUTTON_START)
          )
        {
            reset_print_cursor();
            printf("\nExit...");
            write_config();
            exit(0);
        }
        if (button_pressed){
          item_showed = print_selection(files,arrow_index, file_index, how_much_files);
        }
    }
}



int main(int argc, char *argv[])
{
   if (!fatInitDefault()) {
      printf("fatInitDefault failure: terminating\n");
      exit(1);
   }
   Initialise();

   char** list_of_files;
   struct dirent * Dirent;
   DIR * dir;
   char* p;
   int offset=0;

   read_config();
   dir = opendir(DEFAULT_ROM_PATH);
   list_of_files = (char**)malloc(1000*sizeof(char*));
   while((Dirent = readdir(dir)) != NULL)
   {
      if (strlen(Dirent->d_name)<4) continue;
      p = Dirent->d_name+(strlen(Dirent->d_name)-4);
      if ((*p)!='.')continue;
      if ((stricmp(p,".bin"))&&
          (stricmp(p,".rom"))&&
           (stricmp(p,".int"))&&
          (stricmp(p,".itv"))) continue;
      if (!stricmp(Dirent->d_name,"exec.bin"))
      {
         mainBioses++;
         continue;
      }
      if (!stricmp(Dirent->d_name,"grom.bin"))
      {
         mainBioses++;
         continue;
      }
      if (!stricmp(Dirent->d_name,"ecs.bin"))
      {
         ecsBios++;
         continue;
      }
      list_of_files[offset++]=strdup(Dirent->d_name);
   }
   closedir(dir);
   print_disclaimer();

   if ((offset>0)&&(mainBioses>=2))
      choose_rom(list_of_files, offset);
    else
      please_exit();

   return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *rcsid = "$Id: rt-config.c,v 1.2 1997/04/12 12:45:31 david Exp $";

#define FALSE   0
#define TRUE    1

#include "atari.h"
#include "prompts.h"
#include "rt-config.h"

char atari_osa_filename[MAX_FILENAME_LEN];
char atari_osb_filename[MAX_FILENAME_LEN];
char atari_xlxe_filename[MAX_FILENAME_LEN];
char atari_basic_filename[MAX_FILENAME_LEN];
char atari_5200_filename[MAX_FILENAME_LEN];
char atari_disk_dir[MAX_FILENAME_LEN];
char atari_rom_dir[MAX_FILENAME_LEN];
char atari_h1_dir[MAX_FILENAME_LEN];
char atari_h2_dir[MAX_FILENAME_LEN];
char atari_h3_dir[MAX_FILENAME_LEN];
char atari_h4_dir[MAX_FILENAME_LEN];
char print_command[256];
int refresh_rate;
int default_system;
int default_tv_mode;
int hold_option;
int enable_c000_ram;
int enable_sio_patch;
int enable_xcolpf1;

static char *rtconfig_filename1 = "atari800.cfg";
static char *rtconfig_filename2 = "/etc/atari800.cfg";

int RtConfigLoad (char *rtconfig_filename)
{
  FILE *fp;
  int status = TRUE;

  /*
   * Set Configuration Parameters to sensible values
   * in case the configuration file is missing.
   */

  atari_osa_filename[0] = '\0';
  atari_osb_filename[0] = '\0';
  atari_xlxe_filename[0] = '\0';
  atari_basic_filename[0] = '\0';
  atari_5200_filename[0] = '\0';
  atari_disk_dir[0] = '\0';
  atari_rom_dir[0] = '\0';
  atari_h1_dir[0] = '\0';
  atari_h2_dir[0] = '\0';
  atari_h3_dir[0] = '\0';
  atari_h4_dir[0] = '\0';
  strcpy (print_command, "lpr %s");
  refresh_rate = 1;
  default_system = 3;
  default_tv_mode = 1;
  hold_option = 0;
  enable_c000_ram = 0;
  enable_sio_patch = 1;
  enable_xcolpf1 = 0;

  if (rtconfig_filename)
    {
      fp = fopen (rtconfig_filename, "r");
      if (!fp)
        {
          perror (rtconfig_filename);
          exit (1);
        }
    }
  else
    {
      fp = fopen (rtconfig_filename1, "r");
      if (!fp)
        fp = fopen (rtconfig_filename2, "r");
    }

  if (fp)
    {
      char string[256];
      char *ptr;

      fgets (string, 256, fp);

      printf ("Configuration Created by %s", string);

      while (fgets (string, 256, fp))
        {
          RemoveLF (string);
          ptr = strchr(string,'=');
          if (ptr)
            {
              *ptr++ = '\0';

              if (strcmp(string,"OS/A_ROM") == 0)
                strcpy (atari_osa_filename, ptr);
              else if (strcmp(string,"OS/B_ROM") == 0)
                strcpy (atari_osb_filename, ptr);
              else if (strcmp(string,"XL/XE_ROM") == 0)
                strcpy (atari_xlxe_filename, ptr);
              else if (strcmp(string,"BASIC_ROM") == 0)
                strcpy (atari_basic_filename, ptr);
              else if (strcmp(string,"5200_ROM") == 0)
                strcpy (atari_5200_filename, ptr);
              else if (strcmp(string,"DISK_DIR") == 0)
                strcpy (atari_disk_dir, ptr);
              else if (strcmp(string,"ROM_DIR") == 0)
                strcpy (atari_rom_dir, ptr);
              else if (strcmp(string,"H1_DIR") == 0)
                strcpy (atari_h1_dir, ptr);
              else if (strcmp(string,"H2_DIR") == 0)
                strcpy (atari_h2_dir, ptr);
              else if (strcmp(string,"H3_DIR") == 0)
                strcpy (atari_h3_dir, ptr);
              else if (strcmp(string,"H4_DIR") == 0)
                strcpy (atari_h4_dir, ptr);
              else if (strcmp(string,"PRINT_COMMAND") == 0)
                strcpy (print_command, ptr);
              else if (strcmp(string,"SCREEN_REFRESH_RATIO") == 0)
                sscanf (ptr,"%d", &refresh_rate);
              else if (strcmp(string,"HOLD_OPTION") == 0)
                sscanf (ptr,"%d", &hold_option);
              else if (strcmp(string,"ENABLE_C000_RAM") == 0)
                sscanf (ptr,"%d", &enable_c000_ram);
              else if (strcmp(string,"ENABLE_SIO_PATCH") == 0)
                sscanf (ptr,"%d", &enable_sio_patch);
              else if (strcmp(string,"ENABLE_XCOLPF1") == 0)
                sscanf (ptr,"%d", &enable_xcolpf1);
              else if (strcmp(string,"DEFAULT_SYSTEM") == 0)
                {
                  if (strcmp(ptr,"Atari OS/A") == 0)
                    default_system = 1;
                  else if (strcmp(ptr,"Atari OS/B") == 0)
                    default_system = 2;
                  else if (strcmp(ptr,"Atari XL") == 0)
                    default_system = 3;
                  else if (strcmp(ptr,"Atari XE") == 0)
                    default_system = 4;
                  else if (strcmp(ptr,"Atari 5200") == 0)
                    default_system = 5;
                  else
                    printf ("Invalid System: %s\n", ptr);
                }
              else if (strcmp(string,"DEFAULT_TV_MODE") == 0)
                {
                  if (strcmp(ptr,"PAL") == 0)
                    default_tv_mode = 1;
                  else if (strcmp(ptr,"NTSC") == 0)
                    default_tv_mode = 2;
                  else
                    printf ("Invalid TV Mode: %s\n", ptr);
                }
              else
                printf ("Unrecognized Variable: %s\n", string);
            }
          else
            {
              printf ("Ignored Config Line: %s\n", string);
            }
        }


      fclose (fp);
    }
  else
    {
      status = FALSE;
    }

  return status;
}

void RtConfigSave (void)
{
  FILE *fp;

  fp = fopen (rtconfig_filename1, "w");
  if (!fp)
    {
      perror (rtconfig_filename1);
      exit (1);
    }

  printf ("\nWriting: %s\n\n", rtconfig_filename1);

  fprintf (fp, "%s\n", ATARI_TITLE);
  fprintf (fp, "OS/A_ROM=%s\n", atari_osa_filename);
  fprintf (fp, "OS/B_ROM=%s\n", atari_osb_filename);
  fprintf (fp, "XL/XE_ROM=%s\n", atari_xlxe_filename);
  fprintf (fp, "BASIC_ROM=%s\n", atari_basic_filename);
  fprintf (fp, "5200_ROM=%s\n", atari_5200_filename);
  fprintf (fp, "DISK_DIR=%s\n", atari_disk_dir);
  fprintf (fp, "ROM_DIR=%s\n", atari_rom_dir);
  fprintf (fp, "H1_DIR=%s\n", atari_h1_dir);
  fprintf (fp, "H2_DIR=%s\n", atari_h2_dir);
  fprintf (fp, "H3_DIR=%s\n", atari_h3_dir);
  fprintf (fp, "H4_DIR=%s\n", atari_h4_dir);
  fprintf (fp, "PRINT_COMMAND=%s\n", print_command);
  fprintf (fp, "SCREEN_REFRESH_RATIO=%d\n", refresh_rate);

  fprintf (fp, "DEFAULT_SYSTEM=Atari ");
  switch (default_system)
    {
      case 1 :
        fprintf (fp, "OS/A\n");
        break;
      case 2 :
        fprintf (fp, "OS/B\n");
        break;
      case 3 :
        fprintf (fp, "XL\n");
        break;
      case 4 :
        fprintf (fp, "XE\n");
        break;
      case 5 :
        fprintf (fp, "5200\n");
        break;
    } 

  if (default_tv_mode == 1)
    fprintf (fp,"DEFAULT_TV_MODE=PAL\n");
  else
    fprintf (fp,"DEFAULT_TV_MODE=NTSC\n");

  fprintf (fp,"HOLD_OPTION=%d\n", hold_option);
  fprintf (fp,"ENABLE_C000_RAM=%d\n", enable_c000_ram);
  fprintf (fp,"ENABLE_SIO_PATCH=%d\n", enable_sio_patch);
  fprintf (fp,"ENABLE_XCOLPF1=%d\n", enable_xcolpf1);

  fclose (fp);
}

void RtConfigUpdate (void)
{
  GetString ("Enter path to Atari OS/A ROM [%s] ", atari_osa_filename);
  GetString ("Enter path to Atari OS/B ROM [%s] ", atari_osb_filename);
  GetString ("Enter path to Atari XL/XE ROM [%s] ", atari_xlxe_filename);
  GetString ("Enter path to Atari BASIC ROM [%s] ", atari_basic_filename);
  GetString ("Enter path to Atari 5200 ROM [%s] ", atari_5200_filename);
  GetString ("Enter path for disk images [%s] ", atari_disk_dir);
  GetString ("Enter path for ROM images [%s] ", atari_rom_dir);
  GetString ("Enter path for H1: device [%s] ", atari_h1_dir);
  GetString ("Enter path for H2: device [%s] ", atari_h2_dir);
  GetString ("Enter path for H3: device [%s] ", atari_h3_dir);
  GetString ("Enter path for H4: device [%s] ", atari_h4_dir);
  GetString ("Enter print command [%s] ", print_command);
  GetNumber ("Enter default screen refresh ratio 1:[%d] ", &refresh_rate);
  if (refresh_rate < 1)
    refresh_rate = 1;
  else if (refresh_rate > 60)
    refresh_rate = 60;

  do
    {
      GetNumber ("Default System 1) OS/A, 2) OS/B, 3) XL, 4) XE, 5) 5200 [%d] ",
                 &default_system);
    } while ((default_system < 1) || (default_system > 5));

  do
    {
      GetNumber ("Default TV mode 1) PAL, 2) NTSC [%d] ",
                 &default_tv_mode);
    } while ((default_tv_mode < 1) || (default_tv_mode > 2));

  do
    {
      GetNumber ("Hold OPTION during Coldstart [%d] ",
                 &hold_option);
    } while ((hold_option < 0) || (hold_option > 1));

  do
    {
      GetNumber ("Enable C000-CFFF RAM in Atari800 mode [%d] ",
                 &enable_c000_ram);
    } while ((enable_c000_ram < 0) || (enable_c000_ram > 1));

  do
    {
      GetNumber ("Enable SIO PATCH (Recommended) [%d] ",
                 &enable_sio_patch);
    } while ((enable_sio_patch < 0) || (enable_sio_patch > 1));

  do
    {
      GetNumber ("Enable Extended COLPF1 [%d] ",
                 &enable_xcolpf1);
    } while ((enable_xcolpf1 < 0) || (enable_xcolpf1 > 1));
}
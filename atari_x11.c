#include <stdio.h>
#include <stdlib.h>
#ifdef VMS
#include <stat.h>
#else
#include <sys/stat.h>
#endif

#include <sys/time.h>

/*
 * Note: For SHM version check if image_data or the pixmap is needed
 *       Check if rect, nrects, points and npoints are needed.
 *       scanline_ptr.
 */

static char *rcsid = "$Id: atari_x11.c,v 1.42 1997/06/22 17:52:35 david Exp $";

#ifdef XVIEW
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/notice.h>
#include <xview/file_chsr.h>
#endif

#ifdef MOTIF
#include <Xm/MainW.h>
#include <Xm/DrawingA.h>
#include <Xm/MessageB.h>
#include <Xm/FileSB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>

static XtAppContext app;
static Widget toplevel;
static Widget main_w;
static Widget drawing_area;
static Widget fsel_b;
static Widget fsel_d;
static Widget fsel_r;
static Widget rbox_d;
static Widget rbox_r;
static Widget togg_d1, togg_d2, togg_d3, togg_d4;
static Widget togg_d5, togg_d6, togg_d7, togg_d8;
static Widget togg_8k, togg_16k, togg_oss, togg_32k, togg_5200;
static Widget eject_menu;
static Widget disable_menu;
static Widget system_menu;
static int motif_disk_sel = 1;
static int motif_rom_sel = 1;
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifndef VMS
#include "config.h"
#endif
#include "atari.h"
#include "colours.h"
#include "monitor.h"
#include "sio.h"
#include "nas.h"
#include "platform.h"
#include "rt-config.h"

static struct timeval tp;
static struct timezone tzp;

static double basetime;
static int nframes = 0;

#ifdef SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

static XShmSegmentInfo shminfo;
static XImage *image;
extern char *atari_screen;
extern int colour_translation_table[256];
#endif

#ifdef LINUX_JOYSTICK
#include <linux/joystick.h>
#include <fcntl.h>

static int js0;
static int js1;

static int js0_centre_x;
static int js0_centre_y;
static int js1_centre_x;
static int js1_centre_y;

static struct JS_DATA_TYPE js_data;
#endif

#define	FALSE	0
#define	TRUE	1

typedef enum
{
  Small,
  Large,
  Huge
} WindowSize;

static WindowSize windowsize = Large;

enum
{
  MONITOR_NOTHING,
  MONITOR_FPS,
  MONITOR_SIO
} x11_monitor = MONITOR_NOTHING;

static int x11bug = FALSE;
static int private_cmap = FALSE;

static int window_width;
static int window_height;

static Display	*display;
static Screen	*screen;
static Window	window;
#ifndef SHM
static Pixmap	pixmap;
#endif
static Visual	*visual;
static Colormap cmap;

static GC gc;
static GC gc_colour[256];
static int colours[256];

static XComposeStatus	keyboard_status;

#ifdef XVIEW
static Frame frame;
static Panel panel;
static Canvas canvas;
static Menu system_menu;
static Menu coldstart_menu;
static Menu consol_menu;
static Menu options_menu;
static Frame chooser;

static Frame controllers_frame;
static Panel controllers_panel;
static Panel_item keypad_item;
static Panel_item mouse_item;

#ifdef LINUX_JOYSTICK
static Panel_item js0_item;
static Panel_item js1_item;
#endif

static Frame performance_frame;
static Panel performance_panel;
static Panel_item refresh_slider;
#endif

static int	SHIFT = 0x00;
static int	CONTROL = 0x00;
static UBYTE	*image_data;
static int	modified;

static int keypad_mode = -1; /* Joystick */
static int keypad_trig = 1; /* Keypad Trigger Position */
static int keypad_stick = 0x0f; /* Keypad Joystick Position */

static int mouse_mode = -1; /* Joystick, Paddle and Light Pen */
static int mouse_stick; /* Mouse Joystick Position */

static int js0_mode = -1;
static int js1_mode = -1;

static int	last_colour = -1;

#define	NPOINTS	(4096/4)
#define	NRECTS	(4096/4)

static int	nrects = 0;
static int	npoints = 0;

static XPoint		points[NPOINTS];
static XRectangle	rectangles[NRECTS];

static int keyboard_consol;
static int menu_consol;
static int screen_dump = 0;

/*
   ==========================================
   Import a few variables from atari_custom.c
   ==========================================
*/

extern int refresh_rate;
extern double deltatime;

int GetKeyCode (XEvent *event)
{
  KeySym keysym;
  char buffer[128];
  int keycode = AKEY_NONE;

  XLookupString ((XKeyEvent*)event, buffer, 128,
		 &keysym, &keyboard_status);

  switch (event->type)
    {
    case Expose :
#ifndef SHM
      XCopyArea (display, pixmap, window, gc,
		 0, 0,
		 window_width, window_height,
		 0, 0);
#endif
      break;
    case KeyPress :
      switch (keysym)
	{
	case XK_Shift_L :
	case XK_Shift_R :
	  SHIFT = AKEY_SHFT;
	  break;
	case XK_Control_L :
	case XK_Control_R :
	  CONTROL = AKEY_CTRL;
	  break;
	case XK_Caps_Lock :
	  if (SHIFT)
	    keycode = AKEY_CAPSLOCK;
	  else
	    keycode = AKEY_CAPSTOGGLE;
	  break;
	case XK_Shift_Lock :
	  if (x11bug)
	    printf ("XK_Shift_Lock\n");
	  break;
	case XK_Alt_L :
	case XK_Alt_R :
	  keycode = AKEY_ATARI;
	  break;
	case XK_F1 :
          keycode = AKEY_UI;
	  break;
	case XK_F2 :
	  keyboard_consol &= 0x03;
	  keycode = AKEY_NONE;
	  break;
	case XK_F3 :
	  keyboard_consol &= 0x05;
	  keycode = AKEY_NONE;
	  break;
	case XK_F4 :
	  keyboard_consol &= 0x6;
	  keycode = AKEY_NONE;
	  break;
	case XK_F5 :
	  keycode = AKEY_WARMSTART;
	  break;
	case XK_L5 :
	  keycode = AKEY_COLDSTART;
	  break;
	case XK_F6 :
	  keycode = AKEY_PIL;
	  break;
	case XK_F7 :
	  keycode = AKEY_BREAK;
	  break;
	case XK_L8 :
          screen_dump = (1 - screen_dump);
          keycode = AKEY_NONE;
	  break;
	case XK_F8 :
          screen_dump = 2;
          keycode = AKEY_NONE;
	  break;
	case XK_F9 :
	  keycode = AKEY_EXIT;
	  break;
	case XK_F10 :
	  keycode = AKEY_NONE;
	  if (deltatime == 0.0)
	    deltatime = (1.0 / 50.0);
	  else
	    deltatime = 0.0;
	  break;
	case XK_Home :
	  keycode = 0x76;
	  break;
	case XK_Insert :
	  if (SHIFT)
	    keycode = AKEY_INSERT_LINE;
	  else
	    keycode = AKEY_INSERT_CHAR;
	  break;
	case XK_BackSpace :
	  if (CONTROL)
	    keycode = AKEY_DELETE_CHAR;
	  else if (SHIFT)
	    keycode = AKEY_DELETE_LINE;
	  else
	    keycode = AKEY_BACKSPACE;
	  break;
	case XK_Delete :
	  if (CONTROL)
	    keycode = AKEY_DELETE_CHAR;
	  else if (SHIFT)
	    keycode = AKEY_DELETE_LINE;
	  else
	    keycode = AKEY_BACKSPACE;
	  break;
	case XK_Left :
	  keycode = AKEY_LEFT;
	  keypad_stick = STICK_LEFT;
	  break;
	case XK_Up :
	  keycode = AKEY_UP;
	  keypad_stick = STICK_FORWARD;
	  break;
	case XK_Right :
	  keycode = AKEY_RIGHT;
	  keypad_stick = STICK_RIGHT;
	  break;
	case XK_Down :
	  keycode = AKEY_DOWN;
	  keypad_stick = STICK_BACK;
	  break;
	case XK_Escape :
	  keycode = AKEY_ESCAPE;
	  break;
	case XK_Tab :
	  if (CONTROL)
	    keycode = AKEY_CLRTAB;
	  else if (SHIFT)
	    keycode = AKEY_SETTAB;
	  else
	    keycode = AKEY_TAB;
	  break;
	case XK_exclam :
	  keycode = AKEY_EXCLAMATION;
	  break;
	case XK_quotedbl :
	  keycode = AKEY_DBLQUOTE;
	  break;
	case XK_numbersign :
	  keycode = AKEY_HASH;
	  break;
	case XK_dollar :
	  keycode = AKEY_DOLLAR;
	  break;
	case XK_percent :
	  keycode = AKEY_PERCENT;
	  break;
	case XK_ampersand :
	  keycode = AKEY_AMPERSAND;
	  break;
	case XK_quoteright :
	  keycode = AKEY_QUOTE;
	  break;
	case XK_at :
	  keycode = AKEY_AT;
	  break;
	case XK_parenleft :
	  keycode = AKEY_PARENLEFT;
	  break;
	case XK_parenright :
	  keycode = AKEY_PARENRIGHT;
	  break;
	case XK_less :
	  keycode = AKEY_LESS;
	  break;
	case XK_greater :
	  keycode = AKEY_GREATER;
	  break;
	case XK_equal :
	  keycode = AKEY_EQUAL;
	  break;
	case XK_question :
	  keycode = AKEY_QUESTION;
	  break;
	case XK_minus :
	  keycode = AKEY_MINUS;
	  break;
	case XK_plus :
	  keycode = AKEY_PLUS;
	  break;
	case XK_asterisk :
	  keycode = AKEY_ASTERISK;
	  break;
	case XK_slash :
	  keycode = AKEY_SLASH;
	  break;
	case XK_colon :
	  keycode = AKEY_COLON;
	  break;
	case XK_semicolon :
	  keycode = AKEY_SEMICOLON;
	  break;
	case XK_comma :
	  keycode = AKEY_COMMA;
	  break;
	case XK_period :
	  keycode = AKEY_FULLSTOP;
	  break;
	case XK_underscore :
	  keycode = AKEY_UNDERSCORE;
	  break;
	case XK_bracketleft :
	  keycode = AKEY_BRACKETLEFT;
	  break;
	case XK_bracketright :
	  keycode = AKEY_BRACKETRIGHT;
	  break;
	case XK_asciicircum :
	  keycode = AKEY_CIRCUMFLEX;
	  break;
	case XK_backslash :
	  keycode = AKEY_BACKSLASH;
	  break;
	case XK_bar :
	  keycode = AKEY_BAR;
	  break;
	case XK_space :
	  keycode = AKEY_SPACE;
	  keypad_trig = 0;
	  break;
	case XK_Return :
	  keycode = AKEY_RETURN;
	  keypad_stick = STICK_CENTRE;
	  break;
	case XK_0 :
	  keycode = CONTROL | AKEY_0;
	  break;
	case XK_1 :
	  keycode = CONTROL | AKEY_1;
	  break;
	case XK_2 :
	  keycode = CONTROL | AKEY_2;
	  break;
	case XK_3 :
	  keycode = CONTROL | AKEY_3;
	  break;
	case XK_4 :
	  keycode = CONTROL | AKEY_4;
	  break;
	case XK_5 :
	  keycode = CONTROL | AKEY_5;
	  break;
	case XK_6 :
	  keycode = CONTROL | AKEY_6;
	  break;
	case XK_7 :
	  keycode = CONTROL | AKEY_7;
	  break;
	case XK_8 :
	  keycode = CONTROL | AKEY_8;
	  break;
	case XK_9 :
	  keycode = CONTROL | AKEY_9;
	  break;
	case XK_A : case XK_a :
	  keycode = SHIFT | CONTROL | AKEY_a;
	  break;
	case XK_B : case XK_b :
	  keycode = SHIFT | CONTROL | AKEY_b;
	  break;
	case XK_C : case XK_c :
	  keycode = SHIFT | CONTROL | AKEY_c;
	  break;
	case XK_D : case XK_d :
	  keycode = SHIFT | CONTROL | AKEY_d;
	  break;
	case XK_E : case XK_e :
	  keycode = SHIFT | CONTROL | AKEY_e;
	  break;
	case XK_F : case XK_f :
	  keycode = SHIFT | CONTROL | AKEY_f;
	  break;
	case XK_G : case XK_g :
	  keycode = SHIFT | CONTROL | AKEY_g;
	  break;
	case XK_H : case XK_h :
	  keycode = SHIFT | CONTROL | AKEY_h;
	  break;
	case XK_I : case XK_i :
	  keycode = SHIFT | CONTROL | AKEY_i;
	  break;
	case XK_J : case XK_j :
	  keycode = SHIFT | CONTROL | AKEY_j;
	  break;
	case XK_K : case XK_k :
	  keycode = SHIFT | CONTROL | AKEY_k;
	  break;
	case XK_L : case XK_l :
	  keycode = SHIFT | CONTROL | AKEY_l;
	  break;
	case XK_M : case XK_m :
	  keycode = SHIFT | CONTROL | AKEY_m;
	  break;
	case XK_N : case XK_n :
	  keycode = SHIFT | CONTROL | AKEY_n;
	  break;
	case XK_O : case XK_o :
	  keycode = SHIFT | CONTROL | AKEY_o;
	  break;
	case XK_P : case XK_p :
	  keycode = SHIFT | CONTROL | AKEY_p;
	  break;
	case XK_Q : case XK_q :
	  keycode = SHIFT | CONTROL | AKEY_q;
	  break;
	case XK_R : case XK_r :
	  keycode = SHIFT | CONTROL | AKEY_r;
	  break;
	case XK_S : case XK_s :
	  keycode = SHIFT | CONTROL | AKEY_s;
	  break;
	case XK_T : case XK_t :
	  keycode = SHIFT | CONTROL | AKEY_t;
	  break;
	case XK_U : case XK_u :
	  keycode = SHIFT | CONTROL | AKEY_u;
	  break;
	case XK_V : case XK_v :
	  keycode = SHIFT | CONTROL | AKEY_v;
	  break;
	case XK_W : case XK_w :
	  keycode = SHIFT | CONTROL | AKEY_w;
	  break;
	case XK_X : case XK_x :
	  keycode = SHIFT | CONTROL | AKEY_x;
	  break;
	case XK_Y : case XK_y :
	  keycode = SHIFT | CONTROL | AKEY_y;
	  break;
	case XK_Z : case XK_z :
	  keycode = SHIFT | CONTROL | AKEY_z;
	  break;
	case XK_KP_0 :
	  keypad_trig = 0;
	  break;
	case XK_KP_1 :
	  keypad_stick = STICK_LL;
	  break;
	case XK_KP_2 :
	  keypad_stick = STICK_BACK;
	  break;
	case XK_KP_3 :
	  keypad_stick = STICK_LR;
	  break;
	case XK_KP_4 :
	  keypad_stick = STICK_LEFT;
	  break;
	case XK_KP_5 :
	  keypad_stick = STICK_CENTRE;
	  break;
	case XK_KP_6 :
	  keypad_stick = STICK_RIGHT;
	  break;
	case XK_KP_7 :
	  keypad_stick = STICK_UL;
	  break;
	case XK_KP_8 :
	  keypad_stick = STICK_FORWARD;
	  break;
	case XK_KP_9 :
	  keypad_stick = STICK_UR;
	  break;
	default :
	  if (x11bug)
	    printf ("Pressed Keysym = %x\n", (int)keysym);
	  break;
	}
      break;
    case KeyRelease :
      switch (keysym)
	{
	case XK_Shift_L :
	case XK_Shift_R :
	  SHIFT = 0x00;
	  break;
	case XK_Control_L :
	case XK_Control_R :
	  CONTROL = 0x00;
	  break;
	case XK_Caps_Lock :
	  if (SHIFT)
	    keycode = AKEY_CAPSLOCK;
	  else
	    keycode = AKEY_CAPSTOGGLE;
	  break;
	case XK_Shift_Lock :
	  if (x11bug)
	    printf ("XK_Shift_Lock\n");
	  break;
	case XK_F2 :
	case XK_F3 :
	case XK_F4 :
	  keyboard_consol = 0x07;
	  keycode = AKEY_NONE;
          break;
	case XK_KP_0 :
	  keypad_trig = 1;
	  break;
	case XK_KP_1 :
	case XK_KP_2 :
	case XK_KP_3 :
	case XK_KP_4 :
	case XK_KP_5 :
	case XK_KP_6 :
	case XK_KP_7 :
	case XK_KP_8 :
	case XK_KP_9 :
	  keypad_stick = STICK_CENTRE;
	  break;
	default :
	  break;
	}
      break;
    }

  return keycode;
}

static int xview_keycode = AKEY_NONE;

#ifdef XVIEW

void event_proc (Xv_Window window, Event *event, Notify_arg arg)
{
  int keycode;

  keycode = GetKeyCode (event->ie_xevent);
  if (keycode != AKEY_NONE)
    xview_keycode = keycode;
}

static int auto_reboot;

int disk_change (char *a, char *full_filename, char *filename)
{
  int diskno;
  int status;

  diskno = 1;

  if (!auto_reboot)
    diskno = notice_prompt (panel, NULL,
			    NOTICE_MESSAGE_STRINGS,
			      "Insert Disk into which drive?",
			      NULL,
			    NOTICE_BUTTON, "1", 1,
			    NOTICE_BUTTON, "2", 2,
			    NOTICE_BUTTON, "3", 3,
			    NOTICE_BUTTON, "4", 4,
			    NOTICE_BUTTON, "5", 5,
			    NOTICE_BUTTON, "6", 6,
			    NOTICE_BUTTON, "7", 7,
			    NOTICE_BUTTON, "8", 8,
			    NULL);

  if ((diskno < 1) || (diskno > 8))
    {
      printf ("Invalid diskno: %d\n", diskno);
      exit (1);
    }

  SIO_Dismount(diskno);
  if (!SIO_Mount (diskno,full_filename))
    status = XV_ERROR;
  else
    {
      if (auto_reboot)
	Coldstart ();
      status = XV_OK;
    }

  return status;
}

boot_callback ()
{
  auto_reboot = TRUE;

  xv_set (chooser,
	  FRAME_LABEL, "Disk Selector",
	  FILE_CHOOSER_DIRECTORY, atari_disk_dir,
	  FILE_CHOOSER_NOTIFY_FUNC, disk_change,
	  XV_SHOW, TRUE,
	  NULL);
}

insert_callback ()
{
  auto_reboot = FALSE;

  xv_set (chooser,
	  FRAME_LABEL, "Disk Selector",
	  FILE_CHOOSER_DIRECTORY, atari_disk_dir,
	  FILE_CHOOSER_NOTIFY_FUNC, disk_change,
	  XV_SHOW, TRUE,
	  NULL);
}

eject_callback ()
{
  int diskno;

  diskno = notice_prompt (panel, NULL,
			  NOTICE_MESSAGE_STRINGS,
			    "Eject Disk from drive?",
			    NULL,
			  NOTICE_BUTTON, "1", 1,
			  NOTICE_BUTTON, "2", 2,
			  NOTICE_BUTTON, "3", 3,
			  NOTICE_BUTTON, "4", 4,
			  NOTICE_BUTTON, "5", 5,
			  NOTICE_BUTTON, "6", 6,
			  NOTICE_BUTTON, "7", 7,
			  NOTICE_BUTTON, "8", 8,
			  NULL);

  if ((diskno < 1) || (diskno > 8))
    {
      printf ("Invalid diskno: %d\n", diskno);
      exit (1);
    }

  SIO_Dismount(diskno);
}

disable_callback ()
{
  int diskno;

  diskno = notice_prompt (panel, NULL,
			  NOTICE_MESSAGE_STRINGS,
			    "Drive to Disable?",
			    NULL,
			  NOTICE_BUTTON, "1", 1,
			  NOTICE_BUTTON, "2", 2,
			  NOTICE_BUTTON, "3", 3,
			  NOTICE_BUTTON, "4", 4,
			  NOTICE_BUTTON, "5", 5,
			  NOTICE_BUTTON, "6", 6,
			  NOTICE_BUTTON, "7", 7,
			  NOTICE_BUTTON, "8", 8,
			  NULL);

  if ((diskno < 1) || (diskno > 8))
    {
      printf ("Invalid driveno: %d\n", diskno);
      exit (1);
    }

  SIO_DisableDrive(diskno);
}

int rom_change (char *a, char *full_filename, char *filename)
{
  struct stat buf;
  int status = XV_ERROR;
  int yesno;

  stat (full_filename, &buf);

  switch (buf.st_size)
    {
    case 0x2000 :
      Remove_ROM ();
      if (Insert_8K_ROM(full_filename))
	{
	  Coldstart ();
	  status = XV_OK;
	}
      break;
    case 0x4000 :
      yesno = notice_prompt (panel, NULL,
			     NOTICE_MESSAGE_STRINGS,
			       filename,
			       "Is this an OSS Supercartridge?",
			       NULL,
			     NOTICE_BUTTON_YES, "No",
			     NOTICE_BUTTON_NO, "Yes",
			     NULL);
      if (yesno == NOTICE_YES)
	{
	  Remove_ROM ();
	  if (Insert_16K_ROM(full_filename))
	    {
	      Coldstart ();
	      status = XV_OK;
	    }
	}
      else
	{
	  Remove_ROM ();
	  if (Insert_OSS_ROM(full_filename))
	    {
	      Coldstart ();
	      status = XV_OK;
	    }
	}
      break;
    case 0x8000 :
      Remove_ROM ();
      if (machine == Atari5200)
	{
	  if (Insert_32K_5200ROM(full_filename))
	    {
	      Coldstart ();
	      status = XV_OK;
	    }
	}
      else
	{
	  if (Insert_DB_ROM(full_filename))
	    {
	      Coldstart ();
	      status = XV_OK;
	    }
	}
      break;
    default :
      break;
    }

  return status;
}

insert_rom_callback ()
{
  xv_set (chooser,
	  FRAME_LABEL, "ROM Selector",
	  FILE_CHOOSER_DIRECTORY, atari_rom_dir,
	  FILE_CHOOSER_NOTIFY_FUNC, rom_change,
	  XV_SHOW, TRUE,
	  NULL);
}

remove_rom_callback ()
{
  Remove_ROM ();
  Coldstart ();
}

enable_pill_callback ()
{
  EnablePILL ();
  Coldstart ();
}

exit_callback ()
{
  exit (1);
}

option_callback ()
{
  menu_consol &= 0x03;
}

select_callback ()
{
  menu_consol &= 0x05;
}

start_callback ()
{
  menu_consol &= 0x6;
}

help_callback ()
{
  xview_keycode = AKEY_HELP;
}

break_callback ()
{
  xview_keycode = AKEY_BREAK;
}

reset_callback ()
{
  Warmstart ();
}

coldstart_callback ()
{
  Coldstart ();
}

coldstart_osa_callback ()
{
  int status;

  status = Initialise_AtariOSA ();
  if (status)
    {
      Menu_item menuitem;

      menuitem = xv_get (consol_menu,
			 MENU_NTH_ITEM, 4);

      xv_set (menuitem,
	      MENU_INACTIVE, TRUE,
	      NULL);
    }
  else
    {
      notice_prompt (panel, NULL,
		     NOTICE_MESSAGE_STRINGS,
		       "Sorry, OS/A ROM Unavailable",
		       NULL,
		     NOTICE_BUTTON, "Cancel", 1,
		     NULL);
    }
}

coldstart_osb_callback ()
{
  int status;

  status = Initialise_AtariOSB ();
  if (status)
    {
      Menu_item menuitem;
      
      menuitem = xv_get (consol_menu,
			 MENU_NTH_ITEM, 4);

      xv_set (menuitem,
	      MENU_INACTIVE, TRUE,
	      NULL);
    }
  else
    {
      notice_prompt (panel, NULL,
		     NOTICE_MESSAGE_STRINGS,
		       "Sorry, OS/B ROM Unavailable",
		       NULL,
		     NOTICE_BUTTON, "Cancel", 1,
		     NULL);
    }
}

coldstart_xl_callback ()
{
  int status;

  status = Initialise_AtariXL ();
  if (status)
    {
      Menu_item menuitem;

      menuitem = xv_get (consol_menu,
			 MENU_NTH_ITEM, 4);
      
      xv_set (menuitem,
	      MENU_INACTIVE, FALSE,
	      NULL);
    }
  else
    {
      notice_prompt (panel, NULL,
		     NOTICE_MESSAGE_STRINGS,
		       "Sorry, XL/XE ROM Unavailable",
		       NULL,
		     NOTICE_BUTTON, "Cancel", 1,
		     NULL);
    }
}

coldstart_xe_callback ()
{
  int status;

  status = Initialise_AtariXE ();
  if (status)
    {
      Menu_item menuitem;

      menuitem = xv_get (consol_menu,
			 MENU_NTH_ITEM, 4);

      xv_set (menuitem,
	      MENU_INACTIVE, FALSE,
	      NULL);
    }
  else
    {
      notice_prompt (panel, NULL,
		     NOTICE_MESSAGE_STRINGS,
		       "Sorry, XL/XE ROM Unavailable",
		       NULL,
		     NOTICE_BUTTON, "Cancel", 1,
		     NULL);
    }
}

coldstart_5200_callback ()
{
  int status;

  status = Initialise_Atari5200 ();
  if (status)
    {
      Menu_item menuitem;

      menuitem = xv_get (consol_menu,
			 MENU_NTH_ITEM, 4);

      xv_set (menuitem,
	      MENU_INACTIVE, FALSE,
	      NULL);
    }
  else
    {
      notice_prompt (panel, NULL,
		     NOTICE_MESSAGE_STRINGS,
		       "Sorry, 5200 ROM Unavailable",
		       NULL,
		     NOTICE_BUTTON, "Cancel", 1,
		     NULL);
    }
}

controllers_ok_callback ()
{
  xv_set (controllers_frame,
	  XV_SHOW, FALSE,
	  NULL);
}

controllers_callback ()
{
  xv_set (controllers_frame,
	  XV_SHOW, TRUE,
	  NULL);
}

void sorry_message ()
{
  notice_prompt (panel, NULL,
		 NOTICE_MESSAGE_STRINGS,
		   "Sorry, controller already assign",
		   "to another device",
		   NULL,
		 NOTICE_BUTTON, "Cancel", 1,
		 NULL);
}

keypad_callback ()
{
  int new_mode;

  new_mode = xv_get (keypad_item, PANEL_VALUE);

  if ((new_mode != mouse_mode) &&
      (new_mode != js0_mode) &&
      (new_mode != js1_mode))
    {
      keypad_mode = new_mode;
    }
  else
    {
      sorry_message ();
      xv_set (keypad_item,
	      PANEL_VALUE, keypad_mode,
	      NULL);
    }
}

mouse_callback ()
{
  int new_mode;

  new_mode = xv_get (mouse_item, PANEL_VALUE);

  if ((new_mode != keypad_mode) &&
      (new_mode != js0_mode) &&
      (new_mode != js1_mode))
    {
      mouse_mode = new_mode;
    }
  else
    {
      sorry_message ();
      xv_set (mouse_item,
	      PANEL_VALUE, mouse_mode,
	      NULL);
    }
}

#ifdef LINUX_JOYSTICK
js0_callback ()
{
  int new_mode;

  new_mode = xv_get (js0_item, PANEL_VALUE);

  if ((new_mode != keypad_mode) &&
      (new_mode != mouse_mode) &&
      (new_mode != js1_mode))
    {
      js0_mode = new_mode;
    }
  else
    {
      sorry_message ();
      xv_set (js0_item,
	      PANEL_VALUE, js0_mode,
	      NULL);
    }
}

js1_callback ()
{
  int new_mode;

  new_mode = xv_get (js1_item, PANEL_VALUE);

  if ((new_mode != keypad_mode) &&
      (new_mode != mouse_mode) &&
      (new_mode != js0_mode))
    {
      js1_mode = new_mode;
    }
  else
    {
      sorry_message ();
      xv_set (js1_item,
	      PANEL_VALUE, js1_mode,
	      NULL);
    }
}
#endif

performance_ok_callback ()
{
  xv_set (performance_frame,
	  XV_SHOW, FALSE,
	  NULL);
}

performance_callback ()
{
  xv_set (performance_frame,
	  XV_SHOW, TRUE,
	  NULL);
}

refresh_callback (Panel_item item, int value, Event *event)
{
  refresh_rate = value;
}

#endif

void Atari_WhatIs (int mode)
{
  switch (mode)
    {
    case 0 :
      printf ("Joystick 0");
      break;
    case 1 :
      printf ("Joystick 1");
      break;
    case 2 :
      printf ("Joystick 2");
      break;
    case 3 :
      printf ("Joystick 3");
      break;
    default :
      printf ("not available");
      break;
    }
}

#ifdef MOTIF
void motif_boot_disk (Widget fs, XtPointer client_data,
		      XtPointer cbs)
{
  char *filename;

  if (XmStringGetLtoR(((XmFileSelectionBoxCallbackStruct *) cbs)->value,
		      XmSTRING_DEFAULT_CHARSET, &filename))
    {
      if (*filename)
	{
	  SIO_Dismount(1);
	  if (SIO_Mount (1, filename))
	    Coldstart ();
	}

      XtFree (filename);
    }

  XtUnmanageChild (fs);
  XtPopdown (XtParent(fs));
}

void motif_select_disk (Widget toggle, XtPointer client_data, XtPointer cbs)
{
  motif_disk_sel = (int) client_data;
}

void motif_insert_disk (Widget fs, XtPointer client_data, XtPointer cbs)
{
  char *filename;

  if (XmStringGetLtoR(((XmFileSelectionBoxCallbackStruct *) cbs)->value,
		      XmSTRING_DEFAULT_CHARSET, &filename))
    {
      if (*filename)
	{
	  SIO_Dismount(motif_disk_sel);
	  SIO_Mount (motif_disk_sel, filename);
	}

      XtFree (filename);
    }

  XtUnmanageChild (fs);
  XtPopdown (XtParent(fs));
}

void motif_select_rom (Widget toggle, XtPointer client_data, XtPointer cbs)
{
  motif_rom_sel = (int) client_data;
}

void motif_insert_rom (Widget fs, XtPointer client_data, XtPointer cbs)
{
  char *filename;
  int ret;

  if (XmStringGetLtoR(((XmFileSelectionBoxCallbackStruct *) cbs)->value,
		      XmSTRING_DEFAULT_CHARSET, &filename))
    {
      if (*filename)
	{
	  Remove_ROM ();
	  switch (motif_rom_sel)
	  {
	  case 1:
	    ret = Insert_8K_ROM(filename);
	    break;
	  case 2:
	    ret = Insert_16K_ROM(filename);
	    break;
	  case 3:
	    ret = Insert_OSS_ROM(filename);
	    break;
	  case 4:
	    ret = Insert_DB_ROM(filename);
	    break;
	  case 5:
	    ret = Insert_32K_5200ROM(filename);
	    break;
	  default:
	    ret = 0;
	    break;
	  }
	  if (ret)
	  {
	    Coldstart ();
	  }
	}

      XtFree (filename);
    }

  XtUnmanageChild (fs);
  XtPopdown (XtParent(fs));
}

void motif_fs_cancel (Widget fs, XtPointer client_data, XtPointer call_data)
{
  XtUnmanageChild (fs);
  XtPopdown (XtParent(fs));
}

void motif_eject_cback (Widget button, XtPointer client_data, XtPointer cbs)
{
  SIO_Dismount(((int) client_data) + 1);
}

void motif_disable_cback (Widget button, XtPointer client_data, XtPointer cbs)
{
  SIO_DisableDrive(((int) client_data) + 1);
}

void update_fsel(Widget fsel)
{
  XmString dirmask;

  XtVaGetValues(fsel, XmNdirMask, &dirmask, NULL);
  XmFileSelectionDoSearch(fsel, dirmask);
}

void motif_system_cback (Widget w, XtPointer item_no, XtPointer cbs)
{
  XmString t;
  int status;
  char *errmsg = NULL;

  switch ((int) item_no)
    {
    case 0 :
      update_fsel(fsel_b);
      XtManageChild (fsel_b);
      XtPopup (XtParent(fsel_b), XtGrabNone);
      break;
    case 1 :
      /* insert disk */
      update_fsel(fsel_d);
      XtManageChild (fsel_d);
      XtPopup (XtParent(fsel_d), XtGrabNone);
      break;
    case 2 :
      /* eject disk */
      /* handled by pullright menu */
      break;
    case 3 :
      /* disable drive */
      /* handled by pullright menu */
      break;
    case 4 :
      /* insert rom */
      update_fsel(fsel_r);
      XtManageChild (fsel_r);
      XtPopup (XtParent(fsel_r), XtGrabNone);
      break;
    case 5 :
      Remove_ROM ();
      Coldstart ();
      break;
    case 6 :
      EnablePILL ();
      Coldstart ();
      break;
    case 7 :
      status = Initialise_AtariOSA ();
      if (status == 0)
	errmsg = "Sorry, OS/A ROM Unavailable";
      break;
    case 8 :
      status = Initialise_AtariOSB ();
      if (status == 0)
	errmsg = "Sorry, OS/B ROM Unavailable";
      break;
    case 9 :
      status = Initialise_AtariXL ();
      if (status == 0)
	errmsg = "Sorry, XL/XE ROM Unavailable";
      break;
    case 10 :
      status = Initialise_AtariXE ();
      if (status == 0)
	errmsg = "Sorry, XL/XE ROM Unavailable";
      break;
    case 11 :
      status = Initialise_Atari5200 ();
      if (status == 0)
	errmsg = "Sorry, 5200 ROM Unavailable";
      break;
    case 12 :
      exit (0);
    }

  if (errmsg)
    {
      static Widget dialog = NULL;

      if (!dialog)
	{
	  Arg arg[1];

	  dialog = XmCreateErrorDialog (main_w, "message", arg, 0);

	  XtVaSetValues (dialog,
			 XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			 NULL);

	  XtUnmanageChild (XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON));
	  XtUnmanageChild (XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	}

      t = XmStringCreateSimple (errmsg);
      XtVaSetValues (dialog,
		     XmNmessageString, t,
		     NULL);
      XmStringFree (t);
      XtManageChild (dialog);
    }
}

void motif_consol_cback (Widget w, XtPointer item_no, XtPointer cbs)
{
  switch ((int) item_no)
    {
    case 0 :
      menu_consol &= 0x03; /* Option Pressed */
      break;
    case 1 :
      menu_consol &= 0x05; /* Select Pressed */
      break;
    case 2 :
      menu_consol &= 0x06; /* Start Pressed */
      break;
    case 3 :
      xview_keycode = AKEY_HELP;
      break;
    case 4 :
      xview_keycode = AKEY_BREAK;
      break;
    case 5 :
      Warmstart ();
      break;
    case 6 :
      Coldstart ();
      break;
    }
}

void motif_keypress (Widget w, XtPointer client_data, XEvent *event,
		     Boolean *continue_to_dispatch)
{
  int keycode;

  keycode = GetKeyCode (event);
  if (keycode != AKEY_NONE)
    xview_keycode = keycode;
}

void motif_exposure (Widget w, XtPointer client_data, XEvent *event,
		     Boolean *continue_to_dispatch)
{
  modified = TRUE;
}
#endif

void Atari_Initialise (int *argc, char *argv[])
{
#ifndef XVIEW
#ifndef MOTIF
  XSetWindowAttributes	xswda;
#endif
#endif

  XGCValues	xgcvl;

  int depth;
  int i, j;
  int mode = 0;

#ifdef XVIEW
  int ypos;

  xv_init (XV_INIT_ARGC_PTR_ARGV, argc, argv, NULL);
#endif

#ifdef MOTIF
  toplevel = XtVaAppInitialize (&app, "Atari800",
				NULL, 0,
				argc, argv, NULL,
				XtNtitle, ATARI_TITLE,
				NULL);
#endif

  gettimeofday (&tp, &tzp);
  basetime = tp.tv_sec + (tp.tv_usec / 1000000.0);

  for (i=j=1;i<*argc;i++)
    {
      if (strcmp(argv[i],"-small") == 0)
	windowsize = Small;
      else if (strcmp(argv[i],"-large") == 0)
	windowsize = Large;
      else if (strcmp(argv[i],"-huge") == 0)
	windowsize = Huge;
      else if (strcmp(argv[i],"-x11bug") == 0)
	x11bug = TRUE;
      else if (strcmp(argv[i],"-fps") == 0)
	x11_monitor = MONITOR_FPS;
      else if (strcmp(argv[i],"-sio") == 0)
	x11_monitor = MONITOR_SIO;
      else if (strcmp(argv[i],"-private_cmap") == 0)
        private_cmap = TRUE;
      else if (strcmp(argv[i],"-keypad") == 0)
        {
          if (keypad_mode == -1)
            keypad_mode = mode++;
        }
      else
	{
	  if (strcmp(argv[i],"-help") == 0)
	    {
	      printf ("\t-small        Small window (%dx%d)\n",
		      ATARI_WIDTH, ATARI_HEIGHT);
	      printf ("\t-large        Large window (%dx%d)\n",
		      ATARI_WIDTH*2, ATARI_HEIGHT*2);
	      printf ("\t-huge         Huge window (%dx%d)\n",
		      ATARI_WIDTH*3, ATARI_HEIGHT*3);
	      printf ("\t-x11bug       Enable debug code in atari_x11.c\n");
	    }

	  argv[j++] = argv[i];
	}
    }

  *argc = j;

#ifdef NAS
  NAS_Initialise (argc, argv);
#endif

#ifdef VOXWARE
  Voxware_Initialise (argc, argv);
#endif

#ifdef SHM
  if (windowsize != Small)
    {
      printf ("X Shared memory version only supports small window\n");
      windowsize = Small;
    }
#endif

  switch (windowsize)
    {
    case Small :
      window_width = ATARI_WIDTH;
      window_height = ATARI_HEIGHT;
      break;
    case Large :
      window_width = ATARI_WIDTH * 2;
      window_height = ATARI_HEIGHT * 2;
      break;
    case Huge :
      window_width = ATARI_WIDTH * 3;
      window_height = ATARI_HEIGHT * 3;
      break;
    }

#ifdef LINUX_JOYSTICK
  js0 = open ("/dev/js0", O_RDONLY, 0777);
  if (js0 != -1)
    {
      int status;

      status = read (js0, &js_data, JS_RETURN);
      if (status != JS_RETURN)
	{
	  perror ("/dev/js0");
	  exit (1);
	}

      js0_centre_x = js_data.x;
      js0_centre_y = js_data.y;

      if (x11bug)
	printf ("Joystick 0: centre_x = %d, centry_y = %d\n",
		js0_centre_x, js0_centre_y);

      js0_mode = mode++;
    }

  js1 = open ("/dev/js1", O_RDONLY, 0777);
  if (js1 != -1)
    {
      int status;

      status = read (js1, &js_data, JS_RETURN);
      if (status != JS_RETURN)
	{
	  perror ("/dev/js1");
	  exit (1);
	}

      js1_centre_x = js_data.x;
      js1_centre_y = js_data.y;

      if (x11bug)
	printf ("Joystick 1: centre_x = %d, centry_y = %d\n",
		js1_centre_x, js1_centre_y);

      js1_mode = mode++;
    }
#endif

  mouse_mode = mode++;
  if (keypad_mode == -1)
    keypad_mode = mode++;

#ifdef XVIEW
  frame = (Frame)xv_create ((Xv_opaque)NULL, FRAME,
			    FRAME_LABEL, ATARI_TITLE,
			    FRAME_SHOW_RESIZE_CORNER, FALSE,
			    XV_WIDTH, window_width,
			    XV_HEIGHT, window_height + 27,
			    FRAME_SHOW_FOOTER, TRUE,
			    XV_SHOW, TRUE,
			    NULL);

  panel = (Panel)xv_create (frame, PANEL,
			    XV_HEIGHT, 25,
			    XV_SHOW, TRUE,
			    NULL);

  system_menu = xv_create ((Xv_opaque)NULL, MENU,
			   MENU_ITEM,
			     MENU_STRING, "Boot Disk",
			     MENU_NOTIFY_PROC, boot_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Insert Disk",
			     MENU_NOTIFY_PROC, insert_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Eject Disk",
			     MENU_NOTIFY_PROC, eject_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Disable Drive",
			     MENU_NOTIFY_PROC, disable_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Insert Cartridge",
			     MENU_NOTIFY_PROC, insert_rom_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Remove Cartridge",
			     MENU_NOTIFY_PROC, remove_rom_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Enable PILL",
			     MENU_NOTIFY_PROC, enable_pill_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Atari 800 OS/A",
			     MENU_NOTIFY_PROC, coldstart_osa_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Atari 800 OS/B",
			     MENU_NOTIFY_PROC, coldstart_osb_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Atari 800XL",
			     MENU_NOTIFY_PROC, coldstart_xl_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Atari 130XE",
			     MENU_NOTIFY_PROC, coldstart_xe_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Atari 5200",
			     MENU_NOTIFY_PROC, coldstart_5200_callback,
			     NULL,
			   MENU_ITEM,
			     MENU_STRING, "Exit",
			     MENU_NOTIFY_PROC, exit_callback,
			     NULL,
			   NULL);

  xv_create (panel, PANEL_BUTTON,
	     PANEL_LABEL_STRING, "System",
	     PANEL_ITEM_MENU, system_menu,
	     NULL);

  consol_menu = (Menu)xv_create ((Xv_opaque)NULL, MENU,
				 MENU_ITEM,
				   MENU_STRING, "Option",
				   MENU_NOTIFY_PROC, option_callback,
				   NULL,
				 MENU_ITEM,
				   MENU_STRING, "Select",
				   MENU_NOTIFY_PROC, select_callback,
				   NULL,
				 MENU_ITEM,
				   MENU_STRING, "Start",
				   MENU_NOTIFY_PROC, start_callback,
				   NULL,
				 MENU_ITEM,
				   MENU_STRING, "Help",
				   MENU_NOTIFY_PROC, help_callback,
				   MENU_INACTIVE, (machine == Atari),
				   NULL,
				 MENU_ITEM,
				   MENU_STRING, "Break",
				   MENU_NOTIFY_PROC, break_callback,
				   NULL,
				 MENU_ITEM,
				   MENU_STRING, "Reset",
				   MENU_NOTIFY_PROC, reset_callback,
				   NULL,
				 MENU_ITEM,
				   MENU_STRING, "Coldstart",
				   MENU_NOTIFY_PROC, coldstart_callback,
				   NULL,
				 NULL);

  xv_create (panel, PANEL_BUTTON,
	     PANEL_LABEL_STRING, "Console",
	     PANEL_ITEM_MENU, consol_menu,
	     NULL);

  options_menu = (Menu)xv_create ((Xv_opaque)NULL, MENU,
				  MENU_ITEM,
				    MENU_STRING, "Controllers",
				    MENU_NOTIFY_PROC, controllers_callback,
				    NULL,
				  MENU_ITEM,
				    MENU_STRING, "Performance",
				    MENU_NOTIFY_PROC, performance_callback,
				    NULL,
				  NULL);

  xv_create (panel, PANEL_BUTTON,
	     PANEL_LABEL_STRING, "Options",
	     PANEL_ITEM_MENU, options_menu,
	     NULL);

  canvas = (Canvas)xv_create (frame, CANVAS,
			      CANVAS_WIDTH, window_width,
			      CANVAS_HEIGHT, window_height,
			      NULL);
/*
   =====================================
   Create Controller Configuration Frame
   =====================================
*/
  controllers_frame = (Frame)xv_create (frame, FRAME_CMD,
				       FRAME_LABEL, "Controller Configuration",
				       XV_WIDTH, 300,
				       XV_HEIGHT, 150,
				       NULL);

  controllers_panel = (Panel)xv_get (controllers_frame, FRAME_CMD_PANEL,
				     NULL);

  ypos = 10;
  keypad_item = (Panel_item)xv_create (controllers_panel, PANEL_CHOICE_STACK,
				       PANEL_VALUE_X, 150,
				       PANEL_VALUE_Y, ypos,
				       PANEL_LAYOUT, PANEL_HORIZONTAL,
				       PANEL_LABEL_STRING, "Numeric Keypad",
				       PANEL_CHOICE_STRINGS,
				         "Joystick 1",
				         "Joystick 2",
				         "Joystick 3",
				         "Joystick 4",
				         NULL,
				       PANEL_VALUE, keypad_mode,
				       PANEL_NOTIFY_PROC, keypad_callback,
				       NULL);
  ypos += 25;

  mouse_item = (Panel_item)xv_create (controllers_panel, PANEL_CHOICE_STACK,
				      PANEL_VALUE_X, 150,
				      PANEL_VALUE_Y, ypos,
				      PANEL_LAYOUT, PANEL_HORIZONTAL,
				      PANEL_LABEL_STRING, "Mouse",
				      PANEL_CHOICE_STRINGS,
				        "Joystick 1",
				        "Joystick 2",
				        "Joystick 3",
				        "Joystick 4",
				        "Paddle 1",
				        "Paddle 2",
				        "Paddle 3",
				        "Paddle 4",
				        "Paddle 5",
				        "Paddle 6",
				        "Paddle 7",
				        "Paddle 8",
				        "Light Pen",
				        NULL,
				      PANEL_VALUE, mouse_mode,
				      PANEL_NOTIFY_PROC, mouse_callback,
				      NULL);
  ypos += 25;

#ifdef LINUX_JOYSTICK
  if (js0 != -1)
    {
      js0_item = (Panel_item)xv_create (controllers_panel, PANEL_CHOICE_STACK,
					PANEL_VALUE_X, 150,
					PANEL_VALUE_Y, ypos,
					PANEL_LAYOUT, PANEL_HORIZONTAL,
					PANEL_LABEL_STRING, "/dev/js0",
					PANEL_CHOICE_STRINGS,
					  "Joystick 1",
					  "Joystick 2",
					  "Joystick 3",
					  "Joystick 4",
					NULL,
					PANEL_VALUE, js0_mode,
					PANEL_NOTIFY_PROC, js0_callback,
					NULL);
      ypos += 25;
    }

  if (js1 != -1)
    {
      js1_item = (Panel_item)xv_create (controllers_panel, PANEL_CHOICE_STACK,
					PANEL_VALUE_X, 150,
					PANEL_VALUE_Y, ypos,
					PANEL_LAYOUT, PANEL_HORIZONTAL,
					PANEL_LABEL_STRING, "/dev/js1",
					PANEL_CHOICE_STRINGS,
					  "Joystick 1",
					  "Joystick 2",
					  "Joystick 3",
					  "Joystick 4",
					NULL,
					PANEL_VALUE, js1_mode,
					PANEL_NOTIFY_PROC, js1_callback,
					NULL);
      ypos += 25;
    }
#endif

  xv_create (controllers_panel, PANEL_BUTTON,
	     XV_X, 130,
	     XV_Y, 125,
	     PANEL_LABEL_STRING, "OK",
	     PANEL_NOTIFY_PROC, controllers_ok_callback,
	     NULL);
/*
   ======================================
   Create Performance Configuration Frame
   ======================================
*/
  performance_frame = (Frame)xv_create (frame, FRAME_CMD,
					FRAME_LABEL, "Performance Configuration",
					XV_WIDTH, 400,
					XV_HEIGHT, 100,
					NULL);

  performance_panel = (Panel)xv_get (performance_frame, FRAME_CMD_PANEL,
				     NULL);

  ypos = 10;
  refresh_slider = (Panel_item)xv_create (performance_panel, PANEL_SLIDER,
					  PANEL_VALUE_X, 155,
					  PANEL_VALUE_Y, ypos,
					  PANEL_LAYOUT, PANEL_HORIZONTAL,
					  PANEL_LABEL_STRING, "Screen Refresh Rate",
					  PANEL_VALUE, refresh_rate,
					  PANEL_MIN_VALUE, 1,
					  PANEL_MAX_VALUE, 32,
					  PANEL_SLIDER_WIDTH, 100,
					  PANEL_TICKS, 32,
					  PANEL_NOTIFY_PROC, refresh_callback,
					  NULL);
  ypos += 25;

  xv_create (performance_panel, PANEL_BUTTON,
	     XV_X, 180,
	     XV_Y, 75,
	     PANEL_LABEL_STRING, "OK",
	     PANEL_NOTIFY_PROC, performance_ok_callback,
	     NULL);
/*
   ====================
   Get X Window Objects
   ====================
*/
  display = (Display*)xv_get(frame, XV_DISPLAY);
  if (!display)
    {
      printf ("Failed to open display\n");
      exit (1);
    }

  screen = XDefaultScreenOfDisplay (display);
  if (!screen)
    {
      printf ("Unable to get screen\n");
      exit (1);
    }

  visual = XDefaultVisualOfScreen (screen);
  if (!visual)
    {
      printf ("Unable to get visual\n");
      exit (1);
    }

  window = (Window)xv_get(canvas_paint_window(canvas), XV_XID);
  depth = XDefaultDepthOfScreen (screen);
  cmap = XDefaultColormapOfScreen(screen);

  chooser = (Frame)xv_create (frame, FILE_CHOOSER,
			      FILE_CHOOSER_TYPE, FILE_CHOOSER_OPEN,
			      NULL);

  xv_set (canvas_paint_window(canvas),
	  WIN_EVENT_PROC, event_proc,
	  WIN_CONSUME_EVENTS, WIN_ASCII_EVENTS, WIN_MOUSE_BUTTONS, NULL,
	  NULL);
#endif

#ifdef MOTIF
  {
    Widget menubar;

    XmString s_system;
    XmString s_boot_disk;
    XmString s_insert_disk;
    XmString s_eject_disk;
    XmString s_disable_drive;
    XmString s_insert_cart;
    XmString s_remove_cart;
    XmString s_enable_pill;
    XmString s_osa;
    XmString s_osb;
    XmString s_osxl;
    XmString s_osxe;
    XmString s_os5200;
    XmString s_exit;

    XmString s_console;
    XmString s_option;
    XmString s_select;
    XmString s_start;
    XmString s_help;
    XmString s_break;
    XmString s_warmstart;
    XmString s_coldstart;

    XmString s_label;

    XmString s_d1, s_d2, s_d3, s_d4;
    XmString s_d5, s_d6, s_d7, s_d8;

    char *tmpstr;
    XmString xmtmpstr;

    Arg args[8];
    int n;

    main_w = XtVaCreateManagedWidget ("main_window",
				      xmMainWindowWidgetClass, toplevel,
				      NULL);

    s_system = XmStringCreateSimple ("System");
    s_boot_disk = XmStringCreateSimple ("Boot Disk...");
    s_insert_disk = XmStringCreateSimple ("Insert Disk...");
    s_eject_disk = XmStringCreateSimple ("Eject Disk");
    s_disable_drive = XmStringCreateSimple ("Disable Drive");
    s_insert_cart = XmStringCreateSimple ("Insert Cartridge...");
    s_remove_cart = XmStringCreateSimple ("Remove Cartridge");
    s_enable_pill = XmStringCreateSimple ("Enable PILL");
    s_osa = XmStringCreateSimple ("Atari 800 OS/A");
    s_osb = XmStringCreateSimple ("Atari 800 OS/B");
    s_osxl = XmStringCreateSimple ("Atari 800XL");
    s_osxe = XmStringCreateSimple ("Atari 130XE");
    s_os5200 = XmStringCreateSimple ("Atari 5200");
    s_exit = XmStringCreateSimple ("Exit");

    s_console = XmStringCreateSimple ("Console");
    s_option = XmStringCreateSimple ("Option");
    s_select = XmStringCreateSimple ("Select");
    s_start = XmStringCreateSimple ("Start");
    s_help = XmStringCreateSimple ("Help");
    s_break = XmStringCreateSimple ("Break");
    s_warmstart = XmStringCreateSimple ("Warmstart");
    s_coldstart = XmStringCreateSimple ("Coldstart");
;
    menubar = XmVaCreateSimpleMenuBar (main_w, "menubar",
				       XmVaCASCADEBUTTON, s_system, 'S',
				       XmVaCASCADEBUTTON, s_console, 'C',
				       NULL);

    system_menu =
    XmVaCreateSimplePulldownMenu (menubar, "system_menu", 0, motif_system_cback,
				  XmVaPUSHBUTTON, s_boot_disk, 'o', NULL, NULL,
				  XmVaPUSHBUTTON, s_insert_disk, 'I', NULL, NULL,
				  XmVaCASCADEBUTTON, s_eject_disk, 'j',
				  XmVaCASCADEBUTTON, s_disable_drive, 'D',
				  XmVaSEPARATOR,
				  XmVaPUSHBUTTON, s_insert_cart, 'n', NULL, NULL,
				  XmVaPUSHBUTTON, s_remove_cart, 'R', NULL, NULL,
				  XmVaPUSHBUTTON, s_enable_pill, 'P', NULL, NULL,
				  XmVaSEPARATOR,
				  XmVaPUSHBUTTON, s_osa, 'A', NULL, NULL,
				  XmVaPUSHBUTTON, s_osb, 'B', NULL, NULL,
				  XmVaPUSHBUTTON, s_osxl, 'L', NULL, NULL,
				  XmVaPUSHBUTTON, s_osxe, 'E', NULL, NULL,
				  XmVaPUSHBUTTON, s_os5200, '5', NULL, NULL,
				  XmVaSEPARATOR,
				  XmVaPUSHBUTTON, s_exit, 'x', NULL, NULL,
				  NULL);

    XmVaCreateSimplePulldownMenu (menubar, "console_menu", 1, motif_consol_cback,
				  XmVaPUSHBUTTON, s_option, 'O', NULL, NULL,
				  XmVaPUSHBUTTON, s_select, 't', NULL, NULL,
				  XmVaPUSHBUTTON, s_start, 'S', NULL, NULL,
				  XmVaSEPARATOR,
				  XmVaPUSHBUTTON, s_help, 'H', NULL, NULL,
				  XmVaPUSHBUTTON, s_break, 'B', NULL, NULL,
				  XmVaSEPARATOR,
				  XmVaPUSHBUTTON, s_warmstart, 'W', NULL, NULL,
				  XmVaPUSHBUTTON, s_coldstart, 'C', NULL, NULL,
				  NULL);

    XmStringFree (s_system);
    XmStringFree (s_boot_disk);
    XmStringFree (s_insert_disk);
    XmStringFree (s_eject_disk);
    XmStringFree (s_disable_drive);
    XmStringFree (s_insert_cart);
    XmStringFree (s_remove_cart);
    XmStringFree (s_enable_pill);
    XmStringFree (s_osa);
    XmStringFree (s_osb);
    XmStringFree (s_osxl);
    XmStringFree (s_osxe);
    XmStringFree (s_os5200);
    XmStringFree (s_exit);

    XmStringFree (s_console);
    XmStringFree (s_option);
    XmStringFree (s_select);
    XmStringFree (s_start);
    XmStringFree (s_help);
    XmStringFree (s_break);
    XmStringFree (s_warmstart);
    XmStringFree (s_coldstart);

    XtManageChild (menubar);

    fsel_b = XmCreateFileSelectionDialog (toplevel, "boot_disk", NULL, 0);
    XtAddCallback (fsel_b, XmNokCallback, motif_boot_disk, NULL);
    XtAddCallback (fsel_b, XmNcancelCallback, motif_fs_cancel, NULL);

    fsel_d = XmCreateFileSelectionDialog (toplevel, "load_disk", NULL, 0);
    XtAddCallback (fsel_d, XmNokCallback, motif_insert_disk, NULL);
    XtAddCallback (fsel_d, XmNcancelCallback, motif_fs_cancel, NULL);

    n = 0;
    XtSetArg(args[n], XmNradioBehavior, True); n++;
    XtSetArg(args[n], XmNradioAlwaysOne, True); n++;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
    rbox_d = XmCreateWorkArea(fsel_d, "rbox_d", args, n);
    XtManageChild(rbox_d);

    s_label = XmStringCreateSimple("D1:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    XtSetArg(args[n], XmNset, True); n++;
    togg_d1 = XmCreateToggleButtonGadget(rbox_d, "togg_d1", args, n);
    XtManageChild(togg_d1);
    XmStringFree(s_label);
    XtAddCallback (togg_d1, XmNarmCallback, motif_select_disk, (XtPointer) 1);

    s_label = XmStringCreateSimple("D2:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d2 = XmCreateToggleButtonGadget(rbox_d, "togg_d2", args, n);
    XtManageChild(togg_d2);
    XmStringFree(s_label);
    XtAddCallback (togg_d2, XmNarmCallback, motif_select_disk, (XtPointer) 2);

    s_label = XmStringCreateSimple("D3:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d3 = XmCreateToggleButtonGadget(rbox_d, "togg_d3", args, n);
    XtManageChild(togg_d3);
    XmStringFree(s_label);
    XtAddCallback (togg_d3, XmNarmCallback, motif_select_disk, (XtPointer) 3);

    s_label = XmStringCreateSimple("D4:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d4 = XmCreateToggleButtonGadget(rbox_d, "togg_d4", args, n);
    XtManageChild(togg_d4);
    XmStringFree(s_label);
    XtAddCallback (togg_d4, XmNarmCallback, motif_select_disk, (XtPointer) 4);

    s_label = XmStringCreateSimple("D5:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d5 = XmCreateToggleButtonGadget(rbox_d, "togg_d5", args, n);
    XtManageChild(togg_d5);
    XmStringFree(s_label);
    XtAddCallback (togg_d5, XmNarmCallback, motif_select_disk, (XtPointer) 5);

    s_label = XmStringCreateSimple("D6:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d6 = XmCreateToggleButtonGadget(rbox_d, "togg_d6", args, n);
    XtManageChild(togg_d6);
    XmStringFree(s_label);
    XtAddCallback (togg_d6, XmNarmCallback, motif_select_disk, (XtPointer) 6);

    s_label = XmStringCreateSimple("D7:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d7 = XmCreateToggleButtonGadget(rbox_d, "togg_d7", args, n);
    XtManageChild(togg_d7);
    XmStringFree(s_label);
    XtAddCallback (togg_d7, XmNarmCallback, motif_select_disk, (XtPointer) 7);

    s_label = XmStringCreateSimple("D8:");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_d8 = XmCreateToggleButtonGadget(rbox_d, "togg_d8", args, n);
    XtManageChild(togg_d8);
    XmStringFree(s_label);
    XtAddCallback (togg_d8, XmNarmCallback, motif_select_disk, (XtPointer) 8);


    fsel_r = XmCreateFileSelectionDialog (toplevel, "load_rom", NULL, 0);
    XtAddCallback (fsel_r, XmNokCallback, motif_insert_rom, NULL);
    XtAddCallback (fsel_r, XmNcancelCallback, motif_fs_cancel, NULL);

    n = 0;
    XtSetArg(args[n], XmNradioBehavior, True); n++;
    XtSetArg(args[n], XmNradioAlwaysOne, True); n++;
    rbox_r = XmCreateWorkArea(fsel_r, "rbox_r", args, n);
    XtManageChild(rbox_r);

    s_label = XmStringCreateSimple("8K");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    XtSetArg(args[n], XmNset, True); n++;
    togg_8k = XmCreateToggleButtonGadget(rbox_r, "togg_8k", args, n);
    XtManageChild(togg_8k);
    XmStringFree(s_label);
    XtAddCallback (togg_8k, XmNarmCallback, motif_select_rom, (XtPointer) 1);

    s_label = XmStringCreateSimple("16K");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_16k = XmCreateToggleButtonGadget(rbox_r, "togg_16k", args, n);
    XtManageChild(togg_16k);
    XmStringFree(s_label);
    XtAddCallback (togg_16k, XmNarmCallback, motif_select_rom, (XtPointer) 2);

    s_label = XmStringCreateSimple("OSS 16K Bank Switched");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_oss = XmCreateToggleButtonGadget(rbox_r, "togg_oss", args, n);
    XtManageChild(togg_oss);
    XmStringFree(s_label);
    XtAddCallback (togg_oss, XmNarmCallback, motif_select_rom, (XtPointer) 3);

    s_label = XmStringCreateSimple("DB 32K Bank Switched");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_32k = XmCreateToggleButtonGadget(rbox_r, "togg_32k", args, n);
    XtManageChild(togg_32k);
    XmStringFree(s_label);
    XtAddCallback (togg_32k, XmNarmCallback, motif_select_rom, (XtPointer) 4);

    s_label = XmStringCreateSimple("5200 32K");
    n = 0;
    XtSetArg(args[n], XmNlabelString, s_label); n++;
    togg_5200 = XmCreateToggleButtonGadget(rbox_r, "togg_5200", args, n);
    XtManageChild(togg_5200);
    XmStringFree(s_label);
    XtAddCallback (togg_5200, XmNarmCallback, motif_select_rom, (XtPointer) 5);

    tmpstr = (char *) XtMalloc(strlen(atari_disk_dir + 3));
    strcpy(tmpstr, atari_disk_dir);
    strcat(tmpstr, "/*");
    xmtmpstr = XmStringCreateSimple(tmpstr);
    XmFileSelectionDoSearch(fsel_b, xmtmpstr);
    XmFileSelectionDoSearch(fsel_d, xmtmpstr);
    XmStringFree(xmtmpstr);
    XtFree(tmpstr);

    tmpstr = (char *) XtMalloc(strlen(atari_rom_dir + 3));
    strcpy(tmpstr, atari_rom_dir);
    strcat(tmpstr, "/*");
    xmtmpstr = XmStringCreateSimple(tmpstr);
    XmFileSelectionDoSearch(fsel_r, xmtmpstr);
    XmStringFree(xmtmpstr);
    XtFree(tmpstr);

    s_d1 = XmStringCreateSimple("D1:");
    s_d2 = XmStringCreateSimple("D2:");
    s_d3 = XmStringCreateSimple("D3:");
    s_d4 = XmStringCreateSimple("D4:");
    s_d5 = XmStringCreateSimple("D5:");
    s_d6 = XmStringCreateSimple("D6:");
    s_d7 = XmStringCreateSimple("D7:");
    s_d8 = XmStringCreateSimple("D8:");
    eject_menu = XmVaCreateSimplePulldownMenu(system_menu,
					      "eject_disk", 2,
					      motif_eject_cback,
					   XmVaPUSHBUTTON, s_d1, '1', NULL, NULL,
					   XmVaPUSHBUTTON, s_d2, '2', NULL, NULL,
					   XmVaPUSHBUTTON, s_d3, '3', NULL, NULL,
					   XmVaPUSHBUTTON, s_d4, '4', NULL, NULL,
					   XmVaPUSHBUTTON, s_d5, '5', NULL, NULL,
					   XmVaPUSHBUTTON, s_d6, '6', NULL, NULL,
					   XmVaPUSHBUTTON, s_d7, '7', NULL, NULL,
					   XmVaPUSHBUTTON, s_d8, '8', NULL, NULL,
					   NULL);
    disable_menu = XmVaCreateSimplePulldownMenu(system_menu,
					      "disable_disk", 3,
					      motif_disable_cback,
					   XmVaPUSHBUTTON, s_d1, '1', NULL, NULL,
					   XmVaPUSHBUTTON, s_d2, '2', NULL, NULL,
					   XmVaPUSHBUTTON, s_d3, '3', NULL, NULL,
					   XmVaPUSHBUTTON, s_d4, '4', NULL, NULL,
					   XmVaPUSHBUTTON, s_d5, '5', NULL, NULL,
					   XmVaPUSHBUTTON, s_d6, '6', NULL, NULL,
					   XmVaPUSHBUTTON, s_d7, '7', NULL, NULL,
					   XmVaPUSHBUTTON, s_d8, '8', NULL, NULL,
					   NULL);
    XmStringFree(s_d1);
    XmStringFree(s_d2);
    XmStringFree(s_d3);
    XmStringFree(s_d4);
    XmStringFree(s_d5);
    XmStringFree(s_d6);
    XmStringFree(s_d7);
    XmStringFree(s_d8);

    drawing_area = XtVaCreateManagedWidget ("Canvas",
					    xmDrawingAreaWidgetClass, main_w,
					    XmNunitType, XmPIXELS,
					    XmNheight, window_height,
					    XmNwidth, window_width,
					    XmNresizePolicy, XmNONE,
					    NULL);

    XtAddEventHandler (drawing_area,
		       KeyPressMask | KeyReleaseMask,
		       False,
		       motif_keypress, NULL);

    XtAddEventHandler (drawing_area,
		       ExposureMask, 
		       False,
		       motif_exposure, NULL);

    XtRealizeWidget (toplevel);
  }

  display = XtDisplay (drawing_area);

  window = XtWindow (drawing_area);

  screen = XDefaultScreenOfDisplay (display);
  if (!screen)
    {
      printf ("Unable to get screen\n");
      exit (1);
    }

  visual = XDefaultVisualOfScreen (screen);
  if (!visual)
    {
      printf ("Unable to get visual\n");
      exit (1);
    }

  depth = XDefaultDepthOfScreen (screen);
  cmap = XDefaultColormapOfScreen(screen);
#endif

#ifndef MOTIF
#ifndef XVIEW
  display = XOpenDisplay (NULL);
  if (!display)
    {
      printf ("Failed to open display\n");
      exit (1);
    }

  screen = XDefaultScreenOfDisplay (display);
  if (!screen)
    {
      printf ("Unable to get screen\n");
      exit (1);
    }

  visual = XDefaultVisualOfScreen (screen);
  if (!visual)
    {
      printf ("Unable to get visual\n");
      exit (1);
    }

  depth = XDefaultDepthOfScreen (screen);

  if (private_cmap)
    cmap = XCreateColormap (display,
                            XRootWindowOfScreen(screen),
                            visual,
                            AllocNone);
  else
    cmap = XDefaultColormapOfScreen(screen);

  xswda.event_mask = KeyPressMask | KeyReleaseMask | ExposureMask;
  xswda.colormap = cmap;

  window = XCreateWindow (display,
			  XRootWindowOfScreen(screen),
			  50, 50,
			  window_width, window_height, 3, depth,
			  InputOutput, visual,
			  CWEventMask | CWBackPixel | CWColormap,
			  &xswda);

  XStoreName (display, window, ATARI_TITLE);
#endif
#endif

#ifdef SHM
  {
    int major;
    int minor;
    Bool pixmaps;
    Status status;

    status = XShmQueryVersion (display, &major, &minor, &pixmaps);
    if (!status)
      {
	printf ("X Shared Memory extensions not available\n");
	exit (1);
      }

    printf ("Using X11 Shared Memory Extensions\n");

    image = XShmCreateImage (display, visual, depth, ZPixmap,
			     NULL, &shminfo, window_width, window_height);

    shminfo.shmid = shmget (IPC_PRIVATE,
			    (ATARI_HEIGHT+16) * ATARI_WIDTH,
			    IPC_CREAT | 0777);
    shminfo.shmaddr = image->data = atari_screen = shmat (shminfo.shmid, 0, 0);
    shminfo.readOnly = False;

    XShmAttach (display, &shminfo);

    XSync (display, False);

    shmctl (shminfo.shmid, IPC_RMID, 0);
  }
#else
  pixmap = XCreatePixmap (display, window,
			  window_width, window_height, depth);
#endif

  for (i=0;i<256;i+=2)
    {
      XColor	colour;

      int	rgb = colortable[i];
      int	status;

      colour.red = (rgb & 0x00ff0000) >> 8;
      colour.green = (rgb & 0x0000ff00);
      colour.blue = (rgb & 0x000000ff) << 8;

      status = XAllocColor (display,
			    cmap,
			    &colour);

      colours[i] = colour.pixel;
      colours[i+1] = colour.pixel;

#ifdef SHM
      colour_translation_table[i] = colours[i];
      colour_translation_table[i+1] = colours[i+1];
#endif
    }

  for (i=0;i<256;i++)
    {
      xgcvl.background = colours[0];
      xgcvl.foreground = colours[i];

      gc_colour[i] = XCreateGC (display, window,
				GCForeground | GCBackground,
				&xgcvl);
    }

  xgcvl.background = colours[0];
  xgcvl.foreground = colours[0];

  gc = XCreateGC (display, window,
		  GCForeground | GCBackground,
		  &xgcvl);

#ifndef SHM
  XFillRectangle (display, pixmap, gc, 0, 0,
		  window_width, window_height);
#endif

  XMapWindow (display, window);

  XSync (display, False);
/*
   ============================
   Storage for Atari 800 Screen
   ============================
*/
  image_data = (UBYTE*) malloc (ATARI_WIDTH * ATARI_HEIGHT);
  if (!image_data)
    {
      printf ("Failed to allocate space for image\n");
      exit (1);
    }

  keyboard_consol = 7;
  menu_consol = 7;

  if (x11bug)
    {
      printf ("Initial X11 controller configuration\n");
      printf ("------------------------------------\n\n");
      printf ("Keypad is "); Atari_WhatIs (keypad_mode); printf ("\n");
      printf ("Mouse is "); Atari_WhatIs (mouse_mode); printf ("\n");
      printf ("/dev/js0 is "); Atari_WhatIs (js0_mode); printf ("\n");
      printf ("/dev/js1 is "); Atari_WhatIs (js1_mode); printf ("\n");
    }
}

int Atari_Exit (int run_monitor)
{
  int restart;

  if (run_monitor)
    restart = monitor();
  else
    restart = FALSE;

  if (!restart)
    {
      free (image_data);

      XSync (display, True);

      if (private_cmap)
        XFreeColormap (display, cmap);

#ifdef SHM
      XDestroyImage (image);
#else
      XFreePixmap (display, pixmap);
#endif
      XUnmapWindow (display, window);
      XDestroyWindow (display, window);
      XCloseDisplay (display);

#ifdef LINUX_JOYSTICK
      if (js0 != -1)
	close (js0);

      if (js1 != -1)
	close (js1);
#endif

#ifdef NAS
      NAS_Exit ();
#endif

#ifdef VOXWARE 
      Voxware_Exit ();
#endif
    }

  return restart;
}

#ifndef SHM
void Atari_ScanLine_Flush ()
{
  if (windowsize == Small)
    {
      if (npoints != 0)
	{
	  XDrawPoints (display, pixmap, gc_colour[last_colour],
		       points, npoints, CoordModeOrigin);
	  npoints = 0;
	  modified = TRUE;
	}
    }
  else
    {
      if (nrects != 0)
	{
	  XFillRectangles (display, pixmap, gc_colour[last_colour],
			   rectangles, nrects);
	  nrects = 0;
	  modified = TRUE;
	}
    }

  last_colour = -1;
}
#endif

void ScreenDump ()
{
  static char command[128];
  static int frame_num = 0;

  sprintf (command, "xwd -name \"%s\"|xwdtopnm -|ppmtogif>%d.gif",
           ATARI_TITLE, frame_num++);
/*
  sprintf (command, "xwd -name \"%s\"|xwdtopnm -|pnmcut 0 25 384 240 -|ppmtogif>%d.gif",
           ATARI_TITLE, frame_num++);
*/

  system (command);
}

void Atari_DisplayScreen (UBYTE *screen)
{
  static char status_line[64];
  int update_status_line;

#ifdef SHM
  XShmPutImage (display, window, gc, image, 0, 0, 0, 0,
		window_width, window_height, 0);
  XSync(display, FALSE);
#else
  UBYTE *scanline_ptr = image_data;
  int xpos;
  int ypos;

  for (ypos=0;ypos<ATARI_HEIGHT;ypos++)
    {
      for (xpos=0;xpos<ATARI_WIDTH;xpos++)
	{
	  UBYTE colour;

	  colour = *screen++;

	  if (colour != *scanline_ptr)
	    {
	      int flush = FALSE;

	      if (windowsize == Small)
		{
		  if (npoints == NPOINTS)
		    flush = TRUE;
		}
	      else
		{
		  if (nrects == NRECTS)
		    flush = TRUE;
		}

	      if (colour != last_colour)
		flush = TRUE;

	      if (flush)
		{
		  Atari_ScanLine_Flush ();
		  last_colour = colour;
		}

	      if (windowsize == Small)
		{
		  points[npoints].x = xpos;
		  points[npoints].y = ypos;
		  npoints++;
		}
	      else if (windowsize == Large)
		{
		  rectangles[nrects].x = xpos << 1;
		  rectangles[nrects].y = ypos << 1;
		  rectangles[nrects].width = 2;
		  rectangles[nrects].height = 2;
		  nrects++;
		}
	      else
		{
		  rectangles[nrects].x = xpos + xpos + xpos;
		  rectangles[nrects].y = ypos + ypos + ypos;
		  rectangles[nrects].width = 3;
		  rectangles[nrects].height = 3;
		  nrects++;
		}

	      *scanline_ptr++ = colour;
	    }
	  else
	    {
	      scanline_ptr++;
	    }
	}
    }

  Atari_ScanLine_Flush ();

  if (modified)
    {
      XCopyArea (display, pixmap, window, gc, 0, 0,
		 window_width, window_height, 0, 0);
    }
  modified = FALSE;
#endif

  keypad_trig = 1;

  switch (x11_monitor)
    {
    case MONITOR_SIO :
      if (sio_status[0] != '\0')
	{
#ifdef XVIEW
	  strcpy (status_line, sio_status);
#else
	  sprintf (status_line, "%s - %s",
		   ATARI_TITLE, sio_status);
#endif
	  sio_status[0] = '\0';
	  update_status_line = TRUE;
	}
      else
	{
	  update_status_line = FALSE;
	}
      break;
    case MONITOR_FPS :
      {
	double curtime;

	gettimeofday (&tp, &tzp);
	curtime = tp.tv_sec + (tp.tv_usec / 1000000.0);

	nframes++;

	if ((curtime - basetime) >= 2.0)
	  {
#ifdef XVIEW
	    sprintf (status_line, "%.2f FPS",
		     (double)nframes / (curtime - basetime));
#else
	    sprintf (status_line, " %s - %.2f FPS",
		     ATARI_TITLE, (double)nframes / (curtime - basetime));
#endif

	    nframes = 0;
	    basetime = curtime;
	  }
      }
      update_status_line = TRUE;
      break;
    default :
      update_status_line = FALSE;
      break;
    }

  if (update_status_line)
    {
#ifdef XVIEW
      xv_set (frame,
	      FRAME_LEFT_FOOTER, status_line,
	      NULL);
#else
#ifdef MOTIF
      XtVaSetValues(toplevel,
		    XtNtitle, status_line,
		    NULL);
#else
      XStoreName (display, window, status_line);
#endif
#endif
    }

#ifdef XVIEW
  notify_dispatch ();
  XFlush (display);
#endif

#ifdef MOTIF
  while (XtAppPending(app))
    {
      static XEvent event;

      XtAppNextEvent (app, &event);
      XtDispatchEvent (&event);
    }
#endif

#ifdef NAS
  NAS_UpdateSound ();
#endif

#ifdef VOXWARE
  Voxware_UpdateSound ();
#endif

  if (screen_dump)
    {
      ScreenDump ();

      if (screen_dump == 2)
        screen_dump = 0;
    }
}

int Atari_Keyboard (void)
{
  int	keycode = AKEY_NONE;

#ifdef XVIEW
  keycode = xview_keycode;
  xview_keycode = AKEY_NONE;
#else
#ifdef MOTIF
  keycode = xview_keycode;
  xview_keycode = AKEY_NONE;
#else
  if (XEventsQueued (display, QueuedAfterFlush) > 0)
    {
      XEvent	event;

      XNextEvent (display, &event);
      keycode = GetKeyCode (&event);
    }
#endif
#endif

  return keycode;
}

void mouse_joystick (int mode)
{
  Window root_return;
  Window child_return;
  int root_x_return;
  int root_y_return;
  int win_x_return;
  int win_y_return;
  int mask_return;

  mouse_stick = 0x0f;

  XQueryPointer (display, window, &root_return, &child_return,
		 &root_x_return, &root_y_return,
		 &win_x_return, &win_y_return,
		 &mask_return);

  if (mode < 5)
    {
      int center_x;
      int center_y;
      int threshold;

      if (windowsize == Small)
	{
	  center_x = ATARI_WIDTH / 2;
	  center_y = ATARI_HEIGHT / 2;
	  threshold = 32;
	}
      else if (windowsize == Large)
	{
	  center_x = (ATARI_WIDTH * 2) / 2;
	  center_y = (ATARI_HEIGHT * 2) / 2;
	  threshold = 64;
	}
      else
	{
	  center_x = (ATARI_WIDTH * 3) / 2;
	  center_y = (ATARI_HEIGHT * 3) / 2;
	  threshold = 96;
	}

      if (win_x_return < (center_x - threshold))
	mouse_stick &= 0xfb;
      if (win_x_return > (center_x + threshold))
	mouse_stick &= 0xf7;
      if (win_y_return < (center_y - threshold))
	mouse_stick &= 0xfe;
      if (win_y_return > (center_y + threshold))
	mouse_stick &= 0xfd;
    }
  else
    {
      if (mask_return)
	mouse_stick &= 0xfb;
    }
}

#ifdef LINUX_JOYSTICK

void read_joystick (int js, int centre_x, int centre_y)
{
  const int threshold = 50;
  int status;

  mouse_stick = 0x0f;

  status = read (js, &js_data, JS_RETURN);
  if (status != JS_RETURN)
    {
      perror ("/dev/js");
      exit (1);
    }

  if (js_data.x < (centre_x - threshold))
    mouse_stick &= 0xfb;
  if (js_data.x > (centre_x + threshold))
    mouse_stick &= 0xf7;
  if (js_data.y < (centre_y - threshold))
    mouse_stick &= 0xfe;
  if (js_data.y > (centre_y + threshold))
    mouse_stick &= 0xfd;
}
#endif

int Atari_PORT (int num)
{
  int nibble_0 = 0x0f;
  int nibble_1 = 0x0f;

  if (num == 0)
    {
      if (keypad_mode == 0)
	nibble_0 = keypad_stick;
      else if (keypad_mode == 1)
	nibble_1 = keypad_stick;

      if (mouse_mode == 0)
	{
	  mouse_joystick (mouse_mode);
	  nibble_0 = mouse_stick;
	}
      else if (mouse_mode == 1)
	{
	  mouse_joystick (mouse_mode);
	  nibble_1 = mouse_stick;
	}

#ifdef LINUX_JOYSTICK
      if (js0_mode == 0)
	{
	  read_joystick (js0, js0_centre_x, js0_centre_y);
	  nibble_0 = mouse_stick;
	}
      else if (js0_mode == 1)
	{
	  read_joystick (js0, js0_centre_x, js0_centre_y);
	  nibble_1 = mouse_stick;
	}

      if (js1_mode == 0)
	{
	  read_joystick (js1, js1_centre_x, js1_centre_y);
	  nibble_0 = mouse_stick;
	}
      else if (js1_mode == 1)
	{
	  read_joystick (js1, js1_centre_x, js1_centre_y);
	  nibble_1 = mouse_stick;
	}
#endif
    }
  else
    {
      if (keypad_mode == 2)
	nibble_0 = keypad_stick;
      else if (keypad_mode == 3)
	nibble_1 = keypad_stick;

      if (mouse_mode == 2)
	{
	  mouse_joystick (mouse_mode);
	  nibble_0 = mouse_stick;
	}
      else if (mouse_mode == 3)
	{
	  mouse_joystick (mouse_mode);
	  nibble_1 = mouse_stick;
	}

#ifdef LINUX_JOYSTICK
      if (js0_mode == 2)
	{
	  read_joystick (js0, js0_centre_x, js0_centre_y);
	  nibble_0 = mouse_stick;
	}
      else if (js0_mode == 3)
	{
	  read_joystick (js0, js0_centre_x, js0_centre_y);
	  nibble_1 = mouse_stick;
	}

      if (js1_mode == 2)
	{
	  read_joystick (js1, js1_centre_x, js1_centre_y);
	  nibble_0 = mouse_stick;
	}
      else if (js1_mode == 3)
	{
	  read_joystick (js1, js1_centre_x, js1_centre_y);
	  nibble_1 = mouse_stick;
	}
#endif
    }

  return (nibble_1 << 4) | nibble_0;
}

int Atari_TRIG (int num)
{
  int	trig = 1;	/* Trigger not pressed */

  if (num == keypad_mode)
    {
      trig = keypad_trig;
    }

  if (num == mouse_mode)
    {
      Window	root_return;
      Window	child_return;
      int	root_x_return;
      int	root_y_return;
      int	win_x_return;
      int	win_y_return;
      int	mask_return;

      if (XQueryPointer (display, window, &root_return, &child_return,
			 &root_x_return, &root_y_return,
			 &win_x_return, &win_y_return,
			 &mask_return))
	{
	  if (mask_return)
	    trig = 0;
	}
    }

#ifdef LINUX_JOYSTICK
  if (num == js0_mode)
    {
      int status;

      status = read (js0, &js_data, JS_RETURN);
      if (status != JS_RETURN)
	{
	  perror ("/dev/js0");
	  exit (1);
	}

      if (js_data.buttons & 0x01)
	trig = 0;
      else
	trig = 1;

      if (js_data.buttons & 0x02)
	xview_keycode = AKEY_SPACE;
    }

  if (num == js1_mode)
    {
      int status;

      status = read (js1, &js_data, JS_RETURN);
      if (status != JS_RETURN)
	{
	  perror ("/dev/js1");
	  exit (1);
	}

      trig = (js_data.buttons & 0x0f) ? 0 : 1;
    }
#endif

  return trig;
}

int Atari_POT (int num)
{
  int	pot;

  if (num == (mouse_mode - 4))
    {
      Window root_return;
      Window child_return;
      int root_x_return;
      int root_y_return;
      int win_x_return;
      int win_y_return;
      int mask_return;

      if (XQueryPointer (display, window, &root_return,
			 &child_return, &root_x_return, &root_y_return,
			 &win_x_return, &win_y_return, &mask_return))
	{
	  switch (windowsize)
	  {
	  case Small:
	      pot = ((float)((ATARI_WIDTH) - win_x_return) /
		     (float)(ATARI_WIDTH)) * 228;
	      break;
	  case Large:
	      pot = ((float)((ATARI_WIDTH * 2) - win_x_return) /
		     (float)(ATARI_WIDTH * 2)) * 228;
	      break;
	  default:
	      pot = ((float)((ATARI_WIDTH * 3) - win_x_return) /
		     (float)(ATARI_WIDTH * 3)) * 228;
	      break;
          }
        }
      else
        {
	  pot = 228;
	}
    }
  else
    {
      pot = 228;
    }

  return pot;
}

int Atari_CONSOL (void)
{
  int temp;

  if (menu_consol != 7)
    {
      temp = menu_consol;
      menu_consol = 0x07;
    }
  else
    {
      temp = keyboard_consol;
    }

  return temp;
}
/*
 *
 * All Input is assumed to be going to RAM
 * All Output is assumed to be coming from either RAM or ROM
 *
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef VMS
#include <unixio.h>
#include <file.h>
#else
#include <fcntl.h>
#ifndef AMIGA
#include <unistd.h>
#endif
#endif

#ifdef DJGPP
#include "djgpp.h"
#endif

static char *rcsid = "$Id: sio.c,v 1.7 1997/03/30 19:36:19 david Exp $";

#define FALSE   0
#define TRUE    1

#include "atari.h"
#include "cpu.h"
#include "sio.h"

#define	MAGIC1	0x96
#define	MAGIC2	0x02

struct ATR_Header
{
  unsigned char	magic1;
  unsigned char	magic2;
  unsigned char	seccountlo;
  unsigned char	seccounthi;
  unsigned char	secsizelo;
  unsigned char	secsizehi;
  unsigned char	hiseccountlo;
  unsigned char	hiseccounthi;
  unsigned char	gash[8];
};

typedef enum Format { XFD, ATR } Format;

static Format	format[MAX_DRIVES];
static int	disk[MAX_DRIVES] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static int	sectorcount[MAX_DRIVES];
static int	sectorsize[MAX_DRIVES];

static enum DriveStatus
{
  Off,
  NoDisk,
  ReadOnly,
  ReadWrite
} drive_status[MAX_DRIVES];

char sio_status[256];
char sio_filename[MAX_DRIVES][FILENAME_LEN];

void SIO_Initialise (int *argc, char *argv[])
{
  int i;

  for (i=0;i<MAX_DRIVES;i++)
    strcpy (sio_filename[i], "Empty");
}

int SIO_Mount (int diskno, char *filename)
{
  struct ATR_Header	header;

  int	fd;

  drive_status[diskno-1] = ReadWrite;
  strcpy (sio_filename[diskno-1], "Empty");

  fd = open (filename, O_RDWR, 0777);
  if (fd == -1)
    {
      fd = open (filename, O_RDONLY, 0777);
      drive_status[diskno-1] = ReadOnly;
    }

  if (fd)
    {
      int	status;

      status = read (fd, &header, sizeof(struct ATR_Header));
      if (status == -1)
	{
	  close (fd);
	  disk[diskno-1] = -1;
	  return FALSE;
	}

      strcpy (sio_filename[diskno-1], filename);

      if ((header.magic1 == MAGIC1) && (header.magic2 == MAGIC2))
	{
	  sectorcount[diskno-1] = header.hiseccounthi << 24 |
	    header.hiseccountlo << 16 |
	      header.seccounthi << 8 |
		header.seccountlo;

	  sectorsize[diskno-1] = header.secsizehi << 8 |
	    header.secsizelo;

	  sectorcount[diskno-1] /= 8;
	  if (sectorsize[diskno-1] == 256)
	    {
	      sectorcount[diskno-1] += 3; /* Compensate for first 3 sectors */
	      sectorcount[diskno-1] /= 2;
	    }

#ifdef DEBUG
	  printf ("ATR: sectorcount = %d, sectorsize = %d\n",
		  sectorcount[diskno-1],
		  sectorsize[diskno-1]);
#endif

	  format[diskno-1] = ATR;
	}
      else
	{
	  format[diskno-1] = XFD;
	}
    }
  else
    {
      drive_status[diskno-1] = NoDisk;
    }

  disk[diskno-1] = fd;

  return (disk[diskno-1] != -1) ? TRUE : FALSE;
}

void SIO_Dismount (int diskno)
{
  if (disk[diskno-1] != -1)
    {
      close (disk[diskno-1]);
      disk[diskno-1] = -1;
      drive_status[diskno-1] = NoDisk;
      strcpy (sio_filename[diskno-1], "Empty");
    }
}

void SIO_DisableDrive (int diskno)
{
  drive_status[diskno-1] = Off;
  strcpy (sio_filename[diskno-1], "Off");
}

void SeekSector (int dskno, int sector)
{
  int	offset;

  sprintf (sio_status, "%d: %d", dskno+1, sector);

  switch (format[dskno])
    {
    case XFD :
      offset = (sector-1)*128;
      break;
    case ATR :
      if (sector < 4)
	offset = (sector-1) * 128 + 16;
      else
	offset = (sector - 1) * sectorsize[dskno] + 16;
/*
	offset = 3*128 + (sector-4) * sectorsize[dskno] + 16;
*/
      break;
    default :
      printf ("Fatal Error in atari_sio.c\n");
      Atari800_Exit (FALSE);
      exit (1);
    }

  lseek (disk[dskno], offset, SEEK_SET);
}

void SIO (void)
{
  /* UBYTE DDEVIC = memory[0x0300]; */
  UBYTE DUNIT = memory[0x0301];
  UBYTE DCOMND = memory[0x0302];
  /* UBYTE DSTATS = memory[0x0303]; */
  UBYTE DBUFLO = memory[0x0304];
  UBYTE DBUFHI = memory[0x0305];
  /* UBYTE DTIMLO = memory[0x0306]; */
  UBYTE DBYTLO = memory[0x0308];
  UBYTE DBYTHI = memory[0x0309];
  UBYTE DAUX1 = memory[0x030a];
  UBYTE DAUX2 = memory[0x030b];

  int	sector;
  int	buffer;
  int	count;
  int	i;

  if (drive_status[DUNIT-1] != Off)
    {
      if (disk[DUNIT-1] != -1)
	{
	  int offset;

	  sector = DAUX1 + DAUX2 * 256;
	  buffer = DBUFLO + DBUFHI * 256;
	  count = DBYTLO + DBYTHI * 256;

	  switch (format[DUNIT-1])
	    {
	    case XFD :
	      offset = (sector-1)*128+0;
	      break;
	    case ATR :
	      if (sector < 4)
		offset = (sector-1) * 128 + 16;
	      else
		offset = (sector - 1) * sectorsize[DUNIT-1] + 16;
/*
   offset = 3*128 + (sector-4) * sectorsize[DUNIT-1] + 16;
*/
	      break;
	    default :
	      printf ("Fatal Error in atari_sio.c\n");
	      Atari800_Exit (FALSE);
	      exit (1);
	    }

	  lseek (disk[DUNIT-1], offset, SEEK_SET);

#ifdef DEBUG
	  printf ("SIO: DCOMND = %x, SECTOR = %d, BUFADR = %x, BUFLEN = %d\n",
		  DCOMND, sector, buffer, count);
#endif

	  switch (DCOMND)
	    {
	    case 0x50 :
	    case 0x57 :
	      if (drive_status[DUNIT-1] == ReadWrite)
		{
		  write (disk[DUNIT-1], &memory[buffer], count);
		  regY = 1;
		  ClrN;
		}
	      else
		{
		  regY = 146;
		  SetN;
		}
	      break;
	    case 0x52 :
	      read (disk[DUNIT-1], &memory[buffer], count);
	      regY = 1;
	      ClrN;
	      break;
	    case 0x21 : /* Single Density Format */
	    case 0x22 : /* Duel Density Format */
	    case 0x66 : /* US Doubler Format - I think! */
	      regY = 1;
	      ClrN;
	      break;
/*
   Status Request from Atari 400/800 Technical Reference Notes

   DVSTAT + 0	Command Status
   DVSTAT + 1	Hardware Status
   DVSTAT + 2	Timeout
   DVSTAT + 3	Unused

   Command Status Bits

   Bit 0 = 1 indicates an invalid command frame was received
   Bit 1 = 1 indicates an invalid data frame was received
   Bit 2 = 1 indicates that a PUT operation was unsuccessful
   Bit 3 = 1 indicates that the diskete is write protected
   Bit 4 = 1 indicates active/standby
   
   plus

   Bit 5 = 1 indicates double density
   Bit 7 = 1 indicates duel density disk (1050 format)
*/
	    case 0x53 :	/* Get Status */
	      for (i=0;i<count;i++)
		{
		  if (sectorsize[DUNIT-1] == 256)
		    memory[buffer+i] = 32 + 16;
		  else
		    memory[buffer+i] = 16;
		}
	      regY = 1;
	      ClrN;
	      break;
	    default :
	      printf ("SIO: DCOMND = %0x\n", DCOMND);
	      regY = 146;
	      SetN;
	      break;
	    }
	}
      else
	{
	  regY = 146;
	  SetN;
	}
    }
  else
    {
      regY = 138;
      SetN;
    }

  memory[0x0303] = regY;
}

static unsigned char cmd_frame[5];
static int ncmd = 0;
static int checksum = 0;

static unsigned char data[256];
static int offst;

static int buffer_offset;
static int buffer_size;

extern int DELAYED_SERIN_IRQ;
extern int DELAYED_SEROUT_IRQ;
extern int DELAYED_XMTDONE_IRQ;

typedef enum
{
  SIO_Normal,
  SIO_Put
} SIO_State;

static SIO_State sio_state = SIO_Normal;

void Command_Frame (void)
{
  sio_state = SIO_Normal;

  switch (cmd_frame[1])
    {
    case 'R' : /* Read */
#ifdef DEBUG
      printf ("Read command\n");
#endif
      {
	int sector;
	int dskno;
	int i;

	dskno = cmd_frame[0] - 0x31;
	sector = cmd_frame[2] + cmd_frame[3] * 256;
#ifdef DEBUG
	printf ("Sector: %d(%x)\n", sector, sector);
#endif
	SeekSector (dskno, sector);

	data[0] = 0x41; /* ACK */
	data[1] = 0x43; /* OPERATION COMPLETE */

	read (disk[dskno], &data[2], 128);
	checksum = 0;
	for (i=2;i<130;i++)
	  {
	    checksum += (unsigned char)data[i];
	    while (checksum > 255)
	      checksum = checksum - 255;
	  }
	data[130] = checksum;

        buffer_offset = 0;
        buffer_size = 131;

	DELAYED_SEROUT_IRQ = 1;
	DELAYED_XMTDONE_IRQ = 3;
	DELAYED_SERIN_IRQ = 150; /* BEFORE 7 */
      }
      break;
    case 'S' : /* Status */
#ifdef DEBUG
      printf ("Status command\n");
#endif
      data[0] = 0x41; /* ACK */
      data[1] = 0x43; /* OPERATION COMPLETE */
      data[2] = 0x10; /* 2ea */
      data[3] = 0x00; /* 2eb */
      data[4] = 0x01; /* 2ec */
      data[5] = 0x00; /* 2ed */
      data[6] = 0x11; /* Checksum */
      buffer_offset = 0;
      buffer_size = 7;

      DELAYED_SEROUT_IRQ = 1;
      DELAYED_XMTDONE_IRQ = 5;
      DELAYED_SERIN_IRQ = 150;
      break;
    case 'W' : /* Write with verify */
    case 'P' : /* Put without verify */
#ifdef DEBUG
      printf ("Put or Write command\n");
#endif
      data[0] = 0x41; /* ACK */
      buffer_offset = 0;
      buffer_size = 1;
      DELAYED_SEROUT_IRQ = 1;
      DELAYED_XMTDONE_IRQ = 3;
      DELAYED_SERIN_IRQ = 150; /* BEFORE 7 */
      sio_state = SIO_Put;
      break;
    case '!' : /* Format */
      printf ("Format command\n");
      break;
    case 'T' : /* Read Address */
      printf ("Read Address command\n");
      break;
    case 'Q' : /* Read Spin */
      printf ("Read Spin command\n");
      break;
    case 'U' : /* Motor On */
      printf ("Motor On command\n");
      break;
    case 'V' : /* Verify Sector */
      printf ("Verify Sector\n");
      break;
    default :
      printf ("Unknown command: %02x\n", cmd_frame[1]);
      printf ("Command frame: %02x %02x %02x %02x %02x\n",
	      cmd_frame[0], cmd_frame[1], cmd_frame[2],
	      cmd_frame[3], cmd_frame[4]);
      buffer_offset = 0;
      buffer_size = 0;
      DELAYED_XMTDONE_IRQ = 3;
      break;
  }
}

void SIO_SEROUT (unsigned char byte, int cmd)
{
  checksum += (unsigned char)byte;
  while (checksum > 255)
    checksum = checksum - 255;

#ifdef DEBUG
  printf ("SIO_SEROUT: byte = %x, checksum = %x, cmd = %d\n",
	  byte, checksum, cmd);
#endif

  if (cmd)
    {
      cmd_frame[ncmd++] = byte;
      if (ncmd == 5)
	{
	  Command_Frame ();

	  offst = 0;
	  checksum = 0;
	  ncmd = 0;
	}
      else
	{
	  DELAYED_SEROUT_IRQ = 1;
	}

      if (cmd_frame[0] == 0)
	ncmd = 0;
    }
  else if (sio_state == SIO_Put)
    {
      data[buffer_offset++] = byte;
      if (buffer_offset == 130)
	{
	  int sector;
	  int dskno;
	  int i;

	  checksum = 0;

	  for (i=1;i<129;i++)
	    {
	      checksum += (unsigned char)data[i];
	      while (checksum > 255)
		checksum = checksum - 255;
	    }

	  if (checksum != data[129])
	    {
	      printf ("Direct SIO Write Error\n");
	      printf ("Calculated Checksum = %x\n", checksum);
	      printf ("Actual Checksum = %x\n", data[129]);
	      exit (1);
	    }

	  dskno = cmd_frame[0] - 0x31;
	  sector = cmd_frame[2] + cmd_frame[3] * 256;

#ifdef DEBUG
	  printf ("Sector: %d(%x)\n", sector, sector);
#endif

	  SeekSector (dskno, sector);

	  write (disk[dskno], &data[1], 128);
	  data[buffer_offset] = 0x41; /* ACK */
	  data[buffer_offset+1] = 0x43; /* OPERATION COMPLETE */
	  buffer_size = buffer_offset + 2;
	  DELAYED_SEROUT_IRQ = 1;
	  DELAYED_XMTDONE_IRQ = 3;
	  DELAYED_SERIN_IRQ = 7;
	  DELAYED_XMTDONE_IRQ = 5;
	  DELAYED_SERIN_IRQ = 150;
	}
      else
	{
	  DELAYED_SEROUT_IRQ = 4;
	}
    }
  else
    {
      DELAYED_SEROUT_IRQ = 1;
      ncmd = 0;
    }
}

int SIO_SERIN (void)
{
  int byte;

  if (buffer_offset < buffer_size)
    {
      byte = (int)data[buffer_offset++];

#ifdef DEBUG
      printf ("SERIN: byte = %x\n", byte);
#endif

      if (buffer_offset < buffer_size)
	{
#ifdef DEBUG
	  printf ("Setting SERIN Interrupt again\n");
#endif
	  DELAYED_SERIN_IRQ = 3;
	  DELAYED_SERIN_IRQ = 4;
	}
    }

  return byte;
}
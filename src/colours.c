#include <stdio.h>
#include <math.h>
#include "atari.h"

#define FALSE	0
#define TRUE	1

#define COLINTENS	80

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

static int palette_loaded = FALSE;
static int min_y = 0, max_y = 0xe0;
static int colintens = COLINTENS;
static int colshift = 40;

int colortable[256];

#ifdef COMPILED_PALETTE
static int old_pal[256] =
{
	0x0, 0x1c1c1c, 0x393939, 0x595959,
	0x797979, 0x929292, 0xababab, 0xbcbcbc,
	0xcdcdcd, 0xd9d9d9, 0xe6e6e6, 0xececec,
	0xf2f2f2, 0xf8f8f8, 0xffffff, 0xffffff,
	0x391701, 0x5e2304, 0x833008, 0xa54716,
	0xc85f24, 0xe37820, 0xff911d, 0xffab1d,
	0xffc51d, 0xffce34, 0xffd84c, 0xffe651,
	0xfff456, 0xfff977, 0xffff98, 0xffff98,
	0x451904, 0x721e11, 0x9f241e, 0xb33a20,
	0xc85122, 0xe36920, 0xff811e, 0xff8c25,
	0xff982c, 0xffae38, 0xffc545, 0xffc559,
	0xffc66d, 0xffd587, 0xffe4a1, 0xffe4a1,
	0x4a1704, 0x7e1a0d, 0xb21d17, 0xc82119,
	0xdf251c, 0xec3b38, 0xfa5255, 0xfc6161,
	0xff706e, 0xff7f7e, 0xff8f8f, 0xff9d9e,
	0xffabad, 0xffb9bd, 0xffc7ce, 0xffc7ce,
	0x50568, 0x3b136d, 0x712272, 0x8b2a8c,
	0xa532a6, 0xb938ba, 0xcd3ecf, 0xdb47dd,
	0xea51eb, 0xf45ff5, 0xfe6dff, 0xfe7afd,
	0xff87fb, 0xff95fd, 0xffa4ff, 0xffa4ff,
	0x280479, 0x400984, 0x590f90, 0x70249d,
	0x8839aa, 0xa441c3, 0xc04adc, 0xd054ed,
	0xe05eff, 0xe96dff, 0xf27cff, 0xf88aff,
	0xff98ff, 0xfea1ff, 0xfeabff, 0xfeabff,
	0x35088a, 0x420aad, 0x500cd0, 0x6428d0,
	0x7945d0, 0x8d4bd4, 0xa251d9, 0xb058ec,
	0xbe60ff, 0xc56bff, 0xcc77ff, 0xd183ff,
	0xd790ff, 0xdb9dff, 0xdfaaff, 0xdfaaff,
	0x51e81, 0x626a5, 0x82fca, 0x263dd4,
	0x444cde, 0x4f5aee, 0x5a68ff, 0x6575ff,
	0x7183ff, 0x8091ff, 0x90a0ff, 0x97a9ff,
	0x9fb2ff, 0xafbeff, 0xc0cbff, 0xc0cbff,
	0xc048b, 0x2218a0, 0x382db5, 0x483ec7,
	0x584fda, 0x6159ec, 0x6b64ff, 0x7a74ff,
	0x8a84ff, 0x918eff, 0x9998ff, 0xa5a3ff,
	0xb1aeff, 0xb8b8ff, 0xc0c2ff, 0xc0c2ff,
	0x1d295a, 0x1d3876, 0x1d4892, 0x1c5cac,
	0x1c71c6, 0x3286cf, 0x489bd9, 0x4ea8ec,
	0x55b6ff, 0x70c7ff, 0x8cd8ff, 0x93dbff,
	0x9bdfff, 0xafe4ff, 0xc3e9ff, 0xc3e9ff,
	0x2f4302, 0x395202, 0x446103, 0x417a12,
	0x3e9421, 0x4a9f2e, 0x57ab3b, 0x5cbd55,
	0x61d070, 0x69e27a, 0x72f584, 0x7cfa8d,
	0x87ff97, 0x9affa6, 0xadffb6, 0xadffb6,
	0xa4108, 0xd540a, 0x10680d, 0x137d0f,
	0x169212, 0x19a514, 0x1cb917, 0x1ec919,
	0x21d91b, 0x47e42d, 0x6ef040, 0x78f74d,
	0x83ff5b, 0x9aff7a, 0xb2ff9a, 0xb2ff9a,
	0x4410b, 0x5530e, 0x66611, 0x77714,
	0x88817, 0x99b1a, 0xbaf1d, 0x48c41f,
	0x86d922, 0x8fe924, 0x99f927, 0xa8fc41,
	0xb7ff5b, 0xc9ff6e, 0xdcff81, 0xdcff81,
	0x2350f, 0x73f15, 0xc4a1c, 0x2d5f1e,
	0x4f7420, 0x598324, 0x649228, 0x82a12e,
	0xa1b034, 0xa9c13a, 0xb2d241, 0xc4d945,
	0xd6e149, 0xe4f04e, 0xf2ff53, 0xf2ff53,
	0x263001, 0x243803, 0x234005, 0x51541b,
	0x806931, 0x978135, 0xaf993a, 0xc2a73e,
	0xd5b543, 0xdbc03d, 0xe1cb38, 0xe2d836,
	0xe3e534, 0xeff258, 0xfbff7d, 0xfbff7d,
	0x401a02, 0x581f05, 0x702408, 0x8d3a13,
	0xab511f, 0xb56427, 0xbf7730, 0xd0853a,
	0xe19344, 0xeda04e, 0xf9ad58, 0xfcb75c,
	0xffc160, 0xffc671, 0xffcb83, 0xffcb83,
};
static int real_pal[256] =
{
  0x323132, 0x3f3e3f, 0x4d4c4d, 0x5b5b5b,
  0x6a696a, 0x797879, 0x888788, 0x979797,
  0xa1a0a1, 0xafafaf, 0xbebebe, 0xcecdce,
  0xdbdbdb, 0xebeaeb, 0xfafafa, 0xffffff,
  0x612e00, 0x6c3b00, 0x7a4a00, 0x885800,
  0x94670c, 0xa5761b, 0xb2842a, 0xc1943a,
  0xca9d43, 0xdaad53, 0xe8bb62, 0xf8cb72,
  0xffd87f, 0xffe88f, 0xfff79f, 0xffffae,
  0x6c2400, 0x773000, 0x844003, 0x924e11,
  0x9e5d22, 0xaf6c31, 0xbc7b41, 0xcc8a50,
  0xd5935b, 0xe4a369, 0xf2b179, 0xffc289,
  0xffcf97, 0xffdfa6, 0xffedb5, 0xfffdc4,
  0x751618, 0x812324, 0x8f3134, 0x9d4043,
  0xaa4e50, 0xb85e60, 0xc66d6f, 0xd57d7f,
  0xde8787, 0xed9596, 0xfca4a5, 0xffb4b5,
  0xffc2c4, 0xffd1d3, 0xffe0e1, 0xffeff0,
  0x620e71, 0x6e1b7c, 0x7b2a8a, 0x8a3998,
  0x9647a5, 0xa557b5, 0xb365c3, 0xc375d1,
  0xcd7eda, 0xdc8de9, 0xea97f7, 0xf9acff,
  0xffbaff, 0xffc9ff, 0xffd9ff, 0xffe8ff,
  0x560f87, 0x611d90, 0x712c9e, 0x7f3aac,
  0x8d48ba, 0x9b58c7, 0xa967d5, 0xb877e5,
  0xc280ed, 0xd090fc, 0xdf9fff, 0xeeafff,
  0xfcbdff, 0xffccff, 0xffdbff, 0xffeaff,
  0x461695, 0x5122a0, 0x6032ac, 0x6e41bb,
  0x7c4fc8, 0x8a5ed6, 0x996de3, 0xa87cf2,
  0xb185fb, 0xc095ff, 0xcfa3ff, 0xdfb3ff,
  0xeec1ff, 0xfcd0ff, 0xffdfff, 0xffefff,
  0x212994, 0x2d359f, 0x3d44ad, 0x4b53ba,
  0x5961c7, 0x686fd5, 0x777ee2, 0x878ef2,
  0x9097fa, 0x96a6ff, 0xaeb5ff, 0xbfc4ff,
  0xcdd2ff, 0xdae3ff, 0xeaf1ff, 0xfafeff,
  0x0f3584, 0x1c418d, 0x2c509b, 0x3a5eaa,
  0x486cb7, 0x587bc5, 0x678ad2, 0x7699e2,
  0x80a2eb, 0x8fb2f9, 0x9ec0ff, 0xadd0ff,
  0xbdddff, 0xcbecff, 0xdbfcff, 0xeaffff,
  0x043f70, 0x114b79, 0x215988, 0x2f6896,
  0x3e75a4, 0x4d83b2, 0x5c92c1, 0x6ca1d2,
  0x74abd9, 0x83bae7, 0x93c9f6, 0xa2d8ff,
  0xb1e6ff, 0xc0f5ff, 0xd0ffff, 0xdeffff,
  0x005918, 0x006526, 0x0f7235, 0x1d8144,
  0x2c8e50, 0x3b9d60, 0x4aac6f, 0x59bb7e,
  0x63c487, 0x72d396, 0x82e2a5, 0x92f1b5,
  0x9ffec3, 0xaeffd2, 0xbeffe2, 0xcefff1,
  0x075c00, 0x146800, 0x227500, 0x328300,
  0x3f910b, 0x4fa01b, 0x5eae2a, 0x6ebd3b,
  0x77c644, 0x87d553, 0x96e363, 0xa7f373,
  0xb3fe80, 0xc3ff8f, 0xd3ffa0, 0xe3ffb0,
  0x1a5600, 0x286200, 0x367000, 0x457e00,
  0x538c00, 0x629b07, 0x70a916, 0x80b926,
  0x89c22f, 0x99d13e, 0xa8df4d, 0xb7ef5c,
  0xc5fc6b, 0xd5ff7b, 0xe3ff8b, 0xf3ff99,
  0x334b00, 0x405700, 0x4d6500, 0x5d7300,
  0x6a8200, 0x7a9100, 0x889e0f, 0x98ae1f,
  0xa1b728, 0xbac638, 0xbfd548, 0xcee458,
  0xdcf266, 0xebff75, 0xfaff85, 0xffff95,
  0x4b3c00, 0x584900, 0x655700, 0x746500,
  0x817400, 0x908307, 0x9f9116, 0xaea126,
  0xb7aa2e, 0xc7ba3e, 0xd5c74d, 0xe5d75d,
  0xf2e56b, 0xfef47a, 0xffff8b, 0xffff9a,
  0x602e00, 0x6d3a00, 0x7a4900, 0x895800,
  0x95670a, 0xa4761b, 0xb2832a, 0xc2943a,
  0xcb9d44, 0xdaac53, 0xe8ba62, 0xf8cb73,
  0xffd77f, 0xffe791, 0xfff69f, 0xffffaf,
};
static int fox_pal[256] =
{
  0x000000, 0x101010, 0x1c1c1c, 0x2c2c2c,
  0x3c3c3c, 0x4c4c4c, 0x585858, 0x686868,
  0x787878, 0x888888, 0x949494, 0xa4a4a4,
  0xb4b4b4, 0xc4c4c4, 0xd0d0d0, 0xe0e0e0,
  0x582000, 0x642c08, 0x6c3410, 0x784018,
  0x804c24, 0x8c582c, 0x986034, 0xa06c3c,
  0xac7844, 0xb4844c, 0xc08c54, 0xcc985c,
  0xd4a468, 0xe0b070, 0xe8b878, 0xf4c480,
  0x540000, 0x600c0c, 0x6c1414, 0x742020,
  0x802c2c, 0x8c3434, 0x984040, 0xa44c4c,
  0xac5454, 0xb86060, 0xc46c6c, 0xd07474,
  0xdc8080, 0xe48c8c, 0xf09494, 0xfca0a0,
  0x580808, 0x641414, 0x6c1c1c, 0x782828,
  0x843434, 0x903c3c, 0x984848, 0xa45454,
  0xb05c5c, 0xbc6868, 0xc47474, 0xd07c7c,
  0xdc8888, 0xe89494, 0xf09c9c, 0xfca8a8,
  0x600034, 0x6c0c3c, 0x741444, 0x80204c,
  0x882c58, 0x943460, 0xa04068, 0xa84c70,
  0xb45478, 0xbc6080, 0xc86c88, 0xd47490,
  0xdc809c, 0xe88ca4, 0xf094ac, 0xfca0b4,
  0x500050, 0x5c0c5c, 0x681468, 0x742074,
  0x7c2c7c, 0x883488, 0x944094, 0xa04ca0,
  0xac54ac, 0xb860b8, 0xc46cc4, 0xd074d0,
  0xd880d8, 0xe48ce4, 0xf094f0, 0xfca0fc,
  0x340060, 0x400c6c, 0x4c1874, 0x582480,
  0x603088, 0x6c3c94, 0x7848a0, 0x8454a8,
  0x905cb4, 0x9c68bc, 0xa874c8, 0xb480d4,
  0xbc8cdc, 0xc898e8, 0xd4a4f0, 0xe0b0fc,
  0x000068, 0x0c0c70, 0x18187c, 0x202884,
  0x2c3490, 0x384098, 0x444ca4, 0x5058ac,
  0x5868b8, 0x6474c0, 0x7080cc, 0x7c8cd4,
  0x8898e0, 0x90a8e8, 0x9cb4f4, 0xa8c0fc,
  0x08205c, 0x142c68, 0x203870, 0x28407c,
  0x344c88, 0x405890, 0x4c649c, 0x5870a8,
  0x6078b0, 0x6c84bc, 0x7890c8, 0x849cd0,
  0x90a8dc, 0x98b0e8, 0xa4bcf0, 0xb0c8fc,
  0x083858, 0x144464, 0x20506c, 0x2c5c78,
  0x386484, 0x447090, 0x507c98, 0x5c88a4,
  0x6894b0, 0x74a0bc, 0x80acc4, 0x8cb8d0,
  0x98c0dc, 0xa4cce8, 0xb0d8f0, 0xbce4fc,
  0x004828, 0x0c5434, 0x185c3c, 0x246848,
  0x307450, 0x3c7c5c, 0x488868, 0x549470,
  0x649c7c, 0x70a884, 0x7cb490, 0x88bc9c,
  0x94c8a4, 0xa0d4b0, 0xacdcb8, 0xb8e8c4,
  0x005000, 0x0c5c0c, 0x186418, 0x247024,
  0x307830, 0x3c843c, 0x489048, 0x549854,
  0x5ca45c, 0x68ac68, 0x74b874, 0x80c480,
  0x8ccc8c, 0x98d898, 0xa4e0a4, 0xb0ecb0,
  0x1c4c00, 0x285808, 0x346010, 0x406c18,
  0x487824, 0x54842c, 0x608c34, 0x6c983c,
  0x78a444, 0x84b04c, 0x90b854, 0x9cc45c,
  0xa4d068, 0xb0dc70, 0xbce478, 0xc8f080,
  0x384000, 0x444c08, 0x4c5814, 0x58641c,
  0x647024, 0x707c2c, 0x788838, 0x849440,
  0x909c48, 0x9ca850, 0xa4b45c, 0xb0c064,
  0xbccc6c, 0xc8d874, 0xd0e480, 0xdcf088,
  0x502800, 0x5c3408, 0x64400c, 0x705014,
  0x785c1c, 0x846824, 0x8c7428, 0x988030,
  0xa09038, 0xac9c40, 0xb4a844, 0xc0b44c,
  0xc8c054, 0xd4d05c, 0xdcdc60, 0xe8e868,
  0x541c00, 0x602808, 0x6c3014, 0x743c1c,
  0x804828, 0x8c5430, 0x985c38, 0xa46844,
  0xac744c, 0xb88058, 0xc48860, 0xd09468,
  0xdca074, 0xe4ac7c, 0xf0b488, 0xfcc090,
};
#endif

void Palette_Initialise(int *argc, char *argv[])
{
  int i, j;
  UBYTE rgb[0x100][3];

  for (i = j = 1; i < *argc; i++)
    {
      if (strcmp(argv[i], "-black") == 0)
	sscanf(argv[++i], "%d", &min_y);
      else if (strcmp(argv[i], "-white") == 0)
	sscanf(argv[++i], "%d", &max_y);
      else if (strcmp(argv[i], "-colors") == 0)
	sscanf(argv[++i], "%d", &colintens);
      else if (strcmp(argv[i], "-colshift") == 0)
	sscanf(argv[++i], "%d", &colshift);
#ifdef COMPILED_PALETTE
      else if (strcmp(argv[i], "-realpal") == 0)
	{
	  memcpy(colortable, real_pal, sizeof(colortable));
	  palette_loaded = TRUE;
	}
      else if (strcmp(argv[i], "-oldpal") == 0)
	{
	  memcpy(colortable, old_pal, sizeof(colortable));
	  palette_loaded = TRUE;
	}
      else if (strcmp(argv[i], "-foxpal") == 0)
	{
	  memcpy(colortable, fox_pal, sizeof(colortable));
	  palette_loaded = TRUE;
	}
#endif
      else
	if (strcmp(argv[i], "-help") == 0)
	{
	  Aprint("\t-black <0-255>   set black level");
	  Aprint("\t-white <0-255>   set white level");
	  Aprint("\t-colors <num>    set color intensity");
	  Aprint("\t-colshift <num>  set color shift");
#ifdef COMPILED_PALETTE
	  Aprint("\t-realpal <num>   use real palette");
	  Aprint("\t-oldpal <num>    use old palette");
	  Aprint("\t-foxpal <num>    use Fox's palette");
#endif
	}
	argv[j++] = argv[i];
    }
  *argc = j;

  if (!palette_loaded)
    /* generate a fresh palette */
    {
      for (i = 0; i < 0x10; i++)
	{
	  int r, b;
	  double angle;

	  if (i == 0)
	    {
	      r = b = 0;
	    }
	  else
	    {
	      angle = M_PI * ((double) i * (1.0 / 7) - (double) colshift * 0.01);
	      r = cos(angle) * (double) colintens;
	      b = cos(angle - M_PI * (2.0 / 3)) * (double) colintens;
	    }
	  for (j = 0; j < 0x10; j++)
	    {
	      int y, r1, g1, b1;

	      y = (max_y * j + min_y * (0xf - j)) / 0xf;
	      r1 = y + r;
	      g1 = y - r - b;
	      b1 = y + b;
#define CLIP_VAR(x) \
  if (x > 0xff) \
    x = 0xff; \
  if (x < 0) \
    x = 0;
	      CLIP_VAR(r1)
	      CLIP_VAR(g1)
	      CLIP_VAR(b1)
	      rgb[i * 16 + j][0] = r1;
	      rgb[i * 16 + j][1] = g1;
	      rgb[i * 16 + j][2] = b1;
	    }
	}
    }
  else
    /* format loaded palette */
    {
      float max_y2, min_y2;

      for (i = 0; i < 0x100; i++)
      {
	j = colortable[i];
	rgb[i][0] = (j >> 16) & 0xff;
	rgb[i][1] = (j >> 8) & 0xff;
	rgb[i][2] = j & 0xff;
      }
      min_y2 = (float)(rgb[0][0] + rgb[0][1] + rgb[0][2]) * 0.3;
      max_y2 = (float)(rgb[15][0] + rgb[15][1] + rgb[15][2]) * 0.3;
      for (i = 0; i < 0x10; i++)
	{
	  for (j = 0; j < 0x10; j++)
	    {
	      float y, r, b;
	      int r1, g1, b1;

	      y = (float)(rgb[i * 16 + j][0]
		+ rgb[i * 16 + j][1]
		+ rgb[i * 16 + j][2]) * (1.0 / 3);
	      r = (float)rgb[i * 16 + j][0] - y;
	      b = (float)rgb[i * 16 + j][2] - y;
	      y = ((y - min_y2) * ((float)max_y / max_y2)) + min_y;
	      r *= (float)colintens / (float)COLINTENS;
	      b *= (float)colintens / (float)COLINTENS;
	      r1 = y + r;
	      g1 = y - r - b;
	      b1 = y + b;
	      CLIP_VAR(r1)
	      CLIP_VAR(g1)
	      CLIP_VAR(b1)
	      rgb[i * 16 + j][0] = r1;
	      rgb[i * 16 + j][1] = g1;
	      rgb[i * 16 + j][2] = b1;
	    }
	}
    }
  for (i = 0; i < 0x100; i++)
  {
    colortable[i] = (rgb[i][0] << 16)
		  + (rgb[i][1] << 8)
		  + (rgb[i][2] << 0);
  }
}

/* returns TRUE if successful */
int read_palette(char *filename) {
	FILE *fp;
	int i;
	if ((fp = fopen(filename,"rb")) == NULL)
		return FALSE;
	for (i = 0; i < 256; i++) {
		int j;
		colortable[i] = 0;
		for (j = 16; j >= 0; j -= 8) {
			int c = fgetc(fp);
			if (c == EOF) {
				fclose(fp);
				return FALSE;
			}
			colortable[i] |= c << j;
		}
	}
	fclose(fp);
	palette_loaded = TRUE;
	return TRUE;
}

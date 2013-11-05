/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1999 by Philippe Bekaert
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that this copyright
 * notice remain intact.
 *
 */

/*
 * genemf.c -- convert fig to Enhanced MetaFile
 *
 * Written by Michael Schrick (2001-03-04)
 *
 * Revision History:
 *
 *   2004/02/10 - Added support for locale text (if iconv() is available),
 *		  arc box, open arc, rotated ellipse, picture,
 *		  dash-triple-dotted line style, and all fill patterns.
 *		  (ITOH Yasufumi)
 *
 *   2003/01/14 - Added text orientation, text alignment.  Corrected color
 *		  table and added text color.  Added transparency, fill
 *		  colors and patterns. (M. Schrick)
 *
 *   2001/10/03 - Added htof functions for big endian machines (M. Schrick)
 *
 *   2001/09/24 - Added filled polygons, circles and ellipses (M. Schrick)
 *
 *   2001/03/04 - Created from gencgm.c (M. Schrick)
 *
 * Limitations:
 *
 * - old style splines are not supported by this driver. New style
 *   (X) splines are automatically converted to polylines and thus
 *   are supported.
 *
 * - Windows 95 supports linestyles only for the thin line.  Lines with
 *   styles will converted to the thin width.  Dash-triple-dotted linestyle
 *   is not supported.
 *   Windows 98/Me don't seem to support linestyles well.
 *
 * - Windows 95/98/Me supports only 12 patterns out of FIG's 22 fill pattern.
 *   Unsupported patterns will be shown differently.
 *
 * - Windows 95/98/Me doesn't support line cap and join styles. The correct
 *   appearance of arrows depends on the cap style the viewer uses (not
 *   known at conversion time, but can be set using the -r driver option).
 *
 * - an EMF file may look quite different when viewed with different
 *   EMF capable drawing programs. This is especially so for text:
 *   the text font e.g. needs to be recognized and supported by the
 *   viewer.  Same is True for special characters in text strings, etc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#if defined(I18N) && defined(USE_ICONV)
#include <iconv.h>
#endif
#include "fig2dev.h"
#include "object.h"
#include "genemf.h"

#define UNDEFVALUE	-100	/* UNDEFined attribute value */
#define EPSILON		1e-4	/* small floating point value */

typedef enum {EMH_RECORD, EMH_DATA} emh_flag;
static ENHMETAHEADER emh;	/* The Enhanced MetaFile header, which is the
				 * first record in the file must be updated
				 * to reflect the total number of records and
				 * the total number of bytes.  This record
				 * contains little endian data and should not
				 * be used internally without a conversion. */
/* Internal variables in host format */
static unsigned emh_nBytes;	/* Size of the metafile in bytes */
static unsigned emh_nRecords;	/* Number of records in the metafile */
static unsigned emh_nHandles;	/* Number of handles in the handle table */

/*
 * Limit maximum number of handles.
 * XXX limit to which number?
 */
static unsigned maxHandles = 50;

/* Data types for keeping track of the device context handles */
enum emfhandletype { EMFH_PEN, EMFH_BRUSH, EMFH_FONT, EMFH_MAX } type;

static unsigned lasthandle[EMFH_MAX];	/* Last device handle */

struct emfhandle {
    struct emfhandle *next;	/* This field must be first */
    struct emfhandle **prev;
    enum emfhandletype type;
    Boolean	is_current;	/* True: currently selected */
    unsigned	handle;
    union {
	struct pen {
	    int pstyle, pwidth, prgb;
	} p;
	struct brush {
	    int bstyle;
	    int brgb, bpattern, bhatchbkrgb;
	} b;
	struct font {
	    int ffont;
	    int fsize;
	    int fangle;
	} f;
    } eh_un;
};

static struct emfhandle *handles;
static struct emfhandle *latesthandle;


static int rounded_arrows;	/* If rounded_arrows is False, the position
				 * of arrows will be corrected for
				 * compensating line width effects. This
				 * correction is not needed if arrows appear
				 * rounded with the used EMF viewer.
				 * See -r driver command line option. */

/* EMF compatibility level */
static enum emfcompatlevel {
    EMF_LEVEL_WIN95,	/* Windows 95 */
    EMF_LEVEL_WIN98,	/* Windows 98/Me */
    EMF_LEVEL_WINNT	/* Windows NT/2000/XP (Windows NT 3.1 and later) */
} emflevel = EMF_LEVEL_WINNT;
const char *const emflevelname[] = { "win95", "win98", "winnt" };

typedef struct Dir {double x, y;} Dir;
#define ARROW_INDENT_DIST	0.8
#define ARROW_POINT_DIST	1.2
/* Arrows appear this much longer with projected line caps. */
#define ARROW_EXTRA_LEN(a)	((double)a->ht / (double)a->wid * a->thickness)

/* update bounding box */
#define UPDATE_BBX_X(x) \
    do { \
	if (bbx_left>(x)) bbx_left=(x); else if (bbx_right<(x)) bbx_right=(x);\
    } while (0)
#define UPDATE_BBX_Y(y) \
    do { \
	if (bbx_top>(y)) bbx_top=(y); else if (bbx_bottom<(y)) bbx_bottom=(y);\
    } while (0)


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
 * fill pattern bitmaps
 */

/* pat0.xbm */
#define pat0_width 8
#define pat0_height 4
static unsigned char pat0_bits[] = {
   0x03, 0x0c, 0x30, 0xc0};

/* pat1.xbm */
#define pat1_width 8
#define pat1_height 4
static unsigned char pat1_bits[] = {
   0xc0, 0x30, 0x0c, 0x03};

/* pat2.xbm */
#define pat2_width 8
#define pat2_height 4
static unsigned char pat2_bits[] = {
   0x81, 0x66, 0x18, 0x66};

#if 0	/* mapped to EMF pattern */
/* pat3.xbm */
#define pat3_width 8
#define pat3_height 8
static unsigned char pat3_bits[] = {
   0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/* pat4.xbm */
#define pat4_width 8
#define pat4_height 8
static unsigned char pat4_bits[] = {
   0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

/* pat5.xbm */
#define pat5_width 8
#define pat5_height 8
static unsigned char pat5_bits[] = {
   0x11, 0x0a, 0x04, 0x0a, 0x11, 0xa0, 0x40, 0xa0};
#endif

/* pat6.xbm */
#define pat6_width 16
#define pat6_height 16
static unsigned char pat6_bits[] = {
   0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
   0x00, 0x80, 0xff, 0xff, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xff, 0xff};

/* pat7.xbm */
#define pat7_width 16
#define pat7_height 16
static unsigned char pat7_bits[] = {
   0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

#if 0	/* mapped to EMF pattern */
/* pat8.xbm */
#define pat8_width 8
#define pat8_height 4
static unsigned char pat8_bits[] = {
   0xff, 0x00, 0x00, 0x00};

/* pat9.xbm */
#define pat9_width 8
#define pat9_height 4
static unsigned char pat9_bits[] = {
   0x88, 0x88, 0x88, 0x88};

/* pat10.xbm */
#define pat10_width 8
#define pat10_height 4
static unsigned char pat10_bits[] = {
   0xff, 0x88, 0x88, 0x88};
#endif

/* pat11.xbm */
#define pat11_width 24
#define pat11_height 24
static unsigned char pat11_bits[] = {
   0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40,
   0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0xff, 0xff, 0xff,
   0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00,
   0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0x00, 0x00, 0xff, 0xff, 0xff,
   0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00,
   0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0x00, 0xff, 0xff, 0xff};

/* pat12.xbm */
#define pat12_width 24
#define pat12_height 24
static unsigned char pat12_bits[] = {
   0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20,
   0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff,
   0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00,
   0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff, 0xff,
   0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00,
   0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff};

/* pat13.xbm */
#define pat13_width 24
#define pat13_height 24
static unsigned char pat13_bits[] = {
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x80, 0x81, 0x80, 0x80, 0x86, 0x80, 0x80, 0x98, 0x80, 0x80, 0xe0,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x81, 0x80, 0x80, 0x86, 0x80, 0x80, 0x98, 0x80, 0x80, 0xe0, 0x80,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x81, 0x80, 0x80, 0x86, 0x80, 0x80, 0x98, 0x80, 0x80, 0xe0, 0x80, 0x80};

/* pat14.xbm */
#define pat14_width 24
#define pat14_height 24
static unsigned char pat14_bits[] = {
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0xc0, 0x80, 0x80, 0xb0, 0x80, 0x80, 0x8c, 0x80, 0x80, 0x83, 0x80,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x80, 0xc0, 0x80, 0x80, 0xb0, 0x80, 0x80, 0x8c, 0x80, 0x80, 0x83,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0xc0, 0x80, 0x80, 0xb0, 0x80, 0x80, 0x8c, 0x80, 0x80, 0x83, 0x80, 0x80};

/* pat15.xbm */
#define pat15_width 16
#define pat15_height 8
static unsigned char pat15_bits[] = {
   0x40, 0x02, 0x30, 0x0c, 0x0e, 0x70, 0x01, 0x80, 0x02, 0x40, 0x0c, 0x30,
   0x70, 0x0e, 0x80, 0x01};

/* pat16.xbm */
#define pat16_width 8
#define pat16_height 8
static unsigned char pat16_bits[] = {
   0x01, 0x01, 0x82, 0x6c, 0x10, 0x10, 0x28, 0xc6};

/* pat17.xbm */
#define pat17_width 16
#define pat17_height 16
static unsigned char pat17_bits[] = {
   0xe0, 0x0f, 0x18, 0x30, 0x04, 0x40, 0x02, 0x80, 0x02, 0x80, 0x01, 0x00,
   0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
   0x02, 0x80, 0x02, 0x80, 0x04, 0x40, 0x18, 0x30};

/* pat18.xbm */
#define pat18_width 30
#define pat18_height 18
static unsigned char pat18_bits[] = {
   0x08, 0x80, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00,
   0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
   0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0xf8, 0x3f,
   0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02, 0x00, 0x02, 0x00,
   0x02, 0x00, 0x02, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x01, 0x00,
   0x08, 0x80, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00};

/* pat19.xbm */
#define pat19_width 16
#define pat19_height 16
static unsigned char pat19_bits[] = {
   0xe0, 0x0f, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01, 0x00,
   0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
   0x02, 0x80, 0x04, 0x40, 0x08, 0x20, 0x10, 0x10};

/* pat20.xbm */
#define pat20_width 8
#define pat20_height 8
static unsigned char pat20_bits[] = {
   0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x80};

/* pat21.xbm */
#define pat21_width 8
#define pat21_height 8
static unsigned char pat21_bits[] = {
   0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x01};

const struct emf_bmpat {
    unsigned width, height;
    unsigned char *bits;
} emf_bm_pattern[22] = {
    { pat0_width, pat0_height, pat0_bits },
    { pat1_width, pat1_height, pat1_bits },
    { pat2_width, pat2_height, pat2_bits },
    { 0, 0, 0 },	/* mapped to EMF pattern */
    { 0, 0, 0 },	/* mapped to EMF pattern */
    { 0, 0, 0 },	/* mapped to EMF pattern */
    { pat6_width, pat6_height, pat6_bits },
    { pat7_width, pat7_height, pat7_bits },
    { 0, 0, 0 },	/* mapped to EMF pattern */
    { 0, 0, 0 },	/* mapped to EMF pattern */
    { 0, 0, 0 },	/* mapped to EMF pattern */
    { pat11_width, pat11_height, pat11_bits },
    { pat12_width, pat12_height, pat12_bits },
    { pat13_width, pat13_height, pat13_bits },
    { pat14_width, pat14_height, pat14_bits },
    { pat15_width, pat15_height, pat15_bits },
    { pat16_width, pat16_height, pat16_bits },
    { pat17_width, pat17_height, pat17_bits },
    { pat18_width, pat18_height, pat18_bits },
    { pat19_width, pat19_height, pat19_bits },
    { pat20_width, pat20_height, pat20_bits },
    { pat21_width, pat21_height, pat21_bits }
};

/* EMF patterns are numbered 0-5 (I use -1 for bitmap patterns) */
int emf_map_pattern[22] = { -1, -1, -1, HS_FDIAGONAL,
			 HS_BDIAGONAL, HS_DIAGCROSS, -1, -1,
			 HS_HORIZONTAL, HS_VERTICAL, HS_CROSS, -1,
			 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static int ltFonts[] = {	/* Convert from LaTeX fonts to PostScript */
		0,		/* default */
		0,		/* Roman */
		2,		/* Bold */
		1,		/* Italic */
		4,		/* Modern */
		12,		/* Typewriter */
		};

static char *lfFaceName[] = {
		"Times",		/* Times-Roman */
		"Times",		/* Times-Italic */
		"Times",		/* Times-Bold */
		"Times",		/* Times-BoldItalic */
		"AvantGarde",		/* AvantGarde */
		"AvantGarde",		/* AvantGarde-BookOblique */
		"AvantGarde",		/* AvantGarde-Demi */
		"AvantGarde",		/* AvantGarde-DemiOblique */
		"Bookman",		/* Bookman-Light */
		"Bookman",		/* Bookman-LightItalic */
		"Bookman",		/* Bookman-Demi */
		"Bookman",		/* Bookman-DemiItalic */
		"Courier",		/* Courier */
		"Courier",		/* Courier-Oblique */
		"Courier",		/* Courier-Bold */
		"Courier",		/* Courier-BoldItalic */
		"Helvetica",		/* Helvetica */
		"Helvetica",		/* Helvetica-Oblique */
		"Helvetica",		/* Helvetica-Bold */
		"Helvetica",		/* Helvetica-BoldOblique */
		"Helvetica-Narrow",	/* Helvetica-Narrow */
		"Helvetica-Narrow",	/* Helvetica-Narrow-Oblique */
		"Helvetica-Narrow",	/* Helvetica-Narrow-Bold */
		"Helvetica-Narrow",	/* Helvetica-Narrow-BoldOblique */
		"NewCenturySchlbk",	/* NewCenturySchlbk-Roman */
		"NewCenturySchlbk",	/* NewCenturySchlbk-Italic */
		"NewCenturySchlbk",	/* NewCenturySchlbk-Bold */
		"NewCenturySchlbk",	/* NewCenturySchlbk-BoldItalic */
		"Palatino",		/* Palatino-Roman */
		"Palatino",		/* Palatino-Italic */
		"Palatino",		/* Palatino-Bold */
		"Palatino",		/* Palatino-BoldItalic */
		"Symbol",		/* Symbol */
		"ZapfChancery",		/* ZapfChancery-MediumItalic */
		"ZapfDingbats"		/* ZapfDingbats */
		};

static int	lfWeight[] = {
		FW_NORMAL,		/* Times-Roman */
		FW_NORMAL,		/* Times-Italic */
		FW_BOLD,		/* Times-Bold */
		FW_BOLD,		/* Times-BoldItalic */
		FW_NORMAL,		/* AvantGarde */
		FW_NORMAL,		/* AvantGarde-BookOblique */
		FW_DEMIBOLD,		/* AvantGarde-Demi */
		FW_DEMIBOLD,		/* AvantGarde-DemiOblique */
		FW_LIGHT,		/* Bookman-Light */
		FW_LIGHT,		/* Bookman-LightItalic */
		FW_DEMIBOLD,		/* Bookman-Demi */
		FW_DEMIBOLD,		/* Bookman-DemiItalic */
		FW_NORMAL,		/* Courier */
		FW_NORMAL,		/* Courier-Oblique */
		FW_BOLD,		/* Courier-Bold */
		FW_BOLD,		/* Courier-BoldItalic */
		FW_NORMAL,		/* Helvetica */
		FW_NORMAL,		/* Helvetica-Oblique */
		FW_BOLD,		/* Helvetica-Bold */
		FW_BOLD,		/* Helvetica-BoldOblique */
		FW_NORMAL,		/* Helvetica-Narrow */
		FW_NORMAL,		/* Helvetica-Narrow-Oblique */
		FW_BOLD,		/* Helvetica-Narrow-Bold */
		FW_BOLD,		/* Helvetica-Narrow-BoldOblique */
		FW_NORMAL,		/* NewCenturySchlbk-Roman */
		FW_NORMAL,		/* NewCenturySchlbk-Italic */
		FW_BOLD,		/* NewCenturySchlbk-Bold */
		FW_BOLD,		/* NewCenturySchlbk-BoldItalic */
		FW_NORMAL,		/* Palatino-Roman */
		FW_NORMAL,		/* Palatino-Italic */
		FW_BOLD,		/* Palatino-Bold */
		FW_BOLD,		/* Palatino-BoldItalic */
		FW_NORMAL,		/* Symbol */
		FW_NORMAL,		/* ZapfChancery-MediumItalic */
		FW_NORMAL		/* ZapfDingbats */
		};

static uchar	lfItalic[] = {
		False,			/* Times-Roman */
		True,			/* Times-Italic */
		False,			/* Times-Bold */
		True,			/* Times-BoldItalic */
		False,			/* AvantGarde */
		True,			/* AvantGarde-BookOblique */
		False,			/* AvantGarde-Demi */
		True,			/* AvantGarde-DemiOblique */
		False,			/* Bookman-Light */
		True,			/* Bookman-LightItalic */
		False,			/* Bookman-Demi */
		True,			/* Bookman-DemiItalic */
		False,			/* Courier */
		True,			/* Courier-Oblique */
		False,			/* Courier-Bold */
		True,			/* Courier-BoldItalic */
		False,			/* Helvetica */
		True,			/* Helvetica-Oblique */
		False,			/* Helvetica-Bold */
		True,			/* Helvetica-BoldOblique */
		False,			/* Helvetica-Narrow */
		True,			/* Helvetica-Narrow-Oblique */
		False,			/* Helvetica-Narrow-Bold */
		True,			/* Helvetica-Narrow-BoldOblique */
		False,			/* NewCenturySchlbk-Roman */
		True,			/* NewCenturySchlbk-Italic */
		False,			/* NewCenturySchlbk-Bold */
		True,			/* NewCenturySchlbk-BoldItalic */
		False,			/* Palatino-Roman */
		True,			/* Palatino-Italic */
		False,			/* Palatino-Bold */
		True,			/* Palatino-BoldItalic */
		False,			/* Symbol */
		False,			/* ZapfChancery-MediumItalic */
		False			/* ZapfDingbats */
		};

static uchar	lfCharSet[] = {
		ANSI_CHARSET,		/* Times-Roman */
		ANSI_CHARSET,		/* Times-Italic */
		ANSI_CHARSET,		/* Times-Bold */
		ANSI_CHARSET,		/* Times-BoldItalic */
		ANSI_CHARSET,		/* AvantGarde */
		ANSI_CHARSET,		/* AvantGarde-BookOblique */
		ANSI_CHARSET,		/* AvantGarde-Demi */
		ANSI_CHARSET,		/* AvantGarde-DemiOblique */
		ANSI_CHARSET,		/* Bookman-Light */
		ANSI_CHARSET,		/* Bookman-LightItalic */
		ANSI_CHARSET,		/* Bookman-Demi */
		ANSI_CHARSET,		/* Bookman-DemiItalic */
		ANSI_CHARSET,		/* Courier */
		ANSI_CHARSET,		/* Courier-Oblique */
		ANSI_CHARSET,		/* Courier-Bold */
		ANSI_CHARSET,		/* Courier-BoldItalic */
		ANSI_CHARSET,		/* Helvetica */
		ANSI_CHARSET,		/* Helvetica-Oblique */
		ANSI_CHARSET,		/* Helvetica-Bold */
		ANSI_CHARSET,		/* Helvetica-BoldOblique */
		ANSI_CHARSET,		/* Helvetica-Narrow */
		ANSI_CHARSET,		/* Helvetica-Narrow-Oblique */
		ANSI_CHARSET,		/* Helvetica-Narrow-Bold */
		ANSI_CHARSET,		/* Helvetica-Narrow-BoldOblique */
		ANSI_CHARSET,		/* NewCenturySchlbk-Roman */
		ANSI_CHARSET,		/* NewCenturySchlbk-Italic */
		ANSI_CHARSET,		/* NewCenturySchlbk-Bold */
		ANSI_CHARSET,		/* NewCenturySchlbk-BoldItalic */
		ANSI_CHARSET,		/* Palatino-Roman */
		ANSI_CHARSET,		/* Palatino-Italic */
		ANSI_CHARSET,		/* Palatino-Bold */
		ANSI_CHARSET,		/* Palatino-BoldItalic */
		SYMBOL_CHARSET,		/* Symbol */
		SYMBOL_CHARSET,		/* ZapfChancery-MediumItalic */
		SYMBOL_CHARSET		/* ZapfDingbats */
		};

static uchar	lfPitchAndFamily[] = {
	VARIABLE_PITCH | FF_ROMAN,	/* Times-Roman */
	VARIABLE_PITCH | FF_ROMAN,	/* Times-Italic */
	VARIABLE_PITCH | FF_ROMAN,	/* Times-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Times-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde-BookOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde-Demi */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde-DemiOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-Light */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-LightItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-Demi */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-DemiItalic */
	   FIXED_PITCH | FF_MODERN,	/* Courier */
	   FIXED_PITCH | FF_MODERN,	/* Courier-Oblique */
	   FIXED_PITCH | FF_MODERN,	/* Courier-Bold */
	   FIXED_PITCH | FF_MODERN,	/* Courier-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Oblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-BoldOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow-Oblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow-BoldOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-Roman */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-Italic */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-Roman */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-Italic */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Symbol */
	VARIABLE_PITCH | FF_ROMAN,	/* ZapfChancery-MediumItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* ZapfDingbats */
		};

#if defined(I18N) && defined(USE_ICONV)
extern Boolean support_i18n;  /* enable i18n support? */

#define FONT_TIMES_ROMAN	0
#define FONT_TIMES_BOLD		2
#define IS_LOCALE_FONT(f) ((f) == FONT_TIMES_ROMAN || (f) == FONT_TIMES_BOLD)

#define FONT_LOCALE_ROMAN	-1
#define FONT_LOCALE_BOLD	-2

enum genemf_lang {
    LANG_DEFAULT,
    LANG_ZH_CN,
    LANG_ZH_TW,
    LANG_JA,
    LANG_KO
} figLanguage;

/* EUC codeset name for iconv(3) */
static const char *const iconvCharset[] = {
    "ASCII",	/* placeholder */
    "eucCN",
    "eucTW",
    "eucJP",
    "eucKR"
};

/* EMF codeset name for iconv(3) */
#ifdef __hpux
# define EMF_ICONV_CODESET	"ucs2"	/* convert to big endian Unicode */
# define EMF_ICONV_NEED_SWAP	1	/* and swap the byte order */
#else
# define EMF_ICONV_CODESET	"UTF-16LE"
#endif

/* localized fonts */
static const struct localefnt {
	const char *FaceName;	/* font name in EUC of the language */
	int Weight;
	uchar Italic, Charset, PitchAndFamily;
} localeFonts[2][5] = { {
    { "Times", FW_NORMAL, False, DEFAULT_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "SimSun",
      FW_NORMAL, False, GB2312_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "MingLiU",
      FW_NORMAL, False, CHINESEBIG5_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "\243\315\243\323 \314\300\304\253",	/* MS Mincho */
      FW_NORMAL, False, SHIFTJIS_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "Batang",
      FW_NORMAL, False, HANGUL_CHARSET, FIXED_PITCH|FF_DONTCARE }
}, {
    { "Times", FW_BOLD, False, DEFAULT_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "SimHei",
      FW_MEDIUM, False, GB2312_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "MingLiU",
      FW_BOLD,   False, CHINESEBIG5_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "\243\315\243\323 \245\264\245\267\245\303\245\257", /* MS Gothic */
      FW_MEDIUM, False, SHIFTJIS_CHARSET, FIXED_PITCH|FF_DONTCARE },
    { "Gungsuh",
      FW_MEDIUM, False, HANGUL_CHARSET, FIXED_PITCH|FF_DONTCARE }
} };
#endif	/* defined(I18N) && defined(USE_ICONV) */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


static void delete_handle();
static void clear_current_handle();
static void select_object();
static void use_handle();
static struct emfhandle *get_handle();
static int conv_color();
static int conv_fill_color();
static int conv_fontindex();
static int conv_capstyle();
static int conv_joinstyle();
static int conv_linetype();
static void edgeattr();
static void edgevis();
static void bkmode();
static void bkcolor();
static void create_brush_pattern();
static void fillstyle();
static void textfont();
static void warn_32bit_pos();
static void pos2point();
static double arrow_length();
static void arrow();
static void arc_arrow();
static void arc_arrow_adjust();
static double arc_radius();
static void translate();
static void rotate();
static void arc_rotate();
static void arcoutline();
static void arcinterior();
static void arc_reverse();
static void circle();
static void ellipse();
static void rotated_ellipse();
static int icprod();
static int cwarc();
static int direction();
static double distance();
static size_t emh_write();
static int is_flip();
static void encode_bitmap();
static void picbox();
static void polygon();
static int polyline_adjust();
static void polyline();
static void rect();
static void roundrect();
static void shape();
static void shape_interior();
static void textunicode();
static void text();
static void textcolr();
static void textalign();
#if defined(I18N) && defined(USE_ICONV)
static void moveto();
#endif

extern FILE *open_picfile();
extern void close_picfile();
#ifdef USE_PNG
extern	int read_png();
#endif

/* Piece of code to avoid unnecessary attribute changes */
#define chkcache(val, cachedval)	\
    if (val == cachedval)		\
	return;				\
    else				\
	cachedval = val;


/* Delete an object by handle structure. */
static void delete_handle(h)
    struct emfhandle *h;
{
    EMRSELECTOBJECT em_so;	/* EMRDELETEOBJECT */

    /* If this is selected, select a stock object before deleting it. */
    if (h->is_current) {
	em_so.emr.iType = htofl(EMR_SELECTOBJECT);
	em_so.emr.nSize = htofl(sizeof(EMRSELECTOBJECT));
	switch (h->type) {
	case EMFH_PEN:
	    em_so.ihObject = htofl(ENHMETA_STOCK_OBJECT | BLACK_PEN);
	    lasthandle[EMFH_PEN] = ENHMETA_STOCK_OBJECT | BLACK_PEN;
	    break;
	case EMFH_BRUSH:
	    em_so.ihObject = htofl(ENHMETA_STOCK_OBJECT | WHITE_BRUSH);
	    lasthandle[EMFH_BRUSH] = ENHMETA_STOCK_OBJECT | WHITE_BRUSH;
	    break;
	case EMFH_FONT:
	    em_so.ihObject = htofl(ENHMETA_STOCK_OBJECT | SYSTEM_FONT);
	    lasthandle[EMFH_FONT] = ENHMETA_STOCK_OBJECT | SYSTEM_FONT;
	    break;
	default:
	    fprintf(stderr, "genemf: unknown handle type %d.\n", h->type);
	    exit(1);
	}

# ifdef __EMF_DEBUG__
	fprintf(stderr, "SelectObject: %u\n", ftohl(em_so.ihObject));
# endif

	emh_write(&em_so, sizeof(EMRSELECTOBJECT), (size_t) 1, EMH_RECORD);
	h->is_current = False;
    }

    em_so.emr.iType = htofl(EMR_DELETEOBJECT);
    em_so.emr.nSize = htofl(sizeof(EMRDELETEOBJECT));

    em_so.ihObject = htofl(h->handle);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "DeleteObject: %u\n", ftohl(em_so.ihObject));
# endif

    emh_write(&em_so, sizeof(EMRDELETEOBJECT), (size_t) 1, EMH_RECORD);
}


/* Clear current mark from objects of the specified type. */
static void clear_current_handle(type)
    enum emfhandletype type;
{
    struct emfhandle *h;

    for (h = handles; h; h = h->next)
	if (h->is_current && h->type == type) {
	    h->is_current = False;
	    break;
	}
}


/* Select object by handle number. */
static void select_object(handle)
    unsigned handle;
{
    EMRSELECTOBJECT em_so;

    em_so.emr.iType = htofl(EMR_SELECTOBJECT);
    em_so.emr.nSize = htofl(sizeof(EMRSELECTOBJECT));
    em_so.ihObject = htofl(handle);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "SelectObject: %u\n", ftohl(em_so.ihObject));
# endif

    emh_write(&em_so, sizeof(EMRSELECTOBJECT), (size_t) 1, EMH_RECORD);
}


/* Add a handle structure to the newest position of handle list. */
static void use_handle(h)
    struct emfhandle *h;
{

    if (h != latesthandle) {
	if (h->prev) {
	    /* unlink from list */
	    if (h->next)
		h->next->prev = h->prev;
	    *h->prev = h->next;
	}
	if (handles == NULL) {
	    handles = h;
	    h->prev = &handles;
	} else
	    h->prev = &latesthandle->next;
	*h->prev = h;
	h->next = NULL;
	latesthandle = h;
    }
}


/*
 * Allocate handle structure.  If the number of currently allocated handle
 * exceeds maximum limit, reuse the least recently used handle.
 */
static struct emfhandle *get_handle(type)
    enum emfhandletype type;
{
    struct emfhandle *h;

    if (emh_nHandles >= maxHandles) {
	/* reuse handle (LRU) */
	for (h = handles; h; h = h->next) {
	    if (!h->is_current) {
		/* use it */
		delete_handle(h);
		goto use;
	    }
	}
	/*
	 * All handles are currently in use.
	 * Although exceeds maxHandles, create new one
	 */
	/* FALLTHROUGH */
    }

    /* allocate new handle */
    if ((h = malloc(sizeof(struct emfhandle))) == NULL) {
	perror("fig2dev: malloc");
	exit(1);
    }
    emh_nHandles++;
    h->handle = emh_nHandles;
    h->prev = NULL;
    h->next = NULL;
    h->is_current = False;
use:
    h->type = type;

    return h;
}


/* Given an index into either the standard color list or into the
 * user defined color list, return the hex RGB value of the color. */
static int conv_color(colorIndex)
    int colorIndex;
{
    int   rgb;
    static const int rgbColors[NUM_STD_COLS] = {
      /* Black     Blue      Green     Cyan      Red       Magenta */
	0x000000, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF,
      /* Yellow    White     Blue4     Blue3     Blue2     LtBlue */
	0xFFFF00, 0xFFFFFF, 0x000090, 0x0000B0, 0x0000D0, 0x87CEFF,
      /* Green4    Green3    Green2    Cyan4     Cyan3     Cyan2  */
	0x009000, 0x00B000, 0x00D000, 0x009090, 0x00B0B0, 0x00D0D0,
      /* Red4      Red3      Red2      Magenta4  Magenta3  Magenta2 */
	0x900000, 0xB00000, 0xD00000, 0x900090, 0xB000B0, 0xD000D0,
      /* Brown4    Brown3    Brown2    Pink4     Pink3     Pink2  */
	0x803000, 0xA04000, 0xC06000, 0xFF8080, 0xFFA0A0, 0xFFC0C0,
      /* Pink      Gold   */
	0xFFE0E0, 0xFFD700
    };

    if (colorIndex == DEFAULT)
	colorIndex = 0;

    if (colorIndex < NUM_STD_COLS)
	rgb = RGB(rgbColors[colorIndex] >> 16,
		  (rgbColors[colorIndex] >> 8) & 0xff,
		  rgbColors[colorIndex] & 0xff);
    else
	rgb = RGB(user_colors[colorIndex-NUM_STD_COLS].r,
		  user_colors[colorIndex-NUM_STD_COLS].g,
		  user_colors[colorIndex-NUM_STD_COLS].b);
    return rgb;
}/* end conv_color */


/* Converts fill pattern color.  The style contains the fill intensity.
 * 0 - 20 - 40 is equivalent to -1.0 - 0.0 - +1.0. */
static int conv_fill_color(style, color)
    int style;
    int color;
{
    int   rgb, b, g, r;
    float f;

    rgb = conv_color(color);
    b = GetBValue(rgb);
    g = GetGValue(rgb);
    r = GetRValue(rgb);

    if (style <= 0) {				/* Black */

	b = 0;
	g = 0;
	r = 0;

    } else if (style < 20) {			/* Darkened */

	f = (float)style / 20.0;
	b = round(b * f);
	g = round(g * f);
	r = round(r * f);

    } else if (style == 20) {			/* Normal */

	/* : */

    } else if (style < 40) {			/* Lightened */

	f = (float)(style - 20) / 20.0;
	b = round(b + f*(255-b));
	g = round(g + f*(255-g));
	r = round(r + f*(255-r));

    } else {/* if (style >= 40) */		/* White */

	b = 0xFF;
	g = 0xFF;
	r = 0xFF;

    }/* end if (style >= 40) */

    return RGB(r, g, b);
}/* end conv_fill_color */


/* Converts Fig font index to index into font table in the pre-amble,
 * taking into account the flags. */
static int conv_fontindex(font, flags)
    int font;
    int flags;
{

    if (flags&4) {			/* PostScript fonts */
	if (font == (-1)) {
	    font = 0;			/* Default PostScript is Roman */
	} else if ((font < 0) || (font >= NUM_PS_FONTS)) {
	    fprintf(stderr, "Unsupported Fig PostScript font index %d.\n",
		    font);
	    font = 0;			/* Default font */
	}
    } else {				/* LaTeX fonts */
	if ((font < 0) || (font > NUM_LATEX_FONTS)) {
	    fprintf(stderr, "Unsupported Fig LaTeX font index %d.\n", font);
	    font = 0;			/* Default font */
	}
	else
	    font = ltFonts[font];	/* Convert to PostScript fonts */
    }
    return font;
}/* end conv_fontindex */


/* Convert fig cap style to EMF line style. */
static int conv_capstyle(cap)
    int	cap;
{

    switch (cap) {
    case 0:	/* Butt. */
	cap = PS_ENDCAP_FLAT;
	break;
    case 1:	/* Round. */
	cap = PS_ENDCAP_ROUND;
	break;
    case 2:	/* Projecting. */
	cap = PS_ENDCAP_SQUARE;
	break;
    default:
	fprintf(stderr, "genemf: unknown cap style %d.\n", cap);
	cap = PS_ENDCAP_FLAT;
	break;
    }
    return cap;
}/* end conv_capstyle */


/* Convert fig join style to EMF join style. */
static int conv_joinstyle(join)
    int	join;
{

    switch (join) {
    case 0:	/* Miter. */
	join = PS_JOIN_MITER;
	break;
    case 1:	/* Round. */
	join = PS_JOIN_ROUND;
	break;
    case 2:	/* Bevel. */
	join = PS_JOIN_BEVEL;
	break;
    default:
	fprintf(stderr, "genemf: unknown join style %d.\n", join);
	join = PS_JOIN_MITER;
	break;
    }
    return join;
}/* end conv_joinstyle */


/* Convert fig line style to EMF line style.  EMF knows 5 styles with
 * fortunately correspond to the first 5 fig line styles.  The triple
 * dotted fig line style is handled as a solid line here. */
static int conv_linetype(type)
    int	type;
{

    if (type < 0)
	type = -1;
    else if (type > 4)
	type %= 5;
    return type;
}/* end conv_linetype */


/* This procedure sets the pen line attributes and selects the pen into
 * the device context. */
static void edgeattr(vis, type, width, color, join, cap)
    int vis;		/* visible */
    int type;		/* line type */
    int width, color;
    int join;		/* join style */
    int cap;		/* cap style */
{
    unsigned handle;
    int rgb;
    EMREXTCREATEPEN em_pn;
    struct emfhandle *h = NULL;
    int style;
    unsigned styleEntry[8];	/* "- . . . " */
    int user_style = False;

    if (width == 0)
	vis = 0;

    /* if a stock object is available, use it */
    if (vis == 0) {
	handle = ENHMETA_STOCK_OBJECT | NULL_PEN;
	goto selectpen;
    }

    rgb = conv_color(color);
    style = PS_GEOMETRIC;

    switch (emflevel) {
    case EMF_LEVEL_WIN98:
	if (type >= 1 && type <= 4) {
	    fprintf(stderr, "Warning: line style may be ignored\n");
	}
	break;

    case EMF_LEVEL_WIN95:
	if (type >= 1 && type <= 4) {
	    if (width < 20) {
		fprintf(stderr, "Warning: this style of line is converted to thin line\n");
		style = PS_COSMETIC;
	    } else
		fprintf(stderr, "Warning: line style may be ignored\n");
	}
	break;

    default:
	break;
    }

    switch (emflevel) {
    case EMF_LEVEL_WINNT:
	if (type == 5) {
	    style |= PS_USERSTYLE;
	    user_style = True;
	} else
	    style |= conv_linetype(type);
	break;

    default:
	if (type == 5) {
	    fprintf(stderr, "Warning: dash-triple-dotted style is converted to solid style\n");
	    type = 0;
	}
	style |= conv_linetype(type);
	break;
    }
    style |= conv_capstyle(cap) | conv_joinstyle(join);

    /* search for the same pen */
    for (h = latesthandle; h != (void *)&handles; h = (void *) h->prev) {
	if (h->type == EMFH_PEN &&
		h->eh_un.p.pstyle == style && h->eh_un.p.pwidth == width &&
		h->eh_un.p.prgb == rgb) {
	    /* found */
	    use_handle(h);
	    handle = h->handle;
	    goto selectpen;
	}
    }

    /* not found -- create new pen */
    h = get_handle(EMFH_PEN);
    handle = h->handle;
    use_handle(h);

    h->eh_un.p.pstyle = style;
    h->eh_un.p.pwidth = width;
    h->eh_un.p.prgb = rgb;

    memset(&em_pn, 0, sizeof(EMREXTCREATEPEN));
    em_pn.emr.iType = htofl(EMR_EXTCREATEPEN);
    em_pn.ihPen = htofl(handle);
    em_pn.offBmi  = htofl(sizeof(EMREXTCREATEPEN));
    em_pn.offBits = htofl(sizeof(EMREXTCREATEPEN));
    em_pn.elp.elpPenStyle = htofl(style);
    em_pn.elp.elpWidth = htofl(width);
    em_pn.elp.elpColor = htofl(rgb);

    if (user_style) {
	/* create dash-triple-dotted style */
	styleEntry[0] = htofl(width * 4);	/* dash */
	styleEntry[1] =				/* gap */
	styleEntry[2] =				/* dot */
	styleEntry[3] =				/* gap */
	styleEntry[4] =				/* dot */
	styleEntry[5] =				/* gap */
	styleEntry[6] =				/* dot */
	styleEntry[7] = htofl(width);		/* gap */
	em_pn.elp.elpNumEntries = htofl(sizeof styleEntry/sizeof styleEntry[0]);
	em_pn.emr.nSize = htofl(sizeof(EMREXTCREATEPEN) + sizeof styleEntry);
    } else {
	em_pn.emr.nSize = htofl(sizeof(EMREXTCREATEPEN));
    }

# ifdef __EMF_DEBUG__
    fprintf(stderr, "CreatePen (%d): style %d, width %d, rgb %x\n",
	handle, style, width, rgb);
# endif

    emh_write(&em_pn, sizeof(EMREXTCREATEPEN), (size_t) 1, EMH_RECORD);
    if (user_style)
	emh_write(&styleEntry, sizeof styleEntry, (size_t) 1, EMH_DATA);

selectpen:
    chkcache(handle, lasthandle[EMFH_PEN]);
    clear_current_handle(EMFH_PEN);
    if (h)
	h->is_current = True;
# ifdef __EMF_DEBUG__
    fprintf(stderr, "Pen:   ");
# endif
    select_object(handle);
}/* end edgeattr */


/* This procedure turns off the edge visibility. */
static void edgevis(onoff)
    int onoff;
{

    edgeattr(onoff, 0, 0, 0, 0, 0);
}/* end edgevis */


static void bkmode(mode)
    int mode;
{
    static int oldbkmode = 0;
    EMRSETBKMODE em_bm;

    chkcache(mode, oldbkmode);
    memset(&em_bm, 0, sizeof(EMRSETBKMODE));
    em_bm.emr.iType = htofl(EMR_SETBKMODE);
    em_bm.emr.nSize = htofl(sizeof(EMRSETBKMODE));
    em_bm.iMode = htofl(mode);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "SetBKMode (mode): %d\n", mode);
# endif

    emh_write(&em_bm, sizeof(EMRSETBKMODE), (size_t) 1, EMH_RECORD);
}


static void bkcolor(rgb)
    int rgb;
{
    static int oldbkcolor = UNDEFVALUE;
    EMRSETBKCOLOR em_bc;

    chkcache(rgb, oldbkcolor);
    memset(&em_bc, 0, sizeof(EMRSETBKCOLOR));
    em_bc.emr.iType = htofl(EMR_SETBKCOLOR);
    em_bc.emr.nSize = htofl(sizeof(EMRSETBKCOLOR));
    em_bc.crColor = htofl(rgb);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "SetBKColor (rgb): %x\n", rgb);
# endif

    emh_write(&em_bc, sizeof(EMRSETBKCOLOR), (size_t) 1, EMH_RECORD);
}


static void create_brush_pattern(bits, pattern)
    unsigned char *bits;
    int pattern;
{
    const struct emf_bmpat *pat = &emf_bm_pattern[pattern];
    unsigned char *b = pat->bits;
    int xbmhbytecnt;
    unsigned x;
    int y;
    unsigned byte, bit;

    xbmhbytecnt = (pat->width + 7) / 8;

    /* EMF bitmap is encoded from bottom to top */
    byte = 0;
    for (y = pat->height - 1; y >=0; y--) {
	bit = 7;
	for (x = 0; x < pat->width; x++) {
	    if (b[xbmhbytecnt * y + x / 8] & (1 << x % 8))
		bits[byte] |= 1 << bit;
	    if (bit-- == 0) {
		byte++;
		bit = 7;
	    }
	}
	/* size of a line must be a multiple of 4 byte */
	if (bit != 7)
	    byte++;
	byte = (byte + 3) & ~(unsigned)3;
    }
}


/* Computes and sets fill color for solid filled shapes (fill style 0 to 40). */
static void fillstyle(fill_color, fill_style, pen_color)
    int fill_color, fill_style, pen_color;
{
    unsigned handle;
    int pattern, hatch;
    int rgb, hatchbkrgb;
    int style;
    EMRCREATEBRUSHINDIRECT em_bi;
    EMRCREATEDIBPATTERNBRUSHPT em_pb;
    BITMAPINFO bmi;
    RGBQUAD hatchcolors[2];
    int width, height;
    size_t bsize;
    unsigned char *bits;
    struct emfhandle *h = NULL;

    if (fill_style < 0) {		/* Unfilled shape */
	style = BS_HOLLOW;
	rgb = 0;
	hatchbkrgb = 0;
	pattern = 0;
    } else if (fill_style <= 40) {	/* Solid filled shape */
	style = BS_SOLID;
	rgb = conv_fill_color(fill_style, fill_color);
	hatchbkrgb = 0;
	pattern = 0;
    } else /* if (fill_style > 40) */ {	/* Pattern filled shape */
	style = BS_HATCHED;
	rgb = conv_color(pen_color);
	hatchbkrgb = conv_color(fill_color);
	pattern = fill_style - 41;
    }

    if ((unsigned)pattern > sizeof emf_bm_pattern / sizeof emf_bm_pattern[0]) {
	fprintf(stderr, "fig2dev: emf: fill pattern %d is not supported\n",
		pattern);
	return;
    }

    if (style == BS_HATCHED && emf_map_pattern[pattern] != -1) {
	/*
	 * Use EMF hatch pattern.  Setting of background color is not in brush.
	 */
	bkmode(OPAQUE);
	bkcolor(hatchbkrgb);

	/* The background color is now handled. */
	hatchbkrgb = 0;
    }

    /* if a stock object is available, use it */
    switch (style) {
    case BS_HOLLOW:
	handle = ENHMETA_STOCK_OBJECT | NULL_BRUSH;
	goto selectbrush;
    case BS_SOLID:
	switch (rgb) {
	case 0xffffff:
	    handle = ENHMETA_STOCK_OBJECT | WHITE_BRUSH;
	    goto selectbrush;
	case 0xc0c0c0:
	    handle = ENHMETA_STOCK_OBJECT | LTGRAY_BRUSH;
	    goto selectbrush;
	case 0x808080:
	    handle = ENHMETA_STOCK_OBJECT | GRAY_BRUSH;
	    goto selectbrush;
	case 0x404040:
	    handle = ENHMETA_STOCK_OBJECT | DKGRAY_BRUSH;
	    goto selectbrush;
	case 0x000000:
	    handle = ENHMETA_STOCK_OBJECT | BLACK_BRUSH;
	    goto selectbrush;
	default:
	    break;
	}
	break;
    default:
	break;
    }

    /* search for the same brush */
    for (h = latesthandle; h != (void *)&handles; h = (void *) h->prev) {
	if (h->type == EMFH_BRUSH &&
		h->eh_un.b.bstyle == style && h->eh_un.b.brgb == rgb &&
		h->eh_un.b.bpattern == pattern &&
		h->eh_un.b.bhatchbkrgb == hatchbkrgb) {
	    /* found */
	    use_handle(h);
	    handle = h->handle;
	    goto selectbrush;
	}
    }

    /* not found -- create new brush */
    h = get_handle(EMFH_BRUSH);
    handle = h->handle;
    use_handle(h);

    h->eh_un.b.bstyle = style;
    h->eh_un.b.brgb = rgb;
    h->eh_un.b.bpattern = pattern;
    h->eh_un.b.bhatchbkrgb = hatchbkrgb;

    hatch = 0;
    if (style == BS_HATCHED && (hatch = emf_map_pattern[pattern]) == -1) {
	/*
	 * create a pattern brush
	 */
	memset(&em_pb, 0, sizeof(EMRCREATEDIBPATTERNBRUSHPT));
	memset(&bmi, 0, sizeof(BITMAPINFO));
	em_pb.emr.iType = htofl(EMR_CREATEDIBPATTERNBRUSHPT);
	em_pb.ihBrush = htofl(handle);
	/* em_pb.iUsage = htofl(DIB_RGB_COLORS);	this is default */
	em_pb.offBmi = htofl(sizeof(EMRCREATEDIBPATTERNBRUSHPT));
	em_pb.cbBmi = htofl(sizeof(BITMAPINFO) + 2 * sizeof(RGBQUAD));
	em_pb.offBits = htofl(sizeof(EMRCREATEDIBPATTERNBRUSHPT) +
			      sizeof(BITMAPINFO) + 2 * sizeof(RGBQUAD));

	width = emf_bm_pattern[pattern].width;
	height = emf_bm_pattern[pattern].height;

	if (emflevel != EMF_LEVEL_WINNT && (width > 8 || height > 8))
	    fprintf(stderr, "Warning: fill pattern %d will not appear properly on Windows 95/98/Me\n",
		    pattern);

	/* Windows 95/98/Me doesn't show 8x4 pat properly.  Convert to 8x8. */
	if (height == 4)
	    height = 8;

	bsize = (width + 31) / 32 * height * 4;

	bmi.bmiHeader.biSizeImage = em_pb.cbBits = htofl((EMFulong) bsize);
	em_pb.emr.nSize = htofl(sizeof(EMRCREATEDIBPATTERNBRUSHPT) +
		sizeof(BITMAPINFO) + 2 * sizeof(RGBQUAD) + bsize);

	bmi.bmiHeader.biSize = htofl(sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biWidth  = htofl(width);
	bmi.bmiHeader.biHeight = htofl(height);
	bmi.bmiHeader.biPlanes = bmi.bmiHeader.biBitCount = htofs(1);
	bmi.bmiHeader.biCompression = htofl(BI_RGB);	/* uncompressed */
	bmi.bmiHeader.biXPelsPerMeter =
		bmi.bmiHeader.biYPelsPerMeter = htofl(2953);	/* 75dpi ? */
	bmi.bmiHeader.biClrUsed = htofl(2);
	/* bmi.bmiHeader.biClrImportant = htofl(0); */

	/* background and foreground colors */
	hatchcolors[0].rgbBlue  = GetBValue(hatchbkrgb);
	hatchcolors[0].rgbGreen = GetGValue(hatchbkrgb);
	hatchcolors[0].rgbRed   = GetRValue(hatchbkrgb);
	hatchcolors[0].rgbReserved = 0;
	hatchcolors[1].rgbBlue  = GetBValue(rgb);
	hatchcolors[1].rgbGreen = GetGValue(rgb);
	hatchcolors[1].rgbRed   = GetRValue(rgb);
	hatchcolors[1].rgbReserved = 0;

	if ((bits = malloc(bsize)) == NULL) {
	    perror("fig2dev: malloc");
	    exit(1);
	}
	memset(bits, 0, bsize);

	create_brush_pattern(bits, pattern);

	/* Windows 95/98/Me doesn't show 8x4 pat properly.  Convert to 8x8. */
	if (emf_bm_pattern[pattern].height == 4)
	    create_brush_pattern(bits + bsize / 2, pattern); /* latter half */

# ifdef __EMF_DEBUG__
	fprintf(stderr, "CreatePatternBrush (%d): pattern %d, rgb %x, bkrgb %x\n",
	    handle, pattern, rgb, hatchbkrgb);
# endif

	emh_write(&em_pb, sizeof(EMRCREATEDIBPATTERNBRUSHPT), (size_t) 1,
		  EMH_RECORD);
	emh_write(&bmi, sizeof(BITMAPINFO), (size_t) 1, EMH_DATA);
	emh_write(hatchcolors, sizeof(RGBQUAD), (size_t) 2, EMH_DATA);
	emh_write(bits, bsize, (size_t) 1, EMH_DATA);
	free(bits);
    } else {
	memset(&em_bi, 0, sizeof(EMRCREATEBRUSHINDIRECT));
	em_bi.emr.iType = htofl(EMR_CREATEBRUSHINDIRECT);
	em_bi.emr.nSize = htofl(sizeof(EMRCREATEBRUSHINDIRECT));
	em_bi.ihBrush = htofl(handle);
	em_bi.lb.lbStyle = htofl(style);
	em_bi.lb.lbColor = htofl(rgb);
	em_bi.lb.lbHatch = htofl(hatch);

# ifdef __EMF_DEBUG__
	fprintf(stderr, "CreateBrush (%d): style %d, rgb %x, hatch %d\n",
	    handle, style, rgb, hatch);
# endif

	emh_write(&em_bi, sizeof(EMRCREATEBRUSHINDIRECT), (size_t) 1,
		  EMH_RECORD);
    }

selectbrush:
    chkcache(handle, lasthandle[EMFH_BRUSH]);
    clear_current_handle(EMFH_BRUSH);
    if (h)
	h->is_current = True;
# ifdef __EMF_DEBUG__
    fprintf(stderr, "Brush: ");
# endif
    select_object(handle);
}/* end fillstyle */


static void textfont(font, size, angle)
    int     font;
    double  size;
    int     angle;	/* tenths degree */
{
    EMREXTCREATEFONTINDIRECTW em_fn;
    short *utext;
    int    n_chars, n_unicode;
    struct emfhandle *h = NULL;
    unsigned handle;
    int fontsz;

    fontsz = -(int)(size * (1200/72.27 * 0.88/*?? font is a bit large*/) + 0.5);

    /* search for the same font */
    for (h = latesthandle; h != (void *)&handles; h = (void *) h->prev) {
	if (h->type == EMFH_FONT &&
		h->eh_un.f.ffont == font && h->eh_un.f.fsize == fontsz &&
		h->eh_un.f.fangle == angle) {
	    /* found */
	    use_handle(h);
	    handle = h->handle;
	    goto selectfont;
	}
    }

    /* not found -- create new font */
    h = get_handle(EMFH_FONT);
    handle = h->handle;
    use_handle(h);

    h->eh_un.f.ffont = font;
    h->eh_un.f.fsize = fontsz;
    h->eh_un.f.fangle = angle;

    memset(&em_fn, 0, sizeof(EMREXTCREATEFONTINDIRECTW));
    em_fn.emr.iType = htofl(EMR_EXTCREATEFONTINDIRECTW);
    em_fn.emr.nSize = htofl(sizeof(EMREXTCREATEFONTINDIRECTW));

    em_fn.ihFont = htofl(handle);

    utext = (short *) em_fn.elfw.elfLogFont.lfFaceName;
    n_unicode = sizeof (em_fn.elfw.elfLogFont.lfFaceName);
    textunicode(
#if defined(I18N) && defined(USE_ICONV)
      (font < 0 /* locale font */)? localeFonts[-1-font][figLanguage].FaceName :
#endif
	    lfFaceName[font],
      &n_chars, &utext, &n_unicode);

    em_fn.elfw.elfLogFont.lfHeight = htofl(fontsz);
    em_fn.elfw.elfLogFont.lfEscapement = em_fn.elfw.elfLogFont.lfOrientation
	= htofl(angle);
#if defined(I18N) && defined(USE_ICONV)
    if (font < 0 /* locale font */) {
	const struct localefnt *lf = &localeFonts[-1-font][figLanguage];

	em_fn.elfw.elfLogFont.lfWeight = htofl(lf->Weight);
	em_fn.elfw.elfLogFont.lfItalic = /*uchar*/lf->Italic;
	em_fn.elfw.elfLogFont.lfCharSet = /*uchar*/lf->Charset;
	em_fn.elfw.elfLogFont.lfPitchAndFamily = /*uchar*/lf->PitchAndFamily;
	HTOFL(em_fn.elfw.elfLogFont.lfWidth,
	    (int)(size * (1200/72.27 *0.40/*?? font is a bit wide*/) + 0.5));
    } else
#endif
    {
	em_fn.elfw.elfLogFont.lfWeight = htofl(lfWeight[font]);
	em_fn.elfw.elfLogFont.lfItalic = /*uchar*/lfItalic[font];
	em_fn.elfw.elfLogFont.lfCharSet = /*uchar*/lfCharSet[font];
	em_fn.elfw.elfLogFont.lfPitchAndFamily =/*uchar*/lfPitchAndFamily[font];
    }

    emh_write(&em_fn, sizeof(EMREXTCREATEFONTINDIRECTW), (size_t)1, EMH_RECORD);

# ifdef __EMF_DEBUG__
    fprintf(stderr,
	"Textfont (%d): %s  Size: %d  Weight: %d  Italic: %d  Angle: %d\n",
	handle,
#  if defined(I18N) && defined(USE_ICONV)
      (font < 0 /* locale font */)? localeFonts[-1-font][figLanguage].FaceName :
#  endif
	lfFaceName[font], -ftohl(em_fn.elfw.elfLogFont.lfHeight),
	lfWeight[font], lfItalic[font], angle);
# endif

selectfont:
    chkcache(handle, lasthandle[EMFH_FONT]);
    clear_current_handle(EMFH_FONT);
    if (h)
	h->is_current = True;
# ifdef __EMF_DEBUG__
    fprintf(stderr, "Font:  ");
# endif
    select_object(handle);
}/* end textfont */


static void warn_32bit_pos()
{
    static int wgiv;

    if (emflevel != EMF_LEVEL_WINNT && !wgiv) {
	fprintf(stderr, "\
Warning: Coordinates exceed 16bit value.\n\
Some figures will be invisible on Windows 95/98/Me.\n");
	wgiv = 1;
    }
}


/* Copies coordinates of pos p to point P. */
static void pos2point(P, p)
    F_point *P;
    F_pos *p;
{

    P->x = p->x; P->y = p->y;
}/* end pos2point */


/* Returns length of the arrow. used to shorten lines/arcs at
 * an end where an arrow needs to be drawn. */
static double arrow_length(a)
    F_arrow *a;
{
    double len;

    switch (a->type) {
    case 0:				/* stick type */
	len = ARROW_EXTRA_LEN(a);
	break;
    case 1:				/* closed triangle */
	len =  a->ht;
	break;
    case 2:				/* indented hat */
	len =  ARROW_INDENT_DIST * a->ht;
	break;
    case 3:				/* pointed hat */
	len =  ARROW_POINT_DIST * a->ht;
	break;
    default:
	len =  0.;
    }
    if (rounded_arrows)
	len -= ARROW_EXTRA_LEN(a);

    return len;
}


/* Draws an arrow ending at point p pointing into direction dir.
 * type and attributes as required by a and l.  This routine works
 * for both lines and arc (in that case l should be a F_arc *). */
static void arrow(P, a, l, dir)
    F_point	*P;
    F_arrow	*a;
    F_line	*l;
    Dir		*dir;
{
    F_point s1, s2, t, p;
    EMRPOLYLINE	em_pl;		/* Little endian format */
    POINTL      aptl[4];	/* Maximum of four points required for arrows */
    POINTS      apts[4];
    unsigned    cpt;		/* Number of points in the array */
    unsigned itype, itype16;
    int bbx_top, bbx_bottom, bbx_left, bbx_right;	/* Bounding box */

    t.x = 0;
    t.y = 0;
    p = *P;
    if (!rounded_arrows) {
	/* Move the arrow backwards in order to let it end
	 * at the correct spot */
	double f = ARROW_EXTRA_LEN(a);
	p.x = round(p.x - f * dir->x);
	p.y = round(p.y - f * dir->y);
    }

    if (a->type == 0) {		/* Old style stick arrow */
	edgeattr(1, 0, (int)a->thickness, l->pen_color, 0, 0);
    } else {			/* Polygonal arrows */
	/* miter join style, butt line cap */
	edgeattr(1, 0, (int)a->thickness, l->pen_color, 0, 0);
	switch (a->style) {
	case 0:
	    fillstyle(7, 20, 0);		/* Fill with white (7) */
	    break;
	case 1:
	    fillstyle(l->pen_color, 20, 0);	/* Use pen color to fill */
	    break;
	default:
	    fprintf(stderr, "Unsupported fig arrow style %d !!\n", a->style);
	}
    }

    /* Start the bounding box */

    bbx_left = bbx_right  = P->x;	/* Use original point for bounding */
    bbx_top  = bbx_bottom = P->y;

    itype = EMR_POLYGON;
    itype16 = EMR_POLYGON16;

    switch (a->type) {

    case 0:				/* Stick type */
	itype = EMR_POLYLINE;
	itype16 = EMR_POLYLINE16;
	cpt = 3;

	s1.x = round(p.x - a->ht * dir->x + a->wid*0.5 * dir->y);
	s1.y = round(p.y - a->ht * dir->y - a->wid*0.5 * dir->x);
	s2.x = round(p.x - a->ht * dir->x - a->wid*0.5 * dir->y);
	s2.y = round(p.y - a->ht * dir->y + a->wid*0.5 * dir->x);
	break;

    case 1:				/* Closed triangle */
	cpt = 3;

	s1.x = round(p.x - a->ht * dir->x + a->wid*0.5 * dir->y);
	s1.y = round(p.y - a->ht * dir->y - a->wid*0.5 * dir->x);
	s2.x = round(p.x - a->ht * dir->x - a->wid*0.5 * dir->y);
	s2.y = round(p.y - a->ht * dir->y + a->wid*0.5 * dir->x);
	break;

    case 2:				/* Indented hat */
	cpt = 4;

	s1.x = round(p.x - a->ht*ARROW_POINT_DIST * dir->x
			 + a->wid*0.5 * dir->y);
	s1.y = round(p.y - a->ht*ARROW_POINT_DIST * dir->y
			 - a->wid*0.5 * dir->x);
	s2.x = round(p.x - a->ht*ARROW_POINT_DIST * dir->x
			 - a->wid*0.5 * dir->y);
	s2.y = round(p.y - a->ht*ARROW_POINT_DIST * dir->y
			 + a->wid*0.5 * dir->x);

	t.x = round(p.x - a->ht * dir->x);
	t.y = round(p.y - a->ht * dir->y);

	UPDATE_BBX_X(t.x);
	UPDATE_BBX_Y(t.y);
	break;

    case 3:				/* Pointed hat */
	cpt = 4;

	s1.x = round(p.x - a->ht*ARROW_INDENT_DIST * dir->x
			 + a->wid*0.5 * dir->y);
	s1.y = round(p.y - a->ht*ARROW_INDENT_DIST * dir->y
			 - a->wid*0.5 * dir->x);
	s2.x = round(p.x - a->ht*ARROW_INDENT_DIST * dir->x
			 - a->wid*0.5 * dir->y);
	s2.y = round(p.y - a->ht*ARROW_INDENT_DIST * dir->y
			 + a->wid*0.5 * dir->x);

	t.x = round(p.x - a->ht * dir->x);
	t.y = round(p.y - a->ht * dir->y);

	UPDATE_BBX_X(t.x);
	UPDATE_BBX_Y(t.y);
	break;

    default:
	fprintf(stderr, "Unsupported fig arrow type %d.\n", a->type);
	return;

    }/* end case */

    UPDATE_BBX_X(s1.x);
    UPDATE_BBX_Y(s1.y);

    UPDATE_BBX_X(s2.x);
    UPDATE_BBX_Y(s2.y);

    em_pl.rclBounds.left   = htofl(bbx_left);	/* Store bounding box */
    em_pl.rclBounds.right  = htofl(bbx_right);
    em_pl.rclBounds.top    = htofl(bbx_top);
    em_pl.rclBounds.bottom = htofl(bbx_bottom);
    em_pl.cptl = htofl(cpt);			/* Number of points */

    if (bbx_left >= -32768 && bbx_right <= 32767
	&& bbx_top >= -32768 && bbx_bottom <= 32767) {

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Arrowhead16 %d rclBounds (ltrb): %d %d %d %d\n",
      cpt,
      bbx_left,  bbx_top, bbx_right, bbx_bottom);
# endif

	/* use 16bit point */
	em_pl.emr.iType = htofl(itype16);

	apts[0].x = htofs(s1.x);
	apts[0].y = htofs(s1.y);
	apts[1].x = htofs(p.x);
	apts[1].y = htofs(p.y);
	apts[2].x = htofs(s2.x);
	apts[2].y = htofs(s2.y);
	apts[3].x = htofs(t.x);
	apts[3].y = htofs(t.y);

	HTOFL(em_pl.emr.nSize, sizeof(EMRPOLYLINE16) + cpt*sizeof(POINTS));
	emh_write(&em_pl, sizeof(EMRPOLYLINE16), (size_t) 1, EMH_RECORD);
	emh_write(apts, sizeof(POINTS), (size_t) cpt, EMH_DATA);
    } else {

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Arrowhead %d rclBounds (ltrb): %d %d %d %d\n",
      cpt,
      bbx_left,  bbx_top, bbx_right, bbx_bottom);
# endif

	/* use 32bit point */
	warn_32bit_pos();

	em_pl.emr.iType = htofl(itype);

	aptl[0].x = htofl(s1.x);
	aptl[0].y = htofl(s1.y);
	aptl[1].x = htofl(p.x);
	aptl[1].y = htofl(p.y);
	aptl[2].x = htofl(s2.x);
	aptl[2].y = htofl(s2.y);
	aptl[3].x = htofl(t.x);
	aptl[3].y = htofl(t.y);

	HTOFL(em_pl.emr.nSize, sizeof(EMRPOLYLINE) + cpt*sizeof(POINTL));
	emh_write(&em_pl, sizeof(EMRPOLYLINE), (size_t) 1, EMH_RECORD);
	emh_write(aptl, sizeof(POINTL), (size_t) cpt, EMH_DATA);
    }
}


/* Draws arc arrow ending at p and starting at q. */
static void arc_arrow(p, q, arw, arc)
    F_pos	*p;
    F_point	*q;
    F_arrow	*arw;
    F_arc	*arc;
{
    F_point P;
    Dir dir; double d;

    if (!arw) return;

    pos2point(&P, p);
    direction(&P, q, &dir, &d);
    arrow(&P, arw, (F_line *)arc, &dir);
}


/* Replaces p by the starting point of the arc arrow ending at p. */
static void arc_arrow_adjust(p, arwpos, arc, r, arw, adir)
    F_point *p;		/* in: end of the arc, out: fixed end of arc */
    F_point *arwpos;	/* out: origin of arrow */
    F_arc *arc;
    double r;
    F_arrow *arw;
    double adir;
{
    double l;
    double alen, th;
    Dir dir; double dirlen;
    double d2, d3;
    F_point origpos, p1, p2, p3;
    struct dpoint {
	double x, y;
    } p4;
    double X, Y, d;
    double cx, cy;

    if (!arw) return;

    cx = arc->center.x; cy = arc->center.y;	/* arc center */
    origpos = *p;	/* save original arc end (to be used later) */

    l = arw->type != 0 ? arrow_length(arw) : arw->ht;
    arc_rotate(p, cx, cy, adir * l / (1.8 * r + l));

    /* length of arrow head */
    alen = arw->ht;
    if (!rounded_arrows)
	alen += ARROW_EXTRA_LEN(arw);

    /*
     * fix arrow root pos
     */

    /* if the arrow is longer than the diameter, no way to fix arrow root pos */
    if (alen >= 2 * r) {
	*arwpos = *p;	/* use the fixed arc end */
	goto fix_cap;
    }

    /* make a chord with the arc and the center line of the arrow head */
    th = 2 * atan(sqrt(1 / (4 * r * r / (alen * alen) - 1)));
    *arwpos = origpos;
    arc_rotate(arwpos, cx, cy, adir * th);

    /*
     * fix arc end pos
     */

    /* direction from arow root to arrow top */
    if (direction(&origpos, arwpos, &dir, &dirlen) == False)
	goto fix_cap;	/* no way to fix */

    /* Calculate the center of the line of arrow top. */
    p1 = origpos;
    if (!rounded_arrows) {
	d = ARROW_EXTRA_LEN(arw);
	p1.x = round(p1.x - d * dir.x);
	p1.y = round(p1.y - d * dir.y);
    }

    /* Calculate other points of the arrow head. */
    d = p1.x - arw->ht * dir.x + arw->wid*0.5 * dir.y; p2.x = round(d);
    d = p1.y - arw->ht * dir.y - arw->wid*0.5 * dir.x; p2.y = round(d);
    d = p1.x - arw->ht * dir.x - arw->wid*0.5 * dir.y; p3.x = round(d);
    d = p1.y - arw->ht * dir.y + arw->wid*0.5 * dir.x; p3.y = round(d);

    /* Calculate distance from the center of the arc to the points. */
    d2 = distance(cx, cy, (double) p2.x, (double) p2.y);
    d3 = distance(cx, cy, (double) p3.x, (double) p3.y);

    /* Use the farther point. */
    if (d3 > d2) {
	p2 = p3;
	d2 = d3;
    }

    /* The farther point shall be out of the arc. */
    if (d2 < r)
	goto fix_cap;

    /* direction from p1 to p2 */
    if (direction(&p2, &p1, &dir, &dirlen) == False)
	goto fix_cap;

    /*
     * Drop a perpendicular line from the arc center
     * to the line on which p1 and p2 are.
     */
    /*
     * The line of the arrow head is
     *	x = p2.x + t * dir.x,
     *	y = p2.y + t * dir.y,  where t is an intermediate parameter.
     *
     * The distance^2 from the center of the arc to a point on the line is
     * (x - cx)^2 + (y - cy)^2 = (dir.x^2 + dir.y^2) * t^2
     *		+ 2*((p2.x - cx) * dir.x + (p2.y - cy) * dir.y) * t
     *		+ (p2.x - cx)^2 + (p2.y - cy)^2
     *	= t^2 + 2*(X * dir.x + Y * dir.y) * t + X^2 + Y^2
     *			where	dir.x^2 + dir.y^2 = 1 (dir is a unit vector),
     *				X = p2.x - cx, Y = p2.y - cy
     *	= t^2 + 2*(X * dir.x + Y * dir.y) * t + (X * dir.x + Y * dir.y)^2
     *		- (X * dir.x + Y * dir.y)^2
     *		+ X^2 + Y^2
     *	= (t + (X * dir.x + Y * dir.y))^2
     *		- X^2*dir.x^2 - 2*X*Y*dir.x*dir.y - Y^2*dir.y^2
     *		+ X^2*(dir.x^2 + dir.y^2) + Y^2*(dir.x^2 + dir.y^2)
     *			where	dir.x^2 + dir.y^2 = 1
     *	= (t + (X * dir.x + Y * dir.y))^2
     *		+ X^2*dir.y^2 - 2*X*Y*dir.x*dir.y + Y^2*dir.x^2
     *	= (t + (X * dir.x + Y * dir.y))^2 + (X * dir.y - Y * dir.x)^2
     */
    X = p2.x - cx;
    Y = p2.y - cy;

    /* Calculate the pedal point of the perpendicular line. */
    d = X * dir.x + Y * dir.y;
    p4.x = p2.x - d * dir.x;
    p4.y = p2.y - d * dir.y;

    /* and the length^2 of the perpendicular line segment is */
    d = X * dir.y - Y * dir.x;
    d = d * d;

    /* length from p4 to the target point */
    d = sqrt(r * r - d);

    /* Calculate target point. */
    p4.x = p4.x + d * dir.x;
    p4.y = p4.y + d * dir.y;

    p->x = round(p4.x);
    p->y = round(p4.y);

fix_cap:
    if (arc->cap_style == 2) {
	/* Projecting cap style is a little longer.  Shorten the arc. */
	arc_rotate(p, cx, cy, adir * (double)arc->thickness / (2.0 * r));
    }
}


static double arc_radius(a)
    F_arc *a;
{

    return (distance((double)a->point[0].x, (double)a->point[0].y,
			a->center.x, a->center.y) +
	    distance((double)a->point[1].x, (double)a->point[1].y,
			a->center.x, a->center.y) +
	    distance((double)a->point[2].x, (double)a->point[2].y,
			a->center.x, a->center.y)) / 3.;
}


static void translate(x, y, tx, ty)
    double *x, *y, tx, ty;
{

    *x += tx; *y += ty;
}/* end translate */


/* Rotates counter clockwise around origin */
static void rotate(x, y, angle)
    double *x, *y, angle;
{
    double s = sin(angle), c = cos(angle), xt = *x, yt = *y;

    *x =  c * xt + s * yt;
    *y = -s * xt + c * yt;
}/* end rotate */


/* Rotates the point p counter clockwise along the arc with center c. */
static void arc_rotate(p, cx, cy, angle)
    F_point *p;
    double cx, cy, angle;
{
    double x = p->x, y = p->y;

    translate(&x, &y, -cx, -cy);
    rotate(&x, &y, angle);
    translate(&x, &y, +cx, +cy);
    p->x = round(x); p->y = round(y);
}


static void arcoutline(a)
    F_arc *a;
{
    EMRARC em_ar;
    double r = arc_radius(a);
    F_point p0, p2;
    F_point arwp0, arwp2;
    int i;

    pos2point(&p0, &a->point[0]);
    pos2point(&p2, &a->point[2]);
    if (a->type == T_OPEN_ARC && a->thickness != 0) {
	/* adjust for arrow heads */
	arc_arrow_adjust(&p0, &arwp0, a, r, a->back_arrow, +1.);
	arc_arrow_adjust(&p2, &arwp2, a, r, a->for_arrow, -1.);

	/* calculate inner product */
	if ((a->point[0].x - a->point[2].x) * (p0.x - p2.x) +
	    (a->point[0].y - a->point[2].y) * (p0.y - p2.y) < 0) {
	    /*
	     * This is probably because the arc is very short and hidden
	     * in the arrow heads.  Skip drawing the arc in this case.
	     */
	    goto draw_arrows;
	}
    }

    i = (a->type == T_OPEN_ARC) ? EMR_ARC : EMR_PIE;
    em_ar.emr.iType = htofl(i);
    em_ar.emr.nSize = htofl(sizeof(EMRARC));
    HTOFL(em_ar.rclBox.left,   round(a->center.x - r));
    HTOFL(em_ar.rclBox.top,    round(a->center.y - r));
    HTOFL(em_ar.rclBox.right,  round(a->center.x + r));
    HTOFL(em_ar.rclBox.bottom, round(a->center.y + r));
    em_ar.ptlStart.x = htofl(p0.x);
    em_ar.ptlStart.y = htofl(p0.y);
    em_ar.ptlEnd.x = htofl(p2.x);
    em_ar.ptlEnd.y = htofl(p2.y);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Arc outline rclBox (ltrb): %d %d %d %d\n",
	ftohl(em_ar.rclBox.left),  ftohl(em_ar.rclBox.top),
	ftohl(em_ar.rclBox.right), ftohl(em_ar.rclBox.bottom));
# endif

    emh_write(&em_ar, sizeof(EMRARC), (size_t) 1, EMH_RECORD);

draw_arrows:
    if (a->type == T_OPEN_ARC && a->thickness != 0) {
	/* draw arrow heads */
	arc_arrow(&a->point[0], &arwp0, a->back_arrow, a);
	arc_arrow(&a->point[2], &arwp2, a->for_arrow, a);
    }
}


static void arcinterior(a)
    F_arc *a;
{
    EMRCHORD em_ch;
    double r = arc_radius(a);

    em_ch.emr.iType = htofl(EMR_CHORD);
    em_ch.emr.nSize = htofl(sizeof(EMRCHORD));
    HTOFL(em_ch.rclBox.left,   round(a->center.x - r));
    HTOFL(em_ch.rclBox.top,    round(a->center.y - r));
    HTOFL(em_ch.rclBox.right,  round(a->center.x + r));
    HTOFL(em_ch.rclBox.bottom, round(a->center.y + r));
    em_ch.ptlStart.x = htofl(a->point[0].x);
    em_ch.ptlStart.y = htofl(a->point[0].y);
    em_ch.ptlEnd.x = htofl(a->point[2].x);
    em_ch.ptlEnd.y = htofl(a->point[2].y);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Arc interior rclBox (ltrb): %d %d %d %d\n",
	ftohl(em_ch.rclBox.left),  ftohl(em_ch.rclBox.top),
	ftohl(em_ch.rclBox.right), ftohl(em_ch.rclBox.bottom));
# endif

    emh_write(&em_ch, sizeof(EMRCHORD), (size_t) 1, EMH_RECORD);
}/* end arcinterior */


/* Reverses arc direction by swapping endpoints and arrows */
static void arc_reverse(arc)
    F_arc *arc;
{
    F_arrow *arw;
    F_pos pp;

    pp = arc->point[0]; arc->point[0] = arc->point[2]; arc->point[2] = pp;
    arw = arc->for_arrow;
    arc->for_arrow = arc->back_arrow;
    arc->back_arrow = arw;
}


static void circle(e)
    F_ellipse *e;
{
    EMRELLIPSE em_el;

    em_el.emr.iType = htofl(EMR_ELLIPSE);
    em_el.emr.nSize = htofl(sizeof(EMRELLIPSE));
    em_el.rclBox.left   = htofl(e->center.x - e->radiuses.x);
    em_el.rclBox.top    = htofl(e->center.y - e->radiuses.x);
    em_el.rclBox.right  = htofl(e->center.x + e->radiuses.x);
    em_el.rclBox.bottom = htofl(e->center.y + e->radiuses.x);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Circle rclBox (ltrb): %d %d %d %d\n",
	ftohl(em_el.rclBox.left),  ftohl(em_el.rclBox.top),
	ftohl(em_el.rclBox.right), ftohl(em_el.rclBox.bottom));
# endif

    emh_write(&em_el, sizeof(EMRELLIPSE), (size_t) 1, EMH_RECORD);
}/* end circle */


/* non-rotated ellipses */
static void ellipse(e)
    F_ellipse *e;
{
    EMRELLIPSE em_el;

    em_el.emr.iType = htofl(EMR_ELLIPSE);
    em_el.emr.nSize = htofl(sizeof(EMRELLIPSE));
    em_el.rclBox.left   = htofl(e->center.x - e->radiuses.x);
    em_el.rclBox.top    = htofl(e->center.y - e->radiuses.y);
    em_el.rclBox.right  = htofl(e->center.x + e->radiuses.x);
    em_el.rclBox.bottom = htofl(e->center.y + e->radiuses.y);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Ellipse rclBox (ltrb): %d %d %d %d\n",
	ftohl(em_el.rclBox.left),  ftohl(em_el.rclBox.top),
	ftohl(em_el.rclBox.right), ftohl(em_el.rclBox.bottom));
# endif

    emh_write(&em_el, sizeof(EMRELLIPSE), (size_t) 1, EMH_RECORD);
}/* end ellipse */


/* draw rotated ellipse as a polygon */
#define ELLIPSE_NPOINT	72
static void rotated_ellipse(e)
    F_ellipse *e;
{
    F_line l;
    F_point pnt[ELLIPSE_NPOINT];
    int i;
    const double delta = 2 * M_PI / (double)ELLIPSE_NPOINT;
    double th;
    double sina = sin(e->angle), cosa = cos(e->angle);
    double ex, ey;

    for (i = 0; i < ELLIPSE_NPOINT; i++) {
	th = delta * i;
	ex = (double) e->radiuses.x * cos(th);
	ey = (double) e->radiuses.y * sin(th);
	pnt[i].x = e->center.x + (int) (cosa * ex + sina * ey);
	pnt[i].y = e->center.y + (int) (-sina * ex + cosa * ey);
	pnt[i].next = &pnt[i + 1];
    }
    pnt[ELLIPSE_NPOINT - 1].next = NULL;

    /*
     * fabricate polygon structure
     */

    /* copy common fields */

    l.style = e->style;
    l.thickness = e->thickness;
    l.pen_color = e->pen_color;
    l.fill_color = e->fill_color;
    l.depth = e->depth;
    l.pen = e->pen;
    l.fill_style = e->fill_style;
    l.style_val = e->style_val;

    /* setup other fields */
    l.points = pnt;
    l.type = T_POLYGON;

    /* just in case... */
    l.for_arrow = l.back_arrow = NULL;
    l.cap_style = 0;
    l.join_style = 0;
    l.radius = 0;
    l.pic = NULL;
    l.comments = NULL;
    l.next = NULL;

    shape(&l, 0, 0, polygon);
}/* end rotated_ellipse */


/* Integer cross product */
static int icprod(x1, y1, x2, y2)
    int x1, y1, x2, y2;
{

    return x1 * y2 - y1 * x2;
}/* end icprod */


/* Returns True if the arc is a clockwise arc. */
static int cwarc(a)
    F_arc *a;
{
    int x1 = a->point[1].x - a->point[0].x,
	y1 = a->point[1].y - a->point[0].y,
	x2 = a->point[2].x - a->point[1].x,
	y2 = a->point[2].y - a->point[1].y;

    return (icprod(x1,y1,x2,y2) > 0);
}/* end cwarc */


/* Computes distance and normalized direction vector from q to p.
 * returns True if the points do not coincide and False if they do. */
static int direction(p, q, dir, dist)
    F_point	*p;
    F_point	*q;
    Dir		*dir;
    double	*dist;
{

    dir->x = p->x - q->x;
    dir->y = p->y - q->y;
    *dist = sqrt((dir->x) * (dir->x) + (dir->y) * (dir->y));
    if (*dist < EPSILON)
	return False;
    dir->x /= *dist;
    dir->y /= *dist;
    return True;
}/* end direction */


static double distance(x1, y1, x2, y2)
    double x1, y1, x2, y2;
{
    double dx = x2-x1, dy=y2-y1;

    return sqrt(dx*dx + dy*dy);
}/* end distance */


/* Write an enhanced metarecord and keep track of number of bytes written
 * and number of records written in the global header record "emh". */
static size_t emh_write(ptr, size, nmemb, flag)
    const void *ptr;
    size_t	size;
    size_t	nmemb;
    emh_flag	flag;
{

    if (flag == EMH_RECORD) emh_nRecords++;
    emh_nBytes += size * nmemb;
    return fwrite(ptr, size, nmemb, tfp);
}/* end emh_write */


static int is_flip(rot, flip)
    int rot, flip;
{

    return (rot == 90 || rot == 270) ? !flip : flip;
}


static void encode_bitmap(pic, bpp, rot)
    F_pic *pic;
    int bpp, rot;
{
    int img_w = pic->bit_size.x;
    int img_h = pic->bit_size.y;
    int flip;
    int i, initi, endi, diri;
    int j, initj, endj, dirj;
    int pos;
    int freebits = 32;
    unsigned bits = 0;
    unsigned char cbits[4];
    unsigned u;

    /*
     * Note: on EMF, you must output the image
     * from left to right and FROM BOTTOM TO TOP.
     */
    switch (rot) {
    case 0:	diri =  1; dirj = -1; break;
    case 90:	diri =  1; dirj =  1; break;
    case 180:	diri = -1; dirj =  1; break;
    case 270:	diri = -1; dirj = -1; break;
    default:
	fprintf(stderr, "fig2dev: emf: pic rotation %d is not supported\n",
		rot);
	return;
	/*NOTREACHED*/
	break;
    }

    if (pic->flipped && (rot == 0 || rot == 180)) {
	diri = -diri;
	dirj = -dirj;
    }

    flip = is_flip(rot, pic->flipped);

    if (diri < 0) {
	initi = img_w - 1; endi = 0;
    } else {
	initi = 0; endi = img_w - 1;
    }
    if (dirj < 0) {
	initj = img_h - 1; endj = 0;
    } else {
	initj = 0; endj = img_h - 1;
    }

#   define SWAP(v1, v2) do { int tmp = v1; v1 = v2; v2 = tmp; } while (0)
    if (flip) {
	SWAP(initi, initj);
	SWAP(endi, endj);
	SWAP(diri, dirj);
    }
#   undef SWAP

#   define WRITEBITS	\
	    do {/* MSB first */						\
		cbits[0] = bits >> 24; cbits[1] = bits >> 16;		\
		cbits[2] = bits >> 8; cbits[3] = bits;			\
		emh_write(cbits, (size_t) 4, (size_t) 1, EMH_DATA);	\
		freebits = 32;						\
		bits = 0;						\
	    } while (0)

    endi += diri;
    endj += dirj;
    for (j = initj; j != endj; j += dirj) {
	for (i = initi; i != endi; i += diri) {
	    /*
	     * normal: x: i, y: j
	     * flip:   x: j, y: i
	     */
	    pos = flip ? i * img_w + j : j * img_w + i;

	    switch (bpp) {
	    case 1: case 4: case 8:
		freebits -= bpp;
		bits |= pic->bitmap[pos] << freebits;
		if (freebits == 0)
		    WRITEBITS;
		break;
	    case 24:
		for (u = 0; u < 3; u++) {
		    freebits -= 8;
		    bits |= pic->bitmap[pos*3 + u] << freebits;
		    if (freebits == 0)
			WRITEBITS;
		}
		break;
	    default:
		/* should not happen */
		fprintf(stderr, "unsupported bpp %d\n", bpp);
		break;
	    }
	}
	/* size of a line of EMF bitmap must be a multiple of 4 byte */
	if (freebits != 32)
	    WRITEBITS;
    }
#   undef WRITEBITS
}


static void picbox(l)
    F_line *l;
{
    EMRSTRETCHDIBITS em_sd;
    BITMAPINFO bmi;
    char buf[512], realname[PATH_MAX];
    int filtype;
    int dx, dy;
    FILE *picf;
    int pllx, plly;
    int img_w, img_h;
    int rotation;
    size_t bsize, coltabsize;
    int bpp;
    unsigned ncol;
    static RGBQUAD coltab[256];
    unsigned u;
    int flip;

    dx = l->points->next->next->x - l->points->x;
    dy = l->points->next->next->y - l->points->y;

    /* rotation (counter clockwise) */
    rotation = 0;
    if (dx < 0 && dy < 0)
       rotation = 180;
    else if (dx < 0 && dy >= 0)
       rotation = 270;
    else if (dy < 0 && dx >= 0)
       rotation = 90;

    if ((picf = open_picfile(l->pic->file, &filtype, True, realname)) == NULL) {
	fprintf(stderr, "fig2dev: %s: No such picture file\n", l->pic->file);
	return;
    }
    if (fread(buf, (size_t) 16, (size_t) 1, picf) != 1) {
	fprintf(stderr, "fig2dev: %s: short read\n", l->pic->file);
	close_picfile(picf, filtype);
	return;
    }
    close_picfile(picf, filtype);

    memset(&em_sd, 0, sizeof(EMRSTRETCHDIBITS));
    em_sd.emr.iType = htofl(EMR_STRETCHDIBITS);

    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = htofl(sizeof(BITMAPINFOHEADER));

#ifdef USE_PNG
    if (strncmp(buf, "\211\120\116\107\015\012\032\012", (size_t) 8) == 0) {
	/* png file */
	if ((picf = open_picfile(l->pic->file, &filtype, True, realname)) == NULL) {
	    perror(l->pic->file);
	    return;
	}
	if (read_png(picf, filtype, l->pic, &pllx, &plly) == 0) {
	    fprintf(stderr, "fig2dev: %s: illegal format\n", l->pic->file);
	    close_picfile(picf, filtype);
	    return;
	}
	close_picfile(picf, filtype);
    }
#endif
    if (l->pic->subtype == P_GIF || l->pic->subtype == P_PCX ||
	l->pic->subtype == P_JPEG || l->pic->subtype == P_PNG) {

	img_w = l->pic->bit_size.x;
	img_h = l->pic->bit_size.y;
	ncol = l->pic->numcols;

	flip = is_flip(rotation, l->pic->flipped);

	/* EMF bitmap requires the every scan line be a multiple of 4 byte */
	if (ncol <= 2) {
	    bpp = 1;
	    bsize = (flip ? (img_h + 31) / 32 * img_w :
			    (img_w + 31) / 32 * img_h) * 4;
	    coltabsize = 2 * sizeof(RGBQUAD);
	} else if (ncol <= 16) {
	    bpp = 4;
	    bsize = (flip ? (img_h + 7) / 8 * img_w :
			    (img_w + 7) / 8 * img_h) * 4;
	    coltabsize = ncol * sizeof(RGBQUAD);
	} else if (ncol <= 256) {
	    bpp = 8;
	    bsize = (flip ? (img_h + 3) / 4 * img_w :
			    (img_w + 3) / 4 * img_h) * 4;
	    coltabsize = ncol * sizeof(RGBQUAD);
	} else {
	    bpp = 24;
	    bsize = (flip ? (img_h * 3 + 3) / 4 * img_w :
			    (img_w * 3 + 3) / 4 * img_h) * 4;
	    coltabsize = 0;
	}

# ifdef __EMF_DEBUG__
	fprintf(stderr, "PicBox: %s: %u cols, %d bpp, bsize %u, img(%d,%d), fig(%d,%d)\n",
	    l->pic->file, ncol, bpp, bsize, img_w, img_h, dx, dy);
# endif

	if (dx >= 0) {
	    em_sd.xDest = em_sd.rclBounds.left = htofl(l->points->x);
	    em_sd.rclBounds.right = htofl(l->points->next->next->x);
	    em_sd.cxDest = htofl(dx);
	} else {
	    em_sd.xDest = em_sd.rclBounds.left =htofl(l->points->next->next->x);
	    em_sd.rclBounds.right = htofl(l->points->x);
	    em_sd.cxDest = htofl(-dx);
	}
	if (dy >= 0) {
	    em_sd.yDest = em_sd.rclBounds.top = htofl(l->points->y);
	    em_sd.rclBounds.bottom = htofl(l->points->next->next->y);
	    em_sd.cyDest = htofl(dy);
	} else {
	    em_sd.yDest = em_sd.rclBounds.top = htofl(l->points->next->next->y);
	    em_sd.rclBounds.bottom = htofl(l->points->y);
	    em_sd.cyDest = htofl(-dy);
	}

	/* em_sd.xSrc = em_sd.ySrc = htofl(0);		already cleared */

	if (flip) {
	    bmi.bmiHeader.biWidth  = em_sd.cxSrc = htofl(img_h);
	    bmi.bmiHeader.biHeight = em_sd.cySrc = htofl(img_w);
	} else {
	    bmi.bmiHeader.biWidth  = em_sd.cxSrc = htofl(img_w);
	    bmi.bmiHeader.biHeight = em_sd.cySrc = htofl(img_h);
	}

	/* em_sd.iUsageSrc = htofl(DIB_RGB_COLORS);	this is default */
	em_sd.dwRop = htofl(SRCCOPY);

	em_sd.offBmiSrc = htofl(sizeof(EMRSTRETCHDIBITS));
	em_sd.cbBmiSrc = htofl(sizeof(BITMAPINFO) + coltabsize);
	em_sd.offBitsSrc = htofl(sizeof(EMRSTRETCHDIBITS) +
				 sizeof(BITMAPINFO) + coltabsize);

	bmi.bmiHeader.biSizeImage = em_sd.cbBitsSrc = htofl(bsize);

	em_sd.emr.nSize = htofl(sizeof(EMRSTRETCHDIBITS) + sizeof(BITMAPINFO) +
				coltabsize + bsize);

	bmi.bmiHeader.biPlanes = htofs(1);
	bmi.bmiHeader.biBitCount = htofs(bpp);
	bmi.bmiHeader.biCompression = htofl(BI_RGB);	/* uncompressed */
	bmi.bmiHeader.biXPelsPerMeter =
		bmi.bmiHeader.biYPelsPerMeter = htofl(2953);	/* 75dpi ? */
	if (bpp <= 8)
	    bmi.bmiHeader.biClrUsed = htofl(ncol);
	/* bmi.bmiHeader.biClrImportant = htofl(0); */

	emh_write(&em_sd, sizeof(EMRSTRETCHDIBITS), (size_t) 1, EMH_RECORD);
	emh_write(&bmi, sizeof(BITMAPINFO), (size_t) 1, EMH_DATA);

	/* color table */
	if (coltabsize) {
	    coltab[0].rgbBlue = coltab[0].rgbGreen = coltab[0].rgbRed = 0;
	    coltab[1].rgbBlue = coltab[1].rgbGreen = coltab[1].rgbRed = 0;
	    for (u = 0; u < ncol; u++) {
		coltab[u].rgbBlue  = l->pic->cmap[BLUE][u];
		coltab[u].rgbGreen = l->pic->cmap[GREEN][u];
		coltab[u].rgbRed   = l->pic->cmap[RED][u];
	    }
	    emh_write(coltab, coltabsize, (size_t) 1, EMH_DATA);
	}

	/* bitmap */
	encode_bitmap(l->pic, bpp, rotation);

	/* need 4 byte align, but the size of EMF bitmap is 4 byte aligned */
    } else {
	fprintf(stderr, "fig2dev: %s: emf: unsupported picture format\n",
	    l->pic->file);
	return;
    }
}/* end picbox */


/* Draws polygon boundary */
static void polygon(l)
    F_line *l;
{
    F_point *p;
    int count;
    EMRPOLYGON em_pg;	/* Polygon in little endian format */
    POINTL *aptl;
    POINTS *apts;
    int bbx_top, bbx_bottom, bbx_left, bbx_right;	/* Bounding box */
    unsigned  cpt;	/* Number of points in the array */

    /* Calculate the number of points and the bounding box. */
    if (!(p = l->points)) return;
    bbx_left = p->x;
    bbx_top  = p->y;
    bbx_right  = p->x;
    bbx_bottom = p->y;
    for (cpt = 0; p; p = p->next) {
	UPDATE_BBX_X(p->x);
	UPDATE_BBX_Y(p->y);
	cpt++;
    }

    /* Windows 95/98/Me: maximum points allowed is approx. 1360 */
    if (cpt > 1360 && emflevel != EMF_LEVEL_WINNT)
	fprintf(stderr, "Warning: polygon has too many points -- may be partially invisible\n");

    /* Store bounding box in little endian format */
    em_pg.cptl = htofl(cpt);
    em_pg.rclBounds.left = htofl(bbx_left);
    em_pg.rclBounds.top  = htofl(bbx_top);
    em_pg.rclBounds.right  = htofl(bbx_right);
    em_pg.rclBounds.bottom = htofl(bbx_bottom);

    if (bbx_left >= -32768 && bbx_right <= 32767
	&& bbx_top >= -32768 && bbx_bottom <= 32767) {

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Polygon16 %d rclBounds (ltrb): %d %d %d %d\n",
	cpt,
	bbx_left,  bbx_top, bbx_right, bbx_bottom);
# endif

	/* use 16bit point */
	/* Fill the array with the points of the polygon */
	if ((apts = malloc(cpt * sizeof(POINTS))) == NULL) {
	    perror("fig2dev: malloc");
	    exit(1);
	}
	for (p=l->points, count=0; p; p=p->next, count++) {
	    apts[count].x = htofs(p->x);
	    apts[count].y = htofs(p->y);
	}

	em_pg.emr.iType = htofl(EMR_POLYGON16);
	em_pg.emr.nSize = htofl(sizeof(EMRPOLYGON16) + cpt * sizeof(POINTS));

	emh_write(&em_pg, sizeof(EMRPOLYGON16), (size_t) 1, EMH_RECORD);
	emh_write(apts, sizeof(POINTS), (size_t) cpt, EMH_DATA);

	free(apts);
    } else {
# ifdef __EMF_DEBUG__
    fprintf(stderr, "Polygon %d rclBounds (ltrb): %d %d %d %d\n",
	cpt,
	bbx_left,  bbx_top, bbx_right, bbx_bottom);
# endif

	/* use 32bit point */
	warn_32bit_pos();

	/* Fill the array with the points of the polygon */
	if ((aptl = malloc(cpt * sizeof(POINTL))) == NULL) {
	    perror("fig2dev: malloc");
	    exit(1);
	}
	for (p=l->points, count=0; p; p=p->next, count++) {
	    aptl[count].x = htofl(p->x);
	    aptl[count].y = htofl(p->y);
	}

	em_pg.emr.iType = htofl(EMR_POLYGON);
	em_pg.emr.nSize = htofl(sizeof(EMRPOLYGON) + cpt * sizeof(POINTL));

	emh_write(&em_pg, sizeof(EMRPOLYGON), (size_t) 1, EMH_RECORD);
	emh_write(aptl, sizeof(POINTL), (size_t) cpt, EMH_DATA);

	free(aptl);
    }
}/* end polygon */


/* Replaces p by the shortened starting point of the line segment. */
static int polyline_adjust(p, q, l)
    F_point *p;
    F_point *q;
    double l;
{
    double d; Dir dir;

    if (direction(p, q, &dir, &d)) {
	p->x = round(p->x - l * dir.x);
	p->y = round(p->y - l * dir.y);
	return (l < d);
    }
    fprintf(stderr, "Warning: Arrow at zero-length line segment omitted.\n");
    return True;
}/* end polyline_arrow_adjust */


/* Draws polyline boundary (with arrows if needed) */
static void polyline(l)
    F_line *l;
{
    F_point *p, *q, p0, pn;
    Dir dir;
    double d;
    EMRPOLYLINE em_pl;	/* Polyline in little endian format */
    POINTL *aptl;
    POINTS *apts;
    double alen, seglen;

    int bbx_top, bbx_bottom, bbx_left, bbx_right;	/* Bounding box */
    unsigned cpt;	/* Number of points in the array */
    unsigned u;

    /* Calculate the number of points and the bounding box. */
    if (!(p = l->points)) return;
    bbx_left = p->x;
    bbx_top  = p->y;
    bbx_right  = p->x;
    bbx_bottom = p->y;
    for (cpt = 0, q = p; p; q = p, p = p->next) {
	UPDATE_BBX_X(p->x);
	UPDATE_BBX_Y(p->y);
	cpt++;
    }
    p0 = *l->points;	/* first point */
    pn = *q;		/* last point */

    if (cpt == 1) {
	/* Draw single point as a short line. */
	cpt = 2;
	pn.y++;
    } else {
	/*
	 * Delete line segments which may overlap arrow heads.
	 */
	if (l->back_arrow) {		/* First point with arrow */
	    alen = arrow_length(l->back_arrow);
	    while (cpt > 1) {
		q = p0.next;
		seglen = distance((double)p0.x, (double)p0.y, (double)q->x, (double)q->y);
		if (seglen > alen) {
		    break;
		} else {
		    /* delete this segment */
		    cpt--;
		    p0 = *q;
		    alen -= seglen;
		}
	    }
	    if (cpt > 1)
		polyline_adjust(&p0, p0.next, alen); /* shorten line segment */
	}
	if (l->for_arrow) {	/* Last point with arrow */
	    alen = arrow_length(l->for_arrow);
	    while (cpt > 1) {
		for (p = &p0, u = cpt; --u > 0; q = p, p = p->next)
		    ;
		seglen = distance((double)p->x, (double)p->y, (double)q->x, (double)q->y);
		if (seglen > alen) {
		    break;
		} else {
		    /* delete this segment */
		    cpt--;
		    pn = *q;
		    alen -= seglen;
		}
	    }
	    if (cpt > 1)
		polyline_adjust(&pn, q, alen);	/* shorten line segment */
	}
	if (cpt <= 1)		/* if all line segments are removed, */
	    goto draw_arrows;	/* skip drawing line segments */
    }

    /* Windows 95/98/Me: maximum points allowed is approx. 1360 */
    if (cpt > 1360 && emflevel != EMF_LEVEL_WINNT)
	fprintf(stderr, "Warning: polyline has too many points -- may be partially invisible\n");

    /* Store bounding box in little endian format */
    em_pl.cptl = htofl(cpt);
    em_pl.rclBounds.left = htofl(bbx_left);
    em_pl.rclBounds.top  = htofl(bbx_top);
    em_pl.rclBounds.right  = htofl(bbx_right);
    em_pl.rclBounds.bottom = htofl(bbx_bottom);

    if (bbx_left >= -32768 && bbx_right <= 32767
	&& bbx_top >= -32768 && bbx_bottom <= 32767) {

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Polyline16 %d rclBounds (ltrb): %d %d %d %d\n",
	cpt,
	bbx_left,  bbx_top, bbx_right, bbx_bottom);
# endif

	/* use 16bit point */
	/* Fill the array with the points of the polyline */
	if ((apts = malloc(cpt * sizeof(POINTS))) == NULL) {
	    perror("fig2dev: malloc");
	    exit(1);
	}
	for (p = &p0, u = 0; u + 1 < cpt; p= p->next, u++) {
	    apts[u].x = htofs(p->x);
	    apts[u].y = htofs(p->y);
	}
	apts[u].x = htofs(pn.x);
	apts[u].y = htofs(pn.y);

	em_pl.emr.iType = htofl(EMR_POLYLINE16);
	HTOFL(em_pl.emr.nSize, sizeof(EMRPOLYLINE16) + cpt * sizeof(POINTS));

	emh_write(&em_pl, sizeof(EMRPOLYLINE16), (size_t) 1, EMH_RECORD);
	emh_write(apts, sizeof(POINTS), (size_t) cpt, EMH_DATA);
	free(apts);
    } else {

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Polyline %d rclBounds (ltrb): %d %d %d %d\n",
	cpt,
	bbx_left,  bbx_top, bbx_right, bbx_bottom);
# endif

	/* use 32bit point */
	warn_32bit_pos();

	/* Fill the array with the points of the polyline */
	if ((aptl = malloc(cpt * sizeof(POINTL))) == NULL) {
	    perror("fig2dev: malloc");
	    exit(1);
	}
	for (p = &p0, u = 0; u + 1 < cpt; p= p->next, u++) {
	    aptl[u].x = htofl(p->x);
	    aptl[u].y = htofl(p->y);
	}
	aptl[u].x = htofl(pn.x);
	aptl[u].y = htofl(pn.y);

	em_pl.emr.iType = htofl(EMR_POLYLINE);
	HTOFL(em_pl.emr.nSize, sizeof(EMRPOLYLINE) + cpt * sizeof(POINTL));

	emh_write(&em_pl, sizeof(EMRPOLYLINE), (size_t) 1, EMH_RECORD);
	emh_write(aptl, sizeof(POINTL), (size_t) cpt, EMH_DATA);
	free(aptl);
    }

draw_arrows:
    if (!l->points->next) {
	if (l->for_arrow || l->back_arrow)
	    fprintf(stderr, "Warning: Arrow at zero-length line segment omitted.\n");
	return;
    }		/* At least two different points now */


    if (l->back_arrow) {
	p = l->points;
	q = l->points->next;
	if (direction(p, q, &dir, &d)) {
	    arrow(p, l->back_arrow, l, &dir);
	}
    }

    if (l->for_arrow) {
	for (q=l->points, p=l->points->next; p->next; q=p, p=p->next) {}
	/* q is the one but last point */
	if (direction(p, q, &dir, &d)) {
	    arrow(p, l->for_arrow, l, &dir);
	}
    }
}/* end polyline */


static void rect(l)
    F_line *l;
{
    EMRRECTANGLE em_rt;

    if (!l->points || !l->points->next || !l->points->next->next) {
	fprintf(stderr, "Warning: Invalid fig box omitted.\n");
	return;
    }

    em_rt.emr.iType = htofl(EMR_RECTANGLE);
    em_rt.emr.nSize = htofl(sizeof(EMRRECTANGLE));
    em_rt.rclBox.left = htofl(l->points->x);
    em_rt.rclBox.top  = htofl(l->points->y);
    em_rt.rclBox.right  = htofl(l->points->next->next->x);
    em_rt.rclBox.bottom = htofl(l->points->next->next->y);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Rectangle rclBox (ltrb): %d %d %d %d\n",
	ftohl(em_rt.rclBox.left), ftohl(em_rt.rclBox.top),
	ftohl(em_rt.rclBox.right), ftohl(em_rt.rclBox.bottom));
# endif

    emh_write(&em_rt, sizeof(EMRRECTANGLE), (size_t) 1, EMH_RECORD);
}/* end rect */


static void roundrect(l)
    F_line *l;
{
    EMRROUNDRECT em_rr;

    if (!l->points || !l->points->next || !l->points->next->next) {
	fprintf(stderr, "Warning: Invalid fig box omitted.\n");
	return;
    }

    em_rr.emr.iType = htofl(EMR_ROUNDRECT);
    em_rr.emr.nSize = htofl(sizeof(EMRROUNDRECT));
    em_rr.rclBox.left = htofl(l->points->x);
    em_rr.rclBox.top  = htofl(l->points->y);
    em_rr.rclBox.right  = htofl(l->points->next->next->x);
    em_rr.rclBox.bottom = htofl(l->points->next->next->y);
    em_rr.szlCorner.cx = em_rr.szlCorner.cy = htofl(l->radius * 2);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "RoundRectangle rclBox (ltrb): %d %d %d %d\n",
	ftohl(em_rr.rclBox.left), ftohl(em_rr.rclBox.top),
	ftohl(em_rr.rclBox.right), ftohl(em_rr.rclBox.bottom));
# endif

    emh_write(&em_rr, sizeof(EMRROUNDRECT), (size_t) 1, EMH_RECORD);
}


/* Draws interior and outline of a simple closed shape.  Cannot be
 * used for polylines and arcs (open shapes) or for arcboxes
 * (closed but not a simple emf primitive). */
static void shape(l, join, cap, drawshape)
    F_line *l;
    int join;		/* join style */
    int cap;		/* cap style */
    void (*drawshape)(F_line *);
{

    edgeattr(1, l->style, l->thickness, l->pen_color, join, cap);
    fillstyle(l->fill_color, l->fill_style, l->pen_color);
    drawshape(l);
}/* end shape */


/* Draws interior of a closed shape, taking into account fill color
 * and pattern.  Boundary must be drawn separately.  Used for
 * polylines, arcboxes and arcs. */
static void shape_interior(l, drawshape)
    F_line *l;
    void (*drawshape)(F_line *);
{

    if (l->fill_style >= 0) {
	edgevis(0);				/* Don't draw edges */
	fillstyle(l->fill_color, l->fill_style, l->pen_color);
	drawshape(l);
    }
}/* end shape_interior */


/* Converts regular strings to unicode strings.  If the utext pointer is
 * null, memory is allocated.  Note, that carriage returns are converted
 * to nulls. */
static void textunicode(str, n_chars, utext, n_unicode)
    char   *str;
    int    *n_chars;
    short  **utext;		/* If *utext is null, memory is allocated */
    int    *n_unicode;
{
#if defined(I18N) && defined(USE_ICONV)
    iconv_t icd = (iconv_t) -1;
    const char *src;
    char *dst, *p;
    size_t srcleft, srccnt, dstleft, dstcnt;
    int i;

    if (figLanguage != LANG_DEFAULT) {
	if ((icd = iconv_open(EMF_ICONV_CODESET, iconvCharset[figLanguage])) ==
		(iconv_t) -1)
	    perror("genemf: iconv");
    }

    src = str;
    srcleft = srccnt = strlen(str);
    if ((dst = (char *) *utext) == NULL) {
	dst = (char *) (*utext = calloc(srcleft + 2/*null termination + align*/,
	    sizeof(short)));
	dstleft = dstcnt = srcleft * sizeof(short);
    } else
	dstleft = dstcnt = *n_unicode;

    if (icd != (iconv_t) -1) {
	if (iconv(icd, &src, &srcleft, &dst, &dstleft) == (size_t) -1)
	    fprintf(stderr, "genemf: iconv: illegal byte sequence\n");

	(void) iconv_close(icd);

	*n_chars = srccnt - srcleft;
	*n_unicode = dstcnt - dstleft;

	/* Convert '\n' to 0 */
	p = (void *)*utext;
	for (i = 0; i < *n_unicode; i += 2) {
#ifdef EMF_ICONV_NEED_SWAP
	    /* swap byte order */
	    char c = p[i];
	    p[i] = p[i + 1];
	    p[i + 1] = c;
#endif
	    if (p[i] == '\n' && p[i+1] == 0)
		p[i] = 0;
	}
    } else {
	*n_chars = srccnt;
	*n_unicode = srccnt * 2;
	for (i = 0; i < srccnt; i++) {
	    if (str[i] == '\n')
		(*utext)[i] = 0;
	    else
		(*utext)[i] = htofs(str[i]);
	}
    }
#else	/* !(defined(I18N) && defined(USE_ICONV)) */
    int    i;

    *n_chars = strlen(str);
    *n_unicode = *n_chars * 2;
    if (*utext == NULL) {
	*utext = (short *)calloc((size_t) *n_unicode + 4 /* null + align */,
	    (size_t) 1);
    }
    for (i = 0; i < *n_chars; i++) {		/* Convert to unicode. */
	if (str[i] == '\n') {
	    (*utext)[i] = 0;
	} else {
	    (*utext)[i] = htofs(str[i]);
	}
    }
#endif
}/* end textunicode */


static void text(x, y, bbx, str)
    int     x, y;		/* reference point */
    RECTL  *bbx;		/* bounding box */
    char   *str;
{
    int    n_chars, n_unicode;
    short	 *utext = NULL;
    EMREXTTEXTOUTW em_tx;	/* Text structure in little endian format */

    memset(&em_tx, 0, sizeof(EMREXTTEXTOUTW));
    em_tx.emr.iType = htofl(EMR_EXTTEXTOUTW);

    em_tx.rclBounds = *bbx;

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Textout |%s| rclBounds (ltrb): %d %d %d %d\n",
	str,
	ftohl(em_tx.rclBounds.left),  ftohl(em_tx.rclBounds.top),
	ftohl(em_tx.rclBounds.right), ftohl(em_tx.rclBounds.bottom));
# endif

    em_tx.iGraphicsMode = htofl(GM_COMPATIBLE);
    em_tx.exScale.bit_uint = htofl(PIXEL_01MM_BITPATTERN);
    em_tx.eyScale.bit_uint = htofl(PIXEL_01MM_BITPATTERN);

    em_tx.emrtext.ptlReference.x = htofl(x);
    em_tx.emrtext.ptlReference.y = htofl(y);

    textunicode(str, &n_chars, &utext, &n_unicode);

    em_tx.emrtext.nChars = htofl(n_unicode / 2); /* number of unicode chars */
    em_tx.emrtext.offString = htofl(sizeof(EMREXTTEXTOUTW));

    n_unicode = (n_unicode + 4) & ~3;	/* null termination + align */
    em_tx.emr.nSize = htofl(sizeof(EMREXTTEXTOUTW) + n_unicode);

    emh_write(&em_tx, sizeof(EMREXTTEXTOUTW), (size_t) 1, EMH_RECORD);
    emh_write(utext, (size_t) 1, (size_t) n_unicode, EMH_DATA);

    free(utext);
}/* end text */


static void textcolr(color)
    int color;
{
    static int oldcolor = UNDEFVALUE;
    EMRSETTEXTCOLOR em_tc;

    bkmode(TRANSPARENT);		/* fig doesn't have text background */
    chkcache(color, oldcolor);

    memset(&em_tc, 0, sizeof(EMRSETTEXTCOLOR));
    em_tc.emr.iType = htofl(EMR_SETTEXTCOLOR);
    em_tc.emr.nSize = htofl(sizeof(EMRSETTEXTCOLOR));
    HTOFL(em_tc.crColor, conv_color(color));

# ifdef __EMF_DEBUG__
    fprintf(stderr, "SetTextColor (rgb): %x\n", ftohl(em_tc.crColor));
# endif

    emh_write(&em_tc, sizeof(EMRSETTEXTCOLOR), (size_t) 1, EMH_RECORD);
}/* end textcolr */


static void textalign(align)
    int align;
{
    static int oldalign = TA_LEFT|TA_TOP|TA_NOUPDATECP;	/* startup default */
    EMRSETTEXTALIGN em_ta;

    chkcache(align, oldalign);
    em_ta.emr.iType = htofl(EMR_SETTEXTALIGN);
    em_ta.emr.nSize = htofl(sizeof(EMRSETTEXTALIGN));
    em_ta.iMode = htofl(align);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "SetTextAlign (mode): 0x%x\n", align);
# endif

    emh_write(&em_ta, sizeof(EMRSETTEXTALIGN), (size_t) 1, EMH_RECORD);
}


#if defined(I18N) && defined(USE_ICONV)
static void moveto(x, y)
    int x, y;
{
    EMRMOVETOEX em_mv;

    em_mv.emr.iType = htofl(EMR_MOVETOEX);
    em_mv.emr.nSize = htofl(sizeof(EMRMOVETOEX));
    em_mv.ptl.x = htofl(x);
    em_mv.ptl.y = htofl(y);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "MoveTo (x, y): %d, %d\n", x, y);
# endif

    emh_write(&em_mv, sizeof(EMRMOVETOEX), (size_t) 1, EMH_RECORD);
}
#endif	/* defined(I18N) && defined(USE_ICONV) */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_option(opt, optarg)
    char	 opt;
    char	*optarg;
{

    rounded_arrows = False;
    switch (opt) {
    case 'r':
	rounded_arrows = True;
	break;

    case 'l':
	if (strcasecmp(optarg, "win95") == 0)
	    emflevel = EMF_LEVEL_WIN95;
	else if (strcasecmp(optarg, "win98") == 0)
	    emflevel = EMF_LEVEL_WIN98;
	else if (strcasecmp(optarg, "winnt") == 0)
	    emflevel = EMF_LEVEL_WINNT;
	else
	    fprintf(stderr, "warning: unknown level %s ignored\n", optarg);
	break;

    case 'f':	/* Ignore magnification, font sizes and lang here */
    case 'm':
    case 's':
    case 'L':
	break;

    default:
	put_msg(Err_badarg, opt, "emf");
	exit(1);
    }
}/* end genemf_option */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_start(objects)
     F_compound	*objects;
{
    EMRSETMAPMODE em_sm;
    short *uni_description = NULL;	/* Unitext description. */
    int n_chars, n_unicode;
    F_comment comm, *p;
    size_t commlen;
    short *ud;
    char *figname;

#if defined(I18N) && defined(USE_ICONV)
    if (support_i18n) {
	char *locale;

	locale = getenv("LANG");
	if (locale == NULL) {
	    fprintf(stderr, "fig2dev: LANG not defined; assuming C locale\n");
	    locale = "C";
	}
	if (strncmp(locale, "zh_CN", (size_t) 5) == 0)
	    figLanguage = LANG_ZH_CN;
	else if (strncmp(locale, "zh_TW", (size_t) 5) == 0)
	    figLanguage = LANG_ZH_TW;
	else if (strncmp(locale, "ja", (size_t) 2) == 0)
	    figLanguage = LANG_JA;
	else if (strncmp(locale, "ko", (size_t) 2) == 0)
	    figLanguage = LANG_KO;
    }
#endif

    memset(&emh, 0, sizeof(ENHMETAHEADER));
    emh.iType = htofl(EMR_HEADER);

    emh.dSignature = htofl(ENHMETA_SIGNATURE);
    emh.nVersion = htofl(ENHMETA_VERSION);
    emh_nHandles = 0;
    handles = NULL;
    latesthandle = (void *) &handles;

    memset(lasthandle, 0, sizeof(lasthandle));	/* Initialize the DC handles */

    /* Create a description string. */

    if (from)
	figname=from;
    else
	figname=strdup("stdin");

    comm.next = objects->comments;
    if ((comm.comment = malloc(strlen(figname) +
		80 + sizeof VERSION + sizeof PATCHLEVEL)) == NULL) {
	perror("fig2dev: malloc");
	exit(1);
    }
    sprintf(comm.comment,
	"Converted from %s using fig2dev %s.%s for %s",
       figname, VERSION, PATCHLEVEL, emflevelname[emflevel]);

    /* prescan comment strings and (over)estimate the total length */
    commlen = 0;
    for (p = &comm; p; p = p->next)
	commlen += strlen(p->comment) + 1;

    commlen = commlen * 2 + 2;	/* 2byte/char (+ align) */
    if ((uni_description = malloc(commlen)) == NULL) {
	perror("fig2dev: malloc");
	exit(1);
    }

    /* convert comment strings to unicode */
    ud = uni_description;
    for (p = &comm; p; p = p->next) {
# ifdef __EMF_DEBUG__
	fprintf(stderr, "%s\n", p->comment);
# endif
	n_unicode = commlen;
	textunicode(p->comment, &n_chars, &ud, &n_unicode);
	commlen -= n_unicode;
	ud += n_unicode/2;
	*ud++ = 0;
    }
    *ud = 0;	/* for align */
    n_unicode = (char *)ud - (char *)uni_description;	/* length in byte */

    emh.nDescription = htofl(n_unicode / 2);
    emh.offDescription = htofl(sizeof(ENHMETAHEADER));

    /* The size of the reference device in pixels.  We will define a pixel
     * in xfig uints of 1200 dots per inch and we'll use an 250 x 200 mm
     * or 10.24 x 8.46 inch viewing area. */

    emh.szlMillimeters.cx = htofl(260);		/* 10.24 inches wide. */
    emh.szlMillimeters.cy = htofl(215);		/*  8.46 inches high. */

    HTOFL(emh.szlDevice.cx, round(260 * 47.244094));
    HTOFL(emh.szlDevice.cy, round(215 * 47.244094));

    /* Inclusive-inclusive bounds in device units (1200 dpi) and
     * 0.01 mm units.  Add 1/20 inch all the way around. */

    emh.rclBounds.left   = htofl(llx - 48);
    emh.rclBounds.top    = htofl(lly - 48);
    emh.rclBounds.right  = htofl(urx + 48);
    emh.rclBounds.bottom = htofl(ury + 48);

    HTOFL(emh.rclFrame.left,  floor((llx - 48) * PIXEL_01MM));
    HTOFL(emh.rclFrame.top,   floor((lly - 48) * PIXEL_01MM));
    HTOFL(emh.rclFrame.right,  ceil((urx + 48) * PIXEL_01MM));
    HTOFL(emh.rclFrame.bottom, ceil((ury + 48) * PIXEL_01MM));

# ifdef __EMF_DEBUG__
    fprintf(stderr, "rclBounds (ltrb): %d %d %d %d\n",
	ftohl(emh.rclBounds.left),  ftohl(emh.rclBounds.top),
	ftohl(emh.rclBounds.right), ftohl(emh.rclBounds.bottom));
    fprintf(stderr, "rclFrame  (ltrb): %d %d %d %d\n",
	ftohl(emh.rclFrame.left),  ftohl(emh.rclFrame.top),
	ftohl(emh.rclFrame.right), ftohl(emh.rclFrame.bottom));
# endif

    n_unicode = (n_unicode + 3) & ~3;	/* null termination + align */
    emh.nSize = htofl(sizeof(ENHMETAHEADER) + n_unicode);

    emh_write(&emh, sizeof(ENHMETAHEADER), (size_t) 1, EMH_RECORD);
    emh_write(uni_description, (size_t) 1, (size_t) n_unicode, EMH_DATA);

    /* The SetMapMode function sets the mapping mode of the specified
     * device context.  The mapping mode defines the unit of measure used
     * to transform page-space units into device-space units, and also
     * defines the orientation of the device's x and y axes. */
    em_sm.emr.iType = htofl(EMR_SETMAPMODE);
    em_sm.emr.nSize = htofl(sizeof(EMRSETMAPMODE));

    /* MM_TEXT Each logical unit is mapped to one device pixel.  Positive
     * x is to the right; positive y is down.  The MM_TEXT mode allows
     * applications to work in device pixels, whose size varies from device
     * to device. */
    em_sm.iMode = htofl(MM_TEXT);
# ifdef __EMF_DEBUG__
    fprintf(stderr, "SetMapMode: %d\n", ftohl(em_sm.iMode));
# endif
    emh_write(&em_sm, sizeof(EMRSETMAPMODE), (size_t) 1, EMH_RECORD);

    free(comm.comment);
    free(uni_description);
}/* end genemf_start */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_arc(_a)
    F_arc *_a;
{
    F_arc a = *_a;

    if (cwarc(&a))
	arc_reverse(&a);	/* make counter clockwise arc */

    switch (a.type) {
    case T_OPEN_ARC:
	/* open arcs should be drawn separately */
	shape_interior((F_line *)(void *)&a, (void (*)(F_line *))arcinterior);
	if (a.thickness > 0) {
	    edgeattr(1, a.style, a.thickness, a.pen_color, 0, a.cap_style);
	    arcoutline(&a);
	}
	break;
    case T_PIE_WEDGE_ARC:
	shape((F_line *)(void *)&a, a.cap_style, 0,
	      (void (*)(F_line *))arcoutline);
	break;
    default:
	fprintf(stderr, "Unsupported fig arc type %d.\n", a.type);
    }
}/* end genemf_arc */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_ellipse(e)
    F_ellipse *e;
{

    switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
	if (fabs(e->angle) > EPSILON && fabs(sin(e->angle)) > EPSILON)
	    rotated_ellipse(e);
	else
	    shape((F_line *)e, 0, 0, (void (*)(F_line *))ellipse);
	break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
	shape((F_line *)e, 0, 0, (void (*)(F_line *))circle);
	break;
    default:
	fprintf(stderr, "Unsupported FIG ellipse type %d.\n", e->type);
    }
}/* end genemf_ellipse */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_line(l)
    F_line *l;
{

    switch (l->type) {
    case T_POLYLINE:
	shape_interior(l, polygon);		/* draw interior */

	if (l->points->next == NULL	/* single point line */
	    && l->cap_style == 0) {	/* butt style */
	    /* draw as projecting style but in smaller size */
	    edgeattr(1, l->style, (l->thickness + 1) / 2, l->pen_color,
		     l->join_style, 2);
	} else {
	    edgeattr(1, l->style, l->thickness, l->pen_color,
		     l->join_style, l->cap_style);
	}

	polyline(l);			/* draw boundary */
	break;
    case T_BOX:
	shape(l, l->join_style, l->cap_style, rect); /* simple closed shape */
	break;
    case T_POLYGON:
	shape(l, l->join_style, l->cap_style, polygon);
	break;
    case T_ARC_BOX:
	shape(l, 0, 0, roundrect);		/* simple closed shape */
	break;
    case T_PIC_BOX:
	picbox(l);
	break;
    default:
	fprintf(stderr, "Unsupported FIG polyline type %d.\n", l->type);
	return;
    }
}/* end genemf_line */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*ARGSUSED*/
void genemf_spline(s)
    F_spline *s;
{
    static int wgiv = 0;

    if (!wgiv) {
	fprintf(stderr, "\
Warning: the EMF driver doesn't support (old style) FIG splines.\n\
Suggestion: convert your (old?) FIG image by loading it into xfig v3.2 or\n\
or higher and saving again.\n");
	wgiv = 1;
    }
}/* end genemf_spline */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_text(t)
    F_text *t;
{
    int font;
    RECTL rclBounds;
    int x, y;
    double sin_theta, cos_theta;
#if defined(I18N) && defined(USE_ICONV)
    unsigned char *s, *s1, bak;
    int nascii, neuc;
#endif

    textcolr(t->color);
    font = conv_fontindex(t->font, t->flags);	/* Convert the font index */

    /*
     * calculate start position
     */
    x = t->base_x;
    y = t->base_y;
    sin_theta = sin(t->angle);
    cos_theta = cos(t->angle);
    switch (t->type) {
    case T_LEFT_JUSTIFIED:
	break;
    case T_CENTER_JUSTIFIED:
	x = x + (-(t->length/2.0) * cos_theta);
	y = y + ((t->length/2.0) * sin_theta);
	break;
    case T_RIGHT_JUSTIFIED:
	x = x + (-t->length * cos_theta);
	y = y + (t->length * sin_theta);
	break;
    default:
	fprintf(stderr, "Unsupported fig text type %d.\n", t->type);
    }

    HTOFL(rclBounds.left,   round(x - t->size * sin_theta));
    HTOFL(rclBounds.top,    round(y - t->size * cos_theta));
    HTOFL(rclBounds.right,  round(x + t->length * cos_theta));
    HTOFL(rclBounds.bottom, round(y + t->length * sin_theta));

#if defined(I18N) && defined(USE_ICONV)
    if (figLanguage != LANG_DEFAULT && IS_LOCALE_FONT(font)) {
	/* prescan text to decide if font switching is needed */
	nascii = neuc = 0;
	for (s = (void *) t->cstring; *s; s++) {
	    if (*s > 127)
		neuc++;
	    else
		nascii++;
	}

	if (neuc == 0)
	    ;		/* ASCII only */
	else if (nascii == 0) {
	    /* locale font only */
	    font = (font == FONT_TIMES_ROMAN) ?
		FONT_LOCALE_ROMAN : FONT_LOCALE_BOLD;
	} else {
	    /*
	     * need font switching
	     */

	    /*
	     * use home grown alignment
	     */
	    /* use and update current position */
	    textalign(TA_LEFT | TA_BASELINE | TA_UPDATECP);

	    /* move to the first (left) position */
	    moveto(x, y);

	    for (s = (void *) t->cstring; *s; s = s1) {
		if (*s & 0x80) {
		    /* EUC 2byte char */
		    for (s1 = s; *s1 & 0x80; s1 += 2)
			;
		    textfont((font == FONT_TIMES_ROMAN) ?
			    FONT_LOCALE_ROMAN : FONT_LOCALE_BOLD,
			t->size, round(t->angle * RAD_01DEG));
		} else {
		    /* ascii */
		    for (s1 = s; *s1 && (*s1 & 0x80) == 0; s1++)
			;
		    textfont(font, t->size, round(t->angle * RAD_01DEG));
		}
		bak = *s1;
		*s1 = '\0';

		text(0, 0, &rclBounds, s);

		*s1 = bak;
	    }
	    return;
	}
    }
#endif	/* defined(I18N) && defined(USE_ICONV) */

    /*
     * Use alignment of EMF, which shall be better than that of home grown.
     * Do not use current position.
     */
    switch (t->type) {
    case T_LEFT_JUSTIFIED:
	textalign(TA_LEFT | TA_BASELINE | TA_NOUPDATECP);
	break;
    case T_CENTER_JUSTIFIED:
	textalign(TA_CENTER | TA_BASELINE | TA_NOUPDATECP);
	break;
    case T_RIGHT_JUSTIFIED:
	textalign(TA_RIGHT | TA_BASELINE | TA_NOUPDATECP);
	break;
    }

    textfont(font, t->size, round(t->angle * RAD_01DEG));
    text(t->base_x, t->base_y, &rclBounds, t->cstring);
}/* end genemf_text */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int genemf_end()
{
    EMREOF em_eof;
    struct emfhandle *h, *h1;

    /* delete all created handles */
    for (h = handles; h; h = h1) {
	delete_handle(h);
	h1 = h->next;
	free(h);
    }

    memset(&em_eof, 0, sizeof(EMREOF));
    em_eof.emr.iType = htofl(EMR_EOF);
    em_eof.emr.nSize = htofl(sizeof(EMREOF));
    em_eof.offPalEntries = htofl(sizeof(EMREOF) - sizeof(EMFulong));
    em_eof.nSizeLast = htofl(sizeof(EMREOF));
    emh_write(&em_eof, sizeof(EMREOF), (size_t) 1, EMH_RECORD);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Metafile Bytes: %d    Records: %d    Handles: %d\n",
	emh_nBytes, emh_nRecords, emh_nHandles);
# endif

    /* Rewrite the updated header record at the beginning of the file. */

    emh.nBytes   = htofl(emh_nBytes);
    emh.nRecords = htofl(emh_nRecords);
    emh_nHandles++;	/* size of handle table is last handle number + 1 */
    emh.nHandles = htofs(emh_nHandles);

    if (fseek(tfp, 0L, SEEK_SET) < 0) {
	fprintf(stderr,"\
fig2dev: error: fseek() failed.  EMF language requires the output is seekable.\
\n\t\tOutput to a plain file, not to a pipe.\n");
	return -1;
    }
    if (fwrite(&emh, (size_t) 1, sizeof(ENHMETAHEADER), tfp)
	    != sizeof(ENHMETAHEADER) || fseek(tfp, 0L, SEEK_END) < 0) {
	fprintf(stderr,"fig2dev: error: failed to write EMF header\n");
	return -1;
    }

    /* all ok */
    return 0;
}/* end genemf_end */


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
struct driver dev_emf = {
	genemf_option,
	genemf_start,
	gendev_null,		/* TODO - Create genemf_grid for 3.2.4 */
	genemf_arc,
	genemf_ellipse,
	genemf_line,
	genemf_spline,
	genemf_text,
	genemf_end,
	INCLUDE_TEXT
};

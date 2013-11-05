/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Copyright (c) 1988 by Conrad Kwok
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
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
 * genepic.c: (E)EPIC driver for fig2dev
 *
 * Converted from fig2epic 5/89 by Micah Beck
 */
/*==================================================================*/
/*	fig2epic (Fig to EPIC converter) 			    */
/*	     Version 1.1d <March 30, 1988>			    */
/*								    */
/*	Written by Conrad Kwok, Division of Computer Science, UCD   */
/*								    */
/*	Permission is granted for freely distribution of this file  */
/*		provided that this message is included.		    */
/*==================================================================*/

/*====================================================================
  Changes:

  Brian Smith
  - add \smash back to text objects
  Gabriel Zachmann <Gabriel.Zachmann@gmx.net>:
  - remove \smash in text objects
  - add option -F; this allows you to define the font in the latex file
	before you \input the eepic file (except the font size).
	You must not mix eepic files generated with and without the -F option.
	However, I think this could be improved easily, I just don't speak TeX
	well enough to feel confident enough to change the way \SetFigFont
	is defined.
  - add option -R: allows for rotated text.

  Version 3.1.2: <Aug 17, 1995>
  Changes from Andre Eickler (eickler@db.fmi.uni-passau.de)
  1. Line thicknesses did not get scaled correctly. This was especially a
	problem with dotted lines. 
  2. Dashlines in Eepic do not work as documented. I included an option "-t"
	to set the stretch of the dashlines. By default, it is set to a
	reasonable value (30, before it defaulted to 0, which made dashlines
	look very sparse).
  3. Arc boxes are now implemented.
  4. New arrow types implemented

  Known problems:
	In the fig2dev documentation you should add a recommendation not to use
	  the -w option of the driver. It does not work due to a bug in Eepic.
	Polylines/Boxes cannot be shaded and have a dotted/dashed border
	  simultaneously. In that case only the shading is used.
	Arc Boxes can only be used with Eepic and Eepic_emu. They can only be
	  solid/unfilled. In every other case, an ordinary box is used.

  Version 1.1d: <March 30, 1989>
  1. Add supports for Gould NP1 (Bsd4.3) (Recieve the changes from
		mcvax!presto.irisa.fr!hleroy@uunet.uu.net. Sorry
		I don't have his/her name)
  2. Add exit(0) before exit in the main.

  Version 1.1c: <Febrary 7, 1989>
  1. Supports all 5 gray level in area fill.

  Version 1.1b: <Febrary 2, 1989>
  1. Draw outline for area-filled figures when using dvips.

  Version 1.1a: <January 18, 1989>
  1. Fix the bug in closed control splines. The routine convertCS(p) is being
     called once too often.

  2. Add supports to Variable line width
  3. Add supports to black, white or shaded area fill.

  Version 1.0d:<September 18, 1988>
  1. Add the option -P for Page mode. Two more configurable parameter---
     Preamble and Postamble.

====================================================================*/

  
#include "fig2dev.h"
#include "object.h"
#include "setfigfont.h"
#include "texfonts.h"

extern float	THICK_SCALE;	/* ratio of dpi/80 */
extern Boolean	FontSizeOnly;	/* defined in setfigfont.c */

#define DrawOutLine
#ifdef DrawOutLine
int OutLine=0;
#endif

#define TopCoord 840		/* 10.5 in * 80 (DPI)            */
				/* Actually, it can be any value */
#define PtPerLine 3
#define ThinLines 0
#define ThickLines 1
#define Epic 0
#define EEpic_emu 1
#define EEpic 2
#define BoxTypeNone 0
#define SolidLineBox 1
#define DashLineBox 2
#define BothBoxType 3
#define Normal 0
#define Economic 1
#define DottedDash 2

static void genepic_ctl_spline(); 
static void genepic_open_spline();
static void genepic_closed_spline(); 
static void quadratic_spline();
static void bezier_spline();
static void arc_arrow();
static void draw_arrow_head();
static void set_style();
static void genepic_itp_spline();
static void quadratic_spline();
static void chaikin_curve();
static void rtop();
static void drawarc();
static void fdraw_arrow_head();
static char *FillCommands();

/* Structure for Point with "double" values */
struct fp_struct {
    double x,y;
};

typedef struct fp_struct FPoint;

/* Local to the file only */
static int	encoding;
static double	Threshold;
static Boolean	linew_spec = False;
static int	CurWidth = 0;
static int	LineStyle = SOLID_LINE;
static int	LLX = 0, LLY = 0;
static char	*LnCmd;
static int	MaxCircleRadius;
static double	DashLen;
static int	PageMode = False;
static int	PatternType=UNFILLED;
static int	PatternColor=WHITE_COLOR;
static struct {
    double mag;
    int size;
} ScaleTbl[5] = {
    { 0.6667, 8 },
    { 0.75  , 9 },
    { 0.8333, 10 },
    { 0.9167, 11 },
    { 1.0   , 12 }
};

/* Definition of Keywords for some of the configurable parameter */
char *Tlangkw[] = { /* The order must match the definition of corr. constants */
    "Epic", "EEpicemu", "EEpic", NULL
};

char *EllCmdkw[] = {
    "ellipse", "oval", NULL
};

char *EllCmdstr[] = {
    "\\%s%s{%d}{%d}}\n", "\\%s%s(%d,%d)}\n"
};

/* Shading that is used instead of hatchings */
#define DEFAULT_SHADING 8
char *FillCommands();

#define TEXT_LINE_SEP '\n'
/* The following two arrays are used to translate characters which
   are special to LaTeX into characters that print as one would expect.
   Note that the <> characters aren't really special LaTeX characters
   but they will not print as themselves unless one is using a font
   like tt. */
char latex_text_specials[] = "\\{}><^~$&#_%";
char *latex_text_mappings[] = {
  "$\\backslash$",
  "$\\{$",
  "$\\}$",
  "$>$",
  "$<$",
  "\\^{}",
  "\\~{}",
  "\\$",
  "\\&",
  "\\#",
  "\\_",
  "\\%"};


/* Configurable parameters */
int	LowerLeftX=0, LowerLeftY=0;
double	SegLen = 0.0625; /* inch */
int	Verbose = False;
int	TopMargin = 5;
int	BottomMargin = 10;
int	DotDist = 5;
int	LineThick = 0;	/* first use as flag to say whether user specified -l */
int	TeXLang = EEpic;
double	DashScale;
int	EllipseCmd=0;
int	UseBox=BoxTypeNone;
int	DashType=Normal;
char	*Preamble="\\documentstyle[epic,eepic]{article}\n\\pagestyle{empty}\\begin{document}\n\\begin{center}\n";
char	*Postamble="\\end{center}\n\\end{document}\n";
int	VarWidth=False;
int	DashStretch=30;
double	ArrowScale=1.0;
int	AllowRotatedText = 0;


void
genepic_option(opt, optarg)
char opt, *optarg;
{
  	int loop, i;

        linew_spec = False;
	FontSizeOnly = False;

        switch (opt) {

	  case 's':
	  case 'm':
	    break;

	  case 'A':
	    ArrowScale = atof(optarg);
	    break;

	  case 'a':
	    fprintf(stderr, "warning: genepic option -a obsolete\n");
	    break;

	  case 'E':
	    encoding = atoi(optarg);
	    if (encoding < 0 || encoding > 2)
	      encoding = 1;
	    break;

	  case 'f':
	    for ( i = 1; i <= MAX_FONT; i++ )
		if ( !strcmp(optarg, texfontnames[i]) ) break;

	    if ( i > MAX_FONT) {
		fprintf(stderr, "warning: non-standard font name %s ignored\n", optarg);
	    } else {
		texfontnames[0] = texfontnames[i];
#ifdef NFSS
		texfontfamily[0] = texfontfamily[i];
		texfontseries[0] = texfontseries[i];
		texfontshape[0] = texfontshape[i];
#endif
	    }
	    break;

          case 'l':
	    linew_spec = True;
            LineThick = atoi(optarg);	/* save user's argument here */
            break;

	  case 'L':
	    for (loop=0; loop < 3; loop++) {
	    	if (strcasecmp(optarg, Tlangkw[loop]) == 0) break;
	    }
	    TeXLang = loop;
	    break;

	  case 'P':
	    PageMode = 1;
	    break;

          case 'S':
            loop = atoi(optarg);
            if (loop < 8 || loop > 12) {
            	put_msg("Scale must be between 8 and 12 inclusively\n");
            	exit(1);
            }
            loop -= 8;
            mag = ScaleTbl[loop].mag;
            font_size = (double) ScaleTbl[loop].size;
            break;

          case 'v':
            Verbose = True;
            break;

	  case 'W':
	  case 'w':
	    VarWidth = opt=='W';
	    break;

	  case 't':
	    DashStretch= atoi(optarg);
	    if (DashStretch < -100)
	      DashStretch = -100;
	    break;

	  case 'F':
	    FontSizeOnly = True;
	    break;

	  case 'R':
	    AllowRotatedText = 1;
	    break;

	  default:
	    put_msg(Err_badarg, opt, "epic");
	    exit(1);
        }
}

static void
fconvertCS(fpt)
FPoint *fpt;
{
    fpt->y = TopCoord - fpt->y;
    fpt->x -= LLX;
    fpt->y -= LLY;
}

static void
convertCS(pt)
F_point *pt;
{
    pt->y = TopCoord - pt->y;
    pt->x -= LLX;
    pt->y -= LLY;
}

void
genepic_start(objects)
F_compound *objects;
{
    int temp;
    F_point pt1, pt2;

    texfontsizes[0] = texfontsizes[1] =
		TEXFONTSIZE(font_size != 0.0? font_size : DEFAULT_FONT_SIZE);

    /* print any whole-figure comments prefixed with "%" */
    if (objects->comments) {
	fprintf(tfp,"%%\n");
	print_comments("% ",objects->comments,"");
	fprintf(tfp,"%%\n");
    }

    switch (TeXLang) {
      case Epic:
        EllipseCmd = 1; /* Oval */
        LnCmd = "drawline";
        break;
      case EEpic_emu:
      case EEpic:
        LnCmd = "path";
        break;
      default:
        put_msg("Program error in main\n");
        break;
    }
    if (PageMode) {
        fputs(Preamble, stdout);
    }

    if (linew_spec)
	LineThick = LineThick * ppi/80.0;
    if (LineThick == 0)
	LineThick = 2.0*ppi/80.0;
    DashScale = ppi/80.0;
    pt1.x = llx;
    pt1.y = lly;
    pt2.x = urx;
    pt2.y = ury;
    convertCS(&pt1);
    convertCS(&pt2);
    if (pt1.x > pt2.x) {
        temp = pt1.x;
        pt1.x = pt2.x;
        pt2.x = temp;
    }
    if (pt1.y > pt2.y) {
        temp = pt1.y;
        pt1.y = pt2.y;
        pt2.y = temp;
    }
    LLX = pt1.x - LowerLeftX;
    LLY = pt1.y - LowerLeftY;
    if (Verbose) {
        fprintf(tfp, "%%\n%% Language in use is %s\n%%\n", Tlangkw[TeXLang]);
    }
    Threshold = 1.0 / ppi * mag;
    fprintf(tfp, "\\setlength{\\unitlength}{%.8fin}\n", Threshold);
    MaxCircleRadius = (int) (40 / 72.27 / Threshold);
    Threshold = SegLen / Threshold;
    define_setfigfont(tfp);
    if (DashStretch)
      fprintf(tfp, "{\\renewcommand{\\dashlinestretch}{%d}\n", DashStretch);
    fprintf(tfp, "\\begin{picture}(%d,%d)(%d,%d)\n",
           pt2.x-pt1.x, pt2.y-pt1.y + TopMargin + BottomMargin,
           LowerLeftX, LowerLeftY-BottomMargin);
}

int
genepic_end()
{
    fprintf(tfp, "\\end{picture}\n");
    if (DashStretch)
      fprintf(tfp, "}\n");
    if (PageMode)
        fputs(Postamble, stdout);

    /* all ok */
    return 0;
}

static void
set_linewidth(w)
int w;
{
    int old_width;

    if (w < 0) return;
    old_width=CurWidth;
    CurWidth = VarWidth ? w : ((w >= LineThick) ? ThickLines : ThinLines);
    if (old_width != CurWidth) {
	if (CurWidth==ThinLines) {
	    fprintf(tfp, "\\thinlines\n");
	} else if (VarWidth) {
	    fprintf(tfp, "\\allinethickness{%4.3fpt}%%\n",w*80.0/ppi);
	} else {
	    fprintf(tfp, "\\thicklines\n");
	}
    }
}

static void
set_pattern(type, color)
int type, color;
{
    static unsigned long patterns[][32] = {

      /* shading data */
      {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
      }, {
	0x00000000, 0x00000000, 0x00000000, 0x00888888, 
	0x88000000, 0x00000000, 0x00000000, 0x00080808, 
	0x08000000, 0x00000000, 0x00000000, 0x00888888, 
	0x88000000, 0x00000000, 0x00000000, 0x00080808, 
	0x08000000, 0x00000000, 0x00000000, 0x00888888, 
	0x88000000, 0x00000000, 0x00000000, 0x00080808, 
	0x08000000, 0x00000000, 0x00000000, 0x00888888, 
	0x88000000, 0x00000000, 0x00000000, 0x00080808, 
      }, {
	0x08101010, 0x10000000, 0x00444444, 0x44000000, 
	0x00011101, 0x11000000, 0x00444444, 0x44000000, 
	0x00101010, 0x10000000, 0x00444444, 0x44000000, 
	0x00010101, 0x01000000, 0x00444444, 0x44000000, 
	0x00101010, 0x10000000, 0x00444444, 0x44000000, 
	0x00011101, 0x11000000, 0x00444444, 0x44000000, 
	0x00101010, 0x10000000, 0x00444444, 0x44000000, 
	0x00010101, 0x01000000, 0x00444444, 0x44000000, 
      }, {
	0x00000000, 0x00115111, 0x51000000, 0x00444444, 
	0x44000000, 0x00151515, 0x15000000, 0x00444444, 
	0x44000000, 0x00511151, 0x11000000, 0x00444444, 
	0x44000000, 0x00151515, 0x15000000, 0x00444444, 
	0x44000000, 0x00115111, 0x51000000, 0x00444444, 
	0x44000000, 0x00151515, 0x15000000, 0x00444444, 
	0x44000000, 0x00511151, 0x11000000, 0x00444444, 
	0x44000000, 0x00151515, 0x15000000, 0x00444444, 
      }, {
	0x44000000, 0x00aaaaaa, 0xaa000000, 0x008a888a, 
	0x88000000, 0x00aaaaaa, 0xaa000000, 0x00888888, 
	0x88000000, 0x00aaaaaa, 0xaa000000, 0x008a8a8a, 
	0x8a000000, 0x00aaaaaa, 0xaa000000, 0x00888888, 
	0x88000000, 0x00aaaaaa, 0xaa000000, 0x008a888a, 
	0x88000000, 0x00aaaaaa, 0xaa000000, 0x00888888, 
	0x88000000, 0x00aaaaaa, 0xaa000000, 0x008a8a8a, 
	0x8a000000, 0x00aaaaaa, 0xaa000000, 0x00888888, 
      }, {
	0x88555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
	0x00555555, 0x55000000, 0x00555555, 0x55000000, 
      }, {
	0x00555555, 0x55000000, 0x00555555, 0x55888888, 
	0x88555555, 0x55000000, 0x00555555, 0x55808080, 
	0x80555555, 0x55000000, 0x00555555, 0x55888888, 
	0x88555555, 0x55000000, 0x00555555, 0x55888088, 
	0x80555555, 0x55000000, 0x00555555, 0x55888888, 
	0x88555555, 0x55000000, 0x00555555, 0x55808080, 
	0x80555555, 0x55000000, 0x00555555, 0x55888888, 
	0x88555555, 0x55000000, 0x00555555, 0x55888088, 
      }, {
	0x80222222, 0x22555555, 0x55808080, 0x80555555, 
	0x55222222, 0x22555555, 0x55880888, 0x08555555, 
	0x55222222, 0x22555555, 0x55808080, 0x80555555, 
	0x55222222, 0x22555555, 0x55080808, 0x08555555, 
	0x55222222, 0x22555555, 0x55808080, 0x80555555, 
	0x55222222, 0x22555555, 0x55880888, 0x08555555, 
	0x55222222, 0x22555555, 0x55808080, 0x80555555, 
	0x55222222, 0x22555555, 0x55080808, 0x08555555, 
      }, {
	0x55888888, 0x88555555, 0x5522a222, 0xa2555555, 
	0x55888888, 0x88555555, 0x552a2a2a, 0x2a555555, 
	0x55888888, 0x88555555, 0x55a222a2, 0x22555555, 
	0x55888888, 0x88555555, 0x552a2a2a, 0x2a555555, 
	0x55888888, 0x88555555, 0x5522a222, 0xa2555555, 
	0x55888888, 0x88555555, 0x552a2a2a, 0x2a555555, 
	0x55888888, 0x88555555, 0x55a222a2, 0x22555555, 
	0x55888888, 0x88555555, 0x552a2a2a, 0x2a555555, 
      }, {
	0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa545454, 
	0x54aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa444444, 
	0x44aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa445444, 
	0x54aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa444444, 
	0x44aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa545454, 
	0x54aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa444444, 
	0x44aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa445444, 
	0x54aaaaaa, 0xaa555555, 0x55aaaaaa, 0xaa444444, 
      }, {
	0x44555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaa555555, 0x55aaaaaa, 0xaa555555, 0x55aaaaaa, 
      }, {
	0xaadddddd, 0xddaaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaad5d5d5, 0xd5aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaadddddd, 0xddaaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaaddd5dd, 0xd5aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaadddddd, 0xddaaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaad5d5d5, 0xd5aaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaadddddd, 0xddaaaaaa, 0xaa555555, 0x55aaaaaa, 
	0xaaddd5dd, 0xd5aaaaaa, 0xaa555555, 0x55aaaaaa, 
      }, {
	0xaa777777, 0x77aaaaaa, 0xaad5d5d5, 0xd5aaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaadd5ddd, 0x5daaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaad5d5d5, 0xd5aaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaa5ddd5d, 0xddaaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaad5d5d5, 0xd5aaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaadd5ddd, 0x5daaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaad5d5d5, 0xd5aaaaaa, 
	0xaa777777, 0x77aaaaaa, 0xaa5ddd5d, 0xddaaaaaa, 
      }, {
	0xaa555555, 0x55bbbbbb, 0xbb555555, 0x55fefefe, 
	0xfe555555, 0x55bbbbbb, 0xbb555555, 0x55eeefee, 
	0xef555555, 0x55bbbbbb, 0xbb555555, 0x55fefefe, 
	0xfe555555, 0x55bbbbbb, 0xbb555555, 0x55efefef, 
	0xef555555, 0x55bbbbbb, 0xbb555555, 0x55fefefe, 
	0xfe555555, 0x55bbbbbb, 0xbb555555, 0x55eeefee, 
	0xef555555, 0x55bbbbbb, 0xbb555555, 0x55fefefe, 
	0xfe555555, 0x55bbbbbb, 0xbb555555, 0x55efefef, 
      }, {
	0xefffffff, 0xffaaaaaa, 0xaa777777, 0x77aaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa777f77, 0x7faaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa777777, 0x77aaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa7f7f7f, 0x7faaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa777777, 0x77aaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa777f77, 0x7faaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa777777, 0x77aaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaa7f7f7f, 0x7faaaaaa, 
      }, {
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
	0xaaffffff, 0xffaaaaaa, 0xaaffffff, 0xffaaaaaa, 
      }, {
	0xaa555555, 0x55ffffff, 0xffdddddd, 0xddffffff, 
	0xff555555, 0x55ffffff, 0xff5ddd5d, 0xddffffff, 
	0xff555555, 0x55ffffff, 0xffdddddd, 0xddffffff, 
	0xff555555, 0x55ffffff, 0xff5d5d5d, 0x5dffffff, 
	0xff555555, 0x55ffffff, 0xffdddddd, 0xddffffff, 
	0xff555555, 0x55ffffff, 0xff5ddd5d, 0xddffffff, 
	0xff555555, 0x55ffffff, 0xffdddddd, 0xddffffff, 
	0xff555555, 0x55ffffff, 0xff5d5d5d, 0x5dffffff, 
      }, {
	0xffeeeeee, 0xeeffffff, 0xffbbbaba, 0xbaffffff, 
	0xffeeeeee, 0xeeffffff, 0xffabbbab, 0xbbffffff, 
	0xffeeeeee, 0xeeffffff, 0xffbbbaba, 0xbaffffff, 
	0xffeeeeee, 0xeeffffff, 0xffbbabbb, 0xabffffff, 
	0xffeeeeee, 0xeeffffff, 0xffbbbaba, 0xbaffffff, 
	0xffeeeeee, 0xeeffffff, 0xffabbbab, 0xbbffffff, 
	0xffeeeeee, 0xeeffffff, 0xffbbbaba, 0xbaffffff, 
	0xffeeeeee, 0xeeffffff, 0xffbbabbb, 0xabffffff, 
      }, {
	0xffffffff, 0xffeeeeee, 0xeeffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffeeeeee, 0xeeffffff, 0xffbfbbbf, 
	0xbbffffff, 0xffeeeeee, 0xeeffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffeeeeee, 0xeeffffff, 0xffbfbfbf, 
	0xbfffffff, 0xffeeeeee, 0xeeffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffeeeeee, 0xeeffffff, 0xffbfbbbf, 
	0xbbffffff, 0xffeeeeee, 0xeeffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffeeeeee, 0xeeffffff, 0xffbfbfbf, 
      }, {
	0xbfffffff, 0xffffffff, 0xffffffff, 0xffbbbbbb, 
	0xbbffffff, 0xffffffff, 0xffffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffffffff, 0xffffffff, 0xffbbbbbb, 
	0xbbffffff, 0xffffffff, 0xffffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffffffff, 0xffffffff, 0xffbbbbbb, 
	0xbbffffff, 0xffffffff, 0xffffffff, 0xfffbfbfb, 
	0xfbffffff, 0xffffffff, 0xffffffff, 0xffbbbbbb, 
	0xbbffffff, 0xffffffff, 0xffffffff, 0xfffbfbfb, 
      }, {
	0xfbffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
      }
    };

    int count, loop1, loop2, i;

    if ( type <= WHITE_FILL || 
	(type >= BLACK_FILL && type < NUMSHADES + NUMTINTS) ||
	type >= NUMSHADES + NUMTINTS + NUMPATTERNS )
      return;

    if (type != PatternType || color != PatternColor) {
	PatternType=type;
	PatternColor=color;
	if (type < NUMSHADES + NUMTINTS)
	  if (color == BLACK_COLOR || color == DEFAULT)
	    i = type;
	  else 
	    i= NUMSHADES - type - 1;
	else
	  i= DEFAULT_SHADING;

	fprintf(tfp, "\\texture{");
	count=0;
	for (loop1=4; loop1>0;) {
	    for (loop2=8; loop2>0; loop2--) 
		fprintf(tfp, "%lx ", patterns[i][count++]);
	    if (--loop1 > 0)
		fprintf(tfp, "\n\t");
	    else
		fprintf(tfp, "}\n");
	}
    }
}

void
genepic_line(line)
F_line *line;
{
    F_point *p, *q;
    int pt_count = 0, temp;
    int boxflag = False, llx, lly, urx, ury;
    int r;
    double dtemp;

    /* print any comments prefixed with "%" */
    print_comments("% ",line->comments,"");

    p = line->points;
    q = p->next;
    convertCS(p);
    if (q == NULL) {
	fprintf(tfp, "\\drawline(%d,%d)(%d,%d)\n", p->x, p->y, p->x, p->y);
	return;
    }
    convertCS(q);
    /* first any backward arrowhead */
    if (line->back_arrow) {
	draw_arrow_head(q, p, line->back_arrow->ht, line->back_arrow->wid,
			line->back_arrow->type, line->back_arrow->style, 
			line->back_arrow->thickness);
    	if (Verbose) fprintf(tfp, "%%\n");
    }
    /* now set attributes */
    set_linewidth(line->thickness);
    set_style(line->style, line->style_val);
    if (line->type == T_ARC_BOX) { /* A box with rounded corners */

      if (TeXLang == Epic || /* Sorry, can't do more */
	  LineStyle != SOLID_LINE || 
	  line->fill_style != UNFILLED) { 
	fprintf(stderr, "Arc box not implemented; substituting box.\n");
	line->type = T_BOX;
      } else {
	if (TeXLang == Epic && (LineStyle != SOLID_LINE || line->fill_style != UNFILLED)) { 
	    fprintf(stderr, "Arc boxes may only have solid lines and be unfilled.\n");
	    LineStyle = SOLID_LINE;
	    line->fill_style = UNFILLED;
	}
	if (Verbose) {
	    fprintf(tfp, "%%\n%% An arcbox\n%%\n");
	}
	/* borrowed from below */
	llx = urx = p->x;
	lly = ury = p->y;
	while (q != NULL) {
	  if (q->x < llx) {
	    llx = q->x;
	  } else if (q->x > urx) {
	    urx = q->x;
	  }
	  if (q->y < lly) {
	    lly = q->y;
	  } else if (q->y > ury) {
	    ury = q->y;
	  }
	  if ((q = q->next))
	     convertCS(q);
	}
	r= line->radius;
	fprintf(tfp, "\\put(%d,%d){\\arc{%d}{1.5708}{3.1416}}\n", llx+r, lly+r, 2*r);
	fprintf(tfp, "\\put(%d,%d){\\arc{%d}{3.1416}{4.7124}}\n", llx+r, ury-r, 2*r);
	fprintf(tfp, "\\put(%d,%d){\\arc{%d}{4.7124}{6.2832}}\n", urx-r, ury-r, 2*r);
	fprintf(tfp, "\\put(%d,%d){\\arc{%d}{0}{1.5708}}\n", urx-r, lly+r, 2*r);
	fprintf(tfp, "\\%s(%d,%d)(%d,%d)\n", LnCmd, llx, lly+r, llx, ury-r);
	fprintf(tfp, "\\%s(%d,%d)(%d,%d)\n", LnCmd, llx+r, ury, urx-r, ury);
	fprintf(tfp, "\\%s(%d,%d)(%d,%d)\n", LnCmd, urx, ury-r, urx, lly+r);
	fprintf(tfp, "\\%s(%d,%d)(%d,%d)\n", LnCmd, urx-r, lly, llx+r, lly);
	return;
      }
    }
    if (line->type == T_BOX) {
	if (Verbose) {
	    fprintf(tfp, "%%\n%% A box\n%%\n");
	}
	switch (LineStyle) {
	  case SOLID_LINE:
	    if (UseBox == BothBoxType || UseBox == SolidLineBox) {
	        boxflag = True;
	    }
	    break;
	  case DASH_LINE:
	    if (UseBox == BothBoxType || UseBox == DashLineBox) {
	        boxflag = True;
	    }
	    break;
	}
	if (boxflag) {
	    llx = urx = p->x;
	    lly = ury = p->y;
	    while (q != NULL) {
	        if (q->x < llx) {
	            llx = q->x;
	        } else if (q->x > urx) {
	            urx = q->x;
	        }
	        if (q->y < lly) {
	            lly = q->y;
	        } else if (q->y > ury) {
	            ury = q->y;
	        }
	        if ((q = q->next))
		   convertCS(q);
	    }
	    switch(LineStyle) {
	      case SOLID_LINE:
	        fprintf(tfp, "\\put(%d,%d){\\framebox(%d,%d){}}\n",
	            llx, lly, urx-llx, ury-lly);
	        break;
	      case DASH_LINE:
		temp = (int) ((urx-llx) / DashLen);
		dtemp = (double) (urx-llx) / temp;
	        fprintf(tfp, "\\put(%d,%d){\\dashbox{%4.3f}(%d,%d){}}\n",
	            llx, lly, dtemp , urx-llx, ury-lly);
	        break;
	      default:
	        put_msg("Program Error! No other line styles allowed.\n");
	        break;
	    }
	    return;
	  }
    } else if (line->type != T_ARC_BOX && Verbose) {
      fprintf(tfp, "%%\n%% A polyline\n%%\n");
    }
    set_pattern(line->fill_style, line->fill_color);
    switch (LineStyle) {
      case SOLID_LINE:
	if (q->next != NULL && strcmp(LnCmd,"path")==0) {
	    if (line->fill_style < UNFILLED)
		line->fill_style = UNFILLED;
	    fprintf(tfp, "%s", FillCommands(line->fill_style, line->fill_color));
	}
	fprintf(tfp, "\\%s", LnCmd);
#ifdef DrawOutLine
	if (line->fill_style != UNFILLED && OutLine == 0)
	    OutLine=1;
#endif
	break;
      case DASH_LINE:
        if ((TeXLang==Epic || TeXLang ==EEpic_emu) && DashType == Economic) {
            fprintf(tfp, "\\drawline[-50]");
        } else {
	    fprintf(tfp, "\\dashline{%4.3f}", DashLen);
	}
	break;
      case DOTTED_LINE:
	fprintf(tfp, "\\dottedline{%d}", DotDist);
	break;
      default:
	fprintf(stderr,"Only solid, dashed, and dotted line styles supported by epic(eepic)\n");
	exit(1);
    }
    fprintf(tfp, "(%d,%d)", p->x, p->y);
    pt_count++;
    while(q->next != NULL) {
	if (++pt_count > PtPerLine) {
	    pt_count=1;
	    fprintf(tfp, "\n\t");
	}
	fprintf(tfp, "(%d,%d)", q->x, q->y);
	p=q;
	q = q->next;
	convertCS(q);
    }
    fprintf(tfp, "(%d,%d)\n", q->x, q->y);
#ifdef DrawOutLine
    if (OutLine == 1) {
	OutLine=0;
	fprintf(tfp, "\\%s", LnCmd);
	p=line->points;
	pt_count=0;
	q=p->next;
	fprintf(tfp, "(%d,%d)", p->x, p->y);
	pt_count++;
	while(q->next != NULL) {
	    if (++pt_count > PtPerLine) {
		pt_count=1;
		fprintf(tfp, "\n\t");
	    }
	    fprintf(tfp, "(%d,%d)", q->x, q->y);
	    p=q;
	    q = q->next;
	}
	fprintf(tfp, "(%d,%d)\n", q->x, q->y);
    }
#endif
    /* finally, any forward arrowhead */
    if (line->for_arrow) {
	draw_arrow_head(p, q, line->for_arrow->ht, line->for_arrow->wid,
			line->for_arrow->type,line->for_arrow->style,
			line->for_arrow->thickness);
    	if (Verbose) fprintf(tfp, "%%\n");
    }
}

static void
set_style(style, dash_len)
int style;
double dash_len;
{
    LineStyle = style;
    if (LineStyle == DASH_LINE) {
        switch (DashType) {
          case DottedDash:
            LineStyle = DOTTED_LINE;
	    DotDist = round(dash_len * DashScale);
            break;
          default:
            DashLen = round(dash_len * DashScale);
            break;
        }
    } else if (LineStyle == DOTTED_LINE) {
	DotDist = round(dash_len * DashScale);
    }

}


static void
genepic_spline(s)
F_spline *s;
{
    /* print any comments prefixed with "%" */
    print_comments("% ",s->comments,"");

    set_linewidth(s->thickness);
    set_style(SOLID_LINE, 0.0);
    if (int_spline(s)) {
	genepic_itp_spline(s);
    } else {
	genepic_ctl_spline(s);
    }
}

static void
genepic_ctl_spline(spl)
F_spline *spl;
{
    if (closed_spline(spl)) {
	genepic_closed_spline(spl);
    } else {
	genepic_open_spline(spl);
    }
}

static void
genepic_open_spline(spl)
F_spline *spl;
{
    F_point *p, *q, *r;
    FPoint first, mid;
    int pt_count = 0;

    p = spl->points;
    q = p->next;
    convertCS(p);
    convertCS(q);
    if (spl->back_arrow) {
      draw_arrow_head(q, p, spl->back_arrow->ht, spl->back_arrow->wid,
                      spl->back_arrow->type,spl->back_arrow->style,
                      spl->back_arrow->thickness);
      if (Verbose) fprintf(tfp, "%%\n");
    }
    if (q->next == NULL) {
	fprintf(tfp, "\\%s(%d,%d)(%d,%d)\n", LnCmd,
	       p->x, p->y, q->x, q->y);
	return;
    }
    if (TeXLang == EEpic || TeXLang == EEpic_emu) {
        fprintf(tfp, "\\spline(%d,%d)\n", p->x, p->y);
        pt_count++;
        while(q->next != NULL) {
             if (++pt_count > PtPerLine) {
                 pt_count=1;
                 fprintf(tfp, "\n\t");
             }
             fprintf(tfp, "(%d,%d)", q->x, q->y);
             p=q;
             q = q->next;
             convertCS(q);
        }
        fprintf(tfp, "(%d,%d)\n", q->x, q->y);
    } else {
        fprintf(tfp, "\\%s(%d,%d)\n", LnCmd, p->x, p->y);
        r = q->next;
        convertCS(r);
        first.x = p->x;
        first.y = p->y;
        while (r->next != NULL) {
            mid.x = (q->x + r->x) / 2.0;
            mid.y = (q->y + r->y) / 2.0;
            chaikin_curve(first.x, first.y, (double) q->x, (double) q->y,
                            mid.x, mid.y);
            first = mid;
            q=r;
            r = r->next;
            convertCS(r);
        }
        chaikin_curve(first.x, first.y, (double) q->x, (double) q->y,
                        (double) r->x, (double) r->y);
        p=q;
        q=r;
	fprintf(tfp, "\n");
    }
    if (spl->for_arrow) {
	draw_arrow_head(p, q, spl->for_arrow->ht, spl->for_arrow->wid,
			spl->for_arrow->type, spl->for_arrow->style,
			spl->for_arrow->thickness);
    	if (Verbose) fprintf(tfp, "%%\n");
    }
}

static void
genepic_closed_spline(spl)
F_spline *spl;
{
    F_point *p;
    double cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
    double x1, y1, x2, y2;

    p = spl->points;
    convertCS(p);
    x1 = p->x;  y1 = p->y;
    p = p->next;
    convertCS(p);
    x2 = p->x;  y2 = p->y;
    cx1 = (x1 + x2) / 2;      cy1 = (y1 + y2) / 2;
    cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;
    for (p = p->next; p != NULL; p = p->next) {
	fprintf(tfp, "\\%s(%.3f,%.3f)", LnCmd, cx1, cy1);
	x1 = x2;  y1 = y2;
	convertCS(p);
	x2 = p->x;  y2 = p->y;
	cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	fprintf(tfp, "\n");
	cx1 = cx4;  cy1 = cy4;
	cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;
    }
    x1 = x2;  y1 = y2;
    p = spl->points->next;
    x2 = p->x;  y2 = p->y;
    cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
    cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
    fprintf(tfp, "\\%s(%.3f,%.3f)", LnCmd, cx1, cy1);
    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
    fprintf(tfp, "\n");
}

static void
chaikin_curve(a1, b1, a2, b2, a3, b3)
double a1, b1, a2, b2, a3, b3;
{
    double xm1, xmid, xm2, ym1, ymid, ym2;

    if (fabs(a1-a3) < Threshold && fabs(b1-b3) < Threshold) {
        fprintf(tfp, "\t(%.3f,%.3f)\n", a3, b3);
    } else {
        xm1 = (a1 + a2) / 2;
        ym1 = (b1 + b2) / 2;
        xm2 = (a2 + a3) / 2;
        ym2 = (b2 + b3) / 2;
        xmid = (xm1 + xm2) / 2;
        ymid = (ym1 + ym2) / 2;
        chaikin_curve(a1, b1, xm1, ym1, xmid, ymid);
        chaikin_curve(xmid, ymid, xm2, ym2, a3, b3);
    }
}

static void
quadratic_spline(a1, b1, a2, b2, a3, b3, a4, b4)
double	a1, b1, a2, b2, a3, b3, a4, b4;
{
    double	x1, y1, x4, y4;
    double	xmid, ymid;

    x1 = a1; y1 = b1;
    x4 = a4; y4 = b4;

    xmid = (a2 + a3) / 2;
    ymid = (b2 + b3) / 2;
    if (fabs(x1 - xmid) < Threshold && fabs(y1 - ymid) < Threshold) {
	fprintf(tfp, "\t(%.3f,%.3f)\n", xmid, ymid);
    } else {
	quadratic_spline(x1, y1, ((x1+a2)/2), ((y1+b2)/2),
			 ((3*a2+a3)/4), ((3*b2+b3)/4), xmid, ymid);
    }

    if (fabs(xmid - x4) < Threshold && fabs(ymid - y4) < Threshold) {
	fprintf(tfp, "\t(%.3f,%.3f)\n", x4, y4);
    } else {
	quadratic_spline(xmid, ymid, ((a2+3*a3)/4), ((b2+3*b3)/4),
			 ((a3+x4)/2), ((b3+y4)/2), x4, y4);
    }
}

static void
genepic_itp_spline(spl)
F_spline *spl;
{
    F_point *p1, *p2;
    FPoint pt1l, pt1r, pt2l, pt2r, tmpfpt;
    F_control *cp1, *cp2;

    p1 = spl->points;
    convertCS(p1);
    cp1 = spl->controls;
    pt1l.x = cp1->lx;
    pt1l.y = cp1->ly;
    pt1r.x = cp1->rx;
    pt1r.y = cp1->ry;
    fconvertCS(&pt1l);
    fconvertCS(&pt1r);
    if (spl->back_arrow) {
      tmpfpt.x = p1->x;
      tmpfpt.y = p1->y;
      fdraw_arrow_head(&pt1r, &tmpfpt,
                       spl->back_arrow->ht, spl->back_arrow->wid,
                       spl->back_arrow->type, spl->back_arrow->style,
                       spl->back_arrow->thickness);
      if (Verbose) fprintf(tfp, "%%\n");
    }

    for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
	 p1 = p2, pt1r = pt2r, p2 = p2->next, cp2 = cp2->next) {
	fprintf(tfp, "\\%s(%d,%d)", LnCmd, p1->x, p1->y);
	convertCS(p2);
	pt2l.x = cp2->lx;
	pt2l.y = cp2->ly;
	pt2r.x = cp2->rx;
	pt2r.y = cp2->ry;
	fconvertCS(&pt2l);
	fconvertCS(&pt2r);
	bezier_spline((double) p1->x, (double) p1->y,
		      pt1r.x, pt1r.y,
		      pt2l.x, pt2l.y,
		      (double) p2->x, (double) p2->y);
	fprintf(tfp, "\n");
    }

    if (spl->for_arrow) {
	tmpfpt.x = p1->x;
	tmpfpt.y = p1->y;
	fdraw_arrow_head(&pt2l, &tmpfpt, 
			 spl->for_arrow->ht, spl->for_arrow->wid,
			 spl->for_arrow->type, spl->for_arrow->style,
			 spl->for_arrow->thickness);
	if (Verbose) fprintf(tfp, "%%\n");
    }
}

static void
bezier_spline(a0, b0, a1, b1, a2, b2, a3, b3)
double	a0, b0, a1, b1, a2, b2, a3, b3;
{
    double	x0, y0, x3, y3;
    double	sx1, sy1, sx2, sy2, tx, ty, tx1, ty1, tx2, ty2, xmid, ymid;

    x0 = a0; y0 = b0;
    x3 = a3; y3 = b3;
    if (fabs(x0 - x3) < Threshold && fabs(y0 - y3) < Threshold) {
	fprintf(tfp, "\t(%.3f,%.3f)\n", x3, y3);
    } else {
	tx = (a1 + a2) / 2;		ty = (b1 + b2) / 2;
	sx1 = (x0 + a1) / 2;	sy1 = (y0 + b1) / 2;
	sx2 = (sx1 + tx) / 2;	sy2 = (sy1 + ty) / 2;
	tx2 = (a2 + x3) / 2;	ty2 = (b2 + y3) / 2;
	tx1 = (tx2 + tx) / 2;	ty1 = (ty2 + ty) / 2;
	xmid = (sx2 + tx1) / 2;	ymid = (sy2 + ty1) / 2;

	bezier_spline(x0, y0, sx1, sy1, sx2, sy2, xmid, ymid);
	bezier_spline(xmid, ymid, tx1, ty1, tx2, ty2, x3, y3);
    }
}

void
genepic_ellipse(ell)
F_ellipse *ell;
{
    F_point pt;

    /* print any comments prefixed with "%" */
    print_comments("% ",ell->comments,"");

    set_linewidth(ell->thickness);
    pt.x = ell->center.x;
    pt.y = ell->center.y;
    convertCS(&pt);
    if (TeXLang == EEpic || TeXLang == EEpic_emu ||
	  ell->radiuses.x != ell->radiuses.y ||
          ell->radiuses.x > MaxCircleRadius) {
	set_pattern(ell->fill_style, ell->fill_color);
        fprintf(tfp, "\\put(%d,%d){", pt.x, pt.y );
#ifndef OLDCODE
        if (EllipseCmd == 0) {
	    if (ell->fill_style < UNFILLED)
		ell->fill_style = UNFILLED;
	    fprintf(tfp, "%s", FillCommands(ell->fill_style, ell->fill_color));
#  ifdef DrawOutLine
	    if (ell->fill_style != UNFILLED && OutLine == 0)
		OutLine = 1;
#  endif
        }
 	fprintf(tfp, EllCmdstr[EllipseCmd],EllCmdkw[EllipseCmd], "",
	       2 * ell->radiuses.x, 2 * ell->radiuses.y);
#  ifdef DrawOutLine
	if (OutLine == 1) {
	    OutLine=0;
            fprintf(tfp, "\\put(%d,%d){", pt.x, pt.y );
	    fprintf(tfp, EllCmdstr[EllipseCmd],EllCmdkw[EllipseCmd], "",
		   2 * ell->radiuses.x, 2 * ell->radiuses.y);
	}
#  endif
#else
	fprintf(tfp, EllCmdstr[EllipseCmd], EllCmdkw[EllipseCmd],
	       (EllipseCmd==0 && ell->fill_style==BLACK_FILL ? "*" : ""),
	       2 * ell->radiuses.x, 2 * ell->radiuses.y);
#endif
    } else {
        fprintf(tfp, "\\put(%d,%d){\\circle", pt.x, pt.y);
        if (ell->fill_style == BLACK_FILL) {
            fputc('*', tfp);
        }
        fprintf(tfp, "{%d}}\n", 2*ell->radiuses.x);
    }
}


void 
setfigfont( text )
    F_text *text;
{
    int texsize;
    double baselineskip;

    texsize = TEXFONTMAG(text);
    baselineskip = (texsize * 1.2);

#ifdef NFSS
    if ( FontSizeOnly )
	fprintf(tfp, "{\\SetFigFont{%d}{%.1f}",
		texsize, baselineskip );
    else
	fprintf(tfp, "{\\SetFigFont{%d}{%.1f}{%s}{%s}{%s}",
		texsize, baselineskip,
		TEXFAMILY(text->font),TEXSERIES(text->font),
		TEXSHAPE(text->font));
#else
    fprintf(tfp, "{\\SetFigFont{%d}{%.1f}{%s}",
	    texsize, baselineskip, TEXFONT(text->font));
#endif
}


extern char *ISO1toTeX[];
extern char *ISO2toTeX[];

void
genepic_text(text)
    F_text *text;
{
    F_point pt;
    char *tpos, *esc_cp, *special_index;
    unsigned char   *cp;
    int rot_angle = 0;

    /* print any comments prefixed with "%" */
    print_comments("% ",text->comments,"");

    pt.x=text->base_x;
    pt.y=text->base_y;
    convertCS(&pt);
    switch (text->type) {
      case T_LEFT_JUSTIFIED:
      case DEFAULT:
	tpos = "[lb]";
	break;
      case T_CENTER_JUSTIFIED:
	tpos = "[b]";
	break;
      case T_RIGHT_JUSTIFIED:
	tpos = "[rb]";
	break;
      default:
	fprintf(stderr, "unknown text position type\n");
	exit(1);
    }
    fprintf(tfp, "\\put(%d,%d){", pt.x, pt.y );
    rot_angle = (int) (text->angle*(180.0/M_PI));
    if ( AllowRotatedText && rot_angle )
	fprintf(tfp,"\\rotatebox[origin=l]{%d}{", rot_angle );
    else
	fprintf(tfp, "\\makebox(0,0)%s{", tpos);

    fprintf(tfp, "\\smash{");

    /* Output a shortstack in case there are multiple lines. */
    for(cp = (unsigned char*)text->cstring; *cp; cp++) {
      if (*cp == TEXT_LINE_SEP) {
    fprintf(tfp, "\\shortstack" );
    /* Output the justification for the shortstack. */
    switch (text->type) {
      case T_LEFT_JUSTIFIED:
      case DEFAULT:
	fprintf(tfp, "[l]");
	break;
      case T_CENTER_JUSTIFIED:
	break;
      case T_RIGHT_JUSTIFIED:
	fprintf(tfp, "[r]");
	break;
      default:
	fprintf(stderr, "unknown text position type\n");
	exit(1);
	}
	break;
      }
    }

    unpsfont(text);
    setfigfont( text );

    if (!special_text(text))
	/* This loop escapes special LaTeX characters. */
	for (cp = (unsigned char*)text->cstring; *cp; cp++) {
      	    if (special_index=strchr(latex_text_specials, *cp)) {
	      /* Write out the replacement.  Implementation note: we can't
		 use puts since that will output an additional newline. */
	      esc_cp=latex_text_mappings[special_index-latex_text_specials];
	      while (*esc_cp)
		fputc(*esc_cp++, tfp);
	    }
	    else if (*cp == TEXT_LINE_SEP) {
	      /* Handle multi-line text strings. The problem being addressed here
		 is a LaTeX bug where LaTeX is unable to handle a font which
		 spans multiple lines.  What we are doing here is closing off
		 the current font, starting a new line, and then resuming with
		 the current font. */
	      fprintf(tfp, "} \\\\\n");

		setfigfont( text );
	    }
	    else
		fputc(*cp, tfp);
      	}
    else 
	for (cp = (unsigned char*)text->cstring; *cp; cp++) {
	    if (*cp == TEXT_LINE_SEP) {
		/* Handle multi-line text strings. */
		fprintf(tfp, "} \\\\\n");
		setfigfont( text );

	    } else {
#ifdef I18N
		extern Boolean support_i18n;
		if (support_i18n && (text->font <= 2))
			fputc(*cp, tfp);
	    else
#endif /* I18N */
	        if (*cp >= 0xa0) {	/* we escape 8-bit char */
	            switch (encoding) {
	                case 0: /* no escaping */
			    fputc(*cp, tfp);
	                    break;
	                case 1: /* iso-8859-1 */
	    		    fprintf(tfp, "%s", ISO1toTeX[(int)*cp-0xa0]);
	                    break;
	                case 2: /* iso-8859-2 */
	    		    fprintf(tfp, "%s", ISO2toTeX[(int)*cp-0xa0]);
	                    break;
	            }
	        } else
		    /* no escaping */
		    fputc(*cp, tfp);
	  }
	}
    fprintf(tfp, "}}}}\n");
}

void
genepic_arc(arc)
F_arc *arc;
{
    FPoint pt1, pt2, ctr, tmp;
    double r1, r2, th1, th2, theta;
    double dx1, dy1, dx2, dy2;
    double arrowfactor;

    /* print any comments prefixed with "%" */
    print_comments("% ",arc->comments,"");

    ctr.x = arc->center.x;
    ctr.y = arc->center.y;
    pt1.x = arc->point[0].x;
    pt1.y = arc->point[0].y;
    pt2.x = arc->point[2].x;
    pt2.y = arc->point[2].y;
    fconvertCS(&ctr);
    fconvertCS(&pt1);
    fconvertCS(&pt2);

    dx1 = pt1.x - ctr.x;
    dy1 = pt1.y - ctr.y;
    dx2 = pt2.x - ctr.x;
    dy2 = pt2.y - ctr.y;

    rtop(dx1, dy1, &r1, &th1);
    rtop(dx2, dy2, &r2, &th2);
    arrowfactor = (r1+r2) / 30.0;
    if (arrowfactor > 1)
	arrowfactor = 1;
    set_linewidth(arc->thickness);
    if (TeXLang == EEpic) {
	set_pattern(arc->fill_style, arc->fill_color);
        fprintf(tfp, "\\put(%4.3f,%4.3f){", ctr.x, ctr.y);
    } else {
	fprintf(tfp, "\\drawline");
    }
    if (TeXLang == EEpic) {
	if (arc->fill_style < UNFILLED)
	    arc->fill_style = UNFILLED;
	fprintf(tfp, "%s", FillCommands(arc->fill_style, arc->fill_color));
#ifdef DrawOutLine
	if (arc->fill_style != UNFILLED && OutLine==0)
	    OutLine=1;
#endif
    }
    if (arc->direction) {
	theta = th2 - th1;
	if (theta < 0) theta += 2 * M_PI;
	th2 = 2*M_PI-th2;
	if (TeXLang == EEpic) {
	    fprintf(tfp, "\\arc{%4.3f}{%2.4f}{%2.4f}}\n", 2*r1, th2, th2+theta);
#ifdef DrawOutLine
	    if (OutLine==1) {
		OutLine=0;
	        fprintf(tfp, "\\put(%4.3f,%4.3f){", ctr.x, ctr.y);
		fprintf(tfp, "\\arc{%4.3f}{%2.4f}{%2.4f}}\n", 2*r1, th2, th2+theta);
	    }
#endif
        } else {
            drawarc(&ctr, r1, 2*M_PI - th2 - theta, theta);
        }
    } else {
	theta = th1 - th2;
	if (theta < 0) theta += 2 * M_PI;
	th1 = 2*M_PI-th1;
	if (TeXLang == EEpic) {
	    fprintf(tfp, "\\arc{%4.3f}{%2.4f}{%2.4f}}\n", 2*r2, th1, th1+theta);
#ifdef DrawOutLine
	    if (OutLine==1) {
		OutLine=0;
	        fprintf(tfp, "\\put(%4.3f,%4.3f){", ctr.x, ctr.y);
		fprintf(tfp, "\\arc{%4.3f}{%2.4f}{%2.4f}}\n", 2*r2, th1, th1+theta);
	    }
#endif
	} else {
            drawarc(&ctr, r2, 2*M_PI - th1 - theta, theta);
        }
    }
    if ((arc->type == T_OPEN_ARC) && (arc->thickness != 0) && 
	(arc->back_arrow || arc->for_arrow)) {
	 if (arc->for_arrow) {
	    arc_arrow(&ctr, &pt2, arc->direction, arc->for_arrow, &tmp);
	    fdraw_arrow_head(&tmp, &pt2,
			 arc->for_arrow->ht*arrowfactor,
			 arc->for_arrow->wid*arrowfactor,
			 arc->for_arrow->type,
			 arc->for_arrow->style,
			 arc->for_arrow->thickness);
    	    if (Verbose)
		fprintf(tfp, "%%\n");
	 }
	 if (arc->back_arrow) {
	    arc_arrow(&ctr, &pt1, !arc->direction, arc->back_arrow, &tmp);
	    fdraw_arrow_head(&tmp, &pt1,
			 arc->back_arrow->ht*arrowfactor,
			 arc->back_arrow->wid*arrowfactor,
			 arc->back_arrow->type,
			 arc->back_arrow->style,
			 arc->back_arrow->thickness);
    	    if (Verbose)
		fprintf(tfp, "%%\n");
	 }
    }
}

static void
drawarc(ctr, r, th1, angle)
FPoint *ctr;
double r, th1, angle;
{
    double delta;
    int division, pt_count = 0;


    division = angle * r / Threshold;
    delta = angle / division;
    division++;
    while (division-- > 0) {
        if (++pt_count > PtPerLine) {
            fprintf(tfp, "\n\t");
            pt_count = 1;
        }
        fprintf(tfp, "(%.3f,%.3f)", ctr->x + cos(th1) * r,
                                ctr->y + sin(th1) * r);
        th1 += delta;
    }
    fprintf(tfp, "\n");
}

/*************************** ARROWS *****************************

 arc_arrow - Computes a point on a line which is a chord to the 
        arc specified by center pt1 and endpoint pt2, where the 
        chord intersects the arc arrow->ht from the endpoint.

 May give strange values if the arrow.ht is larger than about 1/4 of
 the circumference of a circle on which the arc lies.

  This function is copied from xfig-3.2.0-beta3/u_draw.c 
       compute_arcarrow_angle. Thanks to the author.

****************************************************************/

static void
arc_arrow(pt1, pt2, direction, arrow, pt3)
FPoint *pt1, *pt2, *pt3;
int direction;
F_arrow *arrow;
{
    double	r, alpha, beta, dy, dx, x1, x2, y1, y2;
    double	lpt,h;

    x1=pt1->x;
    y1=pt1->y;

    x2=pt2->x;
    y2=pt2->y;

    dy=y2-y1;
    dx=x2-x1;
    r=sqrt(dx*dx+dy*dy);
    h = arrow->ht;
    /* lpt is the amount the arrowhead extends beyond the end of the line */
    lpt = arrow->thickness/30.0/(arrow->wid/h/2.0);
    /* add this to the length */
    h += lpt;

    /* radius too small for this method, use normal method */
    if (h > 2.0*r) {
	arc_tangent(pt1->x,pt1->y,pt2->x,pt2->y,direction,&pt3->x,&pt3->y);
	return;
    }

    beta=atan2(dx,dy);
    if (direction) {
	alpha=2*asin(h/2.0/r);
    } else {
	alpha=-2*asin(h/2.0/r);
    }

    pt3->x=round(x1+r*sin(beta+alpha));
    pt3->y=y1+r*cos(beta+alpha);
}

static void
rtop(x, y, r, th)
double x, y, *r, *th;
{
    *r = sqrt(x*x+y*y);
    *th = acos(x/(*r));
    if (*th < 0) *th = M_PI + *th;
    if (y < 0) *th = 2*M_PI - *th;
}

static void
draw_arrow_head(pt1, pt2, arrowht, arrowwid, type, style, thickness)
F_point *pt1, *pt2;
int type, style;
double arrowht, arrowwid, thickness;
{
    FPoint fpt1, fpt2;

    fpt1.x = pt1->x;
    fpt1.y = pt1->y;
    fpt2.x = pt2->x;
    fpt2.y = pt2->y;
    fdraw_arrow_head(&fpt1, &fpt2, arrowht, arrowwid, type, style, thickness);
}

static void
fdraw_arrow_head(pt1, pt2, arrowht, arrowwid, type, style, thickness)
FPoint *pt1, *pt2;
int type, style;
double arrowht, arrowwid, thickness;
{
    double x1, y1, x2, y2;
    double x,y, xb,yb,dx,dy,l,sina,cosa;
    double xc, yc, xd, yd;

    arrowht/= ArrowScale;
    arrowwid/= ArrowScale;

    x1 = pt1->x;
    y1 = pt1->y;
    x2 = pt2->x;
    y2 = pt2->y;

    dx = x2 - x1;  dy = y1 - y2;
    l = sqrt(dx*dx+dy*dy);
    if (l == 0) {
	 return;
    }
    else {
	 sina = dy / l;  cosa = dx / l;
    }
    xb = x2*cosa - y2*sina;
    yb = x2*sina + y2*cosa;
    x = xb - arrowht;
    y = yb - arrowwid / 2;
    xc = x*cosa + y*sina;
    yc = -x*sina + y*cosa;
    y = yb + arrowwid / 2;
    xd = x*cosa + y*sina;
    yd = -x*sina + y*cosa;

    if (Verbose) fprintf(tfp, "%%\n%% arrow head\n%%\n");

    if (type) {
      if (style == 1)
	fprintf(tfp, "\\blacken");
      else
	fprintf(tfp, "\\whiten");
    }
    set_linewidth((int) thickness);

    switch(type) {
      case 0:
	fprintf(tfp, "\\%s(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)\n", LnCmd,
	      xc, yc, x2, y2, xd, yd);
	break;
      case 1:
	fprintf(tfp, "\\%s(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)\n", LnCmd,
	      xc, yc, x2, y2, xd, yd, xc, yc);
	break;
      case 2:
	fprintf(tfp, "\\%s(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)\n", LnCmd,
	      xc, yc, x2, y2, xd, yd, x2 - (x2-x1)/l*arrowht*0.7, y2 - (y2-y1)/l*arrowht*0.7, xc, yc);
	break;
      case 3:
	fprintf(tfp, "\\%s(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)(%4.3f,%4.3f)\n", LnCmd,
	      xc, yc, x2, y2, xd, yd, x2 - (x2-x1)/l*arrowht*1.3, y2 - (y2-y1)/l*arrowht*1.3, xc, yc);
	break;
    }
}

static char* 
FillCommands(style, color)
int style, color;
{
  static char empty[]= "";
  static char shaded[]= "\\shade";
  static char whiten[]= "\\whiten";
  static char blacken[]= "\\blacken";
  
  if (style == UNFILLED)
    return empty;

  if (style == WHITE_FILL) {
    if (color == BLACK_COLOR || color == DEFAULT)
      return whiten;
    else
      return blacken;
  }

  if (style == BLACK_FILL) {
    if (color == BLACK_COLOR || color == DEFAULT)
      return blacken;
    else
      return whiten;
  }

  return shaded;
}

struct driver dev_epic = {
     	genepic_option,
	genepic_start,
	gendev_null,
	genepic_arc,
	genepic_ellipse,
	genepic_line,
	genepic_spline,
	genepic_text,
	genepic_end,
	INCLUDE_TEXT
};

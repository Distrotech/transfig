/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1999 by T. Sato
 * Parts Copyright (c) 2002 by Anthony Starks
 * Parts Copyright (c) 2002,2003,2004,2005,2006 by Martin Kroeker
 * Parts Copyright (c) 2002 by Brian V. Smith
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
 *
 * SVG driver for fig2dev
 *
 *  from fig2svg -- convert FIG 3.2 to SVG
 *
 *  Original author:  Anthony Starks (ajstarks@home.com)
 *  Created: 17 May 2000 
 *  Converted to gensvg by Brian Smith
 *  Further modified by Martin Kroeker (martin@ruby.chemie.uni-freiburg.de) 
 *  incorporating changes by Philipp Hahn and Justus Piater
 *  
 *  PH: Philipp Hahn
 *  JP: Justus Piater
 *  MK: Martin Kroeker
 *  BS: Brian Smith
 *  RE: Russell Edwards
 *
 *  MK 04-Dec-02: partial support for the symbol font, bigger fontscale, text alignment,
 *  dashed and dotted lines, bugfix for missing % in stroke-color statement of arcs
 *  FIXME: lacks support for arrowheads; fill patterns; percent grayscale fills
 *  MK 08-Dec-02: rotated text; shades and tints of fill colors; filled circles
 *  MK 11-Dec-02: scaling;proper font/slant/weight support; changed arc code
 *  12-Dec-02: fixes by Brian Smith: scale factor, orientation, ellipse fills
 *  MK 14-Dec-02: arc code rewrite, simplified line style handling, 
 *  arrowheads on arcs and lines (FIXME: not clipped), stroke->color command
 *  is simply 'stroke'
 *  MK 15-Dec-02: catch pattern fill flags, convert to tinted fills for now
 *  MK 18-Dec-02: fill patterns; fixes by BS: arrowhead scale & position,
 *  circle by diameter
 *  PH 03-Feb-03: Fix CIRCLE_BY_DIA, color/fill styles, update SVG DTD
 *  MK 10-Feb-03: do not encode space characters when in symbol font;
 *                always encode characters '&', '<' and '>'. Leave non-
 *		  alphabetic characters in the lower half of the symbol 
 *		  font unchanged.
 *  MK 12-Feb-03: Added complete character conversion tables for the symbol
 *		  and dingbat fonts (based on the information in Unicode 
 *		  Inc.'s symbol.txt and zdingbat.txt tables, version 0.2)
 *  MK 18-Feb-03: Added cap and join style fields for line and arc
 *  MK 24-Feb-03: Symbol and Dingbat fonts are no longer translated to 
 *		  font-family="Times" with both bold and italic flags set.
 *  MK 17-Jun-03: Fix for rotation angle bug. Correct rendering of 'tinted'
 *		  colors using code from www.cs.rit.edu. Added forgotten
 *		  pattern fill option for ellipses (circles).
 *  JP 21-Jan-04: Calculate proper bounding box instead of current paper
 *                dimensions. Added missing semicolons in some property
 *                strings, and proper linebreak characters in multi-line
 *                format strings.
 *  MK 23-Jan-04: Pattern-filled objects are now drawn twice - painting the
 *                pattern over the fill color (if any). This solves the problem
 *                of missing color support in pattern fills (as reported by JP)
 *                Corrected filling of ellipses, which was still B/W only.
 *                Fixed bad tiling of diagonal patterns 1 - 3 (the old formula
 *                favoured exact angles over seamless tiling). Updated DTD.
 *  MK 25-Jan-04: Endpoints of polylines are now truncated when arrowheads
 *		  are drawn. Corrected rendering of type 0 (stick) arrowheads.
 *  MK 28-Jan-04: Fix for arc arrowhead orientation.
 *  MK 31-Jan-04: Corrected arc angle calculation (this time for good ?)
 *  MK 22-Feb-04: Picture support
 *  JP  1-Mar-04: Closed arrowheads should use polygons instead of polylines 
 *  JP  3-Mar-04: Corrected font family selection
 *  JP 26-Mar-04: Corrected (and simplified) calculation of white-tinted
 *                fill colors (and removed the HSV/RGB conversion code)
 *  MK 29-Mar-04: Added code for rounded boxes (polyline subtype 4)
 *  MK 30-Mar-04: Added code for boxes, explicit support for polygons
 *  MK 10-Apr-04: Added xml-space:preserve qualifier on texts to preserve 
 *                whitespace. Rewrote fill pattern handling to generate 
 *                patterns as needed - adding support for penwidth and color.
 *                Corrected tiling of all shingle patterns and reversal
 *                of horizontal shingles.
 *  RE  6-May-04: Changed degrees() to double for more precision
 *                Added linewidth() to transform all line widths in the
 *                 same way as genps.c : thin lines get thinner
 *                Changed circle radius to use F_ellipse::radiuses.x instead
 *                 of start and end (which seemed not to work correctly)
 *                 Query: Is this broken for byradius or bydiameter??
 *                Added rotation to ellipses
 *                Changed back to mapping Symbol to Times, greeks look a bit
 *                 better. Ultimately embedding PS fonts would be better.
 *                Removed newlines inside <text> printf, otherwise they get 
 *                 rendered as spaces due to xml:space="preserve"
 *                Removed extraneous comma between two halves of format
 *                 string in gensvg_arc, fixes Seg fault.
 *  MK  3-Aug-04  Split the multi-line format string in gensvg_arc in two to
 *                get rid of (compiler version-dependant) segfaults for good.
 *  MK 11-Sep-05: Added explicit stroke color to text to prevent black outline
 *                on colored text.
 *                Added support for latex-special formatted text, converting
 *                sub- and superscripts to either baseline-shift=sub/super 
 *                (the intended way of doing this in SVG) or "dy" offsets 
 *                (less elegant, but more likely to be supported by browsers
 *                and editors) depending on the NOSUPER define below.
 *                Tested with Batik-1.6, konqueror-3.4, firefox-1.5b1, 
 *                inkscape-0.41
 *  MK 15-Sep-05: Use a font-family list of "Times,Symbol" for symbol
 *		  characters - the Times fontface does not contain all
 *		  elements of the Symbol font on all platforms.
 *  MK  4-Nov-05: Corrected length and appearance of stick-type arrows.
 *  MK  2-Jan-06: Added support for filled arcs.
 *  MK 26-Feb-06: Added support for dashed circles, ellipses and arcs.
 *		  Dash/gap lengths are now drawn according to style_val. 
 *		  Fixed several glitches uncovered by splint.
 *  MK 22-Apr-06: Corrected blue component of shaded colors (was always 
 *		  zero due to missing parentheses around typecast). Corrected
 *		  arrowheads of large arrows by adding an increased miterlimit.
 * 		  Corrected position of backward arrowheads on polylines with
 *		  both forward and backward arrows.
 *  MK  2-Jul-06: Patterns do not inherit their line width from the parent object
 *                (which may be zero if no visible boundary is desired), so always 
 *                use linewidth:1 
 *  MK 22-Oct-06: Changed unicode variant of lowercase phi to match its X11 Symbol 
 *                counterpart.
 *  *********************************************************************************
 *  W3 recommendations for 
 *  1.3 SVG Namespace, Public Identifier and System Identifier
 *
 *  The following are the SVG 1.1 namespace, public identifier and system identifier:
 *
 *  SVG Namespace:
 *      http://www.w3.org/2000/svg
 *	xmlns:xlink="http://www.w3.org/1999/xlink"
 *  Public Identifier for SVG 1.1:
 *      PUBLIC "-//W3C//DTD SVG 1.1//EN"
 *  System Identifier for the SVG 1.1 Recommendation:
 *      http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd
 *
 *  The following is an example document type declaration for an SVG document:
 *
 *  <!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" 
 *           "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
 *
 *  Note that DTD listed in the System Identifier is a modularized DTD (ie. its
 *  contents are spread over multiple files), which means that a validator may have
 *  to fetch the multiple modules in order to validate. For that reason, there is
 *  a single flattened DTD available that corresponds to the SVG 1.1 modularized DTD.
 *  It can be found at http://www.w3.org/Graphics/SVG/1.1/DTD/svg11-flat.dtd. 
 *  *********************************************************************************
 */

/* 
 * use a workaround for sub-/superscripting as long as most browsers do not
 * support the baseline-shift=sub/super attribute    
 */
#define NOSUPER 1
 
#include "fig2dev.h"
#include "object.h"
#include "bound.h"
#include "../../patchlevel.h"

static void svg_arrow();
static void generate_tile(int);
static void svg_dash(int,double);
          
#define PREAMBLE "<?xml version=\"1.0\" standalone=\"no\"?>\n"\
"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n"\
"\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">"
const char   *joinstyle[] = { "miter","round","bevel"};
const char   *capstyle[] = { "butt","round", "square"};
static unsigned int symbolchar[256]=
{0,0,0,0,0,0,0,0,0,0, 
0,0,0,0,0,0,0,0,0,0, 
0,0,0,0,0,0,0,0,0,0, 
0,0,0x0020,0x0021,0x2200,0x0023,0x2203,0x0025,
0x0026,0x220B,0x0028,0x0029,0x2217,0x002B,0x002C,0x2212,0x002E,0x002F,0x0030,
0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,
0x003C,0x003D,0x003E,0x003F,0x2245,0x0391,0x0392,0x03A7,0x0394,0x0395,
0x03A6,0x0393,0x0397,0x0399,0x03D1,0x039A,0x039B,0x039C,0x039D,0x039F,0x03A0,
0x0398,0x03A1,0x03A3,0x03A4,0x03A5,0x03C2,0x03A9,0x039E,0x03A8,0x0396,
0x005B,0x2234,0x005D,0x22A5,0x005F,0xF8E5,0x03B1,0x03B2,0x03C7,0x03B4,0x03B5,
0x03D5 /*0x03C6*/,0x03B3,0x03B7,0x03B9,0x03D5,0x03BA,0x03BB,0x03BC,0x03BD,0x03BF,
0x03C0,0x03B8,0x03C1,0x03C3,0x03C4,0x03C5,0x03D6,0x03C9,0x03BE,0x03C8,0x03B6,
0x007B,0x007C,0x007D,0x223C,0,0,0,0,0,0,0,0,0, 
0,0,0,0,0,0,0,0,0,0, 
0,0,0,0,0,0,0,0,0,0, 0,0,
0,0,0x20AC,0x03D2,0x2032,0x2264,0x2044,0x221E,
0x0192,0x2663,0x2666,0x2665,0x2660,0x2194,0x2190,0x2191,0x2192,0x2193,0x00B0,
0x00B1,0x2033,0x2265,0x00D7,0x221D,0x2202,0x2022,0x00F7,0x2260,0x2261,0x2248,
0x2026,0xF8E6,0xF8E7,0x21B5,0x2135,0x2111,0x211C,0x2118,0x2297,0x2295,0x2205,
0x2229,0x222A,0x2283,0x2287,0x2284,0x2282,0x2286,0x2208,0x2209,0x2220,0x2207,
0xF6DA,0xF6D9,0xF6DB,0x220F,0x221A,0x22C5,0x00AC,0x2227,0x2228,0x21D4,0x21D0,
0x21D1,0x21D2,0x21D3,0x25CA,0x2329,0xF8E8,0xF8E9,0xF8EA,0x2211,0xF8EB,0xF8EC,
0xF8ED,0xF8EE,0xF8EF,0xF8F0,0xF8F1,0xF8F2,0xF8F3,0xF8F4,0,0x232A,0x222B,0x2320,
0xF8F5,0x2321,0xF8F6,0xF8F7,0xF8F8,0xF8F9,0xF8FA,0xF8FB,0xF8FC,0xF8FD,0xF8FE,0
};     
      
static unsigned int dingbatchar[256]=
{0,0,0,0,0,0,0,0,0,0, 
0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0x0020,
0x2701,0x2702,0x2703,0x2704,0x260E,0x2706,0x2707,0x2708,0x2709,0x261B,
0x261E,0x270C,0x270D,0x270E,0x270F,0x2710,0x2711,0x2712,0x2713,0x2714,
0x2715,0x2716,0x2717,0x2718,0x2719,0x271A,0x271B,0x271C,0x271D,0x271E,
0x271F,0x2720,0x2721,0x2722,0x2723,0x2724,0x2725,0x2726,0x2727,0x2605,
0x2729,0x272A,0x272B,0x272C,0x272D,0x272E,0x272F,0x2730,0x2731,0x2732,
0x2733,0x2734,0x2735,0x2736,0x2737,0x2738,0x2739,0x273A,0x273B,0x273C,
0x273D,0x273E,0x273F,0x2740,0x2741,0x2742,0x2743,0x2744,0x2745,0x2746,
0x2747,0x2748,0x2749,0x274A,0x274B,0x25CF,0x274D,0x25A0,0x274F,0x2750,
0x2751,0x2752,0x25B2,0x25BC,0x25C6,0x2756,0x25D7,0x2758,0x2759,0x275A,
0x275B,0x275C,0x275D,0x275E,0,0xF8D7,0xF8D8,0xF8D9,0xF8DA,0xF8DB,
0xF8DC,0xF8DD,0xF8DE,0xF8DF,0xF8E0,0xF8E1,0xF8E2,0xF8E3,0xF8E4,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0x2761,0x2762,0x2763,
0x2764,0x2765,0x2766,0x2767,0x2663,0x2666,0x2665,0x2660,0x2460,0x2461,
0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,0x2469,0x2776,0x2777,
0x2778,0x2779,0x277A,0x277B,0x277C,0x277D,0x277E,0x277F,0x2780,0x2781,
0x2782,0x2783,0x2784,0x2785,0x2786,0x2787,0x2788,0x2789,0x278A,0x278B,
0x278C,0x278D,0x278E,0x278F,0x2790,0x2791,0x2792,0x2793,0x2794,0x2192,
0x2194,0x2195,0x2798,0x2799,0x279A,0x279B,0x279C,0x279D,0x279E,0x279F,
0x27A0,0x27A1,0x27A2,0x27A3,0x27A4,0x27A5,0x27A6,0x27A7,0x27A8,0x27A9,
0x27AA,0x27AB,0x27AC,0x27AD,0x27AE,0x27AF,0,0x27B1,0x27B2,0x27B3,0x27B4,
0x27B5,0x27B6,0x27B7,0x27B8,0x27B9,0x27BA,0x27BB,0x27BC,0x27BD,0x27BE,0
};

/* arrowhead arrays */
Point   points[50], fillpoints[50], clippoints[50];
int     npoints, nfillpoints, nclippoints;
int     arrowx1, arrowy1;	/* first point of object */
int     arrowx2, arrowy2;	/* second point of object */

static int tileno=0; /* number of current tile */ 

static F_point *p;

static unsigned int
rgbColorVal (int colorIndex)
{     				/* taken from genptk.c */
    unsigned int rgb;
    static unsigned int rgbColors[NUM_STD_COLS] = {
      	0x000000, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff,
      	0xffff00, 0xffffff, 0x00008f, 0x0000b0, 0x0000d1, 0x87cfff,
      	0x008f00, 0x00b000, 0x00d100, 0x008f8f, 0x00b0b0, 0x00d1d1,
      	0x8f0000, 0xb00000, 0xd10000, 0x8f008f, 0xb000b0, 0xd100d1,
      	0x803000, 0xa14000, 0xb46100, 0xff8080, 0xffa1a1, 0xffbfbf,
      	0xffe0e0, 0xffd600
    };

    if (colorIndex == DEFAULT)
      	rgb = rgbColors[0];
    else if (colorIndex < NUM_STD_COLS)
      	rgb = rgbColors[colorIndex];
    else
      	rgb = ((user_colors[colorIndex - NUM_STD_COLS].r & 0xff) << 16)
      	    | ((user_colors[colorIndex - NUM_STD_COLS].g & 0xff) << 8)
      	    | (user_colors[colorIndex - NUM_STD_COLS].b & 0xff);
    return rgb;
}

static unsigned int
rgbFillVal (int colorIndex, int area_fill)
{
    unsigned int rgb, r, g, b;
    float t;
    short   tintflag = 0;
    static unsigned int rgbColors[NUM_STD_COLS] = {
      	0x000000, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff,
      	0xffff00, 0xffffff, 0x00008f, 0x0000b0, 0x0000d1, 0x87cfff,
      	0x008f00, 0x00b000, 0x00d100, 0x008f8f, 0x00b0b0, 0x00d1d1,
      	0x8f0000, 0xb00000, 0xd10000, 0x8f008f, 0xb000b0, 0xd100d1,
      	0x803000, 0xa14000, 0xb46100, 0xff8080, 0xffa1a1, 0xffbfbf,
      	0xffe0e0, 0xffd600
    };


    if (colorIndex == DEFAULT)
          	rgb = rgbColors[0];
    else if (colorIndex < NUM_STD_COLS)
      	rgb = rgbColors[colorIndex];
    else
      	rgb = ((user_colors[colorIndex - NUM_STD_COLS].r & 0xff) << 16)
      	    | ((user_colors[colorIndex - NUM_STD_COLS].g & 0xff) << 8)
      	    | (user_colors[colorIndex - NUM_STD_COLS].b & 0xff);

    tintflag = 0;
    if (area_fill > 20) {
      	tintflag = 1;
      	area_fill -= 20;
    }
    if (colorIndex > 0 && colorIndex != 7) {
      	if (tintflag) {
      	    r = ((rgb & ~0xFFFF) >> 16);
      	    g = ((rgb & 0xFF00) >> 8); 
      	    b = (rgb & ~0xFFFF00) ;

	    t= area_fill / 20.;
	    r += t * (0xFF-r);
	    g += t * (0xff-g);
	    b += t * (0xff-b);	

      rgb = ((r &0xff) << 16) + ((g&0xff) << 8) + (b&0xff);
      	}
      	else 
      	    rgb = (((int) ((area_fill / 20.) * ((rgb & ~0xFFFF) >> 16)) << 16) +
      		   ((int) ((area_fill / 20.) * ((rgb & 0xFF00) >> 8)) << 8)
      		   + ((int) ((area_fill / 20.) * (rgb & ~0xFFFF00))) );
    }
    else {
      	if (colorIndex == 0 || colorIndex == DEFAULT)
      	    area_fill = 20 - area_fill;
      	rgb =
      	    ((area_fill * 255 / 20) << 16) + ((area_fill * 255 / 20) << 8) +
      	    area_fill * 255 / 20;
    }

    return rgb;
}

static double
degrees (double angle)
{
   return -angle / M_PI * 180.0;
}

static int
linewidth_adj(int linewidth)
{
   /* Adjustment as in genps.c */
   return (double)linewidth <= THICK_SCALE ? linewidth/2 : linewidth-THICK_SCALE;
}


void
gensvg_option (opt, optarg)
     char    opt;
     char   *optarg;
{
    switch (opt) {
      	case 'L':		/* ignore language and magnif. */
      	case 'm':
      	    break;
      	case 'z':
      	    (void) strcpy (papersize, optarg);
      	    paperspec = True;
      	    break;
      	default:
      	    put_msg (Err_badarg, opt, "svg");
      	    exit (1);
    }
}

void
gensvg_start (objects)
     F_compound *objects;
{
    struct paperdef *pd;
    int     pagewidth = -1, pageheight = -1;
    int     vx, vy, vw, vh;
    time_t  when;
    char    stime[80];

    fprintf (tfp, "%s\n", PREAMBLE);
    fprintf (tfp, "<!-- Creator: %s Version %s Patchlevel %s -->\n",
      	     prog, VERSION, PATCHLEVEL);

    (void) time (&when);
    strcpy (stime, ctime (&when));
    /* remove trailing newline from date/time */
    stime[strlen (stime) - 1] = '\0';
    fprintf (tfp, "<!-- CreationDate: %s -->\n", stime);
    fprintf (tfp, "<!-- Magnification: %.3f -->\n", mag);

    /* convert paper size from ppi to inches */
    for (pd = paperdef; pd->name != NULL; pd++)
      	if (strcasecmp (papersize, pd->name) == 0) {
      	    pagewidth = pd->width;
	    pageheight = pd->height;
	    strcpy (papersize, pd->name);	/* use the "nice" form */
	    break;
	}
    if (pagewidth < 0 || pageheight < 0) {
	(void) fprintf (stderr, "Unknown paper size `%s'\n", papersize);
	exit (1);
    }
    if (landscape) {
	vw = pagewidth;
	pagewidth = pageheight;
	pageheight = vw;
    }
    vx = (int) (llx * mag);
    vy = (int) (lly * mag);
    vw = (int) ((urx - llx) * mag);
    vh = (int) ((ury - lly) * mag);
    fprintf (tfp,
	     "<svg\txmlns=\"http://www.w3.org/2000/svg\"\n\txmlns:xlink=\"http://www.w3.org/1999/xlink\"\n\twidth=\"%.1fin\" height=\"%.1fin\"\n\tviewBox=\"%d %d %d %d\">\n",
	     vw / ppi, vh / ppi, vx, vy, vw, vh);

    if (objects->comments)
	print_comments ("<desc>", objects->comments, "</desc>");
    fprintf (tfp, "<g style=\"stroke-width:.025in; fill:none\">\n");
    /* only define the patterns if one is used */


}

int
gensvg_end ()
{
    fprintf (tfp, "</g>\n</svg>\n");
    return 0;
}

void
gensvg_line (l)
     F_line *l;
{
int px,py,firstpoint;
int px2,py2,width,height,rotation;
double dx,dy,len,cosa,sina,cosa1,sina1;
double hl;


    if (!l->points) return; /*safeguard against old, buggy fig files*/
    
    if (l->type ==5 ) {
	fprintf (tfp,"<!-- Image -->\n");
	fprintf (tfp,"<image xlink:href=\"file://%s\" preserveAspectRatio=\"none\"\n",l->pic->file);
	p=l->points;
	px=p->x;
	py=p->y;
	px2=p->next->next->x;
	py2=p->next->next->y;
	width=px2-px;
	height=py2-py;
	rotation=0;
	if (width<0 && height <0) 
		rotation=180;
	else if (width <0 && height >=0)
		rotation=90;
	else if (width >=0 && height <0)
		rotation=270;
	if (l->pic->flipped) rotation-=90;
	height=abs(height);		
	width=abs(width);	
	px=(px<px2)?px:px2;
	py=(py<py2)?py:py2;
	px2=px+width/2;
	py2=py+height/2;
	if (l->pic->flipped) {
	fprintf (tfp,"transform=\"rotate(%d %d %d) scale(-1,1) translate(%d,%d)\"\n",
	rotation,px2,py2,-2*px2,0);
	} else if (rotation !=0) {
	fprintf (tfp,"transform=\"rotate(%d %d %d)\"\n",rotation,px2,py2);
	}
	
	fprintf (tfp,"x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
	(int)(px*mag), (int)(py*mag), (int)(width*mag), (int)(height*mag)); 
    return;
    }
    
    if (l->type == 2 || l->type == 4) /* box or arc box */
    {
    fprintf (tfp, "<!-- Line: box -->\n");
    print_comments ("<!-- ", l->comments, " -->");
	px=l->points->x;
	py=l->points->y;
	px2=l->points->next->next->x;
	py2=l->points->next->next->y;
	width=abs(px2-px);
	height=abs(py2-py);
	px=(px<px2)?px:px2;
	py=(py<py2)?py:py2;

    fprintf (tfp, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%d\" \n",
    	(int)(px*mag),(int)(py*mag),(int)(width*mag),(int)(height*mag), 
        (l->type == 2 ? 0 : (int)(l->radius*mag)));     
    fprintf (tfp, "style=\"stroke:#%6.6x;stroke-width:%d;\n",
	     rgbColorVal (l->pen_color), (int) ceil (linewidth_adj(l->thickness) * mag));

	fprintf (tfp, "stroke-linejoin:%s; stroke-linecap:%s;\n",
			joinstyle[l->join_style],capstyle[l->cap_style]);

        if (l->style > 0) 
	    svg_dash(l->style,l->style_val);

    if (l->fill_style != -1 )
	fprintf (tfp, "fill:#%6.6x;\n", rgbFillVal (l->fill_color, 
	(l->fill_style>40 ? 20 : l->fill_style)));
    fprintf (tfp, "\"/>\n");

    if (l->fill_style > 40) { /*repeat object to paint pattern over fill */
	
	fprintf (tfp, "<g style=\"stroke:#%6.6x; stroke-width:1\" >\n",
	rgbColorVal(l->pen_color));
	generate_tile(l->fill_style - 40);
	fprintf (tfp, "</g>\n");	
	
    fprintf (tfp, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%d\" \n",
	(int)(px*mag),(int)(py*mag),(int)(width*mag),(int)(height*mag),
        (l->type == 2 ? 0 : (int)(l->radius*mag)));     
    fprintf (tfp, "style=\"stroke:#%6.6x;stroke-width:%d;\n",
	     rgbColorVal (l->pen_color), (int) ceil (linewidth_adj(l->thickness) * mag));

	fprintf (tfp, "stroke-linejoin:%s; stroke-linecap:%s;\n",
			joinstyle[l->join_style],capstyle[l->cap_style]);

        if (l->style > 0) 
	    svg_dash(l->style,l->style_val);

	fprintf (tfp, "fill:url(#tile%d);\n", tileno);
    fprintf (tfp, "\"/>\n");
    }
    return;
    }
        
    fprintf (tfp, "<!-- Line -->\n");
    print_comments ("<!-- ", l->comments, " -->");
    fprintf (tfp, "<%s points=\"", (l->type == 1 ? "polyline" : "polygon"));
	px=py=-100000;
	firstpoint=0;
    for (p = l->points; p; p = p->next) {
	if (px != -100000) {
		if (firstpoint && l->back_arrow) {
		dx=(double)(p->x-px);
		dy=(double)(p->y-py);
		len=sqrt(dx*dx+dy*dy);
		sina1= dy/len;
		cosa1= dx/len;
		if (l->back_arrow->type != 0 )
		hl= l->back_arrow->ht;
		else
                  hl = 1.1 * l->thickness;
		px += (int)(hl * cosa1 +0.5);
		py += (int)(hl * sina1 +0.5);
		firstpoint=0;	
		}
	fprintf(tfp, "%d,%d\n", (int) (px*mag), (int) (py*mag));
	}
	arrowx1 = arrowx2;
	arrowy1 = arrowy2;
	arrowx2 = p->x;
	arrowy2 = p->y;
	if (px == -100000) firstpoint=1;
	px=p->x;
	py=p->y;
    }
	if (!l->for_arrow) 
	   fprintf(tfp, "%d,%d\n", (int) (px*mag), (int) (py*mag));
	else { 
	dx=(double)(arrowx2-arrowx1);
	dy=(double)(arrowy2-arrowy1);
	len=sqrt(dx*dx+dy*dy);
	sina= dy/len;
	cosa= dx/len;
		if (l->for_arrow->type != 0)
		hl= l->for_arrow->ht;
		else
                hl = 1.1*l->thickness;
	px = arrowx2 - (int)(hl * cosa +0.5);
	py = arrowy2 - (int)(hl * sina +0.5);
	   fprintf(tfp, "%d,%d\n", (int) (px*mag), (int) (py*mag));
	}	
    fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
	     rgbColorVal (l->pen_color), (int) ceil (linewidth_adj(l->thickness) * mag));

	fprintf (tfp, "stroke-linejoin:%s; stroke-linecap:%s;\n",
			joinstyle[l->join_style],capstyle[l->cap_style]);

        if (l->style > 0) 
	    svg_dash(l->style,l->style_val);

    if (l->fill_style != -1 )
	fprintf (tfp, "fill:#%6.6x;\n", rgbFillVal (l->fill_color, (l->fill_style>40 ? 20 : l->fill_style)));
    fprintf (tfp, "\"/>\n");

    if (l->fill_style > 40) { /*repeat object to paint pattern over fill */

	fprintf (tfp, "<g style=\"stroke:#%6.6x; stroke-width:1\" >\n",
	rgbColorVal(l->pen_color) );
	generate_tile(l->fill_style - 40);
	fprintf (tfp, "</g>\n");	

    fprintf (tfp, "<%s points=\"", (l->type == 1 ? "polyline" : "polygon"));
    for (p = l->points; p; p = p->next) {
	fprintf (tfp, "%d,%d\n", (int) (p->x * mag), (int) (p->y * mag));
    }

    fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
	     rgbColorVal (l->pen_color), (int) ceil (linewidth_adj(l->thickness) * mag));

	fprintf (tfp, "stroke-linejoin:%s; stroke-linecap:%s;\n",
			joinstyle[l->join_style],capstyle[l->cap_style]);

        if (l->style > 0) 
	    svg_dash(l->style,l->style_val);

	fprintf (tfp, "fill:url(#tile%d);\n", tileno);
    fprintf (tfp, "\"/>\n");
    }
    

    if (l->for_arrow != NULL) {
	arrowx2+=l->thickness*cosa;
	arrowy2+=l->thickness*sina;
	svg_arrow(l, l->for_arrow, l->pen_color);
    }
    if (l->back_arrow != NULL) {
	p = l->points;
	if (!p) return; /*safeguard against old, buggy fig files*/
	arrowx2=p->x - l->thickness*cosa1  ;
	arrowy2=p->y - l->thickness*sina1 ;
	p = p->next;
	if (!p) return; /*safeguard against old, buggy fig files*/
	arrowx1 = p->x;
	arrowy1 = p->y;
	svg_arrow(l, l->back_arrow, l->pen_color);
    }
}


void
gensvg_spline (s) /* not used by fig2dev */
     F_spline *s;
{
    fprintf (tfp, "<!-- Spline -->\n");
    print_comments ("<!-- ", s->comments, " -->");

    fprintf (tfp, "<path style=\"stroke:#%6.6x;stroke-width:%d\" d=\"",
	     rgbColorVal (s->pen_color), (int) ceil (linewidth_adj(s->thickness) * mag));
    fprintf (tfp, "M %d,%d \n C", (int) (s->points->x * mag), (int) (s->points->y * mag));
    for (p = s->points++; p; p = p->next) {
	fprintf (tfp, "%d,%d\n", (int) (p->x * mag), (int) (p->y * mag));
    }
    fprintf (tfp, "\"/>\n");
}

void
gensvg_arc (a)
     F_arc  *a;
{
    double     radius;
    double  x, y, angle, dx, dy;

    fprintf (tfp, "<!-- Arc -->\n");
    print_comments ("<!-- ", a->comments, " -->");

    dx = a->point[0].x - a->center.x;
    dy = a->point[0].y - a->center.y;
    radius = sqrt (dx * dx + dy * dy);
    

	x= (a->point[0].x-a->center.x) * (a->point[2].x-a->center.x)
	   +(a->point[0].y-a->center.y) * (a->point[2].y-a->center.y);
	y= (a->point[0].x-a->center.x) * (a->point[2].y-a->center.y)
	   -(a->point[0].y-a->center.y) * (a->point[2].x-a->center.x);

	if (x == 0.0 && y == 0.0) 
		angle=0.0;
	else
	angle = atan2(y,x);
	
	if (angle <0.0) angle += 2.*M_PI;
	
	angle *= 180./M_PI;	   	   

	if (a->direction==1) angle = 360.-angle;

    fprintf (tfp, "<path style=\"stroke:#%6.6x;stroke-width:%d;stroke-linecap:%s;",
    	     rgbColorVal (a->pen_color), (int) ceil (linewidth_adj(a->thickness) * mag),
	     capstyle[a->cap_style]);

        if (a->style > 0) 
	    svg_dash(a->style,a->style_val);

    if (a->fill_style != -1)
	    fprintf (tfp, "fill:#%6.6x\"\n", rgbFillVal (a->fill_color, (a->fill_style > 40 ? 20 : a->fill_style)));
    else 
            fprintf (tfp,"\"\n");

    fprintf (tfp, "d=\"M %d,%d A %d %d % d % d % d % d % d \" />\n",
             (int) (a->point[0].x * mag), 
	     (int) (a->point[0].y * mag),
	     (int) (radius * mag), (int) (radius * mag),
	     0,
	     (fabs(angle) >180. ) ? 1 : 0, 
	     (fabs(angle) >0. && a->direction==0) ? 1 : 0, 
	     (int) (a->point[2].x * mag), (int) (a->point[2].y * mag));

	if (a->fill_style > 40) {

	fprintf (tfp, "<g style=\"stroke:#%6.6x; stroke-width:1\" >\n",
	rgbColorVal(a->pen_color) );
	generate_tile(a->fill_style - 40);
	fprintf (tfp, "</g>\n");	
	
    fprintf (tfp, "<path style=\"stroke:#%6.6x;stroke-width:%d;stroke-linecap:%s;fill:url(#tile%d);\"\n",
	     rgbColorVal (a->pen_color), (int) ceil (linewidth_adj(a->thickness) * mag),
	     capstyle[a->cap_style],tileno);
    fprintf (tfp, "d=\"M %d,%d A %d %d % d % d % d % d % d \" />\n",
             (int) (a->point[0].x * mag), 
	     (int) (a->point[0].y * mag),
	     (int) (radius * mag), (int) (radius * mag),
	     0,
	     (fabs(angle) >180. ) ? 1 : 0, 
	     (fabs(angle) >0. && a->direction==0) ? 1 : 0, 
	     (int) (a->point[2].x * mag), (int) (a->point[2].y * mag));
	}

    if (a->for_arrow) {
	arrowx2 = a->point[2].x;
	arrowy2 = a->point[2].y;
	compute_arcarrow_angle (a->center.x, a->center.y,
				(double) arrowx2, (double) arrowy2,
				a->direction, a->for_arrow, &arrowx1, &arrowy1);
	svg_arrow(a, a->for_arrow, a->pen_color);
    }

    if (a->back_arrow) {
	arrowx2 = a->point[0].x;
	arrowy2 = a->point[0].y;
	compute_arcarrow_angle (a->center.x, a->center.y,
				(double) arrowx2, (double) arrowy2,
				a->direction ^ 1, a->back_arrow, &arrowx1, &arrowy1);
	svg_arrow(a, a->back_arrow, a->pen_color);
    }
}

void
gensvg_ellipse (e)
     F_ellipse *e;
{
    int cx = (int) (e->center.x * mag);
    int cy = (int) (e->center.y * mag);
    if (e->type == T_CIRCLE_BY_RAD || e->type == T_CIRCLE_BY_DIA) {
        int r = (int) (e->radiuses.x * mag);
	fprintf (tfp, "<!-- Circle -->\n");
	print_comments ("<!-- ", e->comments, " -->");
	fprintf (tfp, "<circle cx=\"%d\" cy=\"%d\" r=\"%d\"\n style=\"", cx, cy, r);
	if (e->fill_style != -1)
	    fprintf (tfp, "fill:#%6.6x;", rgbFillVal (e->fill_color, (e->fill_style > 40 ? 20 : e->fill_style)));
        if (e->style > 0) {
	    svg_dash(e->style,e->style_val);
	}
        fprintf (tfp, "stroke:#%6.6x;stroke-width:%d;\"/>\n",
	     rgbColorVal (e->pen_color), (int) ceil (linewidth_adj(e->thickness) * mag));

	if (e->fill_style > 40) {

	fprintf (tfp, "<g style=\"stroke:#%6.6x; stroke-width:1\" >\n",
	rgbColorVal(e->pen_color) );
	generate_tile(e->fill_style - 40);
	fprintf (tfp, "</g>\n");	
	
	fprintf (tfp, "<circle cx=\"%d\" cy=\"%d\" r=\"%d\"\n style=\"", cx, cy, r);
        fprintf (tfp, "fill:url(#tile%d);\n", tileno);
        fprintf (tfp, "stroke:#%6.6x;stroke-width:%d;\"/>\n",
	     rgbColorVal (e->pen_color), (int) ceil (linewidth_adj(e->thickness) * mag));
	}
    }
    else {
	int rx = (int) (e->radiuses.x * mag);
	int ry = (int) (e->radiuses.y * mag);
	fprintf (tfp, "<!-- Ellipse -->\n");
	print_comments ("<!-- ", e->comments, " -->");
	fprintf (tfp, "<ellipse transform=\"translate(%d,%d) rotate(%.8lf)\" rx=\"%d\" ry=\"%d\"\n style=\"",
		 cx, cy, degrees(e->angle), rx, ry);
	if (e->fill_style != -1)
	    fprintf (tfp, "fill:#%6.6x;", rgbFillVal (e->fill_color, (e->fill_style > 40 ? 20 : e->fill_style)));
        if (e->style > 0) 
	    svg_dash(e->style,e->style_val);

        fprintf (tfp, "stroke:#%6.6x;stroke-width:%d;\"/>\n",
        	 rgbColorVal (e->pen_color), (int) ceil (linewidth_adj(e->thickness) * mag));

	if (e->fill_style > 40) {

	fprintf (tfp, "<g style=\"stroke:#%6.6x; stroke-width:1\" >\n",
	rgbColorVal(e->pen_color) );
	generate_tile(e->fill_style - 40);
	fprintf (tfp, "</g>\n");	
	fprintf (tfp, "<ellipse transform=\"translate(%d,%d) rotate(%.8lf)\" rx=\"%d\" ry=\"%d\"\n style=\"",
		 cx, cy, degrees(e->angle), rx, ry);
        fprintf (tfp, "fill:url(#tile%d);\n", tileno);
        fprintf (tfp, "stroke:#%6.6x;stroke-width:%d;\"/>\n",
	         rgbColorVal (e->pen_color), (int) ceil (linewidth_adj(e->thickness) * mag));
	}
    }	
}

void
gensvg_text (t)
     F_text *t;
{
    unsigned char *cp;
    int ch;
    const char *anchor[3] = { "start", "middle", "end" };
    static const char *family[9] = { "Times", "AvantGarde",
	"Bookman", "Courier", "Helvetica", "Helvetica Narrow",
	"New Century Schoolbook", "Palatino", "Times,Symbol"
    };
    int x = (int) (t->base_x * mag);
    int y = (int) (t->base_y * mag);
    int dy = 0;

    fprintf (tfp, "<!-- Text -->\n");
    print_comments ("<!-- ", t->comments, " -->");

    if (t->angle != 0) {
	fprintf (tfp, "<g transform=\"translate(%d,%d) rotate(%.8lf)\" >\n",
		 x, y, degrees (t->angle));
	x = y = 0;
    }
    fprintf (tfp, "<text xml:space=\"preserve\" x=\"%d\" y=\"%d\" fill=\"#%6.6x\"  font-family=\"%s\" "\
	     "font-style=\"%s\" font-weight=\"%s\" font-size=\"%d\" text-anchor=\"%s\">",
	     x, y, rgbColorVal (t->color), family[t->font / 4],
	     ( (t->font % 2 == 0 || t->font >31) ? "normal" : "italic"),
	     ( (t->font % 4 < 2 || t->font >31) ? "normal" : "bold"), (int) (ceil (t->size * 12 * mag)),
	     anchor[t->type]);

    if (t->font == 32) {
	for (cp = (unsigned char *) t->cstring; *cp; cp++) {
		ch=*cp;
	    fprintf (tfp, "&#%d;", symbolchar[ch]);
	}
    }
    else if (t->font == 34) {
	for (cp = (unsigned char *) t->cstring; *cp; cp++) {
		ch=*cp;
	    fprintf (tfp, "&#%d;", dingbatchar[ch]);
	}
    }
    else if (special_text(t)) {
    int supsub=0;
    int old_dy=0;
    dy=0;
	for (cp = (unsigned char *) t->cstring; *cp; cp++) {
	    ch = *cp;
	    if (( supsub == 2 &&ch == '}' ) || supsub==1) {
#ifdef NOSUPER	        
	        fprintf(tfp,"</tspan><tspan dy=\"%d\">",-dy);
                old_dy=-dy;
#else
	        fprintf(tfp,"</tspan>");
#endif
	        supsub=0;
	        if (ch == '}') {
                  cp++;
                  ch=*cp;
                }
            } 
           if (ch == '_' || ch == '^') {
                supsub=1;
#ifdef NOSUPER
                if (dy != 0) fprintf(tfp,"</tspan>");
                if (ch == '_') dy=(int)(35.*mag); 
                if (ch == '^') dy=(int)(-50.*mag);
                fprintf(tfp,"<tspan font-size=\"%d\" dy=\"%d\">",(int) (ceil (t->size * 8 * mag)),dy+old_dy);
                old_dy=0;
#else
                fprintf(tfp,"<tspan font-size=\"%d\" baseline-shift=\"",(int) (ceil (t->size * 8 * mag)));
                if (ch == '_') fprintf(tfp,"sub\">");
                if (ch == '^') fprintf(tfp,"super\">");
#endif
                cp++;
                ch=*cp;
                if (ch == '{' ) { 
                  supsub=2;
                  cp++;
                  ch=*cp;
                }
            } 
#ifdef NOSUPER
                else old_dy=0;
#endif
	    if (ch < 128 && ch != 38 && ch != 60 && ch != 62 
	    && ch != '$')
		(void)fputc (ch, tfp);
	    else if (ch != '$')
		fprintf (tfp, "&#%d;", ch);
    } 
    } else {
	for (cp = (unsigned char *) t->cstring; *cp; cp++) {
	    ch = *cp;
	    if (ch < 128 && ch != 38 && ch != 60 && ch != 62)
		(void)fputc (ch, tfp);
	    else
		fprintf (tfp, "&#%d;", ch);
	}
    }
#ifdef NOSUPER    
    if (dy != 0) fprintf(tfp,"</tspan>");
#endif    
    fprintf (tfp, "</text>\n");
    if (t->angle != 0)
	fprintf (tfp, "</g>");
}

static void
svg_arrow(F_line *obj, F_arrow *arrow, int pen_color)
{
    int     i;
    if (arrow) {
	calc_arrow(arrowx1, arrowy1, arrowx2, arrowy2,
		    obj->thickness, arrow, 
		    points, &npoints, fillpoints, &nfillpoints, clippoints, &nclippoints);

      fprintf (tfp, "<!-- Arrowhead on XXXpoint %d %d - %d %d-->\n",(int)(arrowx1*mag),(int)(arrowy1*mag),(int)(arrowx2*mag),(int)(arrowy2*mag));
      fprintf (tfp, "<%s points=\"", (arrow->type == 0 ? "polyline" : "polygon"));
      for (i = 0; i < npoints; i++) {
          fprintf (tfp, "%d %d\n", (int) (points[i].x * mag),
      	     (int) (points[i].y * mag));
      }
      if (arrow->type > 0)
          fprintf (tfp, "\n");
      fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;stroke-miterlimit:8;\n",
      	 rgbColorVal (pen_color), (int) ceil (linewidth_adj((int)arrow->thickness) * mag));
      if (arrow->type > 0) {
	    if (arrow->style == 0 && nfillpoints == 0)
		fprintf (tfp, "fill:white;\"/>\n");
	    else {
		if (nfillpoints == 0)
		    fprintf (tfp, "fill:#%6.6x;\"/>\n", rgbColorVal (pen_color));
		else {
		    /* first fill with white */
		    fprintf (tfp, "fill:white;\"/>\n");
		    fprintf (tfp, "<!-- Just filled with white now fill special area -->\n");
		    /* now fill the special area */
		    fprintf (tfp, "<path d=\"M ");
		    for (i = 0; i < nfillpoints; i++) {
			fprintf (tfp, "%d %d\n", (int) (fillpoints[i].x * mag),
			     (int) (fillpoints[i].y * mag));
		    }
		    fprintf (tfp, "Z\n");
		    fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;stroke-miterlimit:8;\n",
			 rgbColorVal (pen_color), (int) ceil (linewidth_adj((int)arrow->thickness) * mag));
		    fprintf (tfp, "fill:#%6.6x;\"/>\n", rgbColorVal (pen_color));
		}
	    }
      } else
          fprintf (tfp, "\"/>\n");
    }
}

void generate_tile(int number) {

	tileno++;
	fprintf (tfp, "<defs>\n");
	fprintf (tfp, "<pattern id=\"tile%d\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n",
		tileno);
	fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
	
	switch(number) {
	case 1:	
	    fprintf (tfp, "<path d=\"M 0 -100 200 20\" />\n");
	    fprintf (tfp, "<path d=\"M 0  -60 200 60\" />\n");
	    fprintf (tfp, "<path d=\"M 0  -20 200 100\" />\n");
	    fprintf (tfp, "<path d=\"M 0   20 200 140\" />\n");
	    fprintf (tfp, "<path d=\"M 0   60 200 180\" />\n");
	    fprintf (tfp, "<path d=\"M 0  100 200 220\" />\n");
	    fprintf (tfp, "<path d=\"M 0  140 200 260\" />\n");
	    fprintf (tfp, "<path d=\"M 0  180 200 300\" />\n");
	break;
	
	case 2:
	    fprintf (tfp, "<path d=\"M 200 -100 0  20\" />\n");
	    fprintf (tfp, "<path d=\"M 200  -60 0  60\" />\n");
	    fprintf (tfp, "<path d=\"M 200  -20 0 100\" />\n");
	    fprintf (tfp, "<path d=\"M 200   20 0 140\" />\n");
	    fprintf (tfp, "<path d=\"M 200   60 0 180\" />\n");
	    fprintf (tfp, "<path d=\"M 200  100 0 220\" />\n");
	    fprintf (tfp, "<path d=\"M 200  140 0 260\" />\n");
	    fprintf (tfp, "<path d=\"M 200  180 0 300\" />\n");
	break;
		
	case 3:
        fprintf (tfp, "<path d=\"M 0 -100 200 20\" />\n");
        fprintf (tfp, "<path d=\"M 200 -100 0 20\" />\n");
        fprintf (tfp, "<path d=\"M 0 -60 200 60\" />\n");
        fprintf (tfp, "<path d=\"M 200 -60 0 60\" />\n");
        fprintf (tfp, "<path d=\"M 0 -20 200 100\" />\n");
        fprintf (tfp, "<path d=\"M 200 -20 0 100\" />\n");
        fprintf (tfp, "<path d=\"M 0 20 200 140\" />\n");
        fprintf (tfp, "<path d=\"M 200 20 0 140\" />\n");
        fprintf (tfp, "<path d=\"M 0 60 200 180\" />\n");
        fprintf (tfp, "<path d=\"M 200 60 0 180\" />\n");
        fprintf (tfp, "<path d=\"M 0 100 200 220\" />\n");
        fprintf (tfp, "<path d=\"M 200 100 0 220\" />\n");
        fprintf (tfp, "<path d=\"M 0 140 200 260\" />\n");
        fprintf (tfp, "<path d=\"M 200 140 0 260\" />\n");
        fprintf (tfp, "<path d=\"M 0 180 200 300\" />\n");
        fprintf (tfp, "<path d=\"M 200 180 0 300\" />\n");
	break;

	case 4:	
	fprintf (tfp, "<path d=\"M 100 0 200 100\" />\n");
	fprintf (tfp, "<path d=\"M 0 0 200 200\" />\n");
	fprintf (tfp, "<path d=\"M 0 100 100 200\" />\n");
	break;
		
	case 5:
	fprintf (tfp, "<path d=\"M 100 0 0 100\" />\n");
	fprintf (tfp, "<path d=\"M 200 0 0 200\" />\n");
	fprintf (tfp, "<path d=\"M 200 100 100 200\" />\n");
	break;	

	case 6:
	fprintf (tfp, "<path d=\"M 100 0 200 100\" />\n");
	fprintf (tfp, "<path d=\"M 0 0 200 200\" />\n");
	fprintf (tfp, "<path d=\"M 0 100 100 200\" />\n");
	fprintf (tfp, "<path d=\"M 100 0 0 100\" />\n");
	fprintf (tfp, "<path d=\"M 200 0 0 200\" />\n");
	fprintf (tfp, "<path d=\"M 200 100 100 200\" />\n");
	break;
		
	case 7:
	fprintf (tfp, "<path d=\"M 0 0 0 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 100 50 100 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 0 200\" />\n");
	break;
	
	case 8:	
	fprintf (tfp, "<path d=\"M 0 0 50 0\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
	fprintf (tfp, "<path d=\"M 50 100 150 100\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 200 0\" />\n");
	break;
	
	case 9:
	fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
	break;
		
	case 10:
	fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
	break;
		
	case 11:
	fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
	break;

	case 12: 
	fprintf (tfp, "<path d=\"M 175 0 150 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 100 50 50 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
	fprintf (tfp, "<path d=\"M 200 150 175 200\" />\n");
	break;
		
	case 13:
	fprintf (tfp, "<path d=\"M 13 0 25 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 100 50 125 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 13 200\" />\n");
	break;
		
		
	case 14:
	fprintf (tfp, "<path d=\"M 0 13 50 25\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
	fprintf (tfp, "<path d=\"M 50 100 150 125\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 200 13\" />\n");
	break;
		
	case 15:
	fprintf (tfp, "<path d=\"M 0 13 50 0\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
	fprintf (tfp, "<path d=\"M 50 125 150 100\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
	fprintf (tfp, "<path d=\"M 150 25 200 13\" />\n");
	break;
	
	case 16:	
	fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 1 0 100 50\" />\n");
	fprintf (tfp, "<path d=\"M 100 50 A 50 50 0 1 0 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 50 100 A 50 50 0 1 0 150 100\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 0 0 50 100\" />\n");
	fprintf (tfp, "<path d=\"M 150 100 A 50 50 0 1 0 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 A 50 50 0 1 0 150 0\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 A 50 50 0 0 0 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 0 0 50 0\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 1 0 100 150\" />\n");
	fprintf (tfp, "<path d=\"M 100 150 A 50 50 0 1 0 200 150\" />\n");
	break;
		
	case 17:
	fprintf (tfp, "<g transform=\"scale(0.5)\" >\n");
	fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 1 0 100 50\" />\n");
	fprintf (tfp, "<path d=\"M 100 50 A 50 50 0 1 0 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 50 100 A 50 50 0 1 0 150 100\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 0 0 50 100\" />\n");
	fprintf (tfp, "<path d=\"M 150 100 A 50 50 0 1 0 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 A 50 50 0 1 0 150 0\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 A 50 50 0 0 0 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 0 0 50 0\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 1 0 100 150\" />\n");
	fprintf (tfp, "<path d=\"M 100 150 A 50 50 0 1 0 200 150\" />\n");
	fprintf (tfp, "</g>\n");
	break;
		
	case 18:
	fprintf (tfp, "<circle cx=\"100\" cy=\"100\" r=\"100\" />\n");
	break;
		
	case 19:
	fprintf (tfp, "<path d=\"M 0 50 45 0 105 0 140 50 200 50 \" />\n");
	fprintf (tfp, "<path d=\"M 0 50 45 100 105 100 140 50 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 45 100 105 100 140 150 200 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 45 200 105 200 140 150 200 150\" />\n");
	break;
		
	case 20:
	fprintf (tfp, "<path d=\"M 0 70 65 0 140 0 200 70 \" />\n");
	fprintf (tfp, "<path d=\"M 0 70 0 130 65 200 140 200 200 130 200 70\" />\n");
	break;
		
	case 21:
	fprintf (tfp, "<path d=\"M 50 0 75 25 100 0 M 150 0 175 25 200 0\" />\n");
	fprintf (tfp, "<path d=\"M 0 50 25 25 75 75 125 25 175 75 200 50\" />\n");
	fprintf (tfp, "<path d=\"M 0 100 25 75 75 125 125 75 175 125 200 100\" />\n");
	fprintf (tfp, "<path d=\"M 0 150 25 125 75 175 125 125 175 175 200 150\" />\n");
	fprintf (tfp, "<path d=\"M 0 200 25 175 75 225 125 175 175 225 200 200\" />\n");
	break;
	
	case 22:
	fprintf (tfp, "<path d=\"M 0 50 25 75 0 100 M 0 150 25 175 0 200\" />\n");
	fprintf (tfp, "<path d=\"M 50 0 25 25 75 75 25 125 75 175 50 200\" />\n");
	fprintf (tfp, "<path d=\"M 100 0 75 25 125 75 75 125 125 175 100 200\" />\n");
	fprintf (tfp, "<path d=\"M 150 0 125 25 175 75 125 125 175 175 150 200\" />\n");
	fprintf (tfp, "<path d=\"M 200 0 175 25 225 75 175 125 225 175 200 200\" />\n");
	break;
	
	}
	fprintf (tfp, "</pattern>\n");
	fprintf (tfp, "</defs>\n");
	
	return;

} /* generate_tile */

void svg_dash(int style, double val)
{
	    fprintf (tfp, "stroke-dasharray:");
	    switch (style) {
	      case 1:
	      default:
	          fprintf(tfp,"%d %d;",(int)(val*10*mag),(int)(val*10*mag));
	          break;
             case 2:
                  fprintf(tfp,"10 %d;",(int)(val*10*mag));
                  break;
             case 3:
                  fprintf(tfp,"%d %d 10 %d;",(int)(val*10*mag),
                  (int)(val*5*mag),(int)(val*5*mag));
                  break;
             case 4:      
                  fprintf(tfp,"%d %d 10 %d 10 %d;",(int)(val*10*mag),
                  (int)(val*3*mag),(int)(val*3*mag),(int)(val*3*mag));
                  break;
             case 5:
                  fprintf(tfp,"%d %d 10 %d 10 %d 10 %d;",(int)(val*10*mag),
                  (int)(val*3*mag),(int)(val*3*mag),(int)(val*3*mag),(int)(val*3*mag));
                  break;
             }     
}

/* driver defs */

struct driver dev_svg = {
    gensvg_option,
    gensvg_start,
    gendev_null,
    gensvg_arc,
    gensvg_ellipse,
    gensvg_line,
    gensvg_spline,
    gensvg_text,
    gensvg_end,
    INCLUDE_TEXT
};

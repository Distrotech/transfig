/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
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
 *	genibmgl.c :	IBMGL driver for fig2dev
 *			IBM 6180 Color Plotter with
 *			IBM Graphics Enhancement Cartridge
 * 	Author E. Robert Tisdale, University of California, 1/92
 *	(edwin@cs.ucla.edu)
 *
 *		adapted from:
 *
 *	genpictex.c :	PiCTeX driver for fig2dev
 *
 * 	Author Micah Beck, Cornell University, 4/88
 *	Color, rotated text and ISO-chars added by Herbert Bauer 11/91
 *	PCL job control option added Brian V. Smith 1/2001
 *      Page size handling, orientation, offset, centering
 *         added by Glenn Burkhardt, 11/2003.
*/

/* Notes on offsets and scaling:

   When sending files to a PCL5 printer, e.g., HP4000, Lexmark E312,
   the origin of the plot is the origin of the PCL logical page, which
   is inside the physical page.  In order to get pictures centered on
   these printers, an offset of x=-0.25, y=-0.5 is needed.  Other printers
   have different built in offsets (sometimes, none!).

   Not all printers support all fonts; some printers support only the
   HP stick font.  Default font size values have been chosen to make
   the characters very close to the Courier fixed spacing font.
*/

#include "fig2dev.h"
#include "object.h"

static set_style();

#define		FONTS 			35
#define		COLORS 			8
#define		PATTERNS 		21
#define		DPR	 	180.0/M_PI	/* degrees/radian	*/
#define		DELTA	 	M_PI/36.0	/* radians		*/
#define		UNITS_PER_INCH		 1016.0	/* plotter units/inch
						   1 plotter unit = .025 mm
						*/

#define		ISO_A4_HEIGHT		842	/* 297 mm, in 1/72" points */
#define         ISO_A4_WIDTH		595     /* 210 mm, in 1/72" points */
#define		ANSI_A_HEIGHT		792	/* 11 in, in 1/72" points */
#define		ANSI_A_WIDTH		612	/* 8.5 in, in 1/72" points */
#define		SPEED_LIMIT		128.0	/* centimeters/second	*/

#ifdef IBMGEC
static	int	ibmgec		 = True;
#else
static	int	ibmgec		 = False;
#endif

static	Boolean	pcljcl		 = False;  /* flag to precede IBMGL (HP/GL) output with PCL job control */
static	Boolean	reflected	 = False;
static	int	fonts		 = FONTS;
static	int	colors		 = COLORS;
static	int	patterns	 = PATTERNS;
static	int	line_color	 = DEFAULT;
static	int	line_style	 = SOLID_LINE;
static	int	fill_pattern	 = DEFAULT;
static	double	dash_length	 = DEFAULT;	/* in pixels		*/
#ifdef A4
static	double	pageheight	 = ISO_A4_HEIGHT;
static	double	pagewidth	 = ISO_A4_WIDTH;
#else
static	double	pageheight	 = ANSI_A_HEIGHT;
static	double	pagewidth	 = ANSI_A_WIDTH;
#endif
static	double	pen_speed	 = SPEED_LIMIT;
static	double	xz		 =  0.0;	/* inches		*/
static	double	yz		 =  0.0;	/* inches		*/

/* Default plottable area, changeable with "-d".  Use ISO-B0 for default
   bounds; the page size parameter can further restrict the plottable view.
*/
static	double	xl		 =  0.0;	/* inches		*/
static	double	yl		 =  0.0;	/* inches		*/
static	double	xu		 = 1456/25.4;	/* inches		*/
static	double	yu		 = 1030/25.4;	/* inches		*/

static	int	pen_number[]	 = { 1, 2, 3, 4, 5, 6, 7, 8, 1};
static	double	pen_thickness[]	 = {.3,.3,.3,.3,.3,.3,.3,.3,.3};

static	int	line_type[]	 =
	   {1, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6,-1,-1,-1,-1,-1,-1,-1,-1};
static	double	line_space[]	 =
	   {.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.5,.3,.3};
static	int	fill_type[]	 =
	   {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 1, 2};
static	double	fill_space[]	 =
	   {.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1};
static	double	fill_angle[]	 =
	   {0,-45,0,45,90,-45,0,45,90,-45,0,45,90,-45,0,45,90, 0, 0, 0, 0};

static	int	standard[]	 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static	int	alternate[]	 = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static	double	slant[]		 = { 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,
	     0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0, 0};

/* Make font width/height close to Courier fixed.
 */
static	double	wide[] 
= {.401,.401,.401,.401,.401,.401,.401,.401,.401,.401,.401,.401,
   .401,.401,.401,.401,.401,.401,.401,.401,.401,.401,.401,.401,
   .401,.401,.401,.401,.401,.401,.401,.401,.401,.401,.401,.401};
static	double	high[] 
= {.561,.561,.561,.561,.561,.561,.561,.561,.561,.561,.561,.561,
   .561,.561,.561,.561,.561,.561,.561,.561,.561,.561,.561,.561,
   .561,.561,.561,.561,.561,.561,.561,.561,.561,.561,.561,.561};

/* Map of Postscript font number to HPGL/2 Fonts
 */
static struct {
    int font, italic, bold;
} psfont2hpgl[] =
{
    /* "Times-Roman",			0 */  { 5, 0, 0 },
    /* "Times-Italic",			1 */  { 5, 1, 0 },
    /* "Times-Bold",			2 */  { 5, 0, 3 },
    /* "Times-BoldItalic",		3 */  { 5, 1, 3 },
    /* "AvantGarde-Book",		4 */  { 31, 0, 0 },
    /* "AvantGarde-BookOblique",	5 */  { 31, 1, 0 },
    /* "AvantGarde-Demi",		6 */  { 31, 0, 3 },
    /* "AvantGarde-DemiOblique",	7 */  { 31, 1, 3 },
    /* "Bookman-Light",			8 */  { 47, 0, 0 },
    /* "Bookman-LightItalic",		9 */  { 47, 1, 0 },
    /* "Bookman-Demi",			10 */ { 47, 0, 3 },
    /* "Bookman-DemiItalic",		11 */ { 47, 1, 3 },
    /* "Courier",			12 */ { 3, 0, 0 },
    /* "Courier-Oblique",		13 */ { 3, 1, 0 },
    /* "Courier-Bold",			14 */ { 3, 0, 3 },
    /* "Courier-BoldOblique",		15 */ { 3, 1, 3 },
    /* "Helvetica",			16 */ { 4, 0, 0 },
    /* "Helvetica-Oblique",		17 */ { 4, 1, 0 },
    /* "Helvetica-Bold",		18 */ { 4, 0, 3 },
    /* "Helvetica-BoldOblique",		19 */ { 4, 1, 3 },
    /* "Helvetica-Narrow",		20 */ { 4, 0, 0 },
    /* "Helvetica-Narrow-Oblique",	21 */ { 4, 1, 0 },
    /* "Helvetica-Narrow-Bold",		22 */ { 4, 0, 3 },
    /* "Helvetica-Narrow-BoldOblique",	23 */ { 4, 1, 3 },
    /* "NewCenturySchlbk-Roman",	24 */ { 23, 0, },
    /* "NewCenturySchlbk-Italic",	25 */ { 23, 1, },
    /* "NewCenturySchlbk-Bold",		26 */ { 23, 0, },
    /* "NewCenturySchlbk-BoldItalic",	27 */ { 23, 1, },
    /* "Palatino-Roman",		28 */ { 15, 0, 0 },
    /* "Palatino-Italic",		29 */ { 15, 1, 0 },
    /* "Palatino-Bold",			30 */ { 15, 0, 3 },
    /* "Palatino-BoldItalic",		31 */ { 15, 1, 3 },
    /* "Symbol",			32 */ { 52, 0, 0 }, /* Univers */
    /* "ZapfChancery-MediumItalic",	33 */ { 43, 0, 0 },
    /* "ZapfDingbats",			34 */ { 45, 0, 0 },
};

static void genibmgl_option(opt, optarg)
char opt, *optarg;
{
	FILE	*ffp;
	int	 font;
	int	 pattern;

	switch (opt) {

	    case 'a':				/* paper size		*/
#ifdef A4
	        pageheight	 = ANSI_A_HEIGHT;
		pagewidth	 = ANSI_A_WIDTH;
#else
		pageheight	 = ISO_A4_HEIGHT;
		pagewidth	 = ISO_A4_WIDTH;
#endif
		break;

	    case 'c':				/* Graphics Enhancement	*/
		ibmgec		 = !ibmgec;	/* Cartridge emulation	*/
		break;

	    case 'd':				/* position and window	*/
		sscanf(optarg, "%lf,%lf,%lf,%lf", &xl,&yl,&xu,&yu);
						/* inches		*/
		break;

	    case 'f':				/* user's characters	*/
		if ((ffp = fopen(optarg, "r")) == NULL)
		    fprintf(stderr, "Couldn't open %s\n", optarg);
		else
		    for (font = 0; font <= fonts; font++)
			fscanf(ffp, "%d%d%lf%lf%lf",
				&standard[font],	/* 0-4 6-9 30-39*/
				&alternate[font],	/* 0-4 6-9 30-39*/
				&slant[font],		/*   degrees	*/
				&wide[font],		/*	~1.0	*/
				&high[font]);		/*	~1.0	*/
		fclose(ffp);
		break;

	    case 'k':				/* precede output with PCL job control */
		pcljcl = True;
		break;

	    case 'l':				/* user's fill patterns	*/
		if ((ffp = fopen(optarg, "r")) == NULL)
		    fprintf(stderr, "Couldn't open %s\n", optarg);
		else
		    for (pattern = 0; pattern < patterns; pattern++)
			fscanf(ffp, "%d%lf%d%lf%lf",
				&line_type[pattern],	/*    -1-6	*/
				&line_space[pattern],	/*   inches	*/
				&fill_type[pattern],	/*     1-5	*/
				&fill_space[pattern],	/*   inches	*/
				&fill_angle[pattern]);	/*   degrees	*/
		fclose(ffp);
		break;

      	    case 'F':				/* use specified font,
						   i.e., SD command
						 */
	    case 's':
	    case 'L':				/* language		*/
		break;

	    case 'm':				/* magnify and offset	*/
		sscanf(optarg, "%lf,%lf,%lf", &mag,&xz,&yz);
						/* inches		*/
		break;

	    case 'p':				/* user's colors	*/
		{
		    FILE	*ffp;
		    int		color;
		    if ((ffp = fopen(optarg, "r")) == NULL)
			fprintf(stderr, "Couldn't open %s\n", optarg);
		    else
			for (color = 0; color <= colors; color++)
			    fscanf(ffp, "%d%lf",
				&pen_number[color],	/*     1-8	*/
				&pen_thickness[color]);	/* millimeters	*/
		    fclose(ffp);
		}
		break;

	    case 'P':				/* portrait mode	*/
		landscape	 = False;
		orientspec	 = True;	/* user-specified	*/
		break;

	    case 'S':				/* select pen velocity	*/
		pen_speed	 = atof(optarg);
		break;

	    case 'v':
		reflected	 = True;	/* mirror image		*/
		break;

	    case 'z':			/* papersize */
	        (void) strcpy (papersize, optarg);
		paperspec = True;	/* user-specified */
		break;	    

	    default:
		put_msg(Err_badarg, opt, "ibmgl");
		exit(1);
	}
}

static double		cpi;			/*       cent/inch	*/
static double		cpp;			/*       cent/pixel	*/
static double		wcmpp;			/* centimeter/point	*/
static double		hcmpp;			/* centimeter/point	*/

void genibmgl_start(objects)
F_compound	*objects;
{
	int	 P1x, P1y, P2x, P2y;
	int	 Xll, Yll, Xur, Yur;
	double	Xmin,Xmax,Ymin,Ymax,xoff=0,yoff=0;
	double	height, width, points_per_inch;
	struct paperdef	*pd;

	if (fabs(mag) < 1.0/2048.0){
	    fprintf(stderr, "|mag| < 1/2048\n");
	    exit(1);
	    }

	if (paperspec) {
	    /* convert ledger (deprecated) to tabloid */
	    if (strcasecmp(papersize, "ledger") == 0)
		strcpy(papersize, "tabloid");

	    for (pd = paperdef; pd->name != NULL; pd++)
		if (strcasecmp (papersize, pd->name) == 0) {
		    pagewidth = pd->width;	/* in points, 1/72" */
		    pageheight = pd->height;
		    strcpy(papersize,pd->name);	/* use the "nice" form */
		    break;
		    }
	
	    if (pagewidth < 0 || pageheight < 0) {
		fprintf (stderr, "Unknown paper size `%s'\n", papersize);
		exit (1);
	        }
	    }

	points_per_inch = 72;
	pagewidth  /= points_per_inch;	/* convert to inches */
	pageheight /= points_per_inch;
	wcmpp = hcmpp = 2.54/points_per_inch;

	if (xl < xu)
	    if (0.0 < xu)
		if (xl < pageheight) {
		    xl	 = (0.0 < xl) ? xl: 0.0;
		    xu	 = (xu < pageheight) ? xu: pageheight;
		    }
		else {
		    fprintf(stderr, "xll >= %.2f\n", pageheight);
		    exit(1);
		    }
	    else {
		fprintf(stderr, "xur <= 0.0\n");
		exit(1);
		}
	else {
	    fprintf(stderr, "xur <= xll\n");
	    exit(1);
	    }

	if (yl < yu)
	    if (0.0 < yu)
		if (yl < pagewidth) {
		    yl	 = (0.0 < yl) ? yl: 0.0;
		    yu	 = (yu < pagewidth) ? yu: pagewidth;
		    }
		else {
		    fprintf(stderr, "yll >= %.2f\n", pagewidth);
		    exit(1);
		    }
	    else {
		fprintf(stderr, "yur <= 0.0\n");
		exit(1);
		}
	else {
	    fprintf(stderr, "yur <= yll\n");
	    exit(1);
	    }

	cpi	 = mag*100.0/sqrt((xu-xl)*(xu-xl) + (yu-yl)*(yu-yl));
	cpp	 = cpi/ppi;

	/* IBMGL start */
	if (pcljcl) {
	    fprintf(tfp,"\033E");
	    if (landscape) fprintf(tfp, "\033&l1O");
	    fprintf(tfp, "\033%%0B");	/* reset and set to HP/GL mode */
	}

	fprintf(tfp, "BP;IN;\n");		/* initialize plotter	*/

	if (!landscape) {			/* portrait mode	*/
	    fprintf(tfp, "RO90;\n");		/* rotate 90 degrees	*/
	    Xll	 = yl*UNITS_PER_INCH;
	    Xur	 = yu*UNITS_PER_INCH;
	    Yll	 = (pageheight - xu)*UNITS_PER_INCH;
	    Yur	 = (pageheight - xl)*UNITS_PER_INCH;
	    height	 = yu - yl;
	    width	 = xu - xl;
	    P1x	 	 = Xll;
	    P2x		 = Xur;
	    if (reflected)			/* upside-down text	*/
		hcmpp	 = -hcmpp;
	    if (reflected) {			/* reflected */
		P1y	 = Yll;
		P2y	 = Yur;
	    } else {
		P1y	 = Yur;
		P2y	 = Yll;
	    }

	    /* If asked to center, use the plot bounds, and page size
	       to get offsets.
	    */
	    if (center) {
		yoff = (pageheight - (urx - llx)*mag/ppi)/2 - llx*mag/ppi;
		xoff = (pagewidth - (ury - lly)*mag/ppi)/2 - lly*mag/ppi;
	    }
	} else {				      /* landscape mode	*/
	    Xll	 = xl*UNITS_PER_INCH;
	    Yll	 = yl*UNITS_PER_INCH;
	    Yur	 = yu*UNITS_PER_INCH;
	    Xur	 = xu*UNITS_PER_INCH;
	    height	 = xu - xl;
	    width	 = yu - yl;
	    if (reflected) {			/* flipped   or not	*/
		wcmpp	 = -wcmpp;		/* backward text	*/
		P1x	 = Xur;
		P2x	 = Xll;
	    } else {				/* normal		*/
		P1x	 = Xll;
		P2x	 = Xur;
	    }
	    P1y	 = Yur;
	    P2y	 = Yll;

	    /* If asked to center, use the plot bounds, border, and page size
	       to get offsets.
	    */
	    if (center) {
		xoff = (pageheight - (urx - llx)*mag/ppi)/2 - llx*mag/ppi;
		yoff = (pagewidth - (ury - lly)*mag/ppi)/2 - lly*mag/ppi;
	    }
	}

	if (xoff < 0) xoff = 0;
	if (yoff < 0) yoff = 0;

	Xmin	 = -xz - xoff;			/* Inches */
	Ymin	 = yz - yoff;
	Xmax	 = -xz - xoff + height/mag;
	Ymax	 = yz - yoff + width/mag;

	fprintf(tfp, "IP%d,%d,%d,%d;\n",    /* reference points for scaling */
		P1x, P1y, P2x, P2y);
	fprintf(tfp, "IW%d,%d,%d,%d;\n",    /* soft clip limits */
		Xll, Yll, Xur, Yur);

	/* 'SC' maps the coordinates used in the plotting commands to the
	   absolute plotter coordinates (units of .025mm).  This
	   can both translate and scale.  The values given are mapped
	   onto the absolute plotter coords given in the 'IP' command
	*/
	fprintf(tfp, "SC%.4f,%.4f,%.4f,%.4f;\n",
		Xmin,Xmax,Ymin,Ymax);
	if (0.0 < pen_speed && pen_speed < SPEED_LIMIT)
	    fprintf(tfp, "VS%.2f;\n", pen_speed);
}

static arc_tangent(x1, y1, x2, y2, direction, x, y)
double	x1, y1, x2, y2, *x, *y;
int	direction;
{
	if (direction) { /* counter clockwise  */
	    *x = x2 - (y2 - y1);
	    *y = y2 + (x2 - x1);
	    }
	else {
	    *x = x2 + (y2 - y1);
	    *y = y2 - (x2 - x1);
	    }
	}

/*	draw arrow heading from (x1, y1) to (x2, y2)	*/

static draw_arrow_head(x1, y1, x2, y2, arrowht, arrowwid)
double	x1, y1, x2, y2, arrowht, arrowwid;
{
	double	x, y, xb, yb, dx, dy, l, sina, cosa;
	double	xc, yc, xd, yd;
	int style;
	double length;

	dx	 = x2 - x1;
	dy	 = y1 - y2;
	l	 = sqrt(dx*dx+dy*dy);
	sina	 = dy/l;
	cosa	 = dx/l;
	xb	 = x2*cosa - y2*sina;
	yb	 = x2*sina + y2*cosa;
	x	 = xb - arrowht;
	y	 = yb - arrowwid/2.0;
	xc	 =  x*cosa + y*sina;
	yc	 = -x*sina + y*cosa;
	y	 = yb + arrowwid/2.0;
	xd	 =  x*cosa + y*sina;
	yd	 = -x*sina + y*cosa;

	/* save line style and set to solid */
	style	 = line_style;
	length	 = dash_length;
	set_style(SOLID_LINE, 0.0);

	fprintf(tfp, "PA%.4f,%.4f;PD%.4f,%.4f,%.4f,%.4f;PU\n",
		xc, yc, x2, y2, xd, yd);

	/* restore line style */
	set_style(style, length);
	}

/* 
 * set_style - issue line style commands as appropriate
 */
static set_style(style, length)
int	style;
double	length;
{
	if (style == line_style)
	    switch (line_style) {
		case SOLID_LINE:
		    break;

		case DASH_LINE:
		    if (dash_length != length && length > 0.0) {
			dash_length  = length;
			fprintf(tfp, "LT2,%.4f;\n", dash_length*2.0*cpp);
			}
		    break;

		case DOTTED_LINE:
		    if (dash_length != length && length > 0.0) {
			dash_length  = length;
			fprintf(tfp, "LT1,%.4f;\n", dash_length*2.0*cpp);
			}
		    break;
		}
	else {
	    line_style = style;
	    switch (line_style) {
		case SOLID_LINE:
		    fprintf(tfp, "LT;\n");
		    break;

		case DASH_LINE:
		    if (dash_length != length && length > 0.0)
			dash_length  = length;
		    if (dash_length > 0.0)
			fprintf(tfp, "LT2,%.4f;\n", dash_length*2.0*cpp);
		    else
			fprintf(tfp, "LT2,-1.0;\n");
		    break;

		case DOTTED_LINE:
		    if (dash_length != length && length > 0.0)
			dash_length  = length;
		    if (dash_length > 0.0)
			fprintf(tfp, "LT1,%.4f;\n", dash_length*2.0*cpp);
		    else
			fprintf(tfp, "LT1,-1.0;\n");
		    break;
		}
	    }
}

/* 
 * set_width - issue line width commands as appropriate
 *		NOTE: HPGL/2 command used
 */
static set_width(w)
    int	w;
{
    static int current_width=-1;

    if (w == current_width) return;

    /* Default line width is 0.3 mm; back off to original xfig pen
       thickness number, and re-size.
    */
    fprintf(tfp, "PW%.1f;\n", w*80/ppi * 0.3);

    current_width = w;
}

/* 
 * set_color - issue line color commands as appropriate
 */
static set_color(color)
    int	color;
{
    static	int	number		 = 0;	/* 1 <= number <= 8		*/
    static	double	thickness	 = 0.3;	/* pen thickness in millimeters	*/
	if (line_color != color) {
	    line_color  = color;
	    color	= (colors + color)%colors;
	    if (number != pen_number[color]) {
		number  = pen_number[color];
		fprintf(tfp, "SP%d;\n", pen_number[color]);
		}
	    if (thickness != pen_thickness[color]) {
		thickness  = pen_thickness[color];
		fprintf(tfp, "PW%.4f;\n", pen_thickness[color]);
		}
	    }
}

static fill_polygon(pattern, color)
    int	pattern;
    int	color;
{
	if (0 < pattern && pattern < patterns) {
	    int		style;
	    double	length;

	    set_color(color);
	    if (fill_pattern != pattern) {
		fill_pattern  = pattern;
		fprintf(tfp, "FT%d,%.4f,%.4f;", fill_type[pattern],
			fill_space[pattern],
			reflected ? -fill_angle[pattern]: fill_angle[pattern]);
		}
	    /*    save line style */
	    style	 = line_style;
	    length	 = dash_length;
	    fprintf(tfp, "LT%d,%.4f;FP;\n",
		    line_type[pattern], line_space[pattern]*cpi);
	    /* restore line style */
	    line_style	 = DEFAULT;
	    dash_length	 = DEFAULT;
	    set_style(style, length);
	    }
}
	
void arc(sx, sy, cx, cy, theta, delta)
    double	sx, sy, cx, cy, theta, delta;
{
	if (ibmgec)
	    if (delta == M_PI/36.0)		/* 5 degrees		*/
		fprintf(tfp, "AA%.4f,%.4f,%.4f;",
			cx, cy, theta*DPR);
	    else
		fprintf(tfp, "AA%.4f,%.4f,%.4f,%.4f;",
			cx, cy, theta*DPR, delta*DPR);
	else {
	    double	alpha;
	    if (theta < 0.0)
		delta = -fabs(delta);
	    else
		delta = fabs(delta);
	    for (alpha = delta; fabs(alpha) < fabs(theta); alpha += delta) {
		fprintf(tfp, "PA%.4f,%.4f;\n",
	    		cx + (sx - cx)*cos(alpha) - (sy - cy)*sin(alpha),
	    		cy + (sy - cy)*cos(alpha) + (sx - cx)*sin(alpha));
		}
	    fprintf(tfp, "PA%.4f,%.4f;\n",
	    	    cx + (sx - cx)*cos(theta) - (sy - cy)*sin(theta),
	    	    cy + (sy - cy)*cos(theta) + (sx - cx)*sin(theta));
	    }
}

void genibmgl_arc(a)
    F_arc	*a;
{
	if (a->thickness != 0 ||
		ibmgec && 0 <= a->fill_style && a->fill_style < patterns) {
	    double	x, y;
	    double	cx, cy, sx, sy, ex, ey;
	    double	dx1, dy1, dx2, dy2, theta;

	    set_style(a->style, a->style_val);
	    set_width(a->thickness);
	    set_color(a->pen_color);

	    cx		 = a->center.x/ppi;
	    cy		 = a->center.y/ppi;
	    sx		 = a->point[0].x/ppi;
	    sy		 = a->point[0].y/ppi;
	    ex		 = a->point[2].x/ppi;
	    ey		 = a->point[2].y/ppi;

	    dx1		 = sx - cx;
	    dy1		 = sy - cy;
	    dx2		 = ex - cx;
	    dy2		 = ey - cy;
	    
	    theta	 = atan2(dy2, dx2) - atan2(dy1, dx1);
	    if (a->direction) {
		if (theta > 0.0)
		    theta	-= 2.0*M_PI;
	    } else {
		if (theta < 0.0)
		    theta	+= 2.0*M_PI;
	    }

	    if (a->type == T_OPEN_ARC && a->thickness != 0 && a->back_arrow) {
		arc_tangent(cx, cy, sx, sy, !a->direction, &x, &y);
		draw_arrow_head(x, y, sx, sy,
		a->back_arrow->ht/ppi, a->back_arrow->wid/ppi);
		}

	    fprintf(tfp, "PA%.4f,%.4f;PM;PD;", sx, sy);
	    arc(sx, sy, cx, cy, theta, DELTA);
	    fprintf(tfp, "PU;PM2;\n");

	    if (a->thickness != 0)
		fprintf(tfp, "EP;\n");

	    if (a->type == T_OPEN_ARC && a->thickness != 0 && a->for_arrow) {
		arc_tangent(cx, cy, ex, ey, a->direction, &x, &y);
		draw_arrow_head(x, y, ex, ey,
			a->for_arrow->ht/ppi, a->for_arrow->wid/ppi);
		}

	    if (0 < a->fill_style && a->fill_style < patterns)
		fill_polygon(a->fill_style, a->fill_color);
	    }
}

void genibmgl_ellipse(e)
    F_ellipse	*e;
{
	if (e->thickness != 0 ||
		ibmgec && 0 <= e->fill_style && e->fill_style < patterns) {
	    int		j;
	    double	alpha;
	    double	angle;
	    double	delta;
	    double	x0, y0;
	    double	a,  b;
	    double	x,  y;

	    set_style(e->style, e->style_val);
	    set_width(e->thickness);
	    set_color(e->pen_color);

	    a		 = e->radiuses.x/ppi;
	    b		 = e->radiuses.y/ppi;
	    x0		 = e->center.x/ppi;
	    y0		 = e->center.y/ppi;
	    angle	 = -e->angle;
	    delta	 = -DELTA;

	    x		 = x0 + cos(angle)*a;
	    y		 = y0 + sin(angle)*a;
	    fprintf(tfp, "PA%.4f,%.4f;PM;PD;\n", x, y);
	    for (j = 1; j <= 72; j++) { 
		alpha	 = j*delta;
		x	 = x0 + cos(angle)*a*cos(alpha)
	    		 - sin(angle)*b*sin(alpha);
		y	 = y0 + sin(angle)*a*cos(alpha)
	    		 + cos(angle)*b*sin(alpha);
		fprintf(tfp, "PA%.4f,%.4f;\n", x, y);
	    }
	    fprintf(tfp, "PU;PM2;\n");

	    if (e->thickness != 0)
		fprintf(tfp, "EP;\n");

	    if (0 < e->fill_style && e->fill_style < patterns)
		fill_polygon((int)e->fill_style, e->fill_color);
	    }
}

void swap(i, j)
    int	*i, *j;
{
	int	t;
	t = *i;
	*i = *j;
	*j = t;
}

void genibmgl_line(l)
    F_line	*l;
{
	if (l->thickness != 0 ||
		ibmgec && 0 <= l->fill_style && l->fill_style < patterns) {
	    F_point	*p, *q;

	    set_style(l->style, l->style_val);
	    set_width(l->thickness);
	    set_color(l->pen_color);

	    p	 = l->points;
	    q	 = p->next;

	    switch (l->type) {
		case	T_POLYLINE:
		case	T_BOX:
		case	T_POLYGON:
		    if (q == NULL)		/* A single point line */
			fprintf(tfp, "PA%.4f,%.4f;PD;PU;\n",
				p->x/ppi, p->y/ppi);
		    else {
			if (l->thickness != 0 && l->back_arrow)
			    draw_arrow_head(q->x/ppi, q->y/ppi,
		    		    p->x/ppi, p->y/ppi,
				    l->back_arrow->ht/ppi,
				    l->back_arrow->wid/ppi);

			fprintf(tfp, "PA%.4f,%.4f;PM;PD%.4f,%.4f;\n",
				p->x/ppi, p->y/ppi,
				q->x/ppi, q->y/ppi);
			while (q->next != NULL) {
			    p	 = q;
			    q	 = q->next;
			    fprintf(tfp, "PA%.4f,%.4f;\n",
				    q->x/ppi, q->y/ppi);
			    }
			fprintf(tfp, "PU;PM2;\n");

			if (l->thickness != 0)
			    fprintf(tfp, "EP;\n");

			if (l->thickness != 0 && l->for_arrow)
		    	    draw_arrow_head(p->x/ppi, p->y/ppi,
				    q->x/ppi, q->y/ppi,
				    l->for_arrow->ht/ppi,
				    l->for_arrow->wid/ppi);

			if (0 < l->fill_style && l->fill_style < patterns)
			    fill_polygon((int)l->fill_style, l->fill_color);
			}
		    break;

		case	T_ARC_BOX: {
		    int		llx, lly, urx, ury;
		    double	 x0,  y0,  x1,  y1;
		    double	dx, dy, angle;

		    llx	 = urx	= p->x;
		    lly	 = ury	= p->y;
		    while ((p = p->next) != NULL) {
			if (llx > p->x)
			    llx = p->x;
			if (urx < p->x)
			    urx = p->x;
			if (lly > p->y)
			    lly = p->y;
			if (ury < p->y)
			    ury = p->y;
			}

		    x0	 = llx/ppi;
		    x1	 = urx/ppi;
		    dx	 = l->radius/ppi;
		    y0	 = ury/ppi;
		    y1	 = lly/ppi;
		    dy	 = -dx;
		    angle = -M_PI/2.0;

		    fprintf(tfp, "PA%.4f,%.4f;PM;PD;\n",  x0, y0 + dy);
		    arc(x0, y0 + dy, x0 + dx, y0 + dy, angle, DELTA);
		    fprintf(tfp, "PA%.4f,%.4f;\n", x1 - dx, y0);
		    arc(x1 - dx, y0, x1 - dx, y0 + dy, angle, DELTA);
		    fprintf(tfp, "PA%.4f,%.4f;\n", x1, y1 - dy);
		    arc(x1, y1 - dy, x1 - dx, y1 - dy, angle, DELTA);
		    fprintf(tfp, "PA%.4f,%.4f;\n", x0 + dx, y1);
		    arc(x0 + dx, y1, x0 + dx, y1 - dy, angle, DELTA);
		    fprintf(tfp, "PA%.4f,%.4f;PU;PM2;\n", x0, y0 + dy);

		    if (l->thickness != 0)
			fprintf(tfp, "EP;\n");

		    if (0 < l->fill_style && l->fill_style < patterns)
			fill_polygon((int)l->fill_style, l->fill_color);
		    }
		    break;

		case	T_PIC_BOX:
		    fprintf(stderr,"Warning: Pictures not supported in IBMGL language\n");
		    break;
		}
	    }
}

#define		THRESHOLD	.05	/* inch */

static bezier_spline(a0, b0, a1, b1, a2, b2, a3, b3)
    double	a0, b0, a1, b1, a2, b2, a3, b3;
{
	double	x0, y0, x3, y3;
	double	sx1, sy1, sx2, sy2, tx, ty, tx1, ty1, tx2, ty2, xmid, ymid;

	x0 = a0; y0 = b0;
	x3 = a3; y3 = b3;
	if (fabs(x0 - x3) < THRESHOLD && fabs(y0 - y3) < THRESHOLD)
	    fprintf(tfp, "PA%.4f,%.4f;\n", x3, y3);

	else {
	    tx   = (a1  + a2 )/2.0;	ty   = (b1  + b2 )/2.0;
	    sx1  = (x0  + a1 )/2.0;	sy1  = (y0  + b1 )/2.0;
	    sx2  = (sx1 + tx )/2.0;	sy2  = (sy1 + ty )/2.0;
	    tx2  = (a2  + x3 )/2.0;	ty2  = (b2  + y3 )/2.0;
	    tx1  = (tx2 + tx )/2.0;	ty1  = (ty2 + ty )/2.0;
	    xmid = (sx2 + tx1)/2.0;	ymid = (sy2 + ty1)/2.0;

	    bezier_spline(x0, y0, sx1, sy1, sx2, sy2, xmid, ymid);
	    bezier_spline(xmid, ymid, tx1, ty1, tx2, ty2, x3, y3);
	    }
}

static void genibmgl_itp_spline(s)
F_spline	*s;
{
	F_point		*p1, *p2;
	F_control	*cp1, *cp2;
	double		x1, x2, y1, y2;

	p1 = s->points;
	cp1 = s->controls;
	x2 = p1->x/ppi; y2 = p1->y/ppi;

	if (s->thickness != 0 && s->back_arrow)
	    draw_arrow_head(cp1->rx/ppi, cp1->ry/ppi, x2, y2,
		    s->back_arrow->ht/ppi, s->back_arrow->wid/ppi);

	fprintf(tfp, "PA%.4f,%.4f;PD;\n", x2, y2);
	for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
		p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {
	    x1	 = x2;
	    y1	 = y2;
	    x2	 = p2->x/ppi;
	    y2	 = p2->y/ppi;
	    bezier_spline(x1, y1, (double)cp1->rx/ppi, cp1->ry/ppi,
		(double)cp2->lx/ppi, cp2->ly/ppi, x2, y2);
	    }
	fprintf(tfp, "PU;\n");

	if (s->thickness != 0 && s->for_arrow)
	    draw_arrow_head(cp1->lx/ppi, cp1->ly/ppi, x2, y2,
		    s->for_arrow->ht/ppi, s->for_arrow->wid/ppi);
	}

static quadratic_spline(a1, b1, a2, b2, a3, b3, a4, b4)
double	a1, b1, a2, b2, a3, b3, a4, b4;
{
	double	x1, y1, x4, y4;
	double	xmid, ymid;

	x1	 = a1; y1 = b1;
	x4	 = a4; y4 = b4;
	xmid	 = (a2 + a3)/2.0;
	ymid	 = (b2 + b3)/2.0;
	if (fabs(x1 - xmid) < THRESHOLD && fabs(y1 - ymid) < THRESHOLD)
	    fprintf(tfp, "PA%.4f,%.4f;\n", xmid, ymid);
	else {
	    quadratic_spline(x1, y1, ((x1+a2)/2.0), ((y1+b2)/2.0),
		((3.0*a2+a3)/4.0), ((3.0*b2+b3)/4.0), xmid, ymid);
	    }

	if (fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD)
	    fprintf(tfp, "PA%.4f,%.4f;\n", x4, y4);
	else {
	    quadratic_spline(xmid, ymid, ((a2+3.0*a3)/4.0), ((b2+3.0*b3)/4.0),
			((a3+x4)/2.0), ((b3+y4)/2.0), x4, y4);
	    }
	}

static void genibmgl_ctl_spline(s)
F_spline	*s;
{
	F_point	*p;
	double	cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
	double	x1, y1, x2, y2;

	p	 = s->points;
	x1	 = p->x/ppi;
	y1	 = p->y/ppi;
	p	 = p->next;
	x2	 = p->x/ppi;
	y2	 = p->y/ppi;
	cx1	 = (x1 + x2)/2.0;
	cy1	 = (y1 + y2)/2.0;
	cx2	 = (x1 + 3.0*x2)/4.0;
	cy2	 = (y1 + 3.0*y2)/4.0;

	if (closed_spline(s))
	    fprintf(tfp, "PA%.4f,%.4f;PD;\n ", cx1, cy1);
	else {
	    if (s->thickness != 0 && s->back_arrow)
		draw_arrow_head(cx1, cy1, x1, y1,
			s->back_arrow->ht/ppi, s->back_arrow->wid/ppi);
	    fprintf(tfp, "PA%.4f,%.4f;PD%.4f,%.4f;\n",
		    x1, y1, cx1, cy1);
	    }

	for (p = p->next; p != NULL; p = p->next) {
	    x1	 = x2;
	    y1	 = y2;
	    x2	 = p->x/ppi;
	    y2	 = p->y/ppi;
	    cx3	 = (3.0*x1 + x2)/4.0;
	    cy3	 = (3.0*y1 + y2)/4.0;
	    cx4	 = (x1 + x2)/2.0;
	    cy4	 = (y1 + y2)/2.0;
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    cx1	 = cx4;
	    cy1	 = cy4;
	    cx2	 = (x1 + 3.0*x2)/4.0;
	    cy2	 = (y1 + 3.0*y2)/4.0;
	    }
	x1	 = x2; 
	y1	 = y2;
	p	 = s->points->next;
	x2	 = p->x/ppi;
	y2	 = p->y/ppi;
	cx3	 = (3.0*x1 + x2)/4.0;
	cy3	 = (3.0*y1 + y2)/4.0;
	cx4	 = (x1 + x2)/2.0;
	cy4	 = (y1 + y2)/2.0;
	if (closed_spline(s)) {
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    fprintf(tfp, "PU;\n");
	    }
	else {
	    fprintf(tfp, "PA%.4f,%.4f;PU;\n", x1, y1);
	    if (s->thickness != 0 && s->for_arrow)
	    	draw_arrow_head(cx1, cy1, x1, y1,
			s->for_arrow->ht/ppi, s->for_arrow->wid/ppi);
	    }
	}

void genibmgl_spline(s)
F_spline	*s;
{
	if (s->thickness != 0) {
	    set_style(s->style, s->style_val);
	    set_width(s->thickness);
	    set_color(s->pen_color);

	    if (int_spline(s))
		genibmgl_itp_spline(s);
	    else
		genibmgl_ctl_spline(s);
	}

	if (0 < s->fill_style && s->fill_style < patterns)
	    fprintf(stderr, "Spline area fill not implemented\n");
}

#define	FONT(T) ((-1 < (T) && (T) < FONTS) ? (T): fonts)
void genibmgl_text(t)
F_text	*t;
{
static	int	font	 = DEFAULT;	/* font				*/
static	int	size	 = DEFAULT;	/* font size	    in points	*/
static	int	cs	 = 0;		/* standard  character set	*/
static	int	ca	 = 0;		/* alternate character set	*/
static	double	theta	 = 0.0;		/* character slant  in degrees	*/
static	double	angle	 = 0.0;		/* label direction  in radians	*/
	double	width;			/* character width  in centimeters */
	double	height;			/* character height in centimeters */
	Boolean newfont=False, newsize=False;

	if (font != FONT(t->font)) {
	    font  = FONT(t->font);
	    /* Simulate italic fonts with a 10 degree slant */
	    if (theta != slant[font]) {
		theta  = slant[font];
		fprintf(tfp, "SL%.4f;", tan(theta*M_PI/180.0));
	    }
	    newfont = True;
	}
	
	if (size != t->size) {
	    size  = t->size;	/* in points */
	    newsize = True;
	    if (!correct_font_size) {
		/* HP Stick Font only:  use the 'SI' command to set the 
		   cap height and pitch.
		*/
		width	 = size*wcmpp*wide[font];
		height	 = size*hcmpp*high[font];
		fprintf(tfp, "SI%.4f,%.4f;", width*mag, height*mag);
	    }
	}

	if (correct_font_size && (newfont || newsize)) {
	    /* Use 'SD' command to set the font */
	    fprintf(tfp, "SD2,1,4,%d,5,%d,6,%d,7,%d;SS;\n",
		    (int)(size*mag+.5), psfont2hpgl[font].italic,
		    psfont2hpgl[font].bold, psfont2hpgl[font].font);
	}

	if (angle != t->angle) {
	    angle  = t->angle;
	    fprintf(tfp, "DI%.4f,%.4f;",
		    cos(angle), sin(reflected ? -angle: angle));
	}
	set_color(t->color);

	fprintf(tfp, "PA%.4f,%.4f;\n", t->base_x/ppi, t->base_y/ppi);

	switch (t->type) {
	    case DEFAULT:
	    case T_LEFT_JUSTIFIED:
		break;
	    case T_CENTER_JUSTIFIED:
		fprintf(tfp, "CP%.4f,0.0;", -(double)(strlen(t->cstring)/2.0));
		break;
	    case T_RIGHT_JUSTIFIED:
		fprintf(tfp, "CP%.4f,0.0;", -(double)(strlen(t->cstring)));
		break;
	    default:
		fprintf(stderr, "unknown text position type\n");
		exit(1);
	}    

	fprintf(tfp, "LB%s\003\n", t->cstring);
}

int
genibmgl_end()
{
	/* IBMGL ending */
	fprintf(tfp, "PU;SP;IN;\n");

	if (pcljcl)
	    fprintf(tfp, "\033%%0A\033E");	/* end job and eject page */

	/* all ok */
	return 0;
}

struct driver dev_ibmgl = {
     	genibmgl_option,
	genibmgl_start,
	gendev_null,
	genibmgl_arc,
	genibmgl_ellipse,
	genibmgl_line,
	genibmgl_spline,
	genibmgl_text,
	genibmgl_end,
	EXCLUDE_TEXT
	};

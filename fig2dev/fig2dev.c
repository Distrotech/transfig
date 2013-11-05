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
 *	Fig2dev : General Fig code translation program
 *
*/
#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include "fig2dev.h"
#include "alloc.h"
#include "object.h"
#include "drivers.h"
#include "bound.h"
#include "read.h"

extern	int	 fig_getopt();
extern	char	*optarg;
extern	int	 optind;
char		 lang[40];
int		 parse_gridspec();
static void	 grid_usage();
int		 gendev_objects();

void	help_msg();
void	depth_option();
int	depth_filter(int);

/* hex names for Fig colors */
char	*Fig_color_names[] = {
		"#000000", "#0000ff", "#00ff00", "#00ffff",
		"#ff0000", "#ff00ff", "#ffff00", "#ffffff",
		"#000090", "#0000b0", "#0000d0", "#87ceff", 
		"#009000", "#00b000", "#00d000", "#009090",
		"#00b0b0", "#00d0d0", "#900000", "#b00000",
		"#d00000", "#900090", "#b000b0", "#d000d0",
		"#803000", "#a04000", "#c06000", "#ff8080",
		"#ffa0a0", "#ffc0c0", "#ffe0e0", "#ffd700"
		};

struct driver *dev = NULL;

#ifdef I18N
char	Usage[] = "Usage: %s [-L language] [-f font] [-s size] [-m scale] [-j] [input [output]]\n";
Boolean support_i18n = False;
#else
char	Usage[] = "Usage: %s [-L language] [-f font] [-s size] [-m scale] [input [output]]\n";
#endif  /* I18N */

char	Err_badarg[] = "Argument -%c unknown to %s driver.";
char	Err_mem[] = "Running out of memory.";

char	*prog;
char	*from = NULL, *to = NULL;
char	*name = NULL;
double	font_size = 0.0;
Boolean	correct_font_size = False;	/* use correct font size */
double	mag = 1.0;
double	fontmag = 1.0;
FILE	*tfp = NULL;

double	ppi;			/* Fig file resolution (e.g. 1200) */
int	llx = 0, lly = 0, urx = 0, ury = 0;
Boolean	landscape;
Boolean	center;
Boolean	orientspec = False;	/* set if the user specs. the orientation */
Boolean	centerspec = False;	/* set if the user specs. the justification */
Boolean	magspec = False;	/* set if the user specs. the magnification */
Boolean	transspec = False;	/* set if the user specs. the GIF transparent color */
Boolean	multispec = False;	/* set if the user specs. multiple pages */
Boolean	paperspec = False;	/* set if the user specs. the paper size (-z) */
Boolean	boundingboxspec = False;/* set if the user specs. the bounding box (-B or -R) */
Boolean maxdimspec = False;	/* set fi the user specs. the max size of the figure (-Z) */
Boolean	pats_used, pattern_used[NUMPATTERNS];
Boolean	multi_page = False;	/* multiple page option for PostScript */
Boolean	overlap = False;	/* overlap pages in multiple page output */
Boolean	metric;			/* true if file specifies Metric */
char	gif_transparent[20]="\0"; /* GIF transp color hex name (e.g. #ff00dd) */
char	papersize[20];		/* paper size */
char	boundingbox[64];	/* boundingbox */
float	THICK_SCALE;		/* convert line thickness from screen res. */
					/* calculated in read_objects() */
char	lang[40];		/* selected output language */
RGB	background;		/* background (if specified by -g) */
Boolean	bgspec = False;		/* flag to say -g was specified */
float	grid_minor_spacing=0.0;	/* grid minor spacing (if any) */
float	grid_major_spacing=0.0;	/* grid major spacing (if any) */
float	mult;			/* multiplier for grid spacing */
char	gscom[1000];		/* to build up a command for ghostscript */
float	max_dimension;		/* if the user chooses max figure dimension (-Z) */

Boolean	psencode_header_done = False; /* if we have already emitted PSencode header */
Boolean	transp_header_done = False;   /* if we have already emitted transparent image header */
Boolean	grayonly = False;	/* convert colors to grayscale (-N option) */

struct obj_rec {
	void (*gendev)();
	char *obj;
	int depth;
};

#define NUMDEPTHS 100

struct depth_opts {
        int d1,d2;
} depth_opt[NUMDEPTHS+1];
int	depth_index = 0;
char	depth_op;		/* '+' for skip all but those listed */
int     adjust_boundingbox = 0;

/* be sure to update NUMPAPERSIZES in fig2dev.h if this table changes */

struct paperdef paperdef[] =
{
    {"Letter", 612, 792}, 	/*  8.5" x 11" */
    {"Legal", 612, 1008}, 	/*  8.5" x 14" */
    {"Tabloid", 792, 1224}, 	/*   11" x 17" */
    {"A",   612, 792},		/*  8.5" x 11" (letter) */
    {"B",   792, 1224},		/*   11" x 17" (tabloid) */
    {"C",  1224, 1584},		/*   17" x 22" */
    {"D",  1584, 2448},		/*   22" x 34" */
    {"E",  2448, 3168},		/*   34" x 44" */
    {"A9",  105, 148},		/*   37 mm x   52 mm */
    {"A8",  148, 210},		/*   52 mm x   74 mm */
    {"A7",  210, 297},		/*   74 mm x  105 mm */
    {"A6",  297, 420},		/*  105 mm x  148 mm */
    {"A5",  420, 595},		/*  148 mm x  210 mm */
    {"A4",  595, 842}, 		/*  210 mm x  297 mm */
    {"A3",  842, 1190},		/*  297 mm x  420 mm */
    {"A2", 1190, 1684},		/*  420 mm x  594 mm */
    {"A1", 1684, 2383},		/*  594 mm x  841 mm */
    {"A0", 2383, 3370},		/*  841 mm x 1189 mm */ 
    {"B10",  91,  127},		/*   32 mm x   45 mm */
    {"B9",  127,  181},		/*   45 mm x   64 mm */
    {"B8",  181,  258},		/*   64 mm x   91 mm */
    {"B7",  258,  363},		/*   91 mm x  128 mm */
    {"B6",  363,  516},		/*  128 mm x  182 mm */
    {"B5",  516,  729}, 	/*  182 mm x  257 mm */
    {"B4",  729, 1032},		/*  257 mm x  364 mm */
    {"B3", 1032, 1460},		/*  364 mm x  515 mm */
    {"B2", 1460, 2064},		/*  515 mm x  728 mm */
    {"B1", 2064, 2920},		/*  728 mm x 1030 mm */
    {"B0", 2920, 4127},		/* 1030 mm x 1456 mm */
    {NULL, 0, 0}
};

void 
put_msg(char *fmt, ...)
{
    va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
        fprintf(stderr, "\n");
}

/* all option letters must be in this string */

#ifdef I18N
#define ARGSTRING	"AaB:b:C:cD:d:E:eFf:G:g:hI:i:jkKl:L:Mm:Nn:OoPp:q:R:rS:s:Tt:UVvX:x:Y:y:WwZ:z:?"
#else
#define ARGSTRING	"AaB:b:C:cD:d:E:eFf:G:g:hI:i:kKl:L:Mm:Nn:OoPp:q:R:rS:s:Tt:UVvX:x:Y:y:WwZ:z:?"
#endif

void
get_args(argc, argv)
int	 argc;
char	*argv[];
{
  	int	 c, i, nvals, nchars;
	double	 atof();
	char	*grid, *p;
	float	 numer, denom;

	prog = *argv;
	/* sum of all arguments */
	while ((c = fig_getopt(argc, argv, ARGSTRING)) != EOF) {

	  /* global (all drivers) option handling */
	  switch (c) {

		case 'h':	/* print version message for -h too */
		case 'V': 
		    printf("fig2dev Version %s Patchlevel %s\n",
							VERSION, PATCHLEVEL);
		    if (c == 'h')
			help_msg();
		    exit(0);
		    break;

	        case 'D':	                /* depth filtering */
		    depth_option(optarg);
		    continue;			/* don't pass to driver */

   	        case 'K':
		    /* adjust bounding box according to selected depth range
		       given with '-D RANGE' option above */
		    adjust_boundingbox = 1;
		    continue;  /* don't pass this option to driver */
		  
		case 'F':			/* use correct font size */
		    correct_font_size = True;
		    break;

		case 'G':			/* Grid */
		    /* if major AND minor are specified */
		    if ((p = strchr(optarg,':'))) {
			/* parse minor grid spec first */
			if (p != optarg) {
			    if ((nvals=parse_gridspec(optarg, &numer, &denom, 
				&grid_minor_spacing, &nchars)) == 2)
				    grid_minor_spacing = numer/denom;
			}
			/* now parse major grid spec */
			if ((nvals=parse_gridspec(p+1, &numer, &denom, 
			     &grid_major_spacing, &nchars)) == 2)
				grid_major_spacing = numer/denom;
		    } else {
			/* only minor grid specified */
			if ((nvals=parse_gridspec(optarg, &numer, &denom,
			     &grid_minor_spacing, &nchars)) == 2)
				grid_minor_spacing = numer/denom;
			p = optarg-1;
		    }
		    /* now parse any units after grid spacing */
		    if (nchars < strlen(p+1)) {
			grid = p+nchars+1;
			if ((strcmp(grid,"i") && strcmp(grid,"in") && strcmp(grid,"inch") &&
				 strcmp(grid,"f") && strcmp(grid,"ft") && strcmp(grid,"feet") &&
				 strcmp(grid,"c") && strcmp(grid,"cm") &&
				 strcmp(grid,"mm") && strcmp(grid,"m") )) {
			    grid_usage();
			    grid_major_spacing = grid_minor_spacing = 0.0;
			    break;
			}
			if (!strcmp(grid,"i") || !strcmp(grid,"in") || !strcmp(grid,"in"))
			    mult =  1.0;
			else if (!strcmp(grid,"f") || !strcmp(grid,"ft") || !strcmp(grid,"feet"))
			    mult = 12.0;
			else if (strcmp(grid,"c") == 0 || strcmp(grid,"cm") == 0)
			    mult = 1.0/2.54;
			else if (strcmp(grid,"mm") == 0)
			    mult = 1.0/25.4;
			else if (strcmp(grid,"m") == 0)
			    mult = 1.0/0.0254;
		    } else {
			mult = 1.0;	/* no units, 1:1 */
		    }
		    break;

		case 'L':			/* set output language */
		    /* save language for gen{gif,jpg,pcx,xbm,xpm,ppm,tif} */
		    strncpy(lang,optarg,sizeof(lang)-1);
		    for (i=0; *drivers[i].name; i++) 
			if (!strcmp(lang, drivers[i].name))
				dev = drivers[i].dev;
		    if (!dev) {
			fprintf(stderr,
				"Unknown graphics language %s\n", lang);
			fprintf(stderr,"Known languages are:\n");
			/* display available languages - 23/01/90 */
			for (i=0; *drivers[i].name; i++)
				fprintf(stderr,"%s ",drivers[i].name);
			fprintf(stderr,"\n");
			exit(1);
		    }
		    break;

		case 'm':			/* set magnification */
		    fontmag = mag = atof(optarg);
		    magspec = True;		/* user-specified */
		    continue;			/* don't pass this option to driver */

		case 's':			/* set default font size */
		    font_size = atof(optarg);
		    /* max size is checked in respective drivers */
		    if (font_size <= 0.0)
			font_size = DEFAULT_FONT_SIZE;
		    break;

		case 'Z':			/* scale to requested size (inches/cm) */
		    maxdimspec = True;
		    max_dimension = atof(optarg);
		    continue;  			/* don't pass this option to driver */

#ifdef I18N
		case 'j':			/* set magnification */
		    support_i18n = True;
		    continue;  			/* don't pass this option to driver */
#endif /* I18N */

		case '?':			/* usage 		*/
			fprintf(stderr,Usage,prog);
			exit(1);
	    }

	    /* pass options through to driver */
	    if (!dev) {
		fprintf(stderr, "No graphics language specified.\n");
		exit(1);
	    }
	    dev->option(c, optarg);
      	}
	/* adjust font size after option loop to make sure we have any -m first,
	   which affects fontmag */
	font_size = font_size * fontmag;

      	if (!dev) {
		fprintf(stderr, "No graphics language specified.\n");
		exit(1);
      	}

	/* make sure user doesn't specify both mag and max dimension */
	if (magspec && maxdimspec) {
		fprintf(stderr, "Must specify only one of -m (magnification) and -Z (max dimension).\n");
		exit(1);
	}

	if (optind < argc)
		from = argv[optind++];	/*  from file  */
	if (optind < argc)
		to   = argv[optind];	/*  to file    */
}

int
parse_gridspec(string, numer, denom, spacing, nchrs)
    char  *string;
    float *numer, *denom, *spacing;
    int	  *nchrs;
{
    int  numvals;

    if ((numvals=sscanf(string,"%f/%f%n",numer,denom,nchrs)) == 2) {
	if (*numer < 0.0 || *denom < 0.0 || *denom == 0.0)
	    return 0;
	else
	    return numvals;
    } else if ((numvals=sscanf(string,"%f%n",spacing,nchrs)) == 1) {
	if (*spacing < 0.0 || *spacing == 0.0)
	    return 0;
	else
	    return numvals;
    }

    grid_usage();
    return 0;
}

static void
grid_usage()
{
     fprintf(stderr,"Can't parse grid spec. Format is -G [minor_tick][:major_tick]unit ");
     fprintf(stderr,"e.g. -G .5:2cm or -G 1/16:1/2in\n");
     fprintf(stderr,"Allowable units are: ");
     fprintf(stderr,"'i', 'in', 'ft', 'feet', 'c', 'cm', 'mm', 'm'.");
     fprintf(stderr,"  Ignoring grid.\n");
}

main(argc, argv)
int	 argc;
char	*argv[];
{
	F_compound	objects;
	int		status;

#ifdef HAVE_SETMODE
	setmode(1,O_BINARY); /* stdout is binary */
#endif

	/* get the options */
	get_args(argc, argv);

	/* read the Fig file */

	if (from)
	    status = read_fig(from, &objects);
	else	/* read from stdin */
	    status = readfp_fig(stdin, &objects);

	/* multiply grid spacing by unit and scale to get FIG units */
	grid_minor_spacing = mult * grid_minor_spacing * ppi;
	grid_major_spacing = mult * grid_major_spacing * ppi;

	if (status != 0) {
	    if (from) 
		read_fail_message(from, status);
	    exit(1);
	}

	if (to == NULL)
	    tfp = stdout;
	else {
	    if (strstr(to, ".fig") == to + strlen(to)-4) {
	   	fprintf(stderr,"Outfile is a .fig file, aborting\n");
		exit(1);
	    }
	    if ((tfp = fopen(to, "wb")) == NULL) {
		fprintf(stderr, "Couldn't open %s\n", to);
		exit(1);
	    }
	}

	/* Compute bounding box of objects, supressing texts if indicated */
	compound_bound(&objects, &llx, &lly, &urx, &ury, dev->text_include);

	/* make sure bounding box has width and height (if there is only latex special
	 * text, it may be 0 width */
	if (urx-llx == 0)
	     urx = llx+100;
	if (ury-lly == 0)
	     ury = lly+100;

	/* if user has specified the maximum dimension, adjust mag to produce that */
	if (maxdimspec) {
	    float maxdim;

	    if (metric)
		max_dimension /= 2.54;
	    /* find larger of width and height */
	    maxdim = MAX(urx - llx, ury - lly)/ppi;
	    /* override any mag specified in the Fig file */
	    mag = max_dimension/maxdim;
	}

	/* If metric, adjust scale for difference between FIG PIX/CM (450) and actual (472.44)
	   NOTE: we don't adjust font sizes by this amount because they are in points,
	   not inches or cm */
	if (metric)
		mag *= 80.0/76.2;

	status = gendev_objects(&objects, dev);
	if ((tfp != stdout) && (tfp != 0)) 
	    (void)fclose(tfp);
	exit(status);
}

void
help_msg()
{
    int i;

    printf("General Options (all drivers):\n");
    printf("  -L language	choose output language (this must be first)\n");
    /* display available languages - 23/01/90 */
    printf("                Available languages are:");
    for (i=0; *drivers[i].name; i++) {
	if (i%9 == 0)
	    printf("\n\t\t  ");
	printf("%s ",drivers[i].name);
    }
    printf("\n");
    printf("  -h		print this message, fig2dev version number and exit\n");
    printf("  -V		print fig2dev version number and exit\n");
    printf("  -D +/-list	include or exclude depths listed\n");
    printf("  -K		adjust bounding box according to selected depths\n" );
    printf("		        given with '-D +/-list' option.\n");
    printf("  -f font	set default font\n");
    printf("  -G minor[:major][unit] draw light gray grid with thin/thick lines at minor/major units.\n");
    printf("		  (e.g. -G .25:1cm draws thin line every .25 cm and thick every 1 cm\n");
    printf("		  Only available for PostScript, EPS, PDF, and bitmap (GIF, JPEG, etc)\n");
    printf("		  drivers for now.\n");
#ifdef I18N
    printf("  -j		enable i18n facility\n");
#endif
    printf("  -m mag	set magnification.  This may not be used with the -Z (maxdim) option.\n");
    printf("  -s size	set default font size in points\n");
    printf("  -Z maxdim	Scale the figure so that the maximum dimension (width or height) is\n");
    printf("		  maxdim inches/cm.  This may not be used with the magnification option (-m).\n");
    printf("\n");

    printf("--------------------------------------------------------------------------------------------\n");
    printf("Vector formats:\n");
    printf("CGM Options:\n");
    printf("  -b dummyarg	generate binary output (dummy argument required after \"-b\")\n");
    printf("  -r		position arrowheads for CGM viewers that display rounded arrowheads\n");

    printf("EPIC Options:\n");
    printf("  -A scale	scale arrowheads by dividing their size by scale\n");	
    printf("  -E num	set encoding for text translation (0 no translation,\n");
    printf("		  1 ISO-8859-1, 2 ISO-8859-2)\n");
#ifdef NFSS
    printf("  -F		don't set font family/series/shape, so you can set it from latex\n");
#endif /* NFSS */
    printf("  -l lwidth	use \"thicklines\" when width of line is > lwidth\n");
    printf("  -P		generate a complete LaTeX file\n");
#ifdef NFSS
    printf("  -R dummyarg	allow rotated text (uses \\rotatebox) (requires compilation with NFSS)\n");
#endif /* NFSS */
    printf("  -S scale	scale figure\n");
    printf("  -t strch  	set the stretch of dashed lines (default = 30)\n");
    printf("  -v		include comments in the output\n");
    printf("  -W		enable variable line width\n");
    printf("  -w		disable variable line width\n");

    printf("EPS (Encapsulated PostScript) Options:\n");
    printf("  -A		add ASCII (EPSI) preview\n");
    printf("  -B \"Wx [Wy X0 Y0]\" force width, height and origin in absolute coordinates\n");
    printf("  -b width	specify width of blank border around figure (1/72 inch)\n");
    printf("  -C dummyarg	add color TIFF preview (for Microsoft apps)\n");
    printf("  -F		use correct font sizes (points instead of 1/80inch)\n");
    printf("  -g color	background color\n");
    printf("  -N		convert all colors to grayscale\n");
    printf("  -n name	set title part of PostScript output to name\n");
    printf("  -R \"Wx [Wy X0 Y0]\" force width, height and origin in relative coordinates\n");
    printf("			 (relative to lower-left of figure bounds)\n");
    printf("  -T		add monochrome TIFF preview (for Microsoft apps)\n");

    printf("GBX (Gerber, RS-247-X)  Options:\n");
    printf("  -d [mm|in]	Output dimensions assumed to be millimeters or inches.\n");
    printf("  -p [pos|neg]	Image Polarity - positive results in lines being drawn.\n");
    printf("		Negative results in lines erasing background\n");

    printf("  -g <x scale>x<y scale>+<x offset>+<y offset>\n");
    printf("		This controls the geometry of the output, scaling the dimensions as\n");
    printf("		shown and applying the given offset.  Typically you will wish to set\n");
    printf("		the y scale to -1, mirroring about the x axis.  This is because Gerber\n");
    printf("		assumes the origin to be bottom left, while xfig selects top left.\n");
    printf("  -f <n digits>.<n digits>\n");
    printf("		This controls the number of digits of precision before and after the\n");
    printf("		implied decimal point.  With -f 5.3 the following number 12345678\n");
    printf("		corresponds to 12345.678.  Whereas with -f 3.5 it corresponds to\n");
    printf("		123.45678.  The default is for 3 places before the decimal point and 5\n");
    printf("		after.  This corresponds, to a range of 0 to 1m in 10 micron increments.\n");
    printf("  -i [on|off]	Controls the output of comments describing the type of objects being\n");
    printf("		output.  The text appears as comments starting with ## on each line in\n");
    printf("		the output file.  By default this is on.\n");

    printf("IBM-GL Options:\n");
    printf("  -a		select ISO A4 paper size if default is ANSI A, or vice versa\n");
    printf("  -c		generate instructions for IBM 6180 plotter\n");
    printf("  -d xll,yll,xur,yur	restrict plotting to area specified by coords\n");
    printf("  -f fontfile	load text character specs from table in file\n");
    printf("  -k		precede output with PCL command to use HP/GL\n");
    printf("  -l pattfile	load patterns for pattern fill from file\n");
    printf("  -m mag,x0,y0	magnification with optional offset in inches\n");
    printf("  -P		rotate figure to portrait (default is landscape)\n");
    printf("  -p pensfile	load plotter pen specs from file\n");
    printf("  -S speed	set pen speed in cm/sec\n");
    printf("  -v		print figure upside-down in portrait or backwards in landscape\n");

    printf("LaTeX Options:\n");
    printf("  -d dmag	set separate magnification for length of line dashes to dmag\n");
    printf("  -E num	set encoding for text translation (0 no translation,\n");
    printf("		  1 ISO-8859-1, 2 ISO-8859-2)\n");
    printf("  -l lwidth	set threshold between thin and thick lines to lwidth\n");
    printf("  -v		verbose mode\n");

    printf("MAP (HTML image map) Options:\n");
    printf("  -b width	specify width of blank border around figure (1/72 inch)\n");

    printf("METAFONT Options:\n");
    printf("  -C code	specifies the starting METAFONT font code\n");
    printf("  -n name	name to use in the output file\n");
    printf("  -p pen_mag	linewidth magnification compared to the original figure\n");
    printf("  -t top	specifies the top of the coordinate system\n");
    printf("  -x xmin	specifies minimum x coordinate of figure (inches)\n");
    printf("  -y ymin	specifies minimum y coordinate of figure (inches)\n");
    printf("  -X xmax	specifies maximum x coordinate of figure (inches)\n");
    printf("  -Y xmax	specifies maximum y coordinate of figure (inches)\n");

    printf("METAPOST Options:\n");
    printf("  -i		fileincludes file content via \\input-command\n");
    printf("  -I		fileadds file content as additional header\n");
    printf("  -o		old mode (no latex)\n");
    printf("  -p number	adds the line \"prologues:=number\"\n");

    printf("PIC Options:\n");
    printf("  -p ext	enables certain PIC extensions (see man pages)\n");

    printf("PICTeX Options:\n");
    printf("  -a		don't output user's login name (anonymous)\n");
    printf("  -E num	set encoding for text translation (0 no translation,\n");
    printf("		  1 ISO-8859-1, 2 ISO-8859-2)\n");

    printf("PostScript Options:\n");
    printf("  -a		don't output user's login name (anonymous)\n");
    printf("  -A		add ASCII (EPSI) preview\n");
    printf("  -b width	specify width of blank border around figure (1/72 inch)\n");
    printf("  -C dummyarg	add color TIFF preview (for Microsoft apps)\n");
    printf("  -c		center figure on page\n");
    printf("  -e		put figure at left edge of page\n");
    printf("  -F		use correct font sizes (points instead of 1/80inch)\n");
    printf("  -g color	background color\n");
    printf("  -l dummyarg	landscape mode (dummy argument required after \"-l\")\n");
    printf("  -M		generate multiple pages for large figure\n");
    printf("  -N        	convert all colors to grayscale\n");
    printf("  -n name	set title part of PostScript output to name\n");
    printf("  -O        	overlap pages in multiple page mode (-M)\n");
    printf("  -p dummyarg	portrait mode (dummy argument required after \"-p\")\n");
    printf("  -T		add monochrome TIFF preview (for Microsoft apps)\n");
    printf("  -x offset	shift figure left/right by offset units (1/72 inch)\n");
    printf("  -y offset	shift figure up/down by offset units (1/72 inch)\n");
    printf("  -z papersize	set the papersize (see man pages for available sizes)\n");

    printf("PDF Options:\n");
    printf("  -a		don't output user's login name (anonymous)\n");
    printf("  -b width	specify width of blank border around figure (1/72 inch)\n");
    printf("  -F		use correct font sizes (points instead of 1/80inch)\n");
    printf("  -g color	background color\n");
    printf("PSTricks Options:\n");
    printf("  -f fontname       set default font\n");
    printf("  -G dummyarg       draw pstricks standard grid (sizes ignored)\n");
    printf("  -l weight         set line weight factor 0 to 2.0 (default 0.5 matches PS driver)\n");
    printf("  -P                generate complete {document}; scale to fit one page\n");
    printf("  -p dir            turn on auto picture conversion to EPS; dir is repository\n");
    printf("  -R 0|1|2          arrow style (default 0-Fig, 1-PST w/Fig sizes, 2-PST default, ignore Fig sizes)\n");
    printf("  -S scale          hard scale factor\n");
    printf("  -t version        set PSTricks version\n");
    printf("  -v                verbose warnings and comments in output file\n");
    printf("  -x marginsize     add margin on left and right (default is tight bounding box)\n");
    printf("  -y marginsize     add margin on over and under (default is tight bounding box)\n");
    printf("  -z 0|1|2          font handling (default 0-full, 1-size only, 2-none,PST default)\n");

    printf("  -M		generate multiple pages for large figure\n");
    printf("  -N        	convert all colors to grayscale\n");
    printf("  -O        	overlap pages in multiple page mode (-M)\n");
    printf("  -S dummyarg	generate single page\n");

    printf("PSTEX Options:\n");
    printf("  -b width	specify width of blank border around figure (1/72 inch)\n");
    printf("  -F		use correct font sizes (points instead of 1/80inch)\n");
    printf("  -g color	background color\n");
    printf("  -n name	set title part of PostScript output to name\n");

    printf("PSTEX_T Options:\n");
    printf("  -b width	specify width of blank border around figure (1/72 inch)\n");
    printf("  -E num	set encoding for text translation (0 no translation,\n");
    printf("		  1 ISO-8859-1, 2 ISO-8859-2)\n");
#ifdef NFSS
    printf("  -F		don't set font family/series/shape, so you can\n");
    printf("		  set it from latex\n");
#endif /* NFSS */
    printf("  -p name	name of the PostScript file to be overlaid\n");

    printf("SHAPE (ShapePar driver) Options:\n");
    printf("  -n name	Set basename of the macro (e.g. \"face\" gives faceshape and facepar)\n");
    printf("Tcl/Tk (tk) and Perl/Tk (ptk) Options:\n");
    printf("  -l dummyarg	landscape mode (dummy argument required after \"-l\")\n");
    printf("  -p dummyarg	portrait mode (dummy argument required after \"-p\")\n");
    printf("  -P		generate canvas of full page size instead of figure bounds\n");
    printf("  -z papersize	set the papersize (see man pages for available sizes)\n");

    printf("---------------------------------------------------------------------\n");
    printf("Bitmap formats:\n");
    printf("Options common to all bitmap formats:\n");
    printf("  -b width	specify width of blank border around figure (pixels)\n");
    printf("  -F		use correct font sizes (points instead of 1/80inch)\n");
    printf("  -g color	background color\n");
    printf("  -N		convert all colors to grayscale\n");
    printf("  -S smooth	specify smoothing factor [1=none, 2=some 4=more]\n");

    printf("GIF Options:\n");
    printf("  -t color	specify GIF transparent color in hexadecimal (e.g. #ff0000=red)\n");

    printf("JPEG Options:\n");
    printf("  -q quality	specify image quality factor (0-100)\n");

}

/* count primitive objects & create pointer array */
static int compound_dump(com, array, count, dev)
    F_compound		*com;
    struct obj_rec	*array;
    int			 count;
    struct driver	*dev;
{
  	F_arc		*a;
	F_compound	*c;
	F_ellipse	*e;
	F_line		*l;
	F_spline	*s;
	F_text		*t;

	for (c = com->compounds; c != NULL; c = c->next)
	  count = compound_dump(c, array, count, dev);
	for (a = com->arcs; a != NULL; a = a->next) {
	  if (array) {
		array[count].gendev = dev->arc;
		array[count].obj = (char *)a;
		array[count].depth = a->depth;
	  }
	  count += 1;
	}
	for (e = com->ellipses; e != NULL; e = e->next) {
	  if (array) {
		array[count].gendev = dev->ellipse;
		array[count].obj = (char *)e;
		array[count].depth = e->depth;
	  }
	  count += 1;
	}
	for (l = com->lines; l != NULL; l = l->next) {
	  if (array) {
		array[count].gendev = dev->line;
		array[count].obj = (char *)l;
		array[count].depth = l->depth;
	  }
	  count += 1;
	}
	for (s = com->splines; s != NULL; s = s->next) {
	  if (array) {
		array[count].gendev = dev->spline;
		array[count].obj = (char *)s;
		array[count].depth = s->depth;
	  }
	  count += 1;
	}
	for (t = com->texts; t != NULL; t = t->next) {
	  if (array) {
		array[count].gendev = dev->text;
		array[count].obj = (char *)t;
		array[count].depth = t->depth;
	  }
	  count += 1;
	}
	return count;
}

int
gendev_objects(objects, dev)
    F_compound		*objects;
    struct driver	*dev;
{
	int	obj_count, rec_comp();
	int	status;
	struct	obj_rec *rec_array, *r; 

	/* dump object pointers to an array */
	obj_count = compound_dump(objects, 0, 0, dev);
	if (!obj_count) {
	    fprintf(stderr, "fig2dev: No objects in Fig file\n");
	    return -1;
	}
	rec_array = (struct obj_rec *)malloc(obj_count*sizeof(struct obj_rec));
	(void)compound_dump(objects, rec_array, 0, dev);

	/* sort object array by depth */
	qsort(rec_array, obj_count, sizeof(struct obj_rec), rec_comp);

	/* generate header */
	(*dev->start)(objects);

	/* draw any grid specified */
	(*dev->grid)(grid_major_spacing, grid_minor_spacing);

	/* generate objects in sorted order */
	for (r = rec_array; r<rec_array+obj_count; r++)
	  if (depth_filter(r->depth))
	    (*(r->gendev))(r->obj);

	/* generate trailer */
	status = (*dev->end)();

	free(rec_array);

	return status;
}

int rec_comp(r1, r2)
    struct obj_rec	*r1, *r2;
{
	return (r2->depth - r1->depth);
}

/* null operation */
void gendev_null() {
    ;
}

/*
 * depth_options:
 *  +range[,range...]
 *  -range[,range...]
 *  where range is:
 *  d       include/exclude this depth
 *  d1:d2   include/exclude this range of depths
 */

static void
depth_usage()
{
    fprintf(stderr,"%s: help for -D option:\n",prog);
    fprintf(stderr,"  -D +rangelist  means keep only depths in rangelist.\n");
    fprintf(stderr,"  -D -rangelist  means keep all depths but those in rangelist.\n");
    fprintf(stderr,"  Rangelist can be a list of numbers or ranges of numbers, e.g.:\n");
    fprintf(stderr,"    10,40,55,60:70,99\n");
    exit(1);
}

void
depth_option(s)
char *s;
{
  struct depth_opts *d;

  switch (depth_op = *s++) {
    case '+':
    case '-':
	break;
    default:
	depth_usage();
  }
  
  for (d = depth_opt; depth_index < NUMDEPTHS && *s; ++depth_index, d++) {
    if (*s == ',') ++s;
    d->d1 = d->d2 = -1;
    d->d1 = strtol(s,&s,10);
    if (d->d1 < 0) 
      depth_usage();
    switch(*s) {		/* what's the delim? */
      case ':':			/* parse a range */
	d->d2 = strtol(s+1,&s,10);
	if (d->d2 < d->d1) 
	    depth_usage();
	break;
      case ',':			/* just start the next one */
	s++;
	break;
    }
  }
  if (depth_index >= NUMDEPTHS) {
    fprintf(stderr,"%s: Too many -D values!\n",prog);
    exit(1);
  }
  d->d1 = -1;
}

int
depth_filter(obj_depth)
int obj_depth;
{
  struct depth_opts *d;
  
  if (depth_index <= 0)		/* no filters were set up */
    return 1;
  for (d = depth_opt; d->d1 >= 0; d++)
    if (d->d2 >= 0) {		/* it's a range comparison */
      if (obj_depth >= d->d1 && obj_depth <= d->d2)
	return (depth_op=='+')? 1:0;
    } else {			/* it's a single-value comparison */
      if (d->d1 == obj_depth)
	return (depth_op=='+')? 1:0;
    }
  return (depth_op=='-')? 1:0;
}

void
gs_broken_pipe(int sig)
{
  fprintf(stderr,"fig2dev: broken pipe (GhostScript aborted?)\n");
  fprintf(stderr,"command was: %s\n", gscom);
  exit(1);
}


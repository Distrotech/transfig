/*
 * TransFig: Facility for Translating Fig code
 *
 * Ported from fig2MP by Klaus Guntermann (guntermann@iti.informatik.tu-darmstadt.de)
 * Original fig2MP Copyright 1995 Dane Dwyer (dwyer@geisel.csl.uiuc.edu)
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
 *  fig2MP -- convert fig to METAPOST code
 *
 *  Copyright 1995 Dane Dwyer (dwyer@geisel.csl.uiuc.edu)
 *  Written while a student at the University of Illinois at Urbana-Champaign
 *
 *  Permission is granted to freely distribute this program provided
 *          this copyright notice is included.
 *
 *  Version 0.00 --  Initial attempt  (8/20/95)
 *  Version 0.01 --  Added user defined colors  (8/27/95)
 *                   Added filled and unfilled arrowheads  (8/27/95)
 *  Version 0.02 --  Changed default mapping of ps fonts to match teTeX
 *                    (standard?)  (9/16/96)
 *
 *  Development for new extensions at TU Darmstadt, Germany starting 1999
 *
 *  Version 0.03 --  Allow to "build" pictures incrementally.
 *                   To achieve this we split the complete figure into
 *                   layers in separate mp-figures. The complete figure
 *                   will be seen when overlapping all layers.
 *                   A layer is combined from adjacent depths in xfig.
 *                   This makes it possible to overlap items also when
 *                   splitting into layers.
 *                   The layered version creates files with extension
 *                   ".mmp" (multi-level MetaPost). These can be processed
 *                   with normal MetaPost, but you have to provide the
 *                   extension of the filename in the call.
 *
 *  Version 0.04 --  Fix handling of special characters (May 2001)
 *                   as suggested by Jorma Laaksonen <jorma.laaksonen@hut.fi>.
 *                   "Normal" text should come out correct without needing
 *                   escapes for (LaTeX). That should make it easier to
 *                   export to different formats with the same result.
 *                   Let's use latex as the primary processor for our mpost
 *                   output. Others probably won't use that anyway.
 *
 *                   Additional options for this processor:
 *                   -I filename     include LaTeX commands
 *                                   immediately into metapost file
 *                   -i filename     let LaTeX input commands
 *                                   when processing with MetaPost
 *
 *  Version 0.05 --  Fix positioning of imported text figures (Aug 2001).
 *                   We need to take into account also their depth.
 *
 *  Version 0.06 --  Fix setting the correct font in math mode (Jun 2002).
 *
 *                   Bug fixed with rotated text.
 *
 *                   Support of scaled fonts.
 *
 *                   Blanks are also special characters (replaced with ~).
 *
 *                   The comments +MP-ADDITIONAL-HEADER and
 *                   -MP-ADDITIONAL-HEADER surround the (la)tex
 *                   header (if you want to automatically replace the header
 *                   with a script afterwards).
 *
 *                   Option -I does not longer include material _into_ the
 *                   header; it includes a whole header given in a file
 *
 *                   Options -I and -i can also be used in old mode (-o).
 *
 *                   Additional and changed options:
 *
 *                   -I filename     include a whole header (from
 *                                   \documentclass... to \begin{document}
 *                                   if using latex) into the metapost file
 *                   -o              old mode (tex, no latex)
 *                   -p number       This option adds the line
 *                                       prologues:=number;
 *                                   to the metapost file.
 *
 *
 *                   In latex mode the font setting mechanism of genmplatex
 *                   is used if the fig text is printed in a latex font. If
 *                   it is printed in a postscript font, the font will be
 *                   specified directly in a metapost label command.
 *
 *
 *                   If the fig text is printed in the default latex font,
 *                   the font of the surrounding latex environment will
 *                   be taken.
 *                   If the fig text is printed in the default postscript
 *                   font, the defaultfont of metapost will be taken.
 *                   (An option which selects this feature has been discarded
 *                   because it is not clear what should be done if such an
 *                   option is not used.)
 *
 *                   Changes by Christian Spannagel <cspannagel@web.de>
 *                   Thanks to Dirk Krause for his suggestions.
 *
 *   02 Feb 2003 - all arrowhead types supported now.  Tim Braun
 */

/*
 *  Limitations:
 *         No fill patterns (just shades) -- Not supported by MetaPost
 *         No embedded images -- Not supported by MetaPost
 *         Only flatback arrows
 *
 *  Assumptions:
 *         11" high paper (can be easily changed below)
 *         xfig coordinates are 1200 pixels per inch
 *         xfig fonts scaled by 72/80
 *         Output is for MetaPost version 0.63
 */

#include <stdio.h>
#include <string.h>
#include "fig2dev.h"
#include "object.h"
#include "texfonts.h"
#include "setfigfont.h"
#include "bound.h"

/*
 *  Definitions
 */
#define GENMP_VERSION	0.05
#define PaperHeight	11.0	/* inches */
#define BPperINCH	72.0
#define MaxyBP		BPperINCH*PaperHeight
#define PIXperINCH	1200.0
#define FIGSperINCH	80.0

/*
 *  Special functions
 */
char	*genmp_fillcolor(int, int);
char	*genmp_pencolor(int);
void	genmp_writefontmacro_tex(double, char *, char *);
void    genmp_writefontmacro_latex(F_text *, char *);
void	do_split(int); /* new procedure to split different depths' objects */
#define	getfont(x,y)	((x & 0x4)?xfigpsfonts[y]:xfiglatexfonts[y])
#define ispsfont(x) (x & 0x4)
#define isdefaultfont(x,y) ((!ispsfont(x) && y==0) || (ispsfont(x) && y==-1))
#define	fig2bp(x)	((double)x * mag * BPperINCH/PIXperINCH)
#define	rad2deg(x)	((double)x * 180.0/3.141592654)
#define	y_off(x)	(MaxyBP-(double)(x))   /* reverse y coordinate */


/* Next two used to generate random font names - AA, AB, AC, ... ZZ */
/* #define	char1()		(((int)(c1++/26) % 26)+'A') */
/* #define	char2()		((c2++ % 26)+'A') */
/* char c1=0,c2=0;*/

int split = False; /* no splitting is default */

/*
 *  Static variables for variant mmp:
 *   fig_number has the "current" figure number which has been created.
 *   last_depth remembers the last level number processed
 *         (we need a sufficiently large initial value)
 */
static int fig_number=0;
static int last_depth=1001;


/* default: latex mode
 * switch to tex mode with option -o (old mode)
 */
static int latexmode=1;


/* variables for the prologues-command */
static long int prologues_nr=0;
static int has_prologues=0;

static char options[1024] = "";

/*
 * default xfig postscript fonts given TeX equivalent names
 */
#define FONTNAMESIZE	8

#define MATHFONTMACRO "\\figtodev@genmp@mathfonts"

static char xfigpsfonts[35][FONTNAMESIZE] = {
        "ptmr",         /* Times-Roman */
        "ptmri",        /* Times-Italic */
        "ptmb",         /* Times-Bold */
        "ptmbi",        /* Times-BoldItalic */
        "pagk",         /* AvantGarde-Book */
        "pagko",        /* AvantGarde-BookOblique */
        "pagd",         /* AvantGarde-Demi */
        "pagdo",        /* AvantGarde-DemiOblique */
        "pbkl",         /* Bookman-Light */
        "pbkli",        /* Bookman-LightItalic */
        "pbkd",         /* Bookman-Demi */
        "pbkdi",        /* Bookman-DemiItalic */
        "pcrr",         /* Courier */
        "pcrro",        /* Courier-Oblique */
        "pcrb",         /* Courier-Bold */
        "pcrbo",        /* Courier-BoldOblique */
        "phvr",         /* Helvetica */
        "phvro",        /* Helvetica-Oblique */
        "phvb",         /* Helvetica-Bold */
        "phvbo",        /* Helvetica-BoldOblique */
        "phvrrn",       /* Helvetica-Narrow */
        "phvron",       /* Helvetica-Narrow-Oblique */
        "phvbrn",       /* Helvetica-Narrow-Bold */
        "phvbon",       /* Helvetica-Narrow-BoldOblique */
        "pncr",         /* NewCenturySchlbk-Roman */
        "pncri",        /* NewCenturySchlbk-Italic */
        "pncb",         /* NewCenturySchlbk-Bold */
        "pncbi",        /* NewCenturySchlbk-BoldItalic */
        "pplr",         /* Palatino-Roman */
        "pplri",        /* Palatino-Italic */
        "pplb",         /* Palatino-Bold */
        "pplbi",        /* Palatino-BoldItalic */
        "psyr",         /* Symbol */
        "pzcmi",        /* ZapfChancery-MediumItalic */
        "pzdr"          /* ZapfDingbats */
};



/*
 * default xfig latex fonts given TeX equivalent names
 */
static char xfiglatexfonts[6][FONTNAMESIZE] = {
	"cmr10",         /* DEFAULT = Computer Modern Roman */
	"cmr10",         /* Computer Modern Roman */
	"cmbx10",        /* Computer Modern Roman Bold */
	"cmti10",        /* Computer Modern Roman Italic */
	"cmss10",        /* Computer Modern Sans Serif */
	"cmtt10",        /* Computer Modern Typewriter */
};

/*
 * default xfig colors in postscript RGB notation
 */
static char xfigcolors[32][17] = {
	"(0.00,0.00,0.00)",		/* black */
	"(0.00,0.00,1.00)",		/* blue */
	"(0.00,1.00,0.00)",		/* green */
	"(0.00,1.00,1.00)",		/* cyan */
	"(1.00,0.00,0.00)",		/* red */
	"(1.00,0.00,1.00)",		/* magenta */
	"(1.00,1.00,0.00)",		/* yellow */
	"(1.00,1.00,1.00)",		/* white */
	"(0.00,0.00,0.56)",		/* blue1 */
	"(0.00,0.00,0.69)",		/* blue2 */
	"(0.00,0.00,0.82)",		/* blue3 */
	"(0.53,0.81,1.00)",		/* blue4 */
	"(0.00,0.56,0.00)",		/* green1 */
	"(0.00,0.69,0.00)",		/* green2 */
	"(0.00,0.82,0.00)",		/* green3 */
	"(0.00,0.56,0.56)",		/* cyan1 */
	"(0.00,0.69,0.69)",		/* cyan2 */
	"(0.00,0.82,0.82)",		/* cyan3 */
	"(0.56,0.00,0.00)",		/* red1 */
	"(0.69,0.00,0.00)",		/* red2 */
	"(0.82,0.00,0.00)",		/* red3 */
	"(0.56,0.00,0.56)",		/* magenta1 */
	"(0.69,0.00,0.69)",		/* magenta2 */
	"(0.82,0.00,0.82)",		/* magenta3 */
	"(0.50,0.19,0.00)",		/* brown1 */
	"(0.63,0.25,0.00)",		/* brown2 */
	"(0.75,0.38,0.00)",		/* brown3 */
	"(1.00,0.50,0.50)",		/* pink1 */
	"(1.00,0.63,0.63)",		/* pink2 */
	"(1.00,0.75,0.75)",		/* pink3 */
	"(1.00,0.88,0.88)",		/* pink4 */
	"(1.00,0.84,0.00)"		/* gold */
};

static char *immediate_insert_filename=NULL;
static char *include_filename=NULL;

void
genmp_start(objects)
F_compound	*objects;
{
    FILE *in;
    fprintf(tfp,"%%\n%% fig2dev (version %s.%s) -L (m)mp version %.2lf --- Preamble\n%%\n", VERSION, PATCHLEVEL, GENMP_VERSION);
	fprintf(tfp,"\n");

    fprintf(tfp,"%%\n%% mp output driver options:\n%% %s\n%%\n\n", options);

	/* print any whole-figure comments prefixed with "%" */
	if (objects->comments) {
	    fprintf(tfp,"%%\n");
	    print_comments("% ",objects->comments, "");
	    fprintf(tfp,"%%\n");
	}

    /* print the "prologues:=n;" line */
    if (has_prologues) {
	fprintf(tfp,"prologues:=%ld;\n\n",prologues_nr);
    }


    fprintf(tfp,"%% +MP-ADDITIONAL-HEADER\n");
    if (latexmode || immediate_insert_filename!=NULL || include_filename!=NULL) {
	fprintf(tfp,"verbatimtex\n");

	if (latexmode) {
  	   fprintf(tfp,"%%&latex\n");
	}

	if (immediate_insert_filename!=NULL &&
	    (in=fopen(immediate_insert_filename,"r"))!=NULL) {
	    /* We do not want to restrict line buffer sizes.
	     * Furthermore we do not expect huge files here.
	     * So just read the contents character by character.
	     */

	    int c;
	    fprintf(tfp,"%% start of included material from %s\n",
		    immediate_insert_filename);
	    while ((c=getc(in))!=EOF) {
		putc(c,tfp);
	    }
	    fprintf(tfp,"%% end of included material\n");
	    fclose(in);
	} else if (latexmode) {
	    /* standard header text */
	    fprintf(tfp,"\\documentclass{article}\n");
	    fprintf(tfp,"\\begin{document}\n");
	}


	if (include_filename!=NULL) {
	    fprintf(tfp,"\\input %s\n",include_filename);
	}

	fprintf(tfp,"etex\n");

    }
    fprintf(tfp,"%% -MP-ADDITIONAL-HEADER\n\n");

    /* latexmode: write the font macros. */
    if (latexmode) {
	fprintf(tfp, "\n%%SetFigFont macros for latex\n");
	fprintf(tfp, "verbatimtex\n");
	define_setfigfont(tfp);
	fprintf(tfp, "\\ifx\\SetFigFontSize\\undefined%%\n");
	fprintf(tfp, "\\gdef\\SetFigFontSize#1#2{%%\n");
	fprintf(tfp, "  \\fontsize{#1}{#2pt}%%\n");
	fprintf(tfp, "  \\selectfont}%%\n");
	fprintf(tfp, "\\fi%%\n");
	fprintf(tfp,"etex\n\n");
    }

	if (split) {
	      /*
	       * We need a bounding box around all objects in all
	       * partial figures. Otherwise overlapping them will not
	       * work properly. We create this bounding area here once
	       * and for all. It will be included in each figure.
	       */
	      fprintf(tfp,"path allbounds;\n");
	      fprintf(tfp,"allbounds = (%.2lf,%.2lf)--(%.2lf,%.2lf)",
		      fig2bp(llx),y_off(fig2bp(lly)),fig2bp(urx),
		      y_off(fig2bp(lly)));
	      fprintf(tfp,"--(%.2lf,%.2lf)--(%.2lf,%.2lf)--cycle;\n",
		      fig2bp(urx),y_off(fig2bp(ury)),
		      fig2bp(llx),y_off(fig2bp(ury)));
	} else {
	   fprintf(tfp,"%% Now draw the figure\n");
	   fprintf(tfp,"beginfig(0)\n");
	}

	fprintf(tfp,"%% Some reasonable defaults\n");
	fprintf(tfp,"  labeloffset:=0;\n");

	/* For our overlapping figures we must keep the (invisible) bounding
	 * box as the delimiting area. Thus we must not have MetaPost
	 * compute the "true" bounds.
	 */
	fprintf(tfp,"  truecorners:=%d;\n",split==0);
	fprintf(tfp,"  bboxmargin:=0;\n");
        }


int
genmp_end()
{
        if (split) {
            /* We must add the bounds for the last figure */
	    fprintf(tfp,"setbounds currentpicture to allbounds;\n");
        }
        /* Close the (last) figure and terminate MetaPost program */
	fprintf(tfp,"endfig;\nend\n");
	return(0);
}

void
genmp_option(opt, optarg)
char opt, *optarg;
{
    int cllen = 0;

    switch (opt) {
      case 'L':
	  if (strcasecmp(optarg,"mmp") == 0){
	      split = True;
	  } else if (strcasecmp(optarg,"mp") == 0) {
	      split = False;
	  } else {
	      fprintf(stderr,"bad language option %s",optarg);
	  }
	  break;
      case 'o':
	  latexmode = 0;
	  break;
      case 'I':
	  if (optarg!=NULL) {
	      immediate_insert_filename=strdup(optarg);
	  }
	  else fprintf(stderr,"Warning: missing argument for '-I'. Ignored.\n");
	  break;
      case 'i':
	  if (optarg!=NULL) {
	      include_filename=strdup(optarg);
	  }
	  else fprintf(stderr,"Warning: missing argument for '-i'. Ignored.\n");
	  break;
      case 'p':
	  if (optarg!=NULL) {
	      has_prologues = 1;
	      prologues_nr = atoi(optarg);
	  } else {
	      fprintf(stderr,"Warning: missing argument for '-p'. Ignored.\n");
	  }
	  break;
      /* other options are silently ignored */
    }

    /* add option (and argument) to the options string */
    cllen = strlen(options);
    if (cllen+4<sizeof(options)) {
	options[cllen++]=' ';
	options[cllen++]='-';
	options[cllen++]=opt;
	options[cllen]='\0';
    }    
    if (optarg != NULL && (cllen+strlen(optarg+2)<sizeof(options))) {
	options[cllen++]=' ';	
	strcat (options, optarg);
    }
}

/* Changes for arrowhead support start here 
   several parts taken and adapted from genps.c
   Copyright (c) 1991 by Micah Beck
   Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
   Parts Copyright (c) 1989-2002 by Brian V. Smith */


/* draws an arrowhead with direction given by from and to points
   and style given by arrow object */
void genmp_drawarrow(from_x, from_y, to_x, to_y, obj, arr)
	int     from_x, from_y, to_x, to_y;
	F_line  *obj;
	F_arrow *arr;
{
    int    i, type;
    Point  points[50], fillpoints[50], clippoints[50];
    int    numpoints, nfillpoints, nclippoints;
    
    type = arr->type;
    
    /* use xfig calculation method to generate arrow outline in points array.
       Information on clipping is discarded */
    calc_arrow(from_x, from_y, to_x, to_y, 
	       obj->thickness, arr, points, &numpoints, fillpoints, &nfillpoints, clippoints, &nclippoints);
    
    fprintf(tfp,"%% Draw arrowhead type %d\n",type);
    fprintf(tfp,"  linecap:=0;\n");     /* butt line cap for arrowheads */
    fprintf(tfp,"  linejoin:=0;\n");	/* miter join for sharp points */
    fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(arr->thickness));
    
    fprintf(tfp,"  path arr;\n");
    fprintf(tfp,"  arr = (%.2lf, %.2lf)",
	    fig2bp(points[0].x),
	    y_off(fig2bp(points[0].y)));
    for (i=1; i<numpoints; i++) {
	fprintf(tfp, "\n      --(%.2lf, %.2lf)",
		fig2bp(points[i].x),
		y_off(fig2bp(points[i].y)));
    }

   if (type != 0 && type != 6 && type < 9)  {
        /* old heads, close the path */
	fprintf(tfp, " -- cycle");
    }
    fprintf(tfp,";\n");

    if (type != 0) {
	if (arr->style == 0) { /* hollow, fill with white */
	    fprintf(tfp,"  fill arr withcolor white;\n");
	} else if (type < 9) {
	    /* solid, fill with color  */
	    fprintf(tfp,"  fill arr withcolor %s;\n",
		    genmp_pencolor(obj->pen_color));
	}
    }
    fprintf(tfp,"  draw arr withcolor %s;\n",
	    genmp_pencolor(obj->pen_color));
}


void genmp_arrowheads(obj, objtype)
	F_line	*obj;
	int	objtype;
{
    double      x1,x2,x3,y1,y2,y3,c,d;
    int         from_x, from_y, to_x, to_y;
    F_point     *p, *old_p;
    F_control   *ctl;
    F_arc       *a;
    F_spline    *s;

    /* generate two points to determine direction of arrowhead.
       Start with forward arrow */
    
    if (obj->for_arrow) {
	
	switch(objtype) {
	    
	  case O_ARC:
	      /* find arrowhead direction for arcs */
	      a = (F_arc *) obj;
	      /* last point */
	      to_x = a->point[2].x;
	      to_y = a->point[2].y;
	      compute_arcarrow_angle(a->center.x, a->center.y, 
				     (double) to_x, (double) to_y,
				     a->direction, a->for_arrow,
				     &from_x, &from_y);
	      break;
	      
	  case O_SPLINE:
	      /* find arrowhead direction for splines.
		 UNTESTED!! Not used by current implementation*/
	      s = (F_spline *) obj;
	      p = s->points;
	      if (int_spline(s)) {
		  ctl = s->controls;
		  /* the two last control points of the interpolated spline
		     determine the direction of the arrow */
		  p = p->next;
		  for ( ; p->next != NULL; p=p->next ) 
		      ctl = ctl->next;
		  /* next-to-last control point */
		  from_x = round(ctl->lx);
		  from_y = round(ctl->ly);
		  /* last point */
		  to_x = p->x;		    
		  to_y = p->y;
	      } else {
		  /* for control point splines, arrow direction has to be calculated
		     adapted from genps.c  */
 		  x1 = p->x;
		  y1 = p->y;
		  old_p = p;
		  p = p->next;
		  c = p->x;
		  d = p->y;
		  x3  = (x1 + c) / 2;
		  y3  = (y1 + d) / 2;

		  /* in case there are only two points in this spline */
		  x2 = x1;
		  y2 = y1;
		  /* go through the points to find the last two */
		  for ( ; p->next != NULL; p = p->next) {
		      old_p = p;
		      x1 = x3;
		      y1 = y3;
		      x2 = c;
		      y2 = d;
		      c = p->x;
		      d = p->y;
		      x3 = (x2 + c) / 2;
		      y3 = (y2 + d) / 2;
		  }
		  
		  /* next to last point */
		  from_x = round(x2);
		  from_y = round(y2);
		  /* last point */
		  to_x = round(c);
		  to_y = round(d);
	      }
	      break;
	      
	  case O_POLYLINE:
	  default:
	      /* the two last points of the polyline determine the direction of the arrow */
	      p = obj->points;
	      old_p = p;    
	      p = p->next;
	      for ( ; p->next != NULL; p=p->next )
		  old_p = p;
	      from_x = old_p->x;
	      from_y = old_p->y;
	      to_x = p->x;		    
	      to_y =p->y;
	}

	/* draw the arrow */
	genmp_drawarrow(from_x, from_y, to_x, to_y, obj, obj->for_arrow);    	    
    }
    
    /* get points for any backward arrowhead */
  if (obj->back_arrow) {

	switch(objtype) {
	    
	  case O_ARC:
	    a = (F_arc *) obj;
	    /* first point */
	    to_x = a->point[0].x;
	    to_y = a->point[0].y;
	    compute_arcarrow_angle(a->center.x, a->center.y,
				   (double) to_x, (double) to_y,
				   a->direction ^ 1, a->back_arrow,
				   &from_x, &from_y);
	    break;
	    
	  case O_SPLINE:
	      /* find arrowhead direction for splines.
		UNTESTED!! Not used by current implementation*/
	      s = (F_spline *) obj;
	      p = s->points;
	      if (int_spline(s)) {
		  ctl = s->controls;
		  /* first point */
		  to_x = p->x;
		  to_y = p->y;
		  /* second point */
		  from_x = round(ctl->rx);
		  from_y = round(ctl->ry);
	      } else {
		  /* first point */
		  to_x = p->x;
		  to_y = p->y;
		  /* second point */
		  from_x = round(((double) p->x + (double) p->next->x) / 2);
		  from_y = round(((double) p->y + (double) p->next->y) / 2);
	      }
	      break;
	      
	  case O_POLYLINE:
	  default:
	      p=obj->points;
	      to_x = p->x;
	      to_y = p->y;
	      from_x = p->next->x;		    
	      from_y = p->next->y;
	}
	
	/* draw the arrow */
	genmp_drawarrow(from_x, from_y, to_x, to_y, obj, obj->back_arrow);
  }
}

/* Changes for arrowhead support end here */

void
genmp_line(l)
F_line *l;
{
	F_point	*p;

	do_split(l->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",l->comments, "");

	fprintf(tfp,"%% Begin polyline object\n");
	switch( l->type) {
	   case 1:            /* Polyline */
	   case 2:            /* Box */
	   case 3:            /* Polygon */
	      fprintf(tfp,"  linecap:=%d;\n",l->cap_style);
	      fprintf(tfp,"  linejoin:=%d;\n",l->join_style);
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(l->thickness));
	      fprintf(tfp,"  path p;\n");
	      p = l->points;
	      fprintf(tfp,"  p = (%.2lf, %.2lf)", fig2bp(p->x),y_off(fig2bp(p->y)));
	      p = p->next;
	      for ( ; p != NULL; p=p->next) {
	         fprintf(tfp,"\n    --(%.2lf, %.2lf)", fig2bp(p->x),
	             y_off(fig2bp(p->y)));
	      }
	      if (l->type != 1)
	         fprintf(tfp,"--cycle;\n");
	      else
	         fprintf(tfp,";\n");
	      if (l->fill_style != -1) {   /* Filled? */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = p--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(l->fill_color,l->fill_style));
	      }
         if (l->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw p ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(l->pen_color));
	         if (l->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",l->style_val/3.3);
	         } else if (l->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",l->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");

		 if ((l->for_arrow != NULL) || (l->back_arrow != NULL))
		     genmp_arrowheads(l, O_POLYLINE);
	      }
	      break;
	   case 4:            /* arc box */
	      fprintf(tfp,"  linecap:=%d;\n",l->cap_style);
	      fprintf(tfp,"  linejoin:=1;\n");   /* rounded necessary for arcbox */
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(l->thickness));
	      fprintf(tfp,"  path p,pb,sw,nw,ne,se;\n");
	      fprintf(tfp,"  pair ll,ul,ur,lr;\n");
	      p = l->points;
	      fprintf(tfp,"  p = (%.2lf,%.2lf)--",fig2bp(p->x),y_off(fig2bp(p->y)));
	      p = (p->next)->next;
         fprintf(tfp,"(%.2lf,%.2lf);\n",fig2bp(p->x),y_off(fig2bp(p->y)));
	      fprintf(tfp,"  ur = urcorner p; ll = llcorner p;\n");
 	      fprintf(tfp,"  ul = ulcorner p; lr = lrcorner p;\n");
	      fprintf(tfp,"  sw = fullcircle scaled %.2lf shifted (ll+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),fig2bp(l->radius),fig2bp(l->radius));
	      fprintf(tfp,"  nw = fullcircle scaled %.2lf shifted (ul+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),fig2bp(l->radius),-fig2bp(l->radius));
	      fprintf(tfp,"  ne = fullcircle rotated 180 scaled %.2lf shifted (ur+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),-fig2bp(l->radius),-fig2bp(l->radius));
	      fprintf(tfp,"  se = fullcircle rotated 180 scaled %.2lf shifted (lr+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),-fig2bp(l->radius),fig2bp(l->radius));
	      fprintf(tfp,"  pb = buildcycle(sw,ll--ul,nw,ul--ur,ne,ur--lr,se,\
lr--ll);\n");
	      if (l->fill_style != -1)
 	         fprintf(tfp,"  fill pb %s;\n",
	            genmp_fillcolor(l->fill_color,l->fill_style));
	      if (l->thickness != 0) {      /* invisible pen? */
	         fprintf(tfp,"  draw pb withcolor %s",
	            genmp_pencolor(l->pen_color));
	         if (l->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",l->style_val/3.3);
	         } else if (l->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",
	            l->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
	      }
	      break;
	   case 5:            /* picture object */
	      fprintf(tfp,"  show \"Picture objects are not supported!\"\n");
	      break;
	   default:
	      fprintf(tfp,"  show \"This Polyline object is not supported!\"\n");
	}
	fprintf(tfp,"%% End polyline object\n");
	return;
}

void
genmp_spline(s)
F_spline *s;
{
	F_point	*p;
	F_control *c;
	int i,j;

	do_split(s->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",s->comments, "");

	fprintf(tfp,"%% Begin spline object\n");
	switch (s->type) {
	   case 0:        /* control point spline (open) */
	   case 1:        /* control point spline (closed) */
	      fprintf(tfp,"  linecap:=%d;\n",s->cap_style);
	      fprintf(tfp,"  pair p[],c[];\n");
	      fprintf(tfp,"  path s;\n");
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(s->thickness));
/* locate "curve points" halfway between given poinsts */
	      p = s->points; i=0;
	      fprintf(tfp,"  p%d = .5[(%.2lf,%.2lf),", i++, fig2bp(p->x),
	         y_off(fig2bp(p->y)));
	      p = p->next;
	      fprintf(tfp,"(%.2lf,%.2lf)];\n",fig2bp(p->x),y_off(fig2bp(p->y)));
	      for ( ; p->next != NULL; ) {    /* at least 3 points */
	         fprintf(tfp,"  p%d = .5[(%.2lf,%.2lf),",i++, fig2bp(p->x),
	            y_off(fig2bp(p->y)));
	         p = p->next;
	         fprintf(tfp,"(%.2lf,%.2lf)];\n",fig2bp(p->x),y_off(fig2bp(p->y)));
	      }
/* locate "control points" 2/3 of way between curve points and given points */
	      p = (s->points)->next; i=0; j=0;
	      for ( ; p->next != NULL; ) {   /* at least 3 points */
	         fprintf(tfp,"  c%d = .666667[p%d,(%.2lf,%.2lf)];\n",j++,i++,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	         fprintf(tfp,"  c%d = .666667[p%d,(%.2lf,%.2lf)];\n",j++,i,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	         p = p->next;
	      }
	      if (s->type == 1) {  /* closed spline */
	         fprintf(tfp,"  c%d = .666667[p%d,(%.2lf,%.2lf)];\n",j++,i++,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	         fprintf(tfp,"  c%d = .666667[p0,(%.2lf,%.2lf)];\n",j++,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	      }
/* now draw the spline */
	      p = s->points; i=0; j=0;
	      if (s->type == 1)    /* closed spline */
	         fprintf(tfp,"  s =\n");
	      else
	         fprintf(tfp,"  s = (%.2lf,%.2lf)..\n", fig2bp(p->x),
	         y_off(fig2bp(p->y)));
	      p = p->next;
	      for ( ; p->next != NULL; ) {  /* 3 or more points */
	         fprintf(tfp,"    p%d..controls c%d and c%d..\n",i++,j,j+1);
	         j += 2;
	         p = p->next;
	      }
	      if (s->type == 1) {     /* closed spline */
	         fprintf(tfp,"    p%d..controls c%d and c%d..cycle;\n",i++,j,j+1);
	         j += 2;
	      } else
	         fprintf(tfp,"    p%d..(%.2lf,%.2lf);\n",i,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	      if (s->fill_style != -1) {   /* Filled? */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = s--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(s->fill_color,s->fill_style));
	      }
	      if (s->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw s ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(s->pen_color));
	         if (s->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",s->style_val/3.3);
	         } else if (s->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",s->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
		 
		 if ((s->for_arrow != NULL) || (s->back_arrow != NULL))
		     genmp_arrowheads(s, O_SPLINE);
	      }
	      break;
	   case 2:         /* interpolated spline (open) */
	   case 3:         /* interpolated spline (closed) */
	      fprintf(tfp,"  linecap:=%d;\n",s->cap_style);
	      fprintf(tfp,"  path s;\n");
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(s->thickness));
	      c = s->controls;
	      p = s->points;
	      fprintf(tfp,"  s = (%.2lf, %.2lf)", fig2bp(p->x),y_off(fig2bp(p->y)));
	      p = p->next;
	      for ( ; p != NULL; p=p->next, c=c->next) {
	         fprintf(tfp,"..controls (%.2lf, %.2lf) and (%.2lf, %.2lf)\n",
	            fig2bp(c->rx), y_off(fig2bp(c->ry)),fig2bp((c->next)->lx),
	            y_off(fig2bp((c->next)->ly)));
	         fprintf(tfp,"  ..(%.2lf,%.2lf)",fig2bp(p->x),y_off(fig2bp(p->y)));
	      }
	      if (s->type == 3)       /* closed spline */
	         fprintf(tfp,"..cycle;\n");
	      else
	         fprintf(tfp,";\n");
	      if (s->fill_style != -1) {   /* Filled? */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = s--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(s->fill_color,s->fill_style));
	      }
	      if (s->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw s ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(s->pen_color));
	         if (s->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",s->style_val/3.3);
	         } else if (s->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",s->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");

		 if ((s->for_arrow != NULL) || (s->back_arrow != NULL))
		     genmp_arrowheads(s, O_SPLINE);
	      }
	      break;
	   default:
	      fprintf(tfp,"  show \"This Spline object is not supported!\"\n");
	}
	fprintf(tfp,"%% End spline object\n");
	return;
}


void
genmp_ellipse(e)
F_ellipse *e;
{

        do_split(e->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",e->comments, "");

	fprintf(tfp,"%% Begin ellipse object\n");
	switch(e->type) {
	   case 1:         /* Ellipse by radius */
	   case 2:         /* Ellipse by diameter */
	   case 3:         /* Circle by radius */
	   case 4:         /* Circle by diameter */
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(e->thickness));
	      fprintf(tfp,"  path c;\n");
	      fprintf(tfp,"  c = fullcircle scaled %.2lf yscaled %.2lf\n",
	         fig2bp((e->radiuses).x*2),
	         fig2bp((e->radiuses).y)/fig2bp((e->radiuses).x));
	      fprintf(tfp,"         rotated %.2lf shifted (%.2lf,%.2lf);\n",
	         rad2deg(e->angle),fig2bp((e->center).x),
	         y_off(fig2bp((e->center).y)));
	      if (e->fill_style != -1)
	         fprintf(tfp," fill c %s;\n",
	            genmp_fillcolor(e->fill_color,e->fill_style));
	      if (e->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw c withcolor %s",
	            genmp_pencolor(e->pen_color));
	         if (e->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",e->style_val/3.3);
	         } else if (e->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",e->style_val/4.75);
	         } else {         /* plain */
	            fprintf(tfp,";\n");
	         }
	      }
	      break;
	   default:
	      fprintf(tfp,"  show \"This Ellipse object is not supported!\"\n");
	}
	fprintf(tfp,"%% End ellipse object\n");
	return;
}

void
genmp_arc(a)
F_arc *a;
{

        do_split(a->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",a->comments, "");

	fprintf(tfp,"%% Begin arc object\n");
	switch (a->type) {
	   case 0:            /* three point arc (open) JFIG BUG */
	   case 1:            /* three point arc (open) */
	   case 2:            /* three point arc (pie wedge) */
	      fprintf(tfp,"  linecap:=%d;\n",a->cap_style);
	      fprintf(tfp,"  linejoin:=0;\n");  /* mitered necessary for pie wedge */
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(a->thickness));
	      fprintf(tfp,"  path a,p,ls,le;\n");
	      fprintf(tfp,"  pair s,e,c;\n");
	      fprintf(tfp,"  c = (%.2lf,%.2lf);\n",fig2bp(a->center.x),
	         y_off(fig2bp(a->center.y)));
	      fprintf(tfp,"  s = (%.2lf,%.2lf);\n",fig2bp(a->point[0].x),
	         y_off(fig2bp(a->point[0].y)));
	      fprintf(tfp,"  e = (%.2lf,%.2lf);\n",fig2bp(a->point[2].x),
	         y_off(fig2bp(a->point[2].y)));
	      fprintf(tfp,"  d := (%.2lf ++ %.2lf)*2.0;\n",
	         fig2bp(a->point[0].x)-fig2bp(a->center.x),
	         fig2bp(a->point[0].y)-fig2bp(a->center.y));
	      fprintf(tfp,"  ls = (0,0)--(d,0) rotated angle (s-c);\n");
	      fprintf(tfp,"  le = (0,0)--(d,0) rotated angle (e-c);\n");
	      fprintf(tfp,"  p = ");
	      if (a->direction != 1)  /* clockwise */
	         fprintf(tfp,"reverse ");
	      fprintf(tfp,"fullcircle scaled d rotated angle (s-c) ");
	      fprintf(tfp,"cutafter le;\n");
	      if (a->type == 2)    /* pie wedge */
	         fprintf(tfp,"  a = buildcycle(ls,p,le) shifted c;\n");
	      else
	         fprintf(tfp,"  a = p shifted c;\n");
	      if (a->fill_style != -1) {   /* Filled arc */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = a--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(a->fill_color,a->fill_style));
	      }
	      if (a->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw a ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(a->pen_color));
	         if (a->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",a->style_val/3.3);
	         } else if (a->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",a->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");

		 if ((a->for_arrow != NULL) || (a->back_arrow != NULL))
		     genmp_arrowheads(a, O_ARC);
	      }
	      break;
	   default:
	      fprintf(tfp,"  show \"This Arc object is not supported! (type=%d)\"\n",a->type);
	}
	fprintf(tfp,"%% End arc object\n");
	return;
}

/* The handling of special characters is "borrowed" from genepic.c */
/* The following two arrays are used to translate characters which
 * are special to (La)TeX into characters that print as one would expect.
 * Note that the <> characters aren't really special (La)TeX characters
 * but they will not print as themselves unless one is using a font
 * like tt.
 */

static char tex_text_specials[] = "\\{}><^~$&#_% ";
static char *tex_text_mappings[] = {
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
  "\\%",
  "~"};


void
genmp_text(t)
F_text *t;
{

	do_split(t->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",t->comments, "");
	fprintf(tfp,"%% Begin text object\n");


    /* The whole next part is just relevant for
     *
     *   text with a latex font  and
     *   special text with a postscript font
     */
    if (! ispsfont(t->flags) || special_text(t) ) {
	fprintf(tfp,"  picture q;\n");

	/* Define fonts */
	if ( latexmode ) {
	    /* Define fonts for latex. */
	    genmp_writefontmacro_latex(t,"mpsetfnt");
	} else {
	    /* Define fonts for tex (just special text) */
	    genmp_writefontmacro_tex(t->size,getfont(t->flags,t->font),"mpsetfnt");
	}


	fprintf(tfp,"  q = btex \\mpsetfnt ");


        /* normal text: escape the special characters. */
	if (!special_text(t)){
	    char *special_index, *cp, *esc_cp;

	    /* This loop escapes special (La)TeX characters. */
	    for(cp = t->cstring; *cp; cp++) {
		if ((special_index=strchr(tex_text_specials, *cp))) {
		    /* Write out the replacement.  Implementation note: we can't
		     * use puts since that will output an additional newline.
		     */
		    esc_cp=tex_text_mappings[special_index-tex_text_specials];
		    while (*esc_cp)
			fputc(*esc_cp++, tfp);
		} else
		    fputc(*cp, tfp);
	    }

	} else if (!latexmode) {
	    /* special text in tex mode:
	     * insert the mathmode font macro at the beginning of each
	     * mathmode block. */
	    char *cp;
	    char tmpstr[15];
	    int is_mathmode = 0;
	    int handle_mathmode = 0;
	    int delim_length = 0;


	    for(cp = t->cstring; *cp; cp+=delim_length) {
		delim_length = 1;
		if ( strchr("$", *cp) ) {
		    handle_mathmode = 1;
		    delim_length = 1;
		} else if ( cp == strstr (cp, "\\begin{math}") ) {
		    handle_mathmode = 1;
		    delim_length = 12;
		} else if ( cp == strstr (cp, "\\end{math}") ) {
		    handle_mathmode = 1;
		    delim_length = 10;
		} else if ( cp == strstr (cp, "\\(") ) {
		    handle_mathmode = 1;
		    delim_length = 2;
		} else if ( cp == strstr (cp, "\\)") ) {
		    handle_mathmode = 1;
		    delim_length = 2;
		} else if ( cp == strstr (cp, "\\$") ) {
		    /* ignore */
		    delim_length = 2;
		}

		strncpy ( tmpstr, cp, delim_length );
		tmpstr[delim_length] = '\0';

		/*if ( !latexmode && handle_mathmode && is_mathmode ) {
		  fprintf (tfp, "}");
		  }*/
		fprintf(tfp, "%s", tmpstr);
		if ( handle_mathmode ) {
		    if (! is_mathmode ) {
			/*fprintf(tfp, "%s {", MATHFONTMACRO);*/
			fprintf(tfp, "%s ", MATHFONTMACRO);
		    }
		    handle_mathmode = 0;
		    is_mathmode = 1 - is_mathmode;
		}
	    }
	} else {
	    /* special text in latex mode: just write the text. */
	    fprintf(tfp, t->cstring);
	}
	fprintf(tfp," etex;\n");

    }




    /* The next section is related to normal text with a ps font. */
    else {

	fprintf(tfp,"  picture q;\n");
	fprintf(tfp,"  q=thelabel.urt(\"");
        fprintf(tfp, t->cstring);
	fprintf(tfp, "\" infont ");
	if (t->font<0) {
	    fprintf(tfp, "defaultfont");
	} else {
	    fprintf(tfp, "\"%s\"", xfigpsfonts[t->font]);
	}
	fprintf(tfp," scaled ");
	fprintf(tfp, "(%dpt/fontsize defaultfont)",(int)(t->size*mag*BPperINCH/FIGSperINCH));
	fprintf(tfp,",(0,0));\n");
    }



    /* The next section is the same for all modes. Take the things done above,
     * rotate them and put them to the right position.
 */

    fprintf(tfp,"  picture p;\n");
    fprintf(tfp,"  p = q rotated %.2lf;\n",rad2deg(t->angle));


    /* To put the text at the correct position, we must have
       the left anchor point of the text; this point must
       be calculated when the text is centered or right justified.
       This also avoids mistakes with rotated text.
       To get the left anchor point of right justified [centered] text,
       we have to walk down the whole [the half] line of the text to the
       beginning of the label. In the case of rotated text, this can be
       calculated with the cos and sin of the rotation angle.*/
    if ( t->type == 0) {
	/* left justified */
	    fprintf(tfp,"  label.urt(p,((%.2lf,%.2lf))+llcorner p) ",
		    fig2bp(t->base_x), y_off(fig2bp(t->base_y)));
	     fprintf(tfp,"withcolor %s;\n",genmp_pencolor(t->color));
    } else if (t->type == 1) {
	/* centered */
	fprintf(tfp,"  label.urt(p,((%.2lf,%.2lf))+xpart (lrcorner q - llcorner q)*(-cosd %.2lf,-sind %.2lf)/2+llcorner p) ",
		fig2bp(t->base_x), y_off(fig2bp(t->base_y)),
		rad2deg(t->angle),rad2deg(t->angle));
	     fprintf(tfp,"withcolor %s;\n",genmp_pencolor(t->color));
    } else if (t->type == 2) {
	/* right justified */
	fprintf(tfp,"  label.urt(p,((%.2lf,%.2lf))+xpart (lrcorner q - llcorner q)*(-cosd %.2lf,-sind %.2lf)+llcorner p) ",
		fig2bp(t->base_x), y_off(fig2bp(t->base_y)),
		rad2deg(t->angle),rad2deg(t->angle));
	     fprintf(tfp,"withcolor %s;\n",genmp_pencolor(t->color));
    } else {
	     fprintf(tfp,"  show \"This Text object is not supported!\"\n");
	}

	fprintf(tfp,"%% End text object\n");

	return;
}



char *
genmp_pencolor(c)
int c;
{
	static char p_string[30];
/* It is assumed that c will never be -1 (default) */
	if ((c > 0) && (c < NUM_STD_COLS))    /* normal color */
           strcpy(p_string,xfigcolors[c]);
	else if (c >= NUM_STD_COLS)     /* user defined color */
	   sprintf(p_string,"(%.2lf,%.2lf,%.2lf)",
	      user_colors[c-NUM_STD_COLS].r/255.0,
	      user_colors[c-NUM_STD_COLS].g/255.0,
	      user_colors[c-NUM_STD_COLS].b/255.0);
	else                        /* black or default color */
	   strcpy(p_string,"(0.00,0.00,0.00)");
	return(p_string);
}

char *
genmp_fillcolor(c,s)
int c,s;
{
	static char f_string[100];

	if (c == 0)    /* black fill */
	   sprintf(f_string,"withcolor (black + %.2lfwhite)",
	      (double)(20-s)/20.0);
	else           /* other fill */
	   sprintf(f_string,"withcolor (%s + %.2lfwhite)",
	      genmp_pencolor(c),(double)(s-20)/20.0);
	return(f_string);
}

void
genmp_writefontmacro_latex(t,name)
F_text *t;
char *name;
{
    /* Code inherited from genlatex.c, and modified*/
    int texsize;
    double baselineskip;


    texsize = TEXFONTMAG(t);
    baselineskip = texsize * 1.2;
    texsize *= BPperINCH/FIGSperINCH;
    baselineskip *= BPperINCH/FIGSperINCH;

    fprintf(tfp,"  verbatimtex\n");
    fprintf(tfp, "   \\def\\%s{%%\n", name);

    /* not default font: set the font and font size. */
    if (! isdefaultfont (t->flags, t->font)) {
#ifdef NFSS
	fprintf(tfp,"       \\SetFigFont{%d}{%.1f}{%s}{%s}{%s}%%\n",
		texsize, baselineskip,
		TEXFAMILY(t->font),TEXSERIES(t->font),TEXSHAPE(t->font));
#else
	fprintf(tfp, "\\SetFigFont{%d}{%.1f}{%s}%%\n",
		texsize, baselineskip, TEXFONT(t->font));
#endif
    }

    /* default font: set the font size only. */
    else {
	fprintf(tfp,"       \\SetFigFontSize{%d}{%.1f}%%\n",
		texsize, baselineskip);
    }

    fprintf(tfp, "    }%%\n");
    fprintf(tfp,"  etex;\n");
}


void
genmp_writefontmacro_tex(psize,font,name)
double psize;
char *font, *name;
{
   double ten, seven, five;

/* For some reason, fonts are bigger than they should be */
	psize *= mag * BPperINCH/FIGSperINCH;

	ten = psize; seven = psize*.7; five = psize*.5;
	fprintf(tfp,"  verbatimtex\n");
	fprintf(tfp,"    \\font\\%s=%s at %.2lfpt\n",name,font,psize);
	fprintf(tfp,"    \\font\\tenrm=cmr10 at %.2lfpt\\font\\sevenrm=cmr7 at %.2lfpt\n",ten,seven);
	fprintf(tfp,"    \\font\\fiverm=cmr5 at %.2lfpt\\font\\teni=cmmi10 at %.2lfpt\n",five,ten);
	fprintf(tfp,"    \\font\\seveni=cmmi7 at %.2lfpt\\font\\fivei=cmmi5 at %.2lfpt\n",seven,five);
	fprintf(tfp,"    \\font\\tensy=cmsy10 at %.2lfpt\\font\\sevensy=cmsy7 at %.2lfpt\n",ten,seven);
	fprintf(tfp,"    \\font\\fivesy=cmsy5 at %.2lfpt\n", five);
	fprintf(tfp,"    \\def%s{%%\n", MATHFONTMACRO);
	fprintf(tfp,"      \\textfont0\\tenrm\\scriptfont0\\sevenrm\\scriptscriptfont0\\fiverm\n");
	fprintf(tfp,"      \\textfont1\\teni\\scriptfont1\\seveni\\scriptscriptfont1\\fivei\n");
	fprintf(tfp,"      \\textfont2\\tensy\\scriptfont2\\sevensy\\scriptscriptfont2\\fivesy}\n");
	fprintf(tfp,"  etex;\n");
}

/*
 * If we are in "split" mode, we must start new figure if the current
 * depth and the last_depth differ by more than one.
 * Depths will be seen with decreasing values.
 */
void
do_split(actual_depth)
int actual_depth;
{
    if (split) {
           if (actual_depth+1 < last_depth) {
	      /* depths differ by more than one */
	      if (fig_number > 0) {
		  /* end the current figure, if we already had one */
		  fprintf(tfp,"setbounds currentpicture to allbounds;\n");
		  fprintf(tfp,"endfig;\n");
	      }
	      /* start a new figure with a comment */
              fprintf(tfp,"%% Now draw objects of depth: %d\n",actual_depth);
	      fprintf(tfp,"beginfig(%d)\n",fig_number++);
	   }
	   last_depth = actual_depth;
        }
}

struct driver dev_mp = {
	genmp_option,
	genmp_start,
	gendev_null,
	genmp_arc,
	genmp_ellipse,
	genmp_line,
	genmp_spline,
	genmp_text,
	genmp_end,
	INCLUDE_TEXT
};


void
stradd(dst,src)
char* dst, src;
{
}

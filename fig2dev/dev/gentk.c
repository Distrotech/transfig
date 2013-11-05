/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1998 by Mike Markowski
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
 *	gentk : Tk driver for fig2dev
 *
 *	Author: Mike Markowski (mm@udel.edu), U of Delaware, 4/98
 */

#include "fig2dev.h"
#include "object.h"
#include "readxbm.h"

#define X(x) ((double)(x)/ppi)
#define Y(y) ((double)(y)/ppi)

#define NONE (0xffffff + 1)

static void
	drawBitmap(F_line *),
	drawLine(F_point *, int, int, int, int, int),
	drawShape(void (*)(), void *, int, int, int, int, int, double),
	niceLine(char *),
	tkArc(void *, unsigned int, unsigned int, unsigned int, int),
	tkEllipse(void *, unsigned int, unsigned int, unsigned int, int),
	tkLine(void *, unsigned int, unsigned int, unsigned int, int, int, double),
	tkPolygon(void *, unsigned int, unsigned int, unsigned int, int, int, double),
	tk_setstyle(int style, double v);

static unsigned int
	rgbColorVal(int);

static char *
	stippleFilename(int);

static char *canvas = "$xfigCanvas";
static char *xbmPathVar = "$xbmPath";
static char *xbmPathName = BITMAPDIR;

#define		TOP	8.5 /* inches */
static int	full_page = False;

/*
 *   g e n T k O p t i o n ( )
 */

void
gentk_option(opt, optarg)
char opt, *optarg;
{
    switch (opt) {
	case 'g':			/* background color */
		if (lookup_X_color(optarg,&background) >= 0) {
		    bgspec = True;
		} else {
		    fprintf(stderr,"Can't parse color '%s', ignoring background option\n",
				optarg);
		}
		break;

	case 'l':			/* landscape mode */
		landscape = True;	/* override the figure file setting */
		orientspec = True;	/* user-specified */
		break;

	case 'p':			/* portrait mode */
		landscape = False;	/* override the figure file setting */
		orientspec = True;	/* user-specified */
		break;

	case 'P':			/* use full page instead of bounding box */
		full_page = True;
		break;

	case 'z':			/* papersize */
		(void) strcpy (papersize, optarg);
		paperspec = True;	/* user-specified */
		break;

	case 'f':			/* ignore magnification, font sizes and lang here */
	case 'm':
	case 's':
	case 'L':
		break;

	default:
		put_msg(Err_badarg, opt, "tk");
		exit(1);
    }

}

/*
 *   g e n T k S t a r t ( )
 *
 *   Any initialization garbage is taken care of here.
 */

void
gentk_start(F_compound *objects)
{
	char		stfp[1024];
	float		wid, ht, swap;
	struct paperdef	*pd;
	char		bkgnd[20];

	sprintf(stfp, "# Produced by fig2dev Version %s Patchlevel %s\n", VERSION, PATCHLEVEL);
	niceLine(stfp);
	ppi = ppi / mag * 80/72.0;

	/* print any whole-figure comments prefixed with "#" */
	if (objects->comments) {
	    fprintf(tfp,"#\n");
	    print_comments("# ",objects->comments, "");
	    fprintf(tfp,"#\n");
	}
	sprintf(stfp, "# The canvas name (\".c\") can be changed to anything you "
		"like.  It only\n");
	niceLine(stfp);
	sprintf(stfp, "# occurs in the following line.  The canvas size"
		" can be changed as well.\n\n");
	niceLine(stfp);

	if ( !full_page ) {
	    /* get width and height in fig units */
	    wid = urx - llx;
	    ht = ury - lly;

	    /* add 1% border around */
	    llx -= round(wid/100.0);
	    lly -= round(ht/100.0);
	    urx += round(wid/100.0);
	    ury += round(ht/100.0);

	    /* recalculate new width and height in inches */
	    wid = 1.0*(urx - llx)/ppi;
	    ht = 1.0*(ury - lly)/ppi;

	} else {
	    /* full page, get the papersize as the width and height for the canvas */
            for (pd = paperdef; pd->name != NULL; pd++)
		if (strcasecmp (papersize, pd->name) == 0) {
		    /* width/height are in dpi, convert to inches */
		    wid = pd->width/80.0;
		    ht = pd->height/80.0;
		    strcpy(papersize,pd->name);	/* use the "nice" form */
		    break;
		}
	
	    if (wid < 0 || ht < 0) {
		(void) fprintf (stderr, "Unknown paper size `%s'\n", papersize);
		exit (1);
	    }

	    sprintf(stfp, "# Page size specified: %s\n",papersize);
	    niceLine(stfp);

	    /* swap for landscape */
	    if ( landscape) {
		sprintf(stfp, "# Landscape orientation\n");
		niceLine(stfp);
		swap = wid;
		wid = ht;
		ht = swap;
	    } else {
		sprintf(stfp, "# Portrait orientation\n");
		niceLine(stfp);
	    }
	}

	sprintf(stfp, "set %s [canvas .c -width %.2fi -height %.2fi]", canvas+1, wid, ht);
 	if (bgspec) {
	    sprintf(bkgnd, " -bg #%02x%02x%02x",
 	    background.red/255,
 	    background.green/255,
 	    background.blue/255);
	    strcat(stfp, bkgnd);
	}
	strcat(stfp, "\n");

	niceLine(stfp);
	sprintf(stfp, "$xfigCanvas config -xscrollincrement 1p -yscrollincrement 1p\n");
	niceLine(stfp);
	if ( !full_page ) {
	    sprintf(stfp, "# Shift canvas by lower of bounding box\n");
	    niceLine(stfp);
	    sprintf(stfp, "$xfigCanvas xview scroll %d u\n",round(llx/16.45*mag));
	    niceLine(stfp);
	    sprintf(stfp, "$xfigCanvas yview scroll %d u\n",round(lly/16.45*mag));
	    niceLine(stfp);
	}

	sprintf(stfp, "pack $xfigCanvas\n");
	niceLine(stfp);
	sprintf(stfp, "# If your fill-pattern bitmaps will be elsewhere,\n");
	niceLine(stfp);
	sprintf(stfp, "# change the next line appropriately, and all will\n");
	niceLine(stfp);
	sprintf(stfp, "# work well without further modification.\n\n");
	niceLine(stfp);
	sprintf(stfp, "set %s %s\n\n", xbmPathVar+1, xbmPathName);
	niceLine(stfp);
	sprintf(stfp, "# The xfig objects begin here.\n");
	niceLine(stfp);
}

/*
 *   g e n T k E n d ( )
 *
 *   Last call!...
 */

int
gentk_end()
{
	char	stfp[64];

	sprintf(stfp, "focus %s\n", canvas);
	niceLine(stfp);

	/* all ok */
	return 0;
}

/*
 *   g e n T k A r c ( )
 *
 *   The Fig arc is one of the few objects that is only a subset of its
 *   equivalent Tk canvas object, since a Tk arc can be oval.  But that
 *   has no bearing on the code written here, so this is a pretty
 *   useless comment...
 */

void
gentk_arc(F_arc *a)
{
    /* print any comments prefixed with "#" */
    print_comments("# ",a->comments, "");

    if (a->style > 0)
	fprintf(stderr, "gentk_arc: only solid lines are supported by Tk.\n");
    if (a->type == T_OPEN_ARC && (a->for_arrow || a->back_arrow))
	fprintf(stderr, "gentk_arc: arc arrows not supported by Tk.\n");
    drawShape(tkArc, (void *) a, a->thickness,
	a->pen_color, a->fill_color, a->fill_style, a->style, a->style_val);
}

/*
 *   g e n t T k E l l i p s e ( )
 */

void
gentk_ellipse(F_ellipse *e)
{
    /* print any comments prefixed with "#" */
    print_comments("# ",e->comments, "");

    switch (e->type) {
	case T_CIRCLE_BY_DIA:
	case T_CIRCLE_BY_RAD:
	case T_ELLIPSE_BY_DIA:
	case T_ELLIPSE_BY_RAD:
		if (e->style > 0)
		    fprintf(stderr, "gentk_ellipse: only solid lines supported.\n");
		drawShape(tkEllipse, (void *) e, e->thickness,
			e->pen_color, e->fill_color, e->fill_style, e->style, e->style_val);
		break;
	default:
		/* Stole this line from Netscape 3.03... */
		fprintf(stderr, "gentk_ellipse: Whatchew talkin' 'bout, Willis?\n");
		return;
		break;
    }
}

/*
 *   g e n T k L i n e ( )
 */

void
gentk_line(F_line *l)
{
    /* print any comments prefixed with "#" */
    print_comments("# ",l->comments, "");

    switch (l->type) {
	case T_ARC_BOX:	/* Fall through to T_BOX... */
		fprintf(stderr, "gentk_line: arc box not supported.\n");
	case T_BOX:
	case T_POLYLINE:
		/* Take care of filled regions first. */
		drawShape(tkPolygon, (void *) l->points, 0,
			l->pen_color, l->fill_color, l->fill_style, 0, 0.0);
		/* Now draw line itself. */
		drawShape(tkLine, (void *) l, l->thickness, l->pen_color,
			NONE, UNFILLED, l->style, l->style_val);
		break;
	case T_PIC_BOX:
		drawBitmap(l);
		break;
	case T_POLYGON:
		drawShape(tkPolygon, (void *) l->points, l->thickness,
			l->pen_color, l->fill_color, l->fill_style, l->style, l->style_val);
		break;
	default:
		fprintf(stderr, "gentk_line: Whatchew talkin' 'bout, Willis?\n");
		break;
    }
}

/*
 *   d r a w B i t m a p ( )
 */

#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) != 0)

static void
drawBitmap(F_line *l)
{
	char	stfp[256];
	double	dx, dy;
	int	x, y;
	F_pic	*p;
	unsigned char	buf[16];
	FILE	*fd;
	int	filtype;	/* file (0) or pipe (1) */
	int	stat;
	FILE	*open_picfile();
	void	close_picfile();
	char	xname[PATH_MAX];

	p = l->pic;

	dx = l->points->next->next->x - l->points->x;
	dy = l->points->next->next->y - l->points->y;
	if (!(dx >= 0. && dy >= 0.))
	    fprintf(stderr, "Rotated images not supported by Tk.\n");

	/* see if GIF first */

	if ((fd=open_picfile(p->file, &filtype, True, xname)) == NULL) {
	    fprintf(stderr,"Can't open image file %s\n",p->file);
	    return;
	}

	/* read header */

	stat = ReadOK(fd,buf,7);
	if (!stat) {
		fprintf(stderr,"Image file %s too short\n",p->file);
		close_picfile(fd,filtype);
		return;
	}

	/* see if GIF or PPM (ASCII or binary) */
	if ((strncmp((char *) buf,"GIF",3) == 0) || 
	    (strncmp((char *) buf,"P3\n",3) == 0) || (strncmp((char *) buf,"P6\n",3) == 0)) {
	    /* GIF or PPM allright, create the image command */
	    /* first make a name without the suffix */
	    char pname[PATH_MAX], *dot;

	    close_picfile(fd,filtype);
	    strcpy(pname,p->file);
	    if ((dot=strchr(pname,'.')))
		*dot='\0';
	    /* image create */
	    sprintf(stfp, "image create photo %s -file %s\n",pname, p->file);
	    niceLine(stfp);
	    niceLine("\n");
	    /* now the canvas image */
	    sprintf(stfp, "%s create image %fi %fi -anchor nw -image %s",
			canvas, X(l->points->x), Y(l->points->y), pname);
	    niceLine(stfp);
	    niceLine("\n");
	} else {
	    /* Try for an X Bitmap file format. */
	    rewind(fd);
	    if (ReadFromBitmapFile(fd, &x, &y, &p->bitmap)) {
		sprintf(stfp, "%s create bitmap %fi %fi -anchor nw",
			canvas, X(l->points->x), Y(l->points->y));
		niceLine(stfp);
		sprintf(stfp, " -bitmap @%s", p->file);
		niceLine(stfp);
		if (l->pen_color != BLACK_COLOR && l->pen_color != DEFAULT) {
			sprintf(stfp, " -foreground #%6.6x",
				rgbColorVal(l->pen_color));
			niceLine(stfp);
		}
		niceLine("\n");
		if (l->fill_color != UNFILLED) {
			sprintf(stfp, " -background #%6.6x",
				rgbColorVal(l->fill_color));
			niceLine(stfp);
		}
		niceLine("\n");
	    } else
		fprintf(stderr, "Only X bitmap and GIF picture objects "
			"are supported in Tk canvases.\n");
	    close_picfile(fd,filtype);
	}
}

/*
 *   g e n T k T e x t ( )
 */

/* define the latex fonts in terms of the postscript fonts */
#define latexFontDefault	0
#define latexFontRoman		1
#define latexFontBold		3
#define latexFontItalic		2
#define latexFontSansSerif	17
#define latexFontTypewriter	13

void
gentk_text(F_text * t)
{
	char		stfp[2048];
	int		i, j;

	/* I'm sure I'm just too dense to have seen a better way of doing this... */
	static struct {
		char	*prefix,
			*suffix;
	} fontNames[36] = {
		{"-adobe-times-medium-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-times-medium-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-times-medium-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-times-bold-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-times-bold-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc avant garde-medium-r-normal-sans-",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc avant garde-medium-o-normal-sans-",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc avant garde-demi-r-normal-sans-",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc avant garde-demi-o-normal-sans-",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc bookman-light-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc bookman-light-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc bookman-demi-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-urw-itc bookman-demi-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-courier-medium-r-normal--",
			"-0-0-0-m-0-iso8859-1"},
		{"-adobe-courier-medium-o-normal--",
			"-0-0-0-m-0-iso8859-1"},
		{"-adobe-courier-bold-r-normal--",
			"-0-0-0-m-0-iso8859-1"},
		{"-adobe-courier-bold-o-normal--",
			"-0-0-0-m-0-iso8859-1"},
		{"-adobe-helvetica-medium-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-medium-o-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-bold-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-bold-o-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-medium-r-narrow--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-medium-o-narrow--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-bold-r-narrow--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-helvetica-bold-o-narrow--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-new century schoolbook-medium-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-new century schoolbook-medium-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-new century schoolbook-bold-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-new century schoolbook-bold-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-palatino-medium-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-palatino-medium-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-palatino-bold-r-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-adobe-palatino-medium-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"--symbol-medium-r-normal--",
			"-0-0-0-p-0-*-*"},
		{"-*-itc zapf chancery-medium-i-normal--",
			"-0-0-0-p-0-iso8859-1"},
		{"-*-itc zapf dingbats-medium-r-normal--",
			"-0-0-0-p-0-*-*"}
	};

	/* print any comments prefixed with "#" */
	print_comments("# ",t->comments, "");

	if (t->angle != 0.)
	    fprintf(stderr, "gentk_text: rotated text not supported by Tk.\n");

	sprintf(stfp, "%s create text %fi %fi", canvas, X(t->base_x),
		Y(t->base_y));
	niceLine(stfp);
	strcpy(stfp, " -text \"");
	j = strlen(stfp);
	for (i = 0; i < strlen(t->cstring); i++) {
		if (t->cstring[i] == '"')
			stfp[j++] = '\\';
		stfp[j++] = t->cstring[i];
	}
	stfp[j++] = '"';
	stfp[j++] = '\0';
	niceLine(stfp);
	switch (t->type) {
	case T_LEFT_JUSTIFIED:
	case DEFAULT:
		sprintf(stfp, " -anchor sw");
		break;
	case T_CENTER_JUSTIFIED:
		/* The Tk default. */
		sprintf(stfp, " -anchor s");
		break;
	case T_RIGHT_JUSTIFIED:
		sprintf(stfp, " -anchor se");
		break;
	default:
		fprintf(stderr, "gentk_text: Unknown text justification\n");
		t->type = T_LEFT_JUSTIFIED;
		break;
	}
	niceLine(stfp);

	if (psfont_text(t)) {
		sprintf(stfp, " -font \"%s%d%s\"", fontNames[t->font+1].prefix,
			(int) (t->size*mag), fontNames[t->font+1].suffix);
		niceLine(stfp);
	} else {	/* Rigid, special, and LaTeX fonts. */
		int fnum;

		switch (t->font) {
		case 0:	/* Default font. */
			fnum = latexFontDefault;
			break;
		case 1:	/* Roman. */
			fnum = latexFontRoman;
			break;
		case 2:	/* Bold. */
			fnum = latexFontBold;
			break;
		case 3:	/* Italic. */
			fnum = latexFontItalic;
			break;
		case 4:	/* Sans Serif. */
			fnum = latexFontSansSerif;
			break;
		case 5:	/* Typewriter. */
			fnum = latexFontTypewriter;
			break;
		default:
			fnum = latexFontDefault;
			fprintf(stderr, "gentk_text: unknown LaTeX font.\n");
			break;
		}
		sprintf(stfp, " -font \"%s%d%s\"", fontNames[fnum].prefix,
			(int) (t->size*mag), fontNames[fnum].suffix);
		niceLine(stfp);
	}
	if (t->color != BLACK_COLOR && t->color != DEFAULT) {
		sprintf(stfp, " -fill #%6.6x", rgbColorVal(t->color));
		niceLine(stfp);
	}

	niceLine("\n");
}

/*
 *   b e z i e r S p l i n e ( )
 *
 *   We could use the "-smooth" option of Tk's "line" canvas object to
 *   get a Bezier spline.  But just to be sure everything is identical,
 *   we'll do it ourselves.  All spline routines were blatantly swiped
 *   from genibmgl.c with only non-mathematical mods.
 */

#define		THRESHOLD	.05	/* inch */

static void
bezierSpline(double a0, double b0, double a1, double b1, double a2, double b2,
	double a3, double b3)
{
	char	stfp[64];
	double	x0, y0, x3, y3;
	double	sx1, sy1, sx2, sy2, tx, ty, tx1, ty1, tx2, ty2, xmid, ymid;

	x0 = a0; y0 = b0;
	x3 = a3; y3 = b3;
	if (fabs(x0 - x3) < THRESHOLD && fabs(y0 - y3) < THRESHOLD) {
		sprintf(stfp, " %.4f %.4f", x3, y3);
		niceLine(stfp);
	} else {
		tx   = (a1  + a2 )/2.0;	ty   = (b1  + b2 )/2.0;
		sx1  = (x0  + a1 )/2.0;	sy1  = (y0  + b1 )/2.0;
		sx2  = (sx1 + tx )/2.0;	sy2  = (sy1 + ty )/2.0;
		tx2  = (a2  + x3 )/2.0;	ty2  = (b2  + y3 )/2.0;
		tx1  = (tx2 + tx )/2.0;	ty1  = (ty2 + ty )/2.0;
		xmid = (sx2 + tx1)/2.0;	ymid = (sy2 + ty1)/2.0;

		bezierSpline(x0, y0, sx1, sy1, sx2, sy2, xmid, ymid);
		bezierSpline(xmid, ymid, tx1, ty1, tx2, ty2, x3, y3);
	}
}

/*
 *   g e n T k I t p S p l i n e ( )
 */

static void
gentk_itpSpline(F_spline *s)
{
	char		dir[8], stfp[256];
	F_arrow		*a;
	F_point		*p1, *p2;
	F_control	*cp1, *cp2;
	double		x1, x2, y1, y2;

	p1 = s->points;
	cp1 = s->controls;
	x2 = p1->x/ppi; y2 = p1->y/ppi;

	sprintf(stfp, "%s create line %.4f %.4f", canvas, x2, y2);
	niceLine(stfp);
	for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
		p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {

		x1	 = x2;
		y1	 = y2;
		x2	 = p2->x/ppi;
		y2	 = p2->y/ppi;
		bezierSpline(x1, y1, (double)cp1->rx/ppi, cp1->ry/ppi,
			(double)cp2->lx/ppi, cp2->ly/ppi, x2, y2);
	}

	a = NULL;
	if (s->for_arrow && !s->back_arrow) {
		a = s->for_arrow;
		strcpy(dir, "last");
	} else if (!s->for_arrow && s->back_arrow) {
		a = s->back_arrow;
		strcpy(dir, "first");
	} else if (s->for_arrow && s->back_arrow) {
		a = s->back_arrow;
		strcpy(dir, "both");
	}
	if (a)
		switch (a->type) {
		case 0:	/* Stick type. */
			sprintf(stfp, " -arrow %s -arrowshape {0 %fi %fi}",
				dir, X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			fprintf(stderr, "Warning: stick arrows do not "
				"work well in Tk.\n");
			break;
		case 1:	/* Closed triangle. */
			sprintf(stfp, " -arrow %s -arrowshape {%fi %fi %fi}",
				dir, X(a->ht), X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			break;
		case 2:	/* Closed with indented butt. */
			sprintf(stfp, " -arrow %s -arrowshape {%fi %fi %fi}",
				dir, 0.8 * X(a->ht), X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			break;
		case 3:	/* Closed with pointed butt. */
			sprintf(stfp, " -arrow %s -arrowshape {%fi %fi %fi}",
				dir, 1.2 * X(a->ht), X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			break;
		default:
			fprintf(stderr, "tkLine: unknown arrow type.\n");
			break;
		}

	switch (s->cap_style) {
	case 0:	/* Butt (Tk default). */
		break;
	case 1:	/* Round. */
		sprintf(stfp, " -capstyle round");
		niceLine(stfp);
		break;
	case 2: /* Projecting. */
		sprintf(stfp, " -capstyle projecting");
		niceLine(stfp);
		break;
	default:
		fprintf(stderr, "tkLine: unknown cap style.\n");
		break;
	}

	if (s->thickness != 1) {
		sprintf(stfp, " -width %d", s->thickness);
		niceLine(stfp);
	}
	if (s->pen_color != BLACK_COLOR && s->pen_color != DEFAULT) {
		sprintf(stfp, " -fill #%6.6x", s->pen_color);
		niceLine(stfp);
	}
}

/*
 *   q u a d r a t i c S p l i n e ( )
 */

static void
quadraticSpline(double a1, double b1, double a2, double b2, double a3,
	double b3, double a4, double b4)
{
	char	stfp[64];
	double	x1, y1, x4, y4;
	double	xmid, ymid;

	x1	 = a1; y1 = b1;
	x4	 = a4; y4 = b4;
	xmid	 = (a2 + a3)/2.0;
	ymid	 = (b2 + b3)/2.0;
	if (fabs(x1 - xmid) < THRESHOLD && fabs(y1 - ymid) < THRESHOLD) {
		sprintf(stfp, " %.4f %.4f;\n", xmid, ymid);
		niceLine(stfp);
	} else {
		quadraticSpline(x1, y1, ((x1+a2)/2.0), ((y1+b2)/2.0),
			((3.0*a2+a3)/4.0), ((3.0*b2+b3)/4.0), xmid, ymid);
	    }

	if (fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD) {
		sprintf(stfp, " %.4f %.4f", x4, y4);
		niceLine(stfp);
	} else {
		quadraticSpline(xmid, ymid, ((a2+3.0*a3)/4.0),
			((b2+3.0*b3)/4.0), ((a3+x4)/2.0), ((b3+y4)/2.0),
			x4, y4);
	}
}

/*
 *   g e n T k C t l S p l i n e ( )
 */

static void
gentk_ctlSpline(F_spline *s)
{
	char	dir[8], stfp[256];
	double	cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
	double	x1, y1, x2, y2;
	F_arrow	*a;
	F_point	*p;

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

	if (closed_spline(s)) {
		sprintf(stfp, "%s create polygon %.4f %.4f", canvas, cx1, cy1);
		niceLine(stfp);
	} else {
		sprintf(stfp, "%s create line", canvas);
		niceLine(stfp);
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
		quadraticSpline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
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
	if (closed_spline(s))
		quadraticSpline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);

	if (closed_spline(s)) {
		if (s->pen_color == NONE) {
			sprintf(stfp, " -outline {}");
			niceLine(stfp);
		} else {
			sprintf(stfp, " -outline #%6.6x", s->pen_color);
			niceLine(stfp);
		}
		if (s->fill_color == NONE) {
			sprintf(stfp, " -fill {}");
			niceLine(stfp);
		} else {
			sprintf(stfp, " -fill #%6.6x", s->fill_color);
			niceLine(stfp);
		}
		if (s->fill_style != NONE) {
			sprintf(stfp, " -stipple @%s",
				stippleFilename(s->fill_style));
			niceLine(stfp);
		}
		if (s->thickness != 1) {
			sprintf(stfp, " -width %d", s->thickness);
			niceLine(stfp);
		}
	} else {

		a = NULL;
		if (s->for_arrow && !s->back_arrow) {
			a = s->for_arrow;
			strcpy(dir, "last");
		} else if (!s->for_arrow && s->back_arrow) {
			a = s->back_arrow;
			strcpy(dir, "first");
		} else if (s->for_arrow && s->back_arrow) {
			a = s->back_arrow;
			strcpy(dir, "both");
		}
		if (a)
			switch (a->type) {
			case 0:	/* Stick type. */
				sprintf(stfp, " -arrow %s -arrowshape "
					"{0 %fi %fi}",
					dir, X(a->ht), X(a->wid)/2.);
				niceLine(stfp);
				fprintf(stderr, "Warning: stick arrows do not "
					"work well in Tk.\n");
				break;
			case 1:	/* Closed triangle. */
				sprintf(stfp, " -arrow %s -arrowshape "
					"{%fi %fi %fi}",
					dir, X(a->ht), X(a->ht), X(a->wid)/2.);
				niceLine(stfp);
				break;
			case 2:	/* Closed with indented butt. */
				sprintf(stfp, " -arrow %s -arrowshape "
					"{%fi %fi %fi}", dir, 0.8 * X(a->ht),
					X(a->ht), X(a->wid)/2.);
				niceLine(stfp);
				break;
			case 3:	/* Closed with pointed butt. */
				sprintf(stfp, " -arrow %s -arrowshape "
					"{%fi %fi %fi}", dir, 1.2 * X(a->ht),
					X(a->ht), X(a->wid)/2.);
				niceLine(stfp);
				break;
			default:
				fprintf(stderr, "tkLine: unknown arrow type.\n");
				break;
			}

		switch (s->cap_style) {
		case 0:	/* Butt (Tk default). */
			break;
		case 1:	/* Round. */
			sprintf(stfp, " -capstyle round");
			niceLine(stfp);
			break;
		case 2: /* Projecting. */
			sprintf(stfp, " -capstyle projecting");
			niceLine(stfp);
			break;
		default:
			fprintf(stderr, "tkLine: unknown cap style.\n");
			break;
		}

		if (s->thickness != 1) {
			sprintf(stfp, " -width %d", s->thickness);
			niceLine(stfp);
		}
		if (s->pen_color != BLACK_COLOR && s->pen_color != DEFAULT) {
			sprintf(stfp, " -fill #%6.6x", s->pen_color);
			niceLine(stfp);
		}
	}
}

/*
 *   g e n T k S p l i n e ( )
 */

void gentk_spline(F_spline *s)
{
	/* print any comments prefixed with "#" */
	print_comments("# ",s->comments, "");

	if (int_spline(s))
		gentk_itpSpline(s);
	else
		gentk_ctlSpline(s);
}

/*
 *   d r a w S h a p e ( )
 *
 *   This routine is way too dependent on hardcoded fill_style values.
 *   Because so many other gen*.c routines use them, though, I'm too
 *   lazy to go through all the code.
 */

static void
drawShape(void (*tkShape)(), void *p, int thickness, int penColor,
	int fillColor, int fillStyle, int style, double style_val)
{
	/* Draw filled and/or stippled region enclosed by shape. */
	if (fillStyle != UNFILLED) {
		switch (fillColor) {
		case DEFAULT:
		case BLACK_COLOR:
		case WHITE_COLOR:
			if (fillStyle < 20) {
				/* Draw underlying shape. */
				tkShape(p, NONE, rgbColorVal(fillColor)
					^ 0xffffff, NONE, 0, 0, 0.0);
				/* Draw stipple pattern in fill color. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					fillStyle, 0, 0, 0.0);
			} else if (fillStyle == 20) {
				/* Draw underlying shape. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					NONE, 0, 0, 0.0);
			} else if (fillStyle <= 40) {
				/* Should never get here... */
				fprintf(stderr, "drawShape: b&w error.\n");
			} else {
				/* Draw underlying shape with fill color. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					NONE, 0, 0, 0.0);
				/* Draw fill pattern with pen color. */
				tkShape(p, NONE, rgbColorVal(penColor),
					fillStyle, 0, 0, 0.0);
			}
			break;
		default:
			if (fillStyle < 20) {
				/* First, fill region with black. */
				tkShape(p, NONE, 0x000000, NONE, 0, 0, 0.0);
				/* Then, stipple pattern in fill color. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					fillStyle, 0, 0, 0.0);
			} else if (fillStyle == 20) {
				/* Full saturation of fill color. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					NONE, 0, 0, 0.0);
			} else if (fillStyle < 40) {
				/* First, fill region with fill color. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					NONE, 0, 0, 0.0);
				/* Then, draw stipple pattern in white. */
				tkShape(p, NONE, 0xffffff, fillStyle-20, 0, 0, 0.0);
			} else if (fillStyle == 40) {
				/* Maximally tinted: white. */
				tkShape(p, NONE, 0xffffff, NONE, 0, 0, 0.0);
			} else {
				/* Draw underlying shape with fill color. */
				tkShape(p, NONE, rgbColorVal(fillColor),
					NONE, 0, 0, 0.0);
				/* Draw fill pattern with pen color. */
				tkShape(p, NONE, rgbColorVal(penColor),
					fillStyle, 0, 0, 0.0);
			}
			break;
		}
	}

	/* Finally draw shape itself. */
	if (thickness > 0)
		tkShape(p, rgbColorVal(penColor), NONE, NONE, thickness/15, style, style_val);
}

/*
 *   t k A r c ( )
 *
 *   Generate the Tk canvas item and options for an arc.  Certain
 *   assumptions are made regarding filling (using Tk chord style)
 *   or unfilled (Tk arc style) to mimic xfig.
 */

void
tkArc(void *shape, unsigned int outlineColor, unsigned int fillColor,
	unsigned int fillPattern, int thickness)
{
	char	stfp[256];
	double	cx, cy,	/* Center of circle containing arc. */
		sx, sy,	/* Start point of arc. */
		ex, ey,	/* Stop point of arc. */
		angle1, angle2, extent, radius, startAngle;
	F_arc	*a;

	a = (F_arc *) shape;
	cx = X(a->center.x);		/* Center. */
	cy = Y(a->center.y);
	sx = X(a->point[0].x) - cx;	/* Start point. */
	sy = cy - Y(a->point[0].y);
	ex = X(a->point[2].x) - cx;	/* End point. */
	ey = cy - Y(a->point[2].y);

	radius = sqrt(sy*sy + sx*sx);
	angle1 = atan2(sy, sx) * 180.0 / M_PI;
	if (angle1 < 0.)
		angle1 += 360.;
	angle2 = atan2(ey, ex) * 180.0 / M_PI;
	if (angle2 < 0.)
		angle2 += 360.;

	if (a->direction == 1) {	/* Counter-clockwise. */
		startAngle = angle1;
		extent = angle2 - angle1;
	} else {			/* Clockwise. */
		startAngle = angle2;
		extent = angle1 - angle2;
	}
	if (extent < 0.)		/* Sweep of arc. */
		extent += 360.;

	sprintf(stfp, "%s create arc", canvas);
	niceLine(stfp);
	/* Coords of bounding rectangle. */
	sprintf(stfp, " %.3fi %.3fi %.3fi %.3fi",
		cx-radius, cy-radius, cx+radius, cy+radius);
	niceLine(stfp);
	/* Start angle in degrees and its extent in degrees. */
	sprintf(stfp, " -start %f -extent %f", startAngle, extent);
	niceLine(stfp);

	if (outlineColor == NONE)
		sprintf(stfp, " -outline {}");
	else
		sprintf(stfp, " -outline #%6.6x", outlineColor);
	niceLine(stfp);

	switch (a->type) {
	case T_OPEN_ARC:
		if (fillColor == NONE)
			sprintf(stfp, " -style arc -fill {}");
		else
			sprintf(stfp, " -style chord -fill #%6.6x", fillColor);
		niceLine(stfp);
		break;
	case T_PIE_WEDGE_ARC:
		if (fillColor == NONE)
			sprintf(stfp, " -style pieslice -fill {}");
		else
			sprintf(stfp, " -style pieslice -fill #%6.6x",
				fillColor);
		niceLine(stfp);
		break;
	default:
		fprintf(stderr, "tkArc: unknown arc type.\n");
		break;
	}

	if (fillPattern != NONE) {
		sprintf(stfp, " -stipple @%s", stippleFilename(fillPattern));
		niceLine(stfp);
	}
	if (thickness != 1) {
		sprintf(stfp, " -width %d", thickness);
		niceLine(stfp);
	}

	niceLine("\n");
}

/*
 *   t k E l l i p s e ( )
 */

void
tkEllipse(void *shape, unsigned int outlineColor, unsigned int fillColor,
	unsigned int fillPattern, int thickness)
{
	char		stfp[256];
	F_ellipse	*e;

	e = (F_ellipse *) shape;
	sprintf(stfp, "%s create oval %fi %fi %fi %fi",
		canvas,
		X(e->center.x - e->radiuses.x),
		Y(e->center.y - e->radiuses.y),
		X(e->center.x + e->radiuses.x),
		Y(e->center.y + e->radiuses.y));
	niceLine(stfp);

	if (outlineColor == NONE)
		sprintf(stfp, " -outline {}");
	else
		sprintf(stfp, " -outline #%6.6x", outlineColor);
	niceLine(stfp);

	if (fillColor == NONE)
		sprintf(stfp, " -fill {}");
	else
		sprintf(stfp, " -fill #%6.6x", fillColor);
	niceLine(stfp);

	if (fillPattern != NONE) {
		sprintf(stfp, " -stipple @%s", stippleFilename(fillPattern));
		niceLine(stfp);
	}

	if (thickness != 1) {
		sprintf(stfp, " -width %d", thickness);
		niceLine(stfp);
	}
	
	niceLine("\n");
}

/*
 *   t k L i n e ( )
 */

static void
tkLine(void * shape, unsigned int penColor, unsigned int fillColor,
	unsigned int fillPattern, int thickness, int style, double style_val)
{
	char		dir[8], stfp[256];
	extern char	*canvas;	/* Tk canvas name. */
	F_arrow		*a;
	F_line		*l;
	F_point		*p, *q;

	l = (F_line *) shape;
	p = l->points;
	q = p->next;

	if (q == NULL) {
		/* Degenerate line (single point). */
		sprintf(stfp, "%s create line %fi %fi %fi %fi",
			canvas, X(p->x), Y(p->y), X(p->x), Y(p->y));
		niceLine(stfp);
	} else {
		sprintf(stfp, "%s create line", canvas);
		niceLine(stfp);
		sprintf(stfp, " %fi %fi", X(p->x), Y(p->y));
		niceLine(stfp);
		for ( /* No op. */ ; q != NULL; q = q->next) {
			sprintf(stfp, " %fi %fi", X(q->x), Y(q->y));
			niceLine(stfp);
		}
	}

	a = NULL;
	if (l->for_arrow && !l->back_arrow) {
		a = l->for_arrow;
		strcpy(dir, "last");
	} else if (!l->for_arrow && l->back_arrow) {
		a = l->back_arrow;
		strcpy(dir, "first");
	} else if (l->for_arrow && l->back_arrow) {
		a = l->back_arrow;
		strcpy(dir, "both");
	}
	if (a)
		switch (a->type) {
		case 0:	/* Stick type. */
			sprintf(stfp, " -arrow %s -arrowshape {0 %fi %fi}",
				dir, X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			fprintf(stderr, "Warning: stick arrows do not "
				"work well in Tk.\n");
			break;
		case 1:	/* Closed triangle. */
			sprintf(stfp, " -arrow %s -arrowshape {%fi %fi %fi}",
				dir, X(a->ht), X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			break;
		case 2:	/* Closed with indented butt. */
			sprintf(stfp, " -arrow %s -arrowshape {%fi %fi %fi}",
				dir, 0.8 * X(a->ht), X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			break;
		case 3:	/* Closed with pointed butt. */
			sprintf(stfp, " -arrow %s -arrowshape {%fi %fi %fi}",
				dir, 1.2 * X(a->ht), X(a->ht), X(a->wid)/2.);
			niceLine(stfp);
			break;
		default:
			fprintf(stderr, "tkLine: unknown arrow type.\n");
			break;
		}

	/* set line style here */
	tk_setstyle(style, style_val);

	switch (l->join_style) {
	    case 0:	/* Miter (Tk default). */
		break;
	    case 1:	/* Round. */
		sprintf(stfp, " -joinstyle round");
		niceLine(stfp);
		break;
	    case 2:	/* Bevel. */
		sprintf(stfp, " -joinstyle bevel");
		niceLine(stfp);
		break;
	    default:
		fprintf(stderr, "tkLine: unknown join style.\n");
		break;
	}

	switch (l->cap_style) {
	    case 0:	/* Butt (Tk default). */
		break;
	    case 1:	/* Round. */
		sprintf(stfp, " -capstyle round");
		niceLine(stfp);
		break;
	    case 2: /* Projecting. */
		sprintf(stfp, " -capstyle projecting");
		niceLine(stfp);
		break;
	    default:
		fprintf(stderr, "tkLine: unknown cap style.\n");
		break;
	}

	if (thickness != 1) {
		sprintf(stfp, " -width %d", thickness);
		niceLine(stfp);
	}
	if (penColor != BLACK_COLOR && penColor != DEFAULT) {
		sprintf(stfp, " -fill #%6.6x", penColor);
		niceLine(stfp);
	}
	niceLine("\n");
}

/*
 *   t k P o l y g o n ( )
 */

static void
tkPolygon(void * shape, unsigned int outlineColor, unsigned int fillColor,
	unsigned int fillPattern, int thickness, int style, double style_val)
{
	char		stfp[256];
	extern char	*canvas;	/* Tk canvas name. */
	F_point 	*p, *q;
	int		pts;

	p = (F_point *) shape;

	q = p->next;
	for (pts = 0; q != NULL; q = q->next) {
		if (++pts == 3)
			break;
	}
	if (pts < 3)
		return;	/* Bail out if it's not a polygon. */

	q = p->next;
	sprintf(stfp, "%s create polygon", canvas);
	niceLine(stfp);
	sprintf(stfp, " %fi %fi", X(p->x), Y(p->y));
	niceLine(stfp);
	/* don't emit last coords - just repeat of first */
	for ( /* No op. */ ; q->next != NULL; q = q->next) {
		sprintf(stfp, " %fi %fi", X(q->x), Y(q->y));
		niceLine(stfp);
	}

	/* set line style here */
	tk_setstyle(style, style_val);

	if (outlineColor == NONE)
		sprintf(stfp, " -outline {}");
	else
		sprintf(stfp, " -outline #%6.6x", outlineColor);
	niceLine(stfp);

	if (fillColor == NONE)
		sprintf(stfp, " -fill {}");
	else
		sprintf(stfp, " -fill #%6.6x", fillColor);
	niceLine(stfp);

	if (fillPattern != NONE) {
		sprintf(stfp, " -stipple @%s", stippleFilename(fillPattern));
		niceLine(stfp);
	}

	if (thickness != 1) {
		sprintf(stfp, " -width %d", thickness);
		niceLine(stfp);
	}

	niceLine("\n");
}

static void 
tk_setstyle(int style, double v)
{
	char stfp[200];

	if (style == DASH_LINE) {
	    if (v > 0.0) sprintf(stfp, " -dash {%d %d}", round(v), round(v));
	} else if (style == DOTTED_LINE) {
	    if (v > 0.0) sprintf(stfp, " -dash {%d %d}", round(v), round(v));
	} else if (style == DASH_DOT_LINE) {
	    if (v > 0.0) sprintf(stfp, " -dash {%d %d %d %d}", 
		round(v), round(v*0.5),
		round(ppi/80.0), round(v*0.5));
	} else if (style == DASH_2_DOTS_LINE) {
	    if (v > 0.0) sprintf(stfp, " -dash {%d %d %d %d %d %d}", 
		round(v), round(v*0.45),
		round(ppi/80.0), round(v*0.333),
		round(ppi/80.0), round(v*0.45));
	} else if (style == DASH_3_DOTS_LINE) {
	    if (v > 0.0) sprintf(stfp, 
                " -dash {%d %d %d %d %d %d %d %d}", 
		round(v), round(v*0.4),
		round(ppi/80.0), round(v*0.3),
		round(ppi/80.0), round(v*0.3),
		round(ppi/80.0), round(v*0.4));
	} else {
	    return;
	}

	niceLine(stfp);
}

/*
 *   r g b C o l o r V a l ( )
 *
 *   Given an indexi into either the standard color list or into the
 *   user defined color list, return the hex RGB value of the color.
 */

static unsigned int
rgbColorVal(int colorIndex)
{
	extern User_color	user_colors[];
	unsigned int	rgb;
	static unsigned int	rgbColors[NUM_STD_COLS] = {
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
		rgb = ((user_colors[colorIndex-NUM_STD_COLS].r & 0xff) << 16)
			| ((user_colors[colorIndex-NUM_STD_COLS].g & 0xff) << 8)
			| (user_colors[colorIndex-NUM_STD_COLS].b & 0xff);
	return rgb;
}

/*
 *   s t i p p l e F i l e n a m e ( )
 *
 *   Given xfig index number, return the filename of the corresponding
 *   stipple pattern.  Tk requires the bitmap to be in a file.
 */

static char *
stippleFilename(int patternIndex)
{
	static char *	fillNames[63] = {
		"sp0.bmp", "sp1.bmp", "sp2.bmp", "sp3.bmp", "sp4.bmp",
		"sp5.bmp", "sp6.bmp", "sp7.bmp", "sp8.bmp", "sp9.bmp",
		"sp10.bmp", "sp11.bmp", "sp12.bmp", "sp13.bmp", "sp14.bmp",
		"sp15.bmp", "sp16.bmp", "sp17.bmp", "sp18.bmp", "sp19.bmp",
		"sp20.bmp",

		"", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "",

		"left30.bmp", "right30.bmp", "crosshatch30.bmp",
		"left45.bmp", "right45.bmp", "crosshatch45.bmp",
		"bricks.bmp", "vert_bricks.bmp", "horizontal.bmp",
		"vertical.bmp", "crosshatch.bmp", "leftshingle.bmp",
		"rightshingle.bmp", "vert_leftshingle.bmp",
		"vert_rightshingle.bmp", "fishscales.bmp",
		"small_fishscales.bmp", "circles.bmp", "hexagons.bmp",
		"octagons.bmp", "horiz_saw.bmp", "vert_saw.bmp"
	};
	extern char	*xbmPathVar;	/* Global. */
	static char	path[256];

	sprintf(path, "[file join %s %s]", xbmPathVar, fillNames[patternIndex]);
	return path;
}

/*
 *   n i c e L i n e ( )
 *
 *   Instead of directly calling fprintf()'s, this routine is used so
 *   that lines are printed that are 80 characters long, give or take
 *   a handful.  Otherwise - spline routines in particular - will generate
 *   humongously long lines that are a pain in the butt to edit...
 */

static void
niceLine(char *s)
{
	extern FILE	*tfp;	/* File descriptor of Tk file. */
	int		i, len;
	static int	inQuote = 0;
	static int	pos = 0;

	len = strlen(s);
	for (i = 0; i < len; i++) {
		if (s[i] == '"') {
			inQuote ^= 1;	/* Flip between 0/1. */
			putc(s[i], tfp);
			pos++;
		} else if (!inQuote && (pos > 80) && (s[i] == ' ')) {
			fprintf(tfp, " \\\n  ");
			pos = 2;
		} else {
			putc(s[i], tfp);
			if (s[i] == '\n')
				pos = 0;
			else
				pos++;
		}
	}
}

/*
 *   d e v _ t k
 *
 *   The wrapper used by fig2dev to
 *   isolate the device specific details.
 */

struct driver   dev_tk = {
	gentk_option,
	gentk_start,
	gendev_null,
	gentk_arc,
	gentk_ellipse,
	gentk_line,
	gentk_spline,
	gentk_text,
	gentk_end,
	INCLUDE_TEXT
};

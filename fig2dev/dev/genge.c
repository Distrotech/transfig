/*
 * TransFig: Facility for Translating Fig code
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
 *	genge.c: Graphical Editor driver for fig2dev
 *
 *		This driver is probably not useful for users outside
 *		the SPARK (Simulation Problem Analysis Research Kernel)
 *		user community
 *
*/

#include "fig2dev.h"
#include "object.h"

#define		MAX_FONT_SIZE	31

#define		SCALE(v)	(round((v)*scale))
#define		min(a, b)	(((a) < (b)) ? (a) : (b))

static float	scale;

static void	set_color();
static void	set_linewidth();
static void	set_stip();
static void	set_fill();
static void	set_style();
static void	for_arrow();
static void	back_arrow();
static void	genge_itp_spline();
static void	genge_ctl_spline();

				    /* color mapping		*/
				    /* xfig	ge		*/

static int	GE_COLORS[] = {	 1, /* black	black		*/
				 8, /* blue	blue		*/
				 7, /* green	green		*/
				 6, /* cyan	cyan		*/
				 5, /* red	red		*/
				 4, /* magenta	magenta		*/
				 3, /* yellow	yellow		*/
				 2, /* white	white		*/
				14, /* blue4	blueviolet	*/
				21, /* blue3	unnamed 21	*/
				20, /* blue2	slateblue	*/
				12, /* lightblue skyblue	*/	
				22, /* green4	unnamed 22	*/
				16, /* green3	mediumseagreen	*/
				13, /* green2	limegreen	*/
				23, /* cyan4	unnamed 23	*/
				23, /* cyan3	unnamed 23 (dup)*/
				18, /* cyan2	darkturquoise	*/
				 9, /* red4	maroon		*/
				15, /* red3	firebrick	*/
				24, /* red2	unnamed 24	*/
				25, /* magenta4	unnamed 25	*/
				26, /* magenta3	unnamed 26	*/
				27, /* magenta2	unnamed 27	*/
				28, /* brown4	unnamed 28	*/
				29, /* brown3	unnamed 29	*/
				30, /* brown2	unnamed 30	*/
				10, /* pink4	coral		*/
				17, /* pink3	wheat		*/
				31, /* pink2	unnamed 31	*/
				11, /* pink	thistle		*/
				19, /* gold	goldenrod	*/
		};

void
genge_option(opt, optarg)
char opt;
char *optarg;
{
    switch (opt) {
	case 'f':		/* ignore magnification, font sizes and lang here */
	case 'm':
	case 's':
	case 'L':
	    break;
	default:
	    put_msg(Err_badarg, opt, "ge");
	    exit(1);
    }
}

void
genge_start(objects)
F_compound	*objects;
{
	/* convert dpi to 0.1mm per point */
	scale = 254.0 / ppi;

	/* print any whole-figure comments prefixed with "#" */
	print_comments("# ",objects->comments, "");
}

int
genge_end()
{
    /* all ok */
    return 0;
}

void
genge_line(l)
F_line	*l;
{
	F_point		*p, *q;
	int		i;
	
	/* ge has no picture object yet */
	if (l->type == T_PIC_BOX) {  
		fprintf(stderr,"Warning: Pictures not supported in GE language\n");
		return;
	}

	/* print any comments prefixed with "#" */
	print_comments("# ",l->comments, "");

	fprintf(tfp,"p ");

	set_linewidth(l->thickness);
	set_style(l->style, l->style_val);
	set_color(l->pen_color);
	set_stip(l->pen_color);
	set_fill(l->fill_style, l->fill_color);

	/* backward arrowhead, if any */
	if (l->back_arrow && l->thickness > 0)
	    back_arrow(l);

	p = l->points;
	q = p->next;

	if (l->type == T_PIC_BOX) {

	  /* PICTURE OBJECT */
	  /* GE has no picture objects yet */

	} else {
	  /* POLYLINE */
		p = l->points;
		q = p->next;
		if (q == NULL) { /* A single point line */
		    fprintf(tfp, "(%d,%d);\n", SCALE(p->x), SCALE(p->y));
		    return;
		}
		/* go through the points to get the last two */
		while (q->next != NULL) {
		    p = q;
		    q = q->next;
		}

		/* now output the points */
		p = l->points;
		q = p->next;
		fprintf(tfp, "(%d,%d) ", SCALE(p->x), SCALE(p->y));
		i=0;
		while (q->next != NULL) {
		    p = q;
		    q = q->next;
		    fprintf(tfp, "(%d,%d) ", SCALE(p->x), SCALE(p->y));
 	    	    if (!((++i)%5))
			fprintf(tfp, "\n  ");
		}
	}
	/* forward arrowhead, if any */
	if (l->for_arrow && l->thickness > 0)
	    for_arrow(l);

	/* draw last point */
	fprintf(tfp, "(%d,%d);\n", SCALE(q->x), SCALE(q->y));
}

void 
genge_spline(s)
F_spline	*s;
{
	/* print any comments prefixed with "#" */
	print_comments("# ",s->comments, "");

	/* set the line thickness */
	set_linewidth(s->thickness);
	set_style(s->style, s->style_val);
	set_color(s->pen_color);
	set_stip(s->pen_color);
	set_fill(s->fill_style, s->fill_color);

	/* backward arrowhead, if any */
	if (s->back_arrow && s->thickness > 0)
	    back_arrow(s);

	if (int_spline(s))
	    genge_itp_spline(s);
	else
	    genge_ctl_spline(s);

	/* forward arrowhead, if any */
	if (s->for_arrow && s->thickness > 0)
	    for_arrow(s);
}

static void
genge_itp_spline(s)
F_spline	*s;
{
	F_point		*p, *q;
	F_control	*a, *b;
	int		 xmin, ymin;

	a = s->controls;

	a = s->controls;
	p = s->points;
	/* go through the points to find the last two */
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    b = a->next;
	    a = b;
	}

	p = s->points;
	fprintf(tfp, "n %d %d m\n", p->x, p->y);
	xmin = 999999;
	ymin = 999999;
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    xmin = min(xmin, p->x);
	    ymin = min(ymin, p->y);
	    b = a->next;
	    fprintf(tfp, "\t%.1f %.1f %.1f %.1f %d %d curveto\n",
			a->rx, a->ry, b->lx, b->ly, q->x, q->y);
	    a = b;
	}

}

static void
genge_ctl_spline(s)
F_spline	*s;
{
	double		a, b, c, d, x1, y1, x2, y2, x3, y3;
	F_point		*p, *q;
	int		xmin, ymin;

	if (closed_spline(s))
	    fprintf(tfp, "%% Closed spline\n");
	else
	    fprintf(tfp, "%% Open spline\n");

	p = s->points;
	x1 = p->x;
	y1 = p->y;
	p = p->next;
	c = p->x;
	d = p->y;
	x3 = a = (x1 + c) / 2;
	y3 = b = (y1 + d) / 2;

	/* in case there are only two points in this spline */
	x2=x1; y2=y1;
	/* go through the points to find the last two */
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    x1 = x3;
	    y1 = y3;
	    x2 = c;
	    y2 = d;
	    c = q->x;
	    d = q->y;
	    x3 = (x2 + c) / 2;
	    y3 = (y2 + d) / 2;
	}

	/* now output the points */
	set_style(s->style, s->style_val);
	xmin = 999999;
	ymin = 999999;

	p = s->points;
	x1 = p->x;
	y1 = p->y;
	p = p->next;
	c = p->x;
	d = p->y;
	x3 = a = (x1 + c) / 2;
	y3 = b = (y1 + d) / 2;
	/* in case there are only two points in this spline */
	x2=x1; y2=y1;
	if (closed_spline(s))
	    fprintf(tfp, "n %.1f %.1f m\n", a, b);
	else
	    fprintf(tfp, "n %.1f %.1f m %.1f %.1f l\n", x1, y1, x3, y3);
	
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    xmin = min(xmin, p->x);
	    ymin = min(ymin, p->y);
	    x1 = x3;
	    y1 = y3;
	    x2 = c;
	    y2 = d;
	    c = q->x;
	    d = q->y;
	    x3 = (x2 + c) / 2;
	    y3 = (y2 + d) / 2;
	    fprintf(tfp, "\t%.1f %.1f %.1f %.1f %.1f %.1f DrawSplineSection\n",
			x1, y1, x2, y2, x3, y3);
	}
	/*
	* At this point, (x2,y2) and (c,d) are the position of the
	* next-to-last and last point respectively, in the point list
	*/
	if (closed_spline(s)) {
	    fprintf(tfp, "\t%.1f %.1f %.1f %.1f %.1f %.1f DrawSplineSection closepath ",
			x3, y3, c, d, a, b);
	} else {
	    fprintf(tfp, "\t%.1f %.1f l ", c, d);
	}
}

void
genge_arc(a)
F_arc	*a;
{
	int		i;

	/* print any comments prefixed with "#" */
	print_comments("# ",a->comments, "");

	/* Arc */
	fprintf(tfp,"A ");

	set_linewidth(a->thickness);
	set_style(a->style, a->style_val);
	set_color(a->pen_color);
	set_stip(a->pen_color);
	set_fill(a->fill_style, a->fill_color);

	/* backward arrowhead, if any */
	if (a->type == T_OPEN_ARC && a->back_arrow && a->thickness > 0)
	    back_arrow(a);

	for (i=0; i<=2; i++)
	    fprintf(tfp,"(%d,%d) ",SCALE(a->point[i].x), SCALE(a->point[i].y));

	/* forward arrowhead, if any */
	if (a->type == T_OPEN_ARC && a->for_arrow && a->thickness > 0)
	    for_arrow(a);
	fprintf(tfp," ;\n");
}

void
genge_ellipse(e)
F_ellipse	*e;
{
	/* print any comments prefixed with "#" */
	print_comments("# ",e->comments, "");

	/* Ellipse (Circles too) */
	fprintf(tfp, "E ");

	set_linewidth(e->thickness);
	set_style(e->style, e->style_val);
	set_color(e->pen_color);
	set_stip(e->pen_color);
	set_fill(e->fill_style, e->fill_color);

	fprintf(tfp, "(%d,%d) (%d,%d)\n",
		SCALE(e->center.x - e->radiuses.x), SCALE(e->center.y - e->radiuses.y),
		SCALE(e->center.x + e->radiuses.x), SCALE(e->center.y + e->radiuses.y));
}


void
genge_text(t)
F_text	*t;
{
	int x, y, style;

	/* print any comments prefixed with "#" */
	print_comments("# ",t->comments, "");

	/* the code for text is just the string itself in quotes */
	fprintf(tfp,"\"%s\" ",t->cstring);

	set_color(t->color);
	set_stip(t->color);
	switch (t->font %4) {
	    case 0: style = 1;	/* normal text */
		    break;
	    case 1: style = 3;	/* italic text */
		    break;
	    case 2: style = 2;	/* bold text */
		    break;
	    case 3: style = 2;	/* bold-italic text - just use bold */
		    break;
	}

	fprintf(tfp,"f%02d z%02d y%01d ",t->font, min(MAX_FONT_SIZE,(int)t->size), style);

	x = t->base_x;
	y = t->base_y;
	if ((t->type == T_CENTER_JUSTIFIED) || (t->type == T_RIGHT_JUSTIFIED)) {
	     /* adjust the vertex (x,y) for different justification */
	}
	/* vertex */
	fprintf(tfp,"(%d,%d) ",SCALE(x),SCALE(y));
	/* output 0 for length of string which means attributes apply to all */
	fprintf(tfp,"l0 ;\n");
}

static void
for_arrow(l)
    F_line	*l;
{
    fprintf(tfp,"w%01d%01d ",(int)(l->for_arrow->wid/15),(int)(l->for_arrow->ht/15));
}

static void
back_arrow(l)
    F_line	*l;
{
    fprintf(tfp,"v%01d%01d ",(int)(l->back_arrow->wid/15),(int)(l->back_arrow->ht/15));
}

/* set the color attribute */

static void
set_color(col)
    int col;
{
	fprintf(tfp,"c%02d ",GE_COLORS[col]);
}

/* set fill if there is a fill style */

static void
set_fill(style, color)
    int style,color;
{
	if (style != UNFILLED)
	    fprintf(tfp,"C%02d ",GE_COLORS[color]);
}
	
static void
set_stip(stip)
    int stip;
{
	return;
}

/* the dash length, v, is not used in GE */

static void
set_style(s, v)
int	s;
double	v;
{
	if (s == DASH_LINE) {
		fprintf(tfp,"y02 ");
	} else if (s == DOTTED_LINE) {
		fprintf(tfp,"y03 ");
	} else if (s == DASH_DOT_LINE) {
		fprintf(tfp,"y03 ");
	} else if (s == DASH_2_DOTS_LINE) {
		fprintf(tfp,"y03 ");
	} else if (s == DASH_3_DOTS_LINE) {
		fprintf(tfp,"y03 ");
	} else {
		fprintf(tfp,"y01 ");
	}
}

static void
set_linewidth(w)
int	w;
{
	fprintf(tfp, "s%02d ", w);
}

struct
driver dev_ge = {
     	genge_option,
	genge_start,
	gendev_null,
	genge_arc,
	genge_ellipse,
	genge_line,
	genge_spline,
	genge_text,
	genge_end,
	INCLUDE_TEXT
};

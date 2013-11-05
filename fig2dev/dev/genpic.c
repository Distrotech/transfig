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
 *	genpic : PIC driver for fig2dev
 *
 *	Author: Conrad Kwok, UC Davis, 12/88
 *      Modified: Richard Auletta, George Mason Univ., 6/21/89
 *	Added code comments are marked with "rja".
 *      Added: Support for native pic arrowheads.
 *      Added: Support for arrowheads at both ends of lines, arc, splines.
 *
 *      Modified: Stuart Kemp & Dave Bonnell, July, 1991
 *		  James Cook University,
 *		  Australia
 *      Changes:
 *		Added T_ARC_BOX to genpic_line()
 *		Added 'thickness' attribute all over
 *		Added 'fill' attribute to ellipse
 *		Cleaned up the code
 */

#include "fig2dev.h"
#include "object.h"
#include "picfonts.h"
#include "picpsfonts.h"

static void genpic_ctl_spline(), genpic_itp_spline();
static void genpic_open_spline(), genpic_closed_spline();

#define			TOP	10.5	/* top of page is 10.5 inch */
static int LineThickness = 0;
static int OptArcBox = 0;		/* Conditional use */
static int OptLineThick = 0;
static int OptEllipseFill = 0;
static int OptNoUnps = 0;    /* prohibit unpsfont() */

void
genpic_option(opt, optarg)
char opt, *optarg;
{
	switch (opt) {

	case 'f':		/* set default text font */
	        {   int i;

		    for ( i = 1; i <= MAX_FONT; i++ )
			if ( !strcmp(optarg, picfontnames[i]) ) break;

		    if ( i > MAX_FONT)
			fprintf(stderr,
			"warning: non-standard font name %s\n", optarg);
		}
		
		picfontnames[0] = picfontnames[1] = optarg;
		break;

	case 's':
	case 'm':
	case 'L':
		break;

	case 'p':
		if (strcmp(optarg, "all") == 0)
		  OptArcBox = OptLineThick = OptEllipseFill = 1;
		else
		  if (strcmp(optarg, "arc") == 0)
		    OptArcBox = 1;
		  else
		    if (strcmp(optarg, "line") == 0)
		      OptLineThick = 1;
		    else
		      if (strcmp(optarg, "fill") == 0)
			OptEllipseFill = 1;
		      else
			if (strcmp(optarg, "psfont") == 0)
			  OptNoUnps = 1;
			else
			  if (strcmp(optarg, "allps") == 0)
			    OptArcBox =
			      OptLineThick =
				OptEllipseFill =
				  OptNoUnps = 1;
			  else
			    { fprintf(stderr, "Invalid option: %s\n", optarg);
			      exit(1);
			    }
		break;
	      default:
		put_msg(Err_badarg, opt, "pic");
		exit(1);
	}
}

static
double convy(a)
double	a;
{
	return((double) TOP-a);
}

void
genpic_start(objects)
F_compound	*objects;
{
	ppi = ppi/mag;

	/* print any whole-figure comments prefixed with '.\" ' */
	if (objects->comments) {
	    fprintf(tfp,".\\\"\n");
	    print_comments(".\\\" ",objects->comments, "");
	    fprintf(tfp,".\\\"\n");
	}

	fprintf(tfp, ".PS\n.ps %d\n",	 	/* PIC preamble */
		(int)(font_size != 0.0? (int) font_size : DEFAULT_FONT_SIZE));
}

int
genpic_end()
{
  	fprintf(tfp, ".PE\n");				/* PIC ending */

	/* all ok */
	return 0;
}

/*
  The line thickness is, unfortunately, multiple of pixel.
  One pixel thickness is a little too thick on the hard copy
  so I scale it with 0.7; i.e., it's a kludge.  The best way is
  to allow thickness in fraction of pixel.

  Note that the current version of psdit (a ditroff to postcript filter)
  won't take the legitimate line thickness command.
*/

static void
set_linewidth(w)
int	w;
{
	LineThickness = w*80/ppi;
}

static void
AddThickness()
{
  if (OptLineThick && LineThickness)
    fprintf(tfp, " thickness %d", LineThickness);
}

static void
set_style(s, v)
int	s;
double	v;
{
	static float	style_val = -1;

	if (s == DASH_LINE || s == DOTTED_LINE) {
	    if (v == style_val) return;
	    if (v == 0.0) return;
	    style_val = v;
	    fprintf(tfp, "dashwid = %.3f\n", (float)(style_val/80.0));
	    }
}

/*
 * Makes use of the PIC 'box' command
 *
 * Returns 0 if command failed, else non-zero.
 *
 */

static int
genpic_box(l)
F_line *l;
{
  int count, minx, miny, maxx, maxy;
  int Valid;		/* Valid box */
  double width, height;
  F_point *p, *q;
	
  p = l->points;
  q = p->next;
  count = 1;		/* Just a sanity check */
  minx = maxx = p->x;
  miny = maxy = p->y;

  /* Find the boundaries */
  while (q != NULL)
  { count++;
    if (q->x < minx) minx = q->x;
    else
      if (q->x > maxx) maxx = q->x;

    if (q->y < miny) miny = q->y;
    else
      if (q->y > maxy) maxy = q->y;

    q = q->next;
  }

  if ((Valid = (count == 5)))		/* Valid box? */
  { fprintf(tfp, "box");
    if (l->thickness == 0)
      fprintf(tfp, " invis");
    else
      if (l->style_val > 0.0) {
	if (l->style == DASH_LINE)
	  fprintf(tfp, " dashed");
	else if (l->style == DOTTED_LINE)
	  fprintf(tfp, " dotted");
      }

    /* Should have a #define somewhere for the # of fill patterns */
    if (l->fill_style != UNFILLED)
      fprintf(tfp, " fill %.3f", ((double) l->fill_style ) / (double) BLACK_FILL);

    fprintf(tfp, " with .sw at (%.3f,%.3f) ",
	    minx / ppi, convy(maxy / ppi));

    width = (maxx - minx) / ppi;
    if (width < 0.0) width = -width;
    height = convy(maxy / ppi) - convy(miny / ppi);
    if (height < 0.0) height = -height;

    fprintf(tfp, "width %.3f height %.3f", width, height);

    if (OptArcBox && l->type == T_ARC_BOX)
      fprintf(tfp, " rad %.3f", l->radius/ppi);

    AddThickness();

    fprintf(tfp, "\n");
  }

  return(Valid);
}

void
genpic_line(l)
F_line	*l;
{
	F_point		*p, *q;

	/* print any comments */
	print_comments(".\\\" ",l->comments, "");

	if (l->type == T_ARC_BOX && !OptArcBox)
	{ fprintf(stderr, "Arc box not implemented; substituting box.\n");
	  l->type = T_BOX;
	}

	set_linewidth(l->thickness);
	set_style(l->style, l->style_val);
	p = l->points;
	q = p->next;
	if (q == NULL)	/* A single point line */
	{   fprintf(tfp, "line from %.3f,%.3f to %.3f,%.3f",
			p->x/ppi, convy(p->y/ppi), p->x/ppi, convy(p->y/ppi));
	    AddThickness();
	    fprintf(tfp, "\n");
	    return;
	}

	if (l->type == T_BOX || l->type == T_ARC_BOX)
	{ if (genpic_box(l)) return;
	  fprintf(stderr, "Invalid T_BOX or T_ARC_BOX in fig file\n");
          fprintf(stderr, "  Using 'line' instead\n");
	}

	fprintf(tfp, "line");

	if (l->style_val > 0.0)
	{ if (l->style == DASH_LINE)
	    fprintf(tfp, " dashed");
	  else
	    if (l->style == DOTTED_LINE)
	      fprintf(tfp, " dotted");
	}

	/* Place arrowheads or lack there of on the line*/
	if ((l->for_arrow) && (l->back_arrow))
	    fprintf(tfp, " <->");
	else if (l->back_arrow)
	    fprintf(tfp, " <-");
	else if (l->for_arrow)
	    fprintf(tfp, " ->");

	fprintf(tfp, " from %.3f,%.3f", p->x/ppi, convy(p->y/ppi));
	do
	{ fprintf(tfp, " to %.3f,%.3f", q->x/ppi, convy(q->y/ppi));
	  q = q->next;
	} while (q != NULL);

	AddThickness();

	fprintf(tfp, "\n");
}

void
genpic_spline(s)
F_spline	*s;
{
	/* print any comments */
	print_comments(".\\\" ",s->comments, "");

	if (int_spline(s))
	    genpic_itp_spline(s);
	else
	    genpic_ctl_spline(s);
	}

static void
genpic_ctl_spline(s)
F_spline	*s;
{
	if (closed_spline(s))
	    genpic_closed_spline(s);
	else
	    genpic_open_spline(s);
	}

static void
genpic_open_spline(s)
F_spline	*s;
{
	double		x1, y1, x2, y2;
	F_point		*p, *q;

	p = s->points;
	x1 = p->x/ppi; y1 = convy(p->y/ppi);
	p = p->next;
	x2 = p->x/ppi; y2 = convy(p->y/ppi);


	/* Pic's spline supports only solid line style */
	/* set_linewidth(s->thickness); */

	if (p->next == NULL) {
	    fprintf(tfp, "line");

           /* Attach arrowhead as required */
	    if ((s->for_arrow) && (s->back_arrow))
	       fprintf(tfp, " <->");
	    else if (s->back_arrow)
	       fprintf(tfp, " <-");
	    else if (s->for_arrow)
	       fprintf(tfp, " ->");

	    fprintf(tfp, " from %.3f,%.3f to %.3f,%.3f", x1, y1, x2, y2);

	    AddThickness();

	    fprintf(tfp, "\n");

	    return;
	    }

	fprintf(tfp, "spline"); 

           /* Attach arrowhead as required */
	    if ((s->for_arrow) && (s->back_arrow))
	       fprintf(tfp, " <->");
	    else if (s->back_arrow)
	       fprintf(tfp, " <-");
	    else if (s->for_arrow)
	       fprintf(tfp, " ->");

	fprintf(tfp, " from %.3f,%.3f to %.3f,%.3f", x1, y1, x2, y2);

	for (q = p->next; q->next != NULL; p = q, q = q->next)
	    fprintf(tfp, " to %.3f,%.3f", q->x/ppi, convy(q->y/ppi));
	fprintf(tfp, " to %.3f,%.3f", q->x/ppi, convy(q->y/ppi));

	AddThickness();

	fprintf(tfp, "\n");
}

void
genpic_ellipse(e)
F_ellipse	*e;
{
	/* print any comments */
	print_comments(".\\\" ",e->comments, "");

	set_linewidth(e->thickness);
	if (e->type == 3 || e->type == 4)
	  fprintf(tfp, "circle at %.3f,%.3f rad %.3f",
		e->center.x/ppi, convy(e->center.y/ppi),
		e->radiuses.x/ppi);
	else
	  fprintf(tfp, "ellipse at %.3f,%.3f wid %.3f ht %.3f",
		e->center.x/ppi, convy(e->center.y/ppi),
		2 * e->radiuses.x/ppi, 2 * e->radiuses.y/ppi);

	if ( OptEllipseFill && e->fill_style != UNFILLED)
	  fprintf(tfp, " fill %.3f", (double)e->fill_style / (double) BLACK_FILL);

	AddThickness();

	fprintf(tfp, "\n");
}

/*
Text is display on the screen with the base line starting at
(base_x, base_y); some characters extend below this line.
Pic displays the center of the height of text at the given
coordinate. HT_OFFSET is use to compensate all the above factors
so text position in fig 1.4 should be at the same position on
the screen as on the hard copy.
*/
#define			HT_OFFSET	(0.2 / 72.0)

void
genpic_text(t)
F_text	*t;
{
	float	 y;
	int	 size;
        char	*tpos;

	/* print any comments */
	print_comments(".\\\" ",t->comments, "");


	size = PICFONTMAG(t);
	if (!OptNoUnps) {
	  unpsfont(t);
	  fprintf(tfp, "\"\\s%d\\f%s", size, PICFONT(t->font));
	} else {
	  fprintf(tfp, ".ps\n.ps %d\n", size );
	  fprintf(tfp, ".ft\n.ft %s\n", PICPSFONT(t) );
	}

        switch (t->type) {
        case T_LEFT_JUSTIFIED:
        case DEFAULT:
            tpos = "ljust";
            break;
        case T_CENTER_JUSTIFIED:
            tpos = "";
            break;
        case T_RIGHT_JUSTIFIED:
            tpos = "rjust";
            break;
        default:
            fprintf(stderr, "unknown text position type\n");
            exit(1);
        }    
 	y = convy(t->base_y/ppi) + size * HT_OFFSET;
 	if (!OptNoUnps)
 	    fprintf(tfp, "%s\\fP\" at %.3f,%.3f %s\n",
 			t->cstring, t->base_x/ppi, y, tpos);
 	else
 	    fprintf(tfp, "\"%s\" at %.3f,%.3f %s\n.ft \n.ps \n",
			t->cstring, t->base_x/ppi, y, tpos);
	}

void
genpic_arc(a)
F_arc	*a;
{
	double		cx, cy, sx, sy, ex, ey;

	/* print any comments */
	print_comments(".\\\" ",a->comments, "");

	cx = a->center.x/ppi; cy = convy(a->center.y/ppi);
	sx = a->point[0].x/ppi; sy = convy(a->point[0].y/ppi);
	ex = a->point[2].x/ppi; ey = convy(a->point[2].y/ppi);

	set_linewidth(a->thickness);

	fprintf(tfp, "arc ");

	if (a->style_val > 0.0) {
	    if (a->style == DASH_LINE)
		fprintf(tfp, " dashed");
	    else if (a->style == DOTTED_LINE)
		fprintf(tfp, " dotted");
	}

	/* Attach arrowhead as required */
	if ((a->type == T_OPEN_ARC) && (a->thickness != 0) && (a->back_arrow || a->for_arrow)) {
	    if ((a->for_arrow) && (a->back_arrow))
		fprintf(tfp, " <->");
	    else if (a->back_arrow)
		fprintf(tfp, " <-");
	    else if (a->for_arrow)
		fprintf(tfp, " ->");
	}


	fprintf(tfp, " at %.3f,%.3f from %.3f,%.3f to %.3f,%.3f",
		cx, cy, sx, sy, ex, ey);

	if (!a->direction)
	  fprintf(tfp, " cw");

	if (a->fill_style != UNFILLED)
	  fprintf(stderr, "PIC does not support filled arcs ... ignoring 'fill' directive\n");

	AddThickness();
	fprintf(tfp, "\n");
}

#define		THRESHOLD	.05	/* inch */

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
	if (fabs(x1 - xmid) < THRESHOLD && fabs(y1 - ymid) < THRESHOLD)
	{ fprintf(tfp, " to %.3f,%.3f", xmid, ymid);
	}
	else {
	    quadratic_spline(x1, y1, ((x1+a2)/2), ((y1+b2)/2),
			((3*a2+a3)/4), ((3*b2+b3)/4), xmid, ymid);
	    }

	if (fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD)
	{ fprintf(tfp, " to %.3f,%.3f", x4, y4);
	}
	else {
	    quadratic_spline(xmid, ymid, ((a2+3*a3)/4), ((b2+3*b3)/4),
			((a3+x4)/2), ((b3+y4)/2), x4, y4);
	    }
	}

void
genpic_closed_spline(s)
F_spline	*s;
{
	F_point	*p;
	double	cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
	double	x1, y1, x2, y2;

	p = s->points;
	x1 = p->x/ppi;  y1 = convy(p->y/ppi);
	p = p->next;
	x2 = p->x/ppi;  y2 = convy(p->y/ppi);
	cx1 = (x1 + x2) / 2;      cy1 = (y1 + y2) / 2;
	cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;

	for (p = p->next; p != NULL; p = p->next) {
	    fprintf(tfp, "line from %.3f,%.3f ", cx1, cy1);
	    x1 = x2;  y1 = y2;
	    x2 = p->x/ppi;  y2 = convy(p->y/ppi);
	    cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	    cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    AddThickness();
	    fprintf(tfp, "\n");
	    cx1 = cx4;  cy1 = cy4;
	    cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;
	    }
	x1 = x2;  y1 = y2;
	p = s->points->next;
	x2 = p->x/ppi;  y2 = convy(p->y/ppi);
	cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	fprintf(tfp, "line from %.3f,%.3f ", cx1, cy1);
	quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	AddThickness();
	fprintf(tfp, "\n");
}

static void
bezier_spline(a0, b0, a1, b1, a2, b2, a3, b3)
double	a0, b0, a1, b1, a2, b2, a3, b3;
{
	double	x0, y0, x3, y3;
	double	sx1, sy1, sx2, sy2, tx, ty, tx1, ty1, tx2, ty2, xmid, ymid;

	x0 = a0; y0 = b0;
	x3 = a3; y3 = b3;
	if (fabs(x0 - x3) < THRESHOLD && fabs(y0 - y3) < THRESHOLD)
	{ fprintf(tfp, " to %.3f,%.3f", x3, y3);
	}
	else {
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
genpic_itp_spline(s)
F_spline	*s;
{
	F_point		*p1, *p2, *pfirst;
	F_control	*cp1, *cp2;
	double		x1, x2, y1, y2;

	p1 = s->points;
	cp1 = s->controls;
	cp2 = cp1->next;
	x2 = p1->x/ppi; y2 = convy(p1->y/ppi);

         pfirst = p1->next;/*save first to test in loop*/
	for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
		p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {

	    fprintf(tfp, "line ");

           /* Attach arrowhead as required */

	    if ((s->back_arrow) && (p2 == pfirst))
	       fprintf(tfp, " <- ");
	    else if ((s->for_arrow) && (p2->next == NULL))
	       fprintf(tfp, " -> ");

	    fprintf(tfp, " from %.3f,%.3f", x2, y2);

	    x1 = x2; y1 = y2;
	    x2 = p2->x/ppi; y2 = convy(p2->y/ppi);
	    bezier_spline(x1, y1, (double)cp1->rx/ppi, convy(cp1->ry/ppi),
		(double)cp2->lx/ppi, convy(cp2->ly/ppi), x2, y2);
	    AddThickness();
	    fprintf(tfp, "\n");
	    }

	}

struct driver dev_pic = {
     	genpic_option,
	genpic_start,
	gendev_null,
	genpic_arc,
	genpic_ellipse,
	genpic_line,
	genpic_spline,
	genpic_text,
	genpic_end,
	INCLUDE_TEXT
};

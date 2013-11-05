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
 *	gentextyl.c : TeXtyl driver for fig2dev
 *
 * 	Author: Gary Beihl, MCC 8/90
 *	(beihl@mcc.com)
 *
 *      Based on the pictex driver by Micah Beck
 *
 */

#include "fig2dev.h"
#include "object.h"
#include "texfonts.h"

static void putline();

#define rint(a) floor((a)+0.5)     /* close enough? */

static void gentextyl_ctl_spline(), gentextyl_itp_spline();
static void bezier_spline();
static void draw_arrow_head();
static void set_style();
static void rtop();
static void set_linewidth();
static void set_style();

static int		line_style = 0; /* Textyl solid line style */
static int 		linethick = 2;  /* Range is 1-12 `pixels' */
 

static void
gentextyl_option(opt, optarg)
char opt, *optarg;
{
  int i;

	switch (opt) {
		case 'a':
		    /* capfonts = 1; */
		    break;

		case 'f':			/* set default text font */
		    for ( i = 1; i <= MAX_FONT; i++ )
			if ( !strcmp(optarg, texfontnames[i]) ) break;

		    if ( i > MAX_FONT)
			{
			  fprintf(stderr,
				 "warning: non-standard font name %s ignored\n", optarg);
			}
		 else
			{
			  texfontnames[0] = texfontnames[i];
#ifdef NFSS
			  texfontfamily[0] = texfontfamily[i];
			  texfontseries[0] = texfontseries[i];
			  texfontshape[0] = texfontshape[i];
#endif
			}
		    break;

		case 'l':			/* set line thickness */
		    linethick = atoi(optarg);
                    if (linethick < 1 || linethick > 12) {
                      put_msg(Err_badarg, opt, "textyl");
                      exit(1);
                    }
		    break;

		case 'p':
		case 's':
		case 'm':
		case 'L':
		    break;

	default:
		put_msg(Err_badarg, opt, "textyl");
		exit(1);
	}
}

#define			TOP	(10.5)	/* top of page is 10.5 inch */
#define SCALE (65536.0*72.27)
#define measure 'S'

static int
convy(a)
double a;
{
   return (int)(((ury - a) * SCALE) / ppi);
}

static int
convx(a)
double a; {
  float f;
  f = a * SCALE;
  return (int)(f / ppi);
}

void
gentextyl_start(objects)
F_compound	*objects;
{

	texfontsizes[0] = texfontsizes[1] = 
		texfontsizes[(font_size != 0.0? (int) font_size : DEFAULT_FONT_SIZE)+1];

	/* print any whole-figure comments prefixed with "%" */
	if (objects->comments) {
	    fprintf(tfp,"%%\n");
	    print_comments("% ",objects->comments, "");
	    fprintf(tfp,"%%\n");
	}
	/* Textyl start */
	fprintf(tfp, "\\begintyl{%fsp}\n",ury/ppi*SCALE);
}

int
gentextyl_end()
{
  fprintf(tfp,"\\endtyl\n");

  /* all ok */
  return 0;
}


static void
set_linewidth(w)
int	w;
{
/* Nop */
}

void
gentextyl_line(l)
F_line	*l;
{
	F_point		*p, *q;

	/* print any comments prefixed with "%" */
	print_comments("% ",l->comments, "");

	fprintf(tfp, "%%\n%% Fig POLYLINE object\n%%\n");

	set_linewidth(l->thickness);
	set_style(l->style, l->style_val);

	p = l->points;
	q = p->next;
        

	if (q == NULL) { /* A single point line */
	    fprintf(tfp, "\\special{tyl line %c %d L %d %u %u; %u %u}\n", 
               measure,linethick,line_style,
			convx((double)p->x), convy((double)p->y), 
                        convx((double)p->x), convy((double)p->y));
	    return;
	    }
	if (l->back_arrow)
	    draw_arrow_head((double)q->x, (double)q->y, (double)p->x,
		(double)p->y, l->back_arrow->ht, l->back_arrow->wid);
	set_style(l->style, l->style_val);

	while (q->next != NULL) {

	    putline(p->x, p->y, q->x, q->y);
	    p = q;
	    q = q->next;
	    }

	putline(p->x, p->y, q->x, q->y);
	if (l->for_arrow)
	    draw_arrow_head((double)p->x, (double)p->y, (double)q->x,
		(double)q->y, l->for_arrow->ht, l->for_arrow->wid);

	if (l->fill_style && (int)l->fill_style != DEFAULT)
		fprintf(stderr, "Line area fill not implemented\n");
	}

/* 
 * set_style - issue style commands as appropriate
 */
static void
set_style(style, dash_len)
     int style;
     double dash_len;
{
  
  switch (style) {
  case SOLID_LINE:
    line_style = 0;
    break;
    
  case DASH_LINE:
    line_style = 2;
    break;
    
  case DOTTED_LINE:
    line_style = 1;
    break;
  }
}

/*
 * putline
 */
static void
putline (start_x, start_y, end_x, end_y)
int	start_x, start_y, end_x, end_y;
{

   fprintf(tfp, "\\special{tyl line %c %d L %d %u %u; %u %u}\n", 
        measure, linethick,line_style,
	convx((double)start_x), convy((double)start_y), 
        convx((double)end_x), convy((double)end_y));

}


void
gentextyl_spline(s)
F_spline	*s;
{
	/* print any comments prefixed with "%" */
	print_comments("% ",s->comments, "");

	set_linewidth(s->thickness);
	set_style(s->style, s->style_val);

	if (int_spline(s))
	    gentextyl_itp_spline(s);
	else
	    gentextyl_ctl_spline(s);

	if (s->fill_style && (int)s->fill_style != DEFAULT)
		fprintf(stderr, "Spline area fill not implemented\n");
}

void
gentextyl_ellipse(e)
F_ellipse	*e;
{
	int sx, sy;
	int radius;

	/* print any comments prefixed with "%" */
	print_comments("% ",e->comments, "");

	fprintf(tfp, "%%\n%% Fig ELLIPSE\n%%\n");

	set_linewidth(e->thickness);
	set_style(e->style, e->style_val);

	if (e->radiuses.x == e->radiuses.y) {
          fprintf(tfp, "\\special{tyl arc %c %d L 0 %u @ %u,%u 0 360}\n",
            measure,linethick,convx((double)e->radiuses.x),
            convx((double)e->center.x),convy((double)e->center.y));
        }
	else {
          if (e->radiuses.x > e->radiuses.y) {
            sy = 100;
            sx = ((float)e->radiuses.x/(float)e->radiuses.y) * 100.0;
            radius = e->radiuses.y;
          }
          else {
            sx = 100;
            sy = ((float)e->radiuses.y/(float)e->radiuses.x) * 100.0;
            radius = e->radiuses.x;            
          }
                fprintf(tfp, 
                 "\\special{tyl arc %c T %u %u 0 0 0 %d L 0 %u @ %u,%u 0 360}\n",
                  measure,sx,sy,linethick,convx((double)radius),
            convx((double)e->center.x),convy((double)e->center.y));
		if (e->fill_style && (int)e->fill_style != DEFAULT)
			fprintf(stderr, "Ellipse area fill not implemented\n");
		}
	}

#define			HT_OFFSET	(0.2 / 72.0)

void
gentextyl_text(t)
F_text	*t;
{
	double	x, y;

	/* print any comments prefixed with "%" */
	print_comments("% ",t->comments, "");

        fprintf(tfp, "%%\n%% Fig TEXT object\n%%\n");

	x = t->base_x;
	y = t->base_y;

	switch (t->type) {

	    case T_LEFT_JUSTIFIED:
	    case DEFAULT:
		break;
	    default:
		fprintf(stderr, "Warning: Text incorrectly positioned\n");
	        break;
	    }

        fprintf(tfp,"\\special{tyl label %c 1 %u %u \"%s\"}\n",
          measure,convx(x),convy(y),t->cstring);

}

void
gentextyl_arc(a)
F_arc	*a;
{
	double		x, y;
	double		cx, cy, sx, sy, ex, ey;
	double		dx1, dy1, dx2, dy2, r1, r2, th1, th2;

	/* print any comments prefixed with "%" */
	print_comments("% ",a->comments, "");

	set_linewidth(a->thickness);
	set_style(a->style, a->style_val);

	cx = a->center.x; cy = a->center.y;
	sx = a->point[0].x; sy = a->point[0].y;
	ex = a->point[2].x; ey = a->point[2].y;

	if ((a->type == T_OPEN_ARC) && (a->thickness != 0) && (a->back_arrow || a->for_arrow)) {
	    if (a->for_arrow) {
		arc_tangent(cx, cy, ex, ey, !a->direction, &x, &y);
		draw_arrow_head(x, y, ex, ey,
			a->for_arrow->ht, a->for_arrow->wid);
	    }
	    if (a->back_arrow) {
		arc_tangent(cx, cy, sx, sy, a->direction, &x, &y);
		draw_arrow_head(x, y, sx, sy,
			a->back_arrow->ht, a->back_arrow->wid);
	    }
	}


	cy = ury - a->center.y;
	sy = ury - a->point[0].y;
	ey = ury - a->point[2].y;

	dx1 = sx - cx;
	dy1 = sy - cy;
	dx2 = ex - cx;
	dy2 = ey - cy;

	cy = a->center.y;
	sy = a->point[0].y;
	ey = a->point[2].y;
	    
	rtop(dx1, dy1, &r1, &th1);
	rtop(dx2, dy2, &r2, &th2);

	set_linewidth(a->thickness);

	if (a->direction) { /* Counterclockwise */
              fprintf(tfp,"\\special{tyl arc %c %d L 0 %u @ %u,%u %d %d}\n",
                measure,linethick,convx(r1),convx(cx),convy(cy),
                (int)(180/M_PI * th1), (int)(180/M_PI * th2));
	      }
	else {
              fprintf(tfp,"\\special{tyl arc %c %d L 0 %u @ %u,%u %d %d}\n",
                measure,linethick,convx(r1),convx(cx),convy(cy),
                (int)(180/M_PI * th2), (int)(180/M_PI * th1));
	      }

	if (a->fill_style && (int)a->fill_style != DEFAULT)
		fprintf(stderr, "Arc area fill not implemented\n");
	}



/*
 * rtop - rectangular to polar conversion
 */
static void
rtop(x, y, r, th)
double x, y, *r, *th;
{
	*r = sqrt(x*x+y*y);
	*th = acos(x/(*r));

	if (y < 0) *th = 2*M_PI - *th;
}

/*	draw arrow heading from (x1, y1) to (x2, y2)	*/

static void
draw_arrow_head(x1, y1, x2, y2, arrowht, arrowwid)
double	x1, y1, x2, y2;
double  arrowht, arrowwid;
{
	double	x, y, xb, yb, dx, dy, l, sina, cosa;
	double	xc, yc, xd, yd;

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

        fprintf(tfp, "%%\n%% arrow head\n%%\n");

	fprintf(tfp, "\\special{tyl line %c %d %u %u; %u %u}\n",measure,linethick,
		convx(xc), convy(yc), convx(x2), convy(y2));
	fprintf(tfp, "\\special{tyl line %c %d %u %u; %u %u}\n",measure,linethick,
                convx(x2), convy(y2), convx(xd), convy(yd));

	}

#define THRESHOLD (10.0)
double last_x, last_y;

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
	if (fabs(x1 - xmid) < THRESHOLD && fabs(y1 - ymid) < THRESHOLD) {
	    fprintf(tfp, "\\special{tyl line %c %d %u %u; %u %u}\n", 
              measure, linethick,convx(last_x),convy(last_y),
              convx(xmid), convy(ymid));
            last_x = xmid; last_y = ymid;
	}

	else {
	    quadratic_spline(x1, y1, ((x1+a2)/2), ((y1+b2)/2),
			((3*a2+a3)/4), ((3*b2+b3)/4), xmid, ymid);
	    }

	if (fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD) {
	    fprintf(tfp, "\\special{tyl line %c %d %u %u; %u %u}\n", 
              measure, linethick,convx(last_x),convy(last_y),convx(x4), convy(y4));
            last_x = x4; last_y = y4;
	}

	else {
	    quadratic_spline(xmid, ymid, ((a2+3*a3)/4), ((b2+3*b3)/4),
			((a3+x4)/2), ((b3+y4)/2), x4, y4);
	    }
	}

static void
gentextyl_ctl_spline(s)
F_spline	*s;
{
	F_point	*p;
	double	cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
	double	x1, y1, x2, y2;

    	fprintf(tfp, "%%\n%% Fig CONTROL PT SPLINE\n%%\n");

	p = s->points;
	x1 = p->x;  y1 = p->y;
	p = p->next;
	x2 = p->x;  y2 = p->y;
	cx1 = (x1 + x2) / 2;      cy1 = (y1 + y2) / 2;
	cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;

	if (closed_spline(s)) {
	    fprintf(tfp, "%% closed spline\n%%\n");
            last_x = cx1; last_y = cy1;
	    }
	else {
	    fprintf(tfp, "%% open spline\n%%\n");
	    if (s->back_arrow)
	        draw_arrow_head(cx1, cy1, x1, y1,
			s->back_arrow->ht, s->back_arrow->wid);
            fprintf(tfp, "\\special{tyl line %c %d %u %u;%u %u}\n",
              measure,linethick,
              convx(x1),convy(y1),convx(cx1),convy(cy1));
            last_x = cx1; last_y = cy1;
	    }

	for (p = p->next; p != NULL; p = p->next) {
	    x1 = x2;  y1 = y2;
	    x2 = p->x;  y2 = p->y;
	    cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	    cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    cx1 = cx4;  cy1 = cy4;
	    cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;
	    }
	x1 = x2;  y1 = y2;
	p = s->points->next;
	x2 = p->x;  y2 = p->y;
	cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	if (closed_spline(s)) {
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    }
	else {
            fprintf(tfp,"\\special{tyl line %c %d %u %u;%u %u}\n",
              measure,linethick,
              convx(cx1),convy(cy1),convx(x1),convy(y1));
	    if (s->for_arrow)
	    	draw_arrow_head(cx1, cy1, x1, y1,
			s->for_arrow->ht, s->for_arrow->wid);
	    }

	}

static void 
gentextyl_itp_spline(s)
F_spline	*s;
{
  F_point	*p1, *p2;
  F_control	*cp1, *cp2;
  double	 x1, x2, y1, y2;
  
  
  p1 = s->points;
  cp1 = s->controls;
  x2 = p1->x; y2 = p1->y;
  
  if (s->back_arrow)
    draw_arrow_head(cp1->rx, cp1->ry, x2, y2,
		    s->back_arrow->ht, s->back_arrow->wid);
  
  last_x = x2; last_y = y2;
  
  fprintf(tfp, "%%\n%% Fig INTERPOLATED SPLINE\n%%\n");
  for (p2 = p1->next, cp2 = s->controls->next; 
       p2 != NULL; 
       cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {
    x1 = x2; y1 = y2; x2 = p2->x; y2 = p2->y;
    bezier_spline(x1,y1,cp1->rx,cp1->ry,
		  cp2->lx,cp2->ly,x2,y2);
  }
  
  if (s->for_arrow)
    draw_arrow_head(cp1->lx, cp1->ly, x2, y2,
		    s->for_arrow->ht, s->for_arrow->wid);
}

static void
bezier_spline(a0, b0, a1, b1, a2, b2, a3, b3)
double	a0, b0, a1, b1, a2, b2, a3, b3;
{
  double	x0, y0, x3, y3;
  double	sx1, sy1, sx2, sy2, tx, ty, tx1, ty1, tx2, ty2, xmid, ymid;
  
  x0 = a0; y0 = b0;
  x3 = a3; y3 = b3;
  if (fabs(x0 - x3) < THRESHOLD && fabs(y0 - y3) < THRESHOLD)  {
    fprintf(tfp,"\\special{tyl line %c %d %u %u ; %u %u}\n",measure,linethick,
	    convx(last_x),convy(last_y),convx(x3),convy(y3));
    last_x = x3; last_y = y3; 
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

struct driver dev_textyl = {
     	gentextyl_option,
	gentextyl_start,
	gendev_null,
	gentextyl_arc,
	gentextyl_ellipse,
	gentextyl_line,
	gentextyl_spline,
	gentextyl_text,
	gentextyl_end,
	EXCLUDE_TEXT
};


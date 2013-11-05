/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1993 Anthony Starks (ajs@merck.com)
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
 *  fig2MF -- convert fig to mfpic METAFONT code
 *
 *  Copyright (c) 1993 Anthony Starks (ajs@merck.com)
 *
 *  Version 0.00 -- 	Incorporate Tobin's small suggestions
 *  Version 0.01 -- 	Change scaling to inches,
 *			default [x-y]scale is 1/8 inch
 *			slight format changes in cinit()
 *  Version 0.02 --	Fixed pen switching bug
 *  Version 0.03 --	Support new arcthree mode
 *  Version 0.04 --	Token support for text
 *  Version 0.05 --	Integrated into fig2dev - B.V.Smith 11/28/94
 *  Version 0.06 --	Fixed a number of bugs.
 			Converted to mfpic 0.2.10.8 alfa
				/M.Bengtsson July 1, 1998
 */

/* TODO: 
 *  - Dashed and dotted lines
 *  - Arrow heads
 *  - Filled polylines and open splines
 *  - Difference between interpolated and non-interpolated splines
 *  - ...
 */

#include "fig2dev.h"
#include "object.h"

#define dofill(obj)	1.2-((double)obj->fill_style/(double)BLACK_FILL)
#define DEF_PEN		0.5
#define PEN_INCR	0.10
#define INFTY		1e5
typedef struct {
	char *keyword;
	double *value;
} Options;

static double code = 32.0;
static int oldpen = 0;
static double penscale = 1;
static double xscale = 0.125;
static double yscale = 0.125;
static double xl = 0.0;
static double xu = 8.0;
static double yl = 0.0;
static double yu = 8.0;
static double maxy = INFTY;

void
setpen(thickness)
int thickness;
{
  if (thickness != oldpen) {
    fprintf(tfp, "penwd := %.2fpt;\n", 
	    thickness/THICK_SCALE*penscale*yscale*mag);
    fprintf(tfp, "drawpen := pencircle scaled penwd yscaled aspect_ratio;\n");
    oldpen = thickness;
  }
}

void
genmf_start(objects)
F_compound	*objects;
{
	int	curchar;

	if (maxy == INFTY) 
		maxy = yu;
	
	curchar = (int)code;

	fprintf(tfp,"%%\n%% fig2dev -L mf (Version %s Patchlevel %s)\n",
			VERSION, PATCHLEVEL);
	fprintf(tfp,"%%\n");

	/* print any whole-figure comments prefixed with "%" */
	if (objects->comments) {
	    fprintf(tfp,"%%\n");
	    print_comments("% ",objects->comments, "");
	    fprintf(tfp,"%%\n");
	}
	fprintf(tfp,"%%\n%% Preamble\n%%\n");

	fprintf(tfp,"mag:=1; input grafbase.mf; code:=%g;\n", code);
	fprintf(tfp,"interim hdwdr:=1; interim hdten:=1;\n");

	fprintf(tfp,"%%\n%% %s (char %d)\n%%\n",
		(name? name: ((from) ? from : "stdin")), ++curchar);
	fprintf(tfp,"xscale:=%.3f; yscale:=%.3f;\n", xscale*mag, yscale*mag);
	fprintf(tfp,"bounds(%.3f,%.3f,%.3f,%.3f);\n", xl, xu, yl, yu);
	fprintf(tfp,"unitlen:=1.0in#;\n");
	fprintf(tfp,"beginmfpic(incr code);\n");
	setpen(1);
}


int
genmf_end()
{
	fprintf(tfp,"endmfpic;\nend.\n");

	/* all ok */
	return 0;
}

void
genmf_option(opt, optarg)
char opt;
char *optarg;
{
    switch (opt) {
	case 'C':
	    code = atof(optarg);
	    break;
	case 'n':
	    name = optarg;
	    break;
	case 'p':
	    penscale = atof(optarg);
	    break;
	case 't':
	    maxy = atof(optarg);
	    break;
	case 'x':
	    xl = atof(optarg);
	    break;
	case 'y':
	    yl = atof(optarg);
	    break;
	case 'X':
	    xu = atof(optarg);
	    break;
	case 'Y':
	    yu = atof(optarg);
	    break;
	case 'f':		/* ignore magnification, font sizes and lang here */
	case 'm':
	case 's':
	case 'L':
	    break;
	default:
	    put_msg(Err_badarg, opt, "mf");
	    exit(1);
    }
}

void
genmf_line(l)
F_line *l;
{
	F_point	*p;

	/* print any comments prefixed with "%" */
	print_comments("% ",l->comments, "");

	setpen(l->thickness);
	fprintf(tfp, "  store (curpath)\n");
	if ((l->type==1) && (l->fill_style<0)) /* Open polyline */
		fprintf(tfp,"  drawn polyline(false)\n");
	else {	
  /* Closed and/or filled polygon, cover underlying figures 
   * first with white if this polygon is shaded */
		if (l->fill_style == BLACK_FILL)
			fprintf(tfp,"  filled ");
		else if (l->fill_style < BLACK_FILL && l->fill_style > 0)
			fprintf(tfp,"  shade(%fpt) unfilled", dofill(l));
		else if (l->fill_style == 0)
			fprintf(tfp,"  drawn unfilled ");
		else 
			fprintf(tfp,"  drawn ");
		fprintf(tfp,"polyline(true)\n");
	}
	p = l->points;
	fprintf(tfp,"      ((%f, %f)", (float) p->x/ppi, maxy-((float) p->y/ppi));
	p = p->next;
	for ( ; p != NULL; p=p->next) {
	    fprintf(tfp,",\n       (%f, %f)", (float) p->x/ppi, maxy-((float) p->y/ppi));
	}
	fprintf(tfp, ");\n");
	return;
}


void
genmf_spline(s)
F_spline *s;
{
	F_point	*p;

	/* print any comments prefixed with "%" */
	print_comments("% ",s->comments, "");

	setpen(s->thickness);
	fprintf(tfp, "  store (curpath)\n");
	if ((s->type == 0) || (s->type == 2) && (s->fill_style < 0)) /* Open spline */
		fprintf(tfp,"  drawn curve(false)\n");
	else { /* Closed and/or filled spline, see comment above */
		if (s->fill_style == BLACK_FILL)
			fprintf(tfp,"  filled ");
		else if (s->fill_style < BLACK_FILL && s->fill_style > 0)
			fprintf(tfp,"  shade(%fpt) unfilled ", dofill(s));
		else if (s->fill_style == 0)
			fprintf(tfp,"  drawn unfilled ");
		else
			fprintf(tfp,"  drawn ");
		fprintf(tfp,"curve(true)\n");
	}
	p = s->points;
	fprintf(tfp,"      ((%f, %f)", (float) p->x/ppi, maxy-((float) p->y/ppi));
	p = p->next;
	for ( ; p != NULL; p=p->next) {
	    fprintf(tfp,",\n       (%f, %f)", (float) p->x/ppi, maxy-((float) p->y/ppi));
	}
	fprintf(tfp, ");\n");
	return;
}


void
genmf_ellipse(e)
F_ellipse *e;
{
	/* print any comments prefixed with "%" */
	print_comments("% ",e->comments, "");

        setpen(e->thickness);
	fprintf(tfp, "  store (curpath)\n");
	if (e->fill_style == BLACK_FILL)
		fprintf(tfp,"  filled ");
	else if (e->fill_style < BLACK_FILL && e->fill_style > 0)
		fprintf(tfp,"  shade(%fpt) ", dofill(e));
	else
		fprintf(tfp,"  drawn ");
	if (e->type == 3 || e->type == 4)
	{
		fprintf(tfp,"circle((%f,%f),%f);\n",
			(float) e->center.x/ppi, maxy-((float) e->center.y/ppi),
			(float) e->radiuses.x/ppi);
	}
	else if (e->type == 1 || e->type == 2)
	{
		fprintf(tfp,"ellipse((%f,%f),%f,%f,0);\n",
			(float) e->center.x/ppi, maxy-((float) e->center.y/ppi), 
			(float) e->radiuses.x/ppi, (float) e->radiuses.y/ppi);
	}
}

void
genmf_arc(a)
F_arc *a;
{
	/* print any comments prefixed with "%" */
	print_comments("% ",a->comments, "");

	setpen(a->thickness);
	fprintf(tfp, "  store (curpath)\n");
	fprintf(tfp,"  drawn arcppp((%f,%f), (%f,%f), (%f,%f));\n",
		(float) a->point[0].x/ppi, maxy-((float) a->point[0].y/ppi),
		(float) a->point[1].x/ppi, maxy-((float) a->point[1].y/ppi),
		(float) a->point[2].x/ppi, maxy-((float) a->point[2].y/ppi));

	return;
}

void
genmf_text(t)
F_text *t;
{
	fprintf(stderr, "warning: text ignored in MF output: `%s'\n",
		t->cstring);
	return;
}


struct driver dev_mf = {
     	genmf_option,
	genmf_start,
	gendev_null,
	genmf_arc,
	genmf_ellipse,
	genmf_line,
	genmf_spline,
	genmf_text,
	genmf_end,
	INCLUDE_TEXT
};

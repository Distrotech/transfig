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
 *	genpictex.C : PiCTeX driver for fig2dev
 *
 * 	Author Micah Beck, Cornell University, 4/88
 *    Color, rotated text and ISO-chars added by Herbert Bauer 11/91
*/

#include "fig2dev.h"
#include "object.h"
#include "genlatex.h"
#include "setfigfont.h"
#include "texfonts.h"

#define UNIT "cm"       /* dip */
#define CONVUNIT 2.54   /* dip */

extern char	*ISO1toTeX[];
extern char	*ISO2toTeX[];
extern Boolean	 FontSizeOnly;	/* defined in setfigfont.c */

static void	genpictex_ctl_spline(), genpictex_itp_spline();
static void	bezier_spline();
static void	put_box();
static void	set_style();
static void	putline();
static void	rtop();
static void	draw_arrow_head();
static void	set_linewidth();

#define rint(a) floor((a)+0.5)     /* close enough? */
static int		encoding = 1;
static double		dash_length = -1;
static int		line_style = SOLID_LINE;
static char 		*linethick = "1pt";
static char		*plotsymbol = "\\makebox(0,0)[l]{\\tencirc\\symbol{'160}}";
static int		cur_thickness = -1;
static Boolean		anonymous = False;

static void
genpictex_option(opt, optarg)
char opt, *optarg;
{
        int i;

	FontSizeOnly = False;
	switch (opt) {

		case 'a':			/* anonymous (don't output user name) */
		    anonymous = True;
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
		    linethick = optarg;
		    break;

		case 'p':			/* set plot symbol */
		    plotsymbol = optarg;
		    break;

		case 'E':
		    encoding = atoi(optarg);
		    if (encoding < 0 || encoding > 2)
		      encoding = 1;
		    break;

		case 's':
		case 'm':
		case 'L':
		    break;

	default:
		put_msg(Err_badarg, opt, "pictex");
		exit(1);
	}
}

#define			TOP	10.5	/* top of page is 10.5 inch */

static double
convy(a)
double	a;
{
	return((double) TOP-a);
}

void
genpictex_start(objects)
F_compound	*objects;
{
	char		host[256];
	time_t		when;
	struct passwd	*who;

	texfontsizes[0] = texfontsizes[1] = 
		TEXFONTSIZE(font_size != 0.0? font_size : DEFAULT_FONT_SIZE);

	/* PiCTeX start */

	/* announce filename, version etc */

	(void) time(&when);
	fprintf(tfp, "%%Title: %s\n",
		(name? name: ((from) ? from : "stdin")));
	fprintf(tfp, "%%%%Created by: %s Version %s Patchlevel %s\n",
		prog, VERSION, PATCHLEVEL);
	fprintf(tfp, "%%%%CreationDate: %s", ctime(&when));
	if (gethostname(host, sizeof(host)) == -1)
	    (void)strcpy(host, "unknown-host!?!?");
	if ( !anonymous) {
	    who = getpwuid(getuid());
	    if (who)
		fprintf(tfp, "%%%%User: %s@%s (%s)\n",
			who->pw_name, host, who->pw_gecos);
	}

	/* print any whole-figure comments prefixed with "% " */
	if (objects->comments) {
	    fprintf(tfp,"%%\n");
	    print_comments("% ",objects->comments, "");
	    fprintf(tfp,"%%\n");
	}
	fprintf(tfp, "\\font\\thinlinefont=cmr5\n");
	define_setfigfont(tfp);
	fprintf(tfp, "\\mbox{\\beginpicture\n");
	fprintf(tfp, "\\setcoordinatesystem units <%6.5f%s,%6.5f%s>\n",
			mag, UNIT, mag, UNIT);
	fprintf(tfp, "\\unitlength=%6.5f%s\n", mag, UNIT);
	fprintf(tfp, "\\linethickness=%s\n", linethick);
	fprintf(tfp, "\\setplotsymbol ({%s})\n", plotsymbol);
	fprintf(tfp, "\\setshadesymbol ({\\thinlinefont .})\n");
	fprintf(tfp, "\\setlinear\n");
}

int
genpictex_end()
{
	/* PiCTeX ending */
	fprintf(tfp, "\\linethickness=0pt\n");
        fprintf(tfp, "\\putrectangle corners at %6.3f %6.3f and %6.3f %6.3f\n",
		(llx/ppi)*CONVUNIT, (convy(lly/ppi))*CONVUNIT,
		(urx/ppi)*CONVUNIT, (convy(ury/ppi))*CONVUNIT);
	fprintf(tfp, "\\endpicture}\n");

	/* all ok */
	return 0;
}

static void
set_linewidth(w)
int	w;
{

	    if (!w) {
		fprintf(tfp, "\\linethickness=0pt\n");
		cur_thickness = 0;
		}
	    else {
		cur_thickness = round(w*mag*80/ppi);

		if (!cur_thickness || cur_thickness == 1) {
		    cur_thickness = 1;
		    fprintf(tfp, "\\linethickness=%6.3fpt\n", 0.5*cur_thickness);
		    }
		else {
		    fprintf(tfp, "\\linethickness=%dpt\n", cur_thickness-1);
		    }
		}
	    switch (cur_thickness) {
		case 0:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n", "\\thinlinefont \\ ");
		    break;
		case 1:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n", "\\thinlinefont .");
		    break;
		case 2:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n", 
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'160}}");
		    break;
		case 3:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'161}}");
		    break;
		case 4:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'162}}");
		    break;
		case 5:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'163}}");
		    break;
		case 6:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'164}}");
		    break;
		case 7:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'165}}");
		    break;
		case 8:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'166}}");
		    break;
		case 9:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'167}}");
		    break;
		case 10:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'170}}");
		    break;
		case 11:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'171}}");
		    break;
		case 12:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'172}}");
		    break;
		case 13:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'173}}");
		    break;
		case 14:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'174}}");
		    break;
		case 15:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'175}}");
		    break;
		case 16:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'176}}");
		    break;
		default:
		    fprintf(tfp, "\\setplotsymbol ({%s})\n",
				"\\makebox(0,0)[l]{\\tencirc\\symbol{'176}}");
		    break;
	    }
}

void
genpictex_line(l)
F_line	*l;
{
	F_point		*p, *q;
	int		x, y, llx, lly, urx, ury;

	/* print any comments */
	print_comments("% ",l->comments, "");

	fprintf(tfp, "%%\n%% Fig POLYLINE object\n%%\n");

	if (l->type == T_ARC_BOX) { /* A box with rounded corners */
	  fprintf(stderr, "Arc box not implemented; substituting box.\n");
	  l->type = T_BOX;
	}

	set_linewidth(l->thickness);
	set_style(l->style, l->style_val);
	set_color(l->pen_color);

	p = l->points;
	q = p->next;

	if (q == NULL) { /* A single point line */
	    fprintf(tfp, "\\plot %6.3f %6.3f %6.3f %6.3f /\n",
			(p->x/ppi)*CONVUNIT, (convy(p->y/ppi))*CONVUNIT,
			(p->x/ppi)*CONVUNIT, (convy(p->y/ppi))*CONVUNIT);
	    return;
	    }

	if (l->type == T_BOX || l->type == T_ARC_BOX) /* A (rounded corner) box */
	{
	    x = p->x; y=p->y;
	    llx =urx = x;
	    lly =ury = y;
	    while ( q!= NULL )
	    {
		x = q->x; y=q->y;
		if (x < llx) llx = x;
		if (y < lly) lly = y;
		if (x > urx) urx = x;
		if (y > ury) ury = y;
		q = q->next;
	    }
	    put_box (llx, lly, urx, ury, l);
	    return; 
	}

	if (l->back_arrow)
	    draw_arrow_head(q->x/ppi, convy(q->y/ppi), p->x/ppi,
		convy(p->y/ppi), l->back_arrow->ht/ppi, l->back_arrow->wid/ppi);
	set_style(l->style, l->style_val);

	while (q->next != NULL) {

	    putline(p->x, p->y, (double) q->x, (double) q->y,
		    (q->next)->x, (q->next)->y, -1, -1, -1, -1);
	    p = q;
	    q = q->next;
	    }

	putline(p->x, p->y, (double) q->x, (double) q->y, -1, -1,
		l->points->x, l->points->y,
		l->points->next->x,l->points->next->y);
	if (l->for_arrow)
	    draw_arrow_head(p->x/ppi, convy(p->y/ppi), q->x/ppi,
		convy(q->y/ppi), l->for_arrow->ht/ppi, l->for_arrow->wid/ppi);

	if (l->fill_style != UNFILLED)
		fprintf(stderr, "Line area fill not implemented\n");
	reset_color(l->pen_color);
	}


/*
 * draw box
 */
static void
put_box (llx, lly, urx, ury, l)
int	llx, lly, urx, ury;
F_line	*l;
{
	int radius;

	set_color(l->pen_color);
	if (l->type == T_BOX)
	{
	   if (l->fill_style == BLACK_FILL)
	   {
	       fprintf(tfp,"\\linethickness=%6.3f%s\n", 
	   	    ((convy(lly/ppi))-(convy(ury/ppi)))*CONVUNIT*mag, UNIT);
	       fprintf(tfp,"{\\setsolid"); 
	       fprintf(tfp,"\\putrule from %6.3f %6.3f to %6.3f %6.3f }%%\n",
	   	    (llx/ppi)*CONVUNIT,
	   	    ((convy(lly/ppi)+convy(ury/ppi))/2)*CONVUNIT,
	   	    (urx/ppi)*CONVUNIT,
	   	    ((convy(lly/ppi)+convy(ury/ppi))/2)*CONVUNIT);
	       fprintf(tfp,"\\linethickness=%dpt\n", l->thickness);
	   }
	   else if (l->fill_style != UNFILLED && l->fill_style > (int)(BLACK_FILL*3/4))
	   {
	       fprintf(tfp,"\\setshadegrid span <1pt>\n");
	       fprintf(tfp,"\\shaderectangleson\n");
	   }
	   else if (l->fill_style != UNFILLED && l->fill_style > (int)(BLACK_FILL*2/4))
	   {
	       fprintf(tfp,"\\setshadegrid span <2pt>\n");
	       fprintf(tfp,"\\shaderectangleson\n");
	   }
	   else if (l->fill_style != UNFILLED && l->fill_style > (int)(BLACK_FILL*1/4))
	   {
	       fprintf(tfp,"\\setshadegrid span <4pt>\n");
	       fprintf(tfp,"\\shaderectangleson\n");
	   }
	   else if (l->fill_style && l->fill_style == WHITE_FILL)
	   {
	       fprintf(stderr,"WHITE_FILL not implemeted for boxes\n");
	   }

	   fprintf(tfp,"\\putrectangle corners at %6.3f %6.3f and %6.3f %6.3f\n",
	   	(llx/ppi)*CONVUNIT, (convy(lly/ppi))*CONVUNIT,
	   	(urx/ppi)*CONVUNIT, (convy(ury/ppi))*CONVUNIT);

	   if (l->fill_style != UNFILLED
		&& l->fill_style != WHITE_FILL && l->fill_style != BLACK_FILL)
	   {
	       fprintf(tfp,"\\setshadegrid span <5pt>\n");
	       fprintf(tfp,"\\shaderectanglesoff\n");
	   }
	}
	else if (l->type == T_ARC_BOX)
	{
	   radius = l->radius;

	   if (l->fill_style != UNFILLED)
	   {
	       fprintf(stderr,"area fill not implemeted for rounded corner boxes\n");
	   }

	   fprintf(tfp,"\\putrule from %6.3f %6.3f to %6.3f %6.3f\n",
		   ((llx+radius)/ppi)*CONVUNIT, (convy(lly/ppi))*CONVUNIT,
		   ((urx-radius)/ppi)*CONVUNIT, (convy(lly/ppi))*CONVUNIT);
	   fprintf(tfp,"\\putrule from %6.3f %6.3f to %6.3f %6.3f\n",
		   (urx/ppi)*CONVUNIT, (convy((lly+radius)/ppi))*CONVUNIT,
		   (urx/ppi)*CONVUNIT, (convy((ury-radius)/ppi))*CONVUNIT);
	   fprintf(tfp,"\\putrule from %6.3f %6.3f to %6.3f %6.3f\n",
		   ((urx-radius)/ppi)*CONVUNIT, (convy(ury/ppi))*CONVUNIT,
		   ((llx+radius)/ppi)*CONVUNIT, (convy(ury/ppi))*CONVUNIT);
	   fprintf(tfp,"\\putrule from %6.3f %6.3f to %6.3f %6.3f\n",
		   (llx/ppi)*CONVUNIT, (convy((ury-radius)/ppi))*CONVUNIT,
		   (llx/ppi)*CONVUNIT, (convy((lly+radius)/ppi))*CONVUNIT);
	   fprintf(tfp,"\\circulararc -90 degrees from %6.3f %6.3f center at %6.3f %6.3f\n",
		   ((urx-radius)/ppi)*CONVUNIT, (convy(lly/ppi))*CONVUNIT,
		   ((urx-radius)/ppi)*CONVUNIT, (convy((lly+radius)/ppi))*CONVUNIT);
	   fprintf(tfp,"\\circulararc -90 degrees from %6.3f %6.3f center at %6.3f %6.3f\n",
		   (urx/ppi)*CONVUNIT, (convy((ury-radius)/ppi))*CONVUNIT,
		   ((urx-radius)/ppi)*CONVUNIT, (convy((ury-radius)/ppi))*CONVUNIT);
	   fprintf(tfp,"\\circulararc -90 degrees from %6.3f %6.3f center at %6.3f %6.3f\n",
		   ((llx+radius)/ppi)*CONVUNIT, (convy(ury/ppi))*CONVUNIT,
		   ((llx+radius)/ppi)*CONVUNIT, (convy((ury-radius)/ppi))*CONVUNIT);
	   fprintf(tfp,"\\circulararc -90 degrees from %6.3f %6.3f center at %6.3f %6.3f\n",
		   (llx/ppi)*CONVUNIT, (convy((lly+radius)/ppi))*CONVUNIT,
		   ((llx+radius)/ppi)*CONVUNIT, (convy((lly+radius)/ppi))*CONVUNIT);
	}
	reset_color(l->pen_color);
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
	    if (line_style == SOLID_LINE) break;
	    fprintf(tfp, "\\setsolid\n");
	    break;

	case DASH_LINE:
	    if (line_style == DASH_LINE && dash_length == dash_len)
		break;
	    fprintf(tfp, "\\setdashes <%7.4f%s>\n", (0.5*dash_len*(cur_thickness+1)/80.0)*CONVUNIT, UNIT);
	    break;

	case DOTTED_LINE:
	    if (line_style == DOTTED_LINE)
		break;
	    fprintf(tfp, "\\setdots <%7.4f%s>\n", (0.5*dash_len*(cur_thickness+1)/80.0)*CONVUNIT, UNIT);
	    break;
	    }

	line_style = style;
	dash_length = dash_len;
    }

/*
 * putline - use rules if possible
 */
static void
putline (start_x, start_y, end_x, end_y, next_x, next_y,
                first_start_x, first_start_y, first_end_x, first_end_y)
int	start_x, start_y, next_x, next_y;
int	first_start_x, first_start_y, first_end_x, first_end_y;
double	end_x, end_y;
{
    if (line_style == SOLID_LINE &&
	    ((start_x == end_x) || (start_y == end_y))) {
	if (next_x != -1) {
	    if (start_x == end_x && end_y == next_y) {
		if (end_y > start_y)
		    end_y += 0.5*ppi*(cur_thickness-1)/72/mag;
		else
		    end_y -= 0.5*ppi*(cur_thickness-1)/72/mag;
		}
	    else if (start_y == end_y && end_x == next_x) {
		if (end_x > start_x)
		    end_x += 0.5*ppi*(cur_thickness-1)/72/mag;
		else
		    end_x -= 0.5*ppi*(cur_thickness-1)/72/mag;
		}
	    }
	else if (end_x == first_start_x && end_y == first_start_y) {
	    if (start_x == end_x && first_start_y == first_end_y) {
		if (end_y > start_y)
		    end_y += 0.5*ppi*(cur_thickness-1)/72/mag;
		else
		    end_y -= 0.5*ppi*(cur_thickness-1)/72/mag;
		}
	    else if (start_y == end_y && first_start_x == first_end_x) {
		if (end_x > start_x)
		    end_x += 0.5*ppi*(cur_thickness-1)/72/mag;
		else
		    end_x -= 0.5*ppi*(cur_thickness-1)/72/mag;
		}
	    }

	fprintf(tfp, "\\putrule from %6.3f %6.3f to %6.3f %6.3f\n",
		(start_x/ppi)*CONVUNIT, (convy(start_y/ppi))*CONVUNIT,
		(end_x/ppi)*CONVUNIT, (convy(end_y/ppi))*CONVUNIT);
	}
    else {
	fprintf(tfp, "\\plot %6.3f %6.3f %6.3f %6.3f /\n",
		(start_x/ppi)*CONVUNIT, (convy(start_y/ppi))*CONVUNIT,
		(end_x/ppi)*CONVUNIT, (convy(end_y/ppi))*CONVUNIT);
	}
}


void
genpictex_spline(s)
F_spline	*s;
{
	/* print any comments */
	print_comments("% ",s->comments, "");

	fprintf(tfp, "%%\n%% Fig SPLINE\n%%\n");

	set_linewidth(s->thickness);
	set_style(s->style, s->style_val);
	set_color(s->pen_color);

	if (int_spline(s))
	    genpictex_itp_spline(s);
	else
	    genpictex_ctl_spline(s);

	if (s->fill_style != UNFILLED)
		fprintf(stderr, "Spline area fill not implemented\n");
	reset_color(s->pen_color);
}

#define MAXBLACKDIAM 15 /* pt */

void
genpictex_ellipse(e)
F_ellipse	*e;
{
	/* print any comments */
	print_comments("% ",e->comments, "");

	fprintf(tfp, "%%\n%% Fig ELLIPSE\n%%\n");

	set_linewidth(e->thickness);
	set_style(e->style, e->style_val);
	set_color(e->pen_color);

	if ((e->fill_style == BLACK_FILL) && (e->radiuses.x == e->radiuses.y)) {
		if (mag*e->radiuses.x > 0.5*ppi/72*MAXBLACKDIAM)
			fprintf(stderr, "Too big black filled circle substituted by a diameter of %dpt\n", MAXBLACKDIAM);
 		fprintf(tfp, "\\put{\\makebox(0,0)[l]{\\circle*{%6.3f}}} at %6.3f %6.3f\n",
		    (2*e->radiuses.x/ppi)*CONVUNIT,
		    ((e->center.x)/ppi)*CONVUNIT, (convy(e->center.y/ppi))*CONVUNIT);

	    }
	else {

		fprintf(tfp, "\\ellipticalarc axes ratio %6.3f:%-6.3f 360 degrees \n",
		    (e->radiuses.x/ppi)*CONVUNIT, (e->radiuses.y/ppi)*CONVUNIT);
		fprintf(tfp, "\tfrom %6.3f %6.3f center at %6.3f %6.3f\n",
		    ((e->center.x+e->radiuses.x)/ppi)*CONVUNIT,
		    (convy(e->center.y/ppi))*CONVUNIT,
		    (e->center.x/ppi)*CONVUNIT, (convy(e->center.y/ppi))*CONVUNIT);
		if (e->fill_style != UNFILLED)
			fprintf(stderr, "Ellipse area fill not implemented\n");
		}
	reset_color(e->pen_color);
	}

#define			HT_OFFSET	(0.2 / 72.0)

void
genpictex_text(t)
F_text	*t;
{
	double	x, y;
	char *tpos;
	unsigned char *cp;

	/* print any comments */
	print_comments("% ",t->comments, "");

        fprintf(tfp, "%%\n%% Fig TEXT object\n%%\n");

	x = t->base_x/ppi;
	y = convy(t->base_y/ppi);

	switch (t->type) {

	    case T_LEFT_JUSTIFIED:
	    case DEFAULT:
	    	tpos = "[lB]";
		break;

	    case T_CENTER_JUSTIFIED:
	    	tpos = "[B]";
		break;

	    case T_RIGHT_JUSTIFIED:
	    	tpos = "[rB]";
		break;

	    default:
		fprintf(stderr, "Text incorrectly positioned\n");
		return;
	    }

	unpsfont(t);
        { int texsize;
          double baselineskip;

	  texsize = TEXFONTMAG(t);
	  baselineskip = (texsize * 1.2);

#ifdef NFSS
 	  fprintf(tfp, "\\put{\\SetFigFontNFSS{%d}{%.1f}{%s}{%s}{%s}",
				 texsize, baselineskip,
				 TEXFAMILY(t->font),TEXSERIES(t->font),TEXSHAPE(t->font));
#else
 	  fprintf(tfp, "\\put{\\SetFigFont{%d}{%.1f}{%s}",
		texsize, baselineskip, TEXFONT(t->font));
#endif
	}

#ifdef DVIPS
	if(t->angle && t->type == T_LEFT_JUSTIFIED)
	  fprintf(tfp, "\\special{ps:gsave currentpoint currentpoint translate\n-%.1f rotate neg exch neg exch translate}", t->angle*180/M_PI);
#endif

	set_color(t->color);

	if (!special_text(t))

		/* this loop escapes characters "$&%#_{}" */
		/* and deleted characters "~^\" */
		for(cp = (unsigned char*)t->cstring; *cp; cp++) {
	      	    if (strchr("$&%#_{}", *cp)) (void)fputc('\\', tfp);
	      	    if (strchr("~^\\", *cp))
			fprintf(stderr,
				"Bad character in text object '%c'\n" ,*cp);
		    else
			(void)fputc(*cp, tfp);
	      	}
	else 
		for(cp = (unsigned char*)t->cstring; *cp; cp++) {
#ifdef I18N
		    extern Boolean support_i18n;
		    if (support_i18n && (t->font <= 2))
		            fputc(*cp, tfp);
		    else
#endif
		    if (*cp >= 0xa0) {
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
			fputc(*cp, tfp);
		}

	reset_color(t->color);

#ifdef DVIPS
	if(t->angle)
	{
	  if (t->type == T_LEFT_JUSTIFIED)
	       fprintf(tfp, "\\special{ps:currentpoint grestore moveto}");
	  else
	     fprintf(stderr, "Rotated Text only for left justified text\n");
	}
#endif
 	fprintf(tfp, "} %s at %6.3f %6.3f\n",
	    tpos, (x)*CONVUNIT, (y)*CONVUNIT);
	}

void
genpictex_arc(a)
F_arc	*a;
{
	double		x, y;
	double		cx, cy, sx, sy, ex, ey;
	double		dx1, dy1, dx2, dy2, r1, r2, th1, th2, theta;

	/* print any comments */
	print_comments("% ",a->comments, "");

        fprintf(tfp, "%%\n%% Fig CIRCULAR ARC object\n%%\n");

	set_linewidth(a->thickness);
	set_style(a->style, a->style_val);
	set_color(a->pen_color);

	cx = a->center.x/ppi; cy = convy(a->center.y/ppi);
	sx = a->point[0].x/ppi; sy = convy(a->point[0].y/ppi);
	ex = a->point[2].x/ppi; ey = convy(a->point[2].y/ppi);

	if ((a->type == T_OPEN_ARC) && (a->thickness != 0) && (a->back_arrow || a->for_arrow)) {
	    if (a->for_arrow) {
		arc_tangent(cx, cy, ex, ey, a->direction, &x, &y);
		draw_arrow_head(x, y, ex, ey,
			a->for_arrow->ht/ppi, a->for_arrow->wid/ppi);
	    }
	    if (a->back_arrow) {
		arc_tangent(cx, cy, sx, sy, !a->direction, &x, &y);
		draw_arrow_head(x, y, sx, sy,
			a->back_arrow->ht/ppi, a->back_arrow->wid/ppi);
	    }
	}

	dx1 = sx - cx;
	dy1 = sy - cy;
	dx2 = ex - cx;
	dy2 = ey - cy;
	    
	rtop(dx1, dy1, &r1, &th1);
	rtop(dx2, dy2, &r2, &th2);
	theta = th2 - th1;
	if (theta > 0) theta -= 2*M_PI;

/*	set_linewidth(a->thickness); */

	if (a->direction)
		fprintf(tfp, "\\circulararc %6.3f degrees from %6.3f %6.3f center at %6.3f %6.3f\n",
			360+(180/M_PI * theta),
			(sx)*CONVUNIT, (sy)*CONVUNIT, (cx)*CONVUNIT, (cy)*CONVUNIT);
	else
		fprintf(tfp, "\\circulararc %6.3f degrees from %6.3f %6.3f center at %6.3f %6.3f\n",
			-180/M_PI * theta,
			(ex)*CONVUNIT, (ey)*CONVUNIT, (cx)*CONVUNIT, (cy)*CONVUNIT);

	if (a->fill_style != UNFILLED)
		fprintf(stderr, "Arc area fill not implemented\n");
	reset_color(a->pen_color);
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
double	x1, y1, x2, y2, arrowht, arrowwid;
{
	double	x, y, xb, yb, dx, dy, l, sina, cosa;
	double	xc, yc, xd, yd;
	int style;
	double dash;

	dx = x2 - x1;  dy = y1 - y2;

	if (!dx && !dy)
	    return ;

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

	/* save line style and set to solid */
	style = line_style;
	dash = dash_length;
	set_style(SOLID_LINE, 0.0);

        fprintf(tfp, "%%\n%% arrow head\n%%\n");

	fprintf(tfp, "\\plot %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f /\n%%\n",
			(xc)*CONVUNIT, (yc)*CONVUNIT,
			(x2)*CONVUNIT, (y2)*CONVUNIT,
			(xd)*CONVUNIT, (yd)*CONVUNIT);

	/* restore line style */
	set_style(style, dash);
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
	    fprintf(tfp, "\t%6.3f %6.3f\n", (xmid)*CONVUNIT, (ymid)*CONVUNIT);

	else {
	    quadratic_spline(x1, y1, ((x1+a2)/2), ((y1+b2)/2),
			((3*a2+a3)/4), ((3*b2+b3)/4), xmid, ymid);
	    }

	if (fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD)
	    fprintf(tfp, "\t%6.3f %6.3f\n", (x4)*CONVUNIT, (y4)*CONVUNIT);

	else {
	    quadratic_spline(xmid, ymid, ((a2+3*a3)/4), ((b2+3*b3)/4),
			((a3+x4)/2), ((b3+y4)/2), x4, y4);
	    }
	}

static void
genpictex_ctl_spline(s)
F_spline	*s;
{
	F_point	*p;
	double	cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
	double	x1, y1, x2, y2;

    	fprintf(tfp, "%%\n%% Fig CONTROL PT SPLINE\n%%\n");

	p = s->points;
	x1 = p->x/ppi;  y1 = convy(p->y/ppi);
	p = p->next;
	x2 = p->x/ppi;  y2 = convy(p->y/ppi);
	cx1 = (x1 + x2) / 2;      cy1 = (y1 + y2) / 2;
	cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;

	if (closed_spline(s)) {
	    fprintf(tfp, "%% closed spline\n%%\n");
	    fprintf(tfp, "\\plot\t%6.3f %6.3f \n ", (cx1)*CONVUNIT, (cy1)*CONVUNIT);
	    }
	else {
	    fprintf(tfp, "%% open spline\n%%\n");
	    if (s->back_arrow)
	        draw_arrow_head(cx1, cy1, x1, y1,
			s->back_arrow->ht/ppi, s->back_arrow->wid/ppi);
	    fprintf(tfp, "\\plot\t%6.3f %6.3f %6.3f %6.3f\n ",
		(x1)*CONVUNIT, (y1)*CONVUNIT, (cx1)*CONVUNIT, (cy1)*CONVUNIT);
	    }

	for (p = p->next; p != NULL; p = p->next) {
	    x1 = x2;  y1 = y2;
	    x2 = p->x/ppi;  y2 = convy(p->y/ppi);
	    cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	    cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    cx1 = cx4;  cy1 = cy4;
	    cx2 = (x1 + 3 * x2) / 4;  cy2 = (y1 + 3 * y2) / 4;
	    }
	x1 = x2;  y1 = y2;
	p = s->points->next;
	x2 = p->x/ppi;  y2 = convy(p->y/ppi);
	cx3 = (3 * x1 + x2) / 4;  cy3 = (3 * y1 + y2) / 4;
	cx4 = (x1 + x2) / 2;      cy4 = (y1 + y2) / 2;
	if (closed_spline(s)) {
	    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	    fprintf(tfp, "\t/\n");
	    }
	else {
	    fprintf(tfp, "\t /\n\\plot %6.3f %6.3f %6.3f %6.3f /\n",
		(cx1)*CONVUNIT, (cy1)*CONVUNIT, (x1)*CONVUNIT, (y1)*CONVUNIT);
	    if (s->for_arrow)
	    	draw_arrow_head(cx1, cy1, x1, y1,
			s->for_arrow->ht/ppi, s->for_arrow->wid/ppi);
	    }

	}

static void
genpictex_itp_spline(s)
F_spline	*s;
{
	F_point		*p1, *p2;
	F_control	*cp1, *cp2;
	double		x1, x2, y1, y2;

    	fprintf(tfp, "%%\n%% Fig INTERPOLATED PT SPLINE\n%%\n");

	p1 = s->points;
	cp1 = s->controls;
	x2 = p1->x/ppi; y2 = convy(p1->y/ppi);

	if (s->back_arrow)
	    draw_arrow_head(cp1->rx/ppi, convy(cp1->ry/ppi), x2, y2,
		s->back_arrow->ht/ppi, s->back_arrow->wid/ppi);

	fprintf(tfp, "\\plot %6.3f %6.3f ", (x2)*CONVUNIT, (y2)*CONVUNIT);
	for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
		p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {
	    x1 = x2; y1 = y2;
	    x2 = p2->x/ppi; y2 = convy(p2->y/ppi);
	    bezier_spline(x1, y1, (double)cp1->rx/ppi, convy(cp1->ry/ppi),
		(double)cp2->lx/ppi, convy(cp2->ly/ppi), x2, y2);
	    }
	fprintf(tfp, "\t/\n");

	if (s->for_arrow)
	    draw_arrow_head(cp1->lx/ppi, convy(cp1->ly/ppi), x2, y2,
		s->for_arrow->ht/ppi, s->for_arrow->wid/ppi);
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
	    fprintf(tfp, "\t%6.3f %6.3f\n", (x3)*CONVUNIT, (y3)*CONVUNIT);

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

struct driver dev_pictex = {
     	genpictex_option,
	genpictex_start,
	gendev_null,
	genpictex_arc,
	genpictex_ellipse,
	genpictex_line,
	genpictex_spline,
	genpictex_text,
	genpictex_end,
	EXCLUDE_TEXT
};

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

#include "fig2dev.h"
#include "object.h"
#include "free.h"

void
free_arc(list)
F_arc	**list;
{
	F_arc	*a, *arc;

	for (a = *list; a != NULL;) {
	    arc = a;
	    a = a->next;
	    if (arc->for_arrow) free((char*)arc->for_arrow);
	    if (arc->back_arrow) free((char*)arc->back_arrow);
	    free((char*)arc);
	    } 
	*list = NULL;
	}

void
free_compound(list)
F_compound	**list;
{
	F_compound	*c, *compound;

	for (c = *list; c != NULL;) {
	    compound = c;
	    c = c->next;
	    free_arc(&compound->arcs);
	    free_compound(&compound->compounds);
	    free_ellipse(&compound->ellipses);
	    free_line(&compound->lines);
	    free_spline(&compound->splines);
	    free_text(&compound->texts);
	    free((char*)compound);
	    } 
	*list = NULL;
	}

void
free_ellipse(list)
F_ellipse	**list;
{
	F_ellipse	*e, *ellipse;

	for (e = *list; e != NULL;) {
	    ellipse = e;
	    e = e->next;
	    free((char*)ellipse);
	    } 
	*list = NULL;
	}

void
free_line(list)
F_line	**list;
{
	F_line	*l, *line;

	for (l = *list; l != NULL;) {
	    line = l;
	    l = l->next;
	    free_linestorage(line);
	    } 
	*list = NULL;
	}

void
free_text(list)
F_text	**list;
{
	F_text	*t, *text;

	for (t = *list; t != NULL;) {
	    text = t;
	    t = t->next;
	    free(text->cstring);
	    free((char*)text);
	    } 
	*list = NULL;
	}

void
free_spline(list)
F_spline	**list;
{
	F_spline	*s, *spline;

	for (s = *list; s != NULL;) {
	    spline = s;
	    s = s->next;
	    free_splinestorage(spline);
	    }
	*list = NULL;
	}

void
free_splinestorage(s)
F_spline      *s;
{
        F_point		*p, *q;
        F_control	*a, *b;

        for (p = s->points; p != NULL; p = q) {
            q = p->next;
            free((char*)p);
            }
        for (a = s->controls; a != NULL; a = b) {
            b = a->next;
            free((char*)a);
            }
	if (s->for_arrow) free((char*)s->for_arrow);
	if (s->back_arrow) free((char*)s->back_arrow);
        free((char*)s);
        }

void
free_linestorage(l)
F_line	*l;
{
	F_point	*p, *q;

	for (p = l->points; p != NULL; p = q) {
	    q = p->next;
	    free((char*)p);
	    }
	if (l->for_arrow) free((char*)l->for_arrow);
	if (l->back_arrow) free((char*)l->back_arrow);
	free((char*)l);
	}

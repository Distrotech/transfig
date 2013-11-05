/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
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
 */

#include "alloc.h"
#include "fig2dev.h"
#include "object.h"

static double		forward_arrow_wid = 4;
static double		forward_arrow_ht = 8;
static int		forward_arrow_type = 0;
static int		forward_arrow_style = 0;
static double		forward_arrow_thickness = 1;

static double		backward_arrow_wid = 4;
static double		backward_arrow_ht = 8;
static int		backward_arrow_type = 0;
static int		backward_arrow_style = 0;
static double		backward_arrow_thickness = 1;

F_arrow *
forward_arrow()
{
	F_arrow		*a;

	if (NULL == (Arrow_malloc(a))) {
	    put_msg(Err_mem);
	    return(NULL);
	    }
	a->type = forward_arrow_type;
	a->style = forward_arrow_style;
	a->thickness = forward_arrow_thickness*THICK_SCALE;
	a->wid = forward_arrow_wid;
	a->ht = forward_arrow_ht;
	return(a);
	}

F_arrow *
backward_arrow()
{
	F_arrow		*a;

	if (NULL == (Arrow_malloc(a))) {
	    put_msg(Err_mem);
	    return(NULL);
	    }
	a->type = backward_arrow_type;
	a->style = backward_arrow_style;
	a->thickness = backward_arrow_thickness*THICK_SCALE;
	a->wid = backward_arrow_wid;
	a->ht = backward_arrow_ht;
	return(a);
	}

F_arrow *
make_arrow(type, style, thickness, wid, ht)
int	type, style;
double	thickness, wid, ht;
{
	F_arrow		*a;

	if (NULL == (Arrow_malloc(a))) {
	    put_msg(Err_mem);
	    return(NULL);
	    }
	a->type = type;
	a->style = style;
	a->thickness = thickness*THICK_SCALE;
	a->wid = wid;
	a->ht = ht;
	return(a);
	}

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

void
arc_tangent(x1, y1, x2, y2, direction, x, y)
double	x1, y1, x2, y2, *x, *y;
int	direction;
{
	if (direction) { /* counter clockwise  */
	    *x = x2 - (y2 - y1);
	    *y = y2 + (x2 - x1);
	    }
	else {
	    *x = x2 + (y2 - y1);
	    *y = y2 - (x2 - x1);
	    }
	}

void
arc_tangent_int(x1, y1, x2, y2, direction, x, y)
double	x1, y1, x2, y2;
int	*x, *y;
int	direction;
{
	if (direction) { /* counter clockwise  */
	    *x = x2 - (y2 - y1);
	    *y = y2 + (x2 - x1);
	    }
	else {
	    *x = x2 + (y2 - y1);
	    *y = y2 - (x2 - x1);
	    }
	}


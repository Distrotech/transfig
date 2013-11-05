void
calc_arrow(x1, y1, x2, y2, linethick, arrow, 
			points, npoints, fillpoints, nfillpoints, clippts, nclippts)
    int		    x1, y1, x2, y2;
    int		    linethick;
    F_arrow	   *arrow;
    Point	    points[], fillpoints[], clippts[];
    int		   *npoints, *nfillpoints, *nclippts;
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    mx, my;
    double	    ddx, ddy, lpt, tipmv;
    double	    alpha;
    double	    miny, maxy;
    int		    xa, ya, xs, ys;
    double	    wd  = (double) arrow->wid;
    double	    len = (double) arrow->ht;
    double	    th  = arrow->thickness;
    double	    radius;
    double	    angle, init_angle, rads;
    double	    fix_x, fix_y;
    int		    type, style, indx, tip;
    int		    i, np;
    int		    offset, halfthick;

    /* to enlarge the clip area in case the line is thick */
    halfthick = linethick / 2 + 1;

    /* types = 0...10 */
    type = arrow->type;
    /* style = 0 (unfilled) or 1 (filled) */
    style = arrow->style;
    /* index into shape array */
    indx = 2*type + style;

    *npoints = *nfillpoints = 0;
    dx = x2 - x1;
    dy = y1 - y2;
    if (dx==0 && dy==0)
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */
    tipmv = arrow_shapes[indx].tipmv;
    lpt = 0.0;
    if (tipmv > 0.0)
	lpt = th / (2.0 * sin(atan(wd / (tipmv * len))));
    else if (tipmv == 0.0)
	lpt = th / 2.0;	/* types which have blunt end */
			/* (Don't adjust those with tipmv < 0) */

    /* alpha is the angle the line is relative to horizontal */
    alpha = atan2(dy,-dx);

    /* ddx, ddy is amount to move end of line back so that arrowhead point
       ends where line used to */
    ddx = lpt * cos(alpha);
    ddy = lpt * sin(alpha);

    /* move endpoint of line back */
    mx = x2 + ddx;
    my = y2 + ddy;

    l = sqrt(dx * dx + dy * dy);
    sina = dy / l;
    cosa = dx / l;
    xb = mx * cosa - my * sina;
    yb = mx * sina + my * cosa;

    /* (xa,ya) is the rotated endpoint (used in ROTX and ROTY macros) */
    xa =  xb * cosa + yb * sina + 0.5;
    ya = -xb * sina + yb * cosa + 0.5;

    miny =  100000.0;
    maxy = -100000.0;

    if (type == 5 || type == 6) {
	/*
	 * CIRCLE and HALF-CIRCLE arrowheads
	 *
	 * We approximate circles with (40+zoom)/4 points
	 */

	/* use original dx, dy to get starting angle */
	init_angle = compute_angle(dx, dy);

	/* (xs,ys) is a point the length of the arrowhead BACK from
	   the end of the shaft */
	/* for the half circle, use 0.0 */
	xs =  (xb-(type==5? len: 0.0)) * cosa + yb * sina + 0.5;
	ys = -(xb-(type==5? len: 0.0)) * sina + yb * cosa + 0.5;

	/* calc new (dx, dy) from moved endpoint to (xs, ys) */
	dx = mx - xs;
	dy = my - ys;
	/* radius */
	radius = len/2.0;
	fix_x = xs + (dx / (double) 2.0);
	fix_y = ys + (dy / (double) 2.0);
	/* choose number of points for circle - 40+mag/4 points */
	np = round(mag/4.0) + 40;

	if (type == 5) {
	    /* full circle */
	    init_angle = 5.0*M_PI_2 - init_angle;
	    rads = M_2PI;
	} else {
	    /* half circle */
	    init_angle = 3.0*M_PI_2 - init_angle;
	    rads = M_PI;
	}

	/* draw the half or full circle */
	for (i = 0; i < np; i++) {
	    angle = init_angle - (rads * (double) i / (double) (np-1));
	    x = fix_x + round(radius * cos(angle));
	    points[*npoints].x = x;
	    y = fix_y + round(radius * sin(angle));
	    points[*npoints].y = y;
	    (*npoints)++;
	}

	/* set clipping to a box at least as large as the line thickness
	   or diameter of the circle, whichever is larger */
	/* 4 points in clip box */
	miny = MIN(-halfthick, -radius-th/2.0);
	maxy = MAX( halfthick,  radius+th/2.0);

	i=0;
	/* start at new endpoint of line */
	clippts[i].x = ROTXC(0,            -radius-th/2.0);
	clippts[i].y = ROTYC(0,            -radius-th/2.0);
	i++;
	clippts[i].x = ROTXC(0,             miny);
	clippts[i].y = ROTYC(0,             miny);
	i++;
	clippts[i].x = ROTXC(radius+th/2.0, miny);
	clippts[i].y = ROTYC(radius+th/2.0, miny);
	i++;
	clippts[i].x = ROTXC(radius+th/2.0, maxy);
	clippts[i].y = ROTYC(radius+th/2.0, maxy);
	i++;
	clippts[i].x = ROTXC(0,             maxy);
	clippts[i].y = ROTYC(0,             maxy);
	i++;
	*nclippts = i;

    } else {
	/*
	 * ALL OTHER HEADS
	 */

	*npoints = arrow_shapes[indx].numpts;
	/* we'll shift the half arrowheads down by the difference of the main line thickness 
	   and the arrowhead thickness to make it flush with the main line */
	if (arrow_shapes[indx].half)
	    offset = (linethick - arrow->thickness)/2;
	else
	    offset = 0;

	/* fill the points array with the outline */
	for (i=0; i<*npoints; i++) {
	    x = arrow_shapes[indx].points[i].x * len;
	    y = arrow_shapes[indx].points[i].y * wd - offset;
	    miny = MIN(y, miny);
	    maxy = MAX(y, maxy);
	    points[i].x = ROTX(x,y);
	    points[i].y = ROTY(x,y);
	}

	/* and the fill points array if there are fill points different from the outline */
	*nfillpoints = arrow_shapes[indx].numfillpts;
	for (i=0; i<*nfillpoints; i++) {
	    x = arrow_shapes[indx].fillpoints[i].x * len;
	    y = arrow_shapes[indx].fillpoints[i].y * wd - offset;
	    miny = MIN(y, miny);
	    maxy = MAX(y, maxy);
	    fillpoints[i].x = ROTX(x,y);
	    fillpoints[i].y = ROTY(x,y);
	}

	/* to include thick lines in clip area */
	miny = MIN(miny, -halfthick);
	maxy = MAX(maxy, halfthick);

	/* set clipping to the first three points of the arrowhead and
	   the (enlarged) box surrounding it */
	*nclippts = 0;
	if (arrow_shapes[indx].clip) {
		for (i=0; i < 3; i++) {
		    x = arrow_shapes[indx].points[i].x * len;
		    y = arrow_shapes[indx].points[i].y * wd - offset;
		    clippts[i].x = ROTX(x,y);
		    clippts[i].y = ROTY(x,y);
		}

		/* locate the tip of the head */
		tip = arrow_shapes[indx].tipno;

		/* now make the box around it at least as large as the line thickness */
		/* start with last x, lower y */

		clippts[i].x = ROTX(x,miny);
		clippts[i].y = ROTY(x,miny);
		i++;
		/* x tip, same y (note different offset in ROTX/Y2 rotation) */
		clippts[i].x = ROTX2(arrow_shapes[indx].points[tip].x*len + THICK_SCALE, miny);
		clippts[i].y = ROTY2(arrow_shapes[indx].points[tip].x*len + THICK_SCALE, miny);
		i++;
		/* x tip, upper y (note different offset in ROTX/Y2 rotation) */
		clippts[i].x = ROTX2(arrow_shapes[indx].points[tip].x*len + THICK_SCALE, maxy);
		clippts[i].y = ROTY2(arrow_shapes[indx].points[tip].x*len + THICK_SCALE, maxy);
		i++;
		/* first x of arrowhead, upper y */
		clippts[i].x = ROTX(arrow_shapes[indx].points[0].x*len, maxy);
		clippts[i].y = ROTY(arrow_shapes[indx].points[0].x*len, maxy);
		i++;
	}
	/* set the number of points in the clip or bounds */
	*nclippts = i;
    }
}

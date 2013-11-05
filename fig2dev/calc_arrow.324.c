calc_arrow(x1, y1, x2, y2, c1x, c1y, c2x, c2y, arrow, points, npoints, nboundpts)
    int		    x1, y1, x2, y2;
    int		   *c1x, *c1y, *c2x, *c2y;
    F_arrow	   *arrow;
    Point	    points[];
    int		   *npoints, *nboundpts;
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    mx, my;
    double	    ddx, ddy, lpt, tipmv;
    double	    alpha;
    double	    miny, maxy;
    double	    thick;
    int		    xa, ya, xs, ys;
    double	    xt, yt;
    float	    wd = arrow->wid;
    float	    ht = arrow->ht;
    int		    type, style, indx;
    int		    i, np;

    /* types = 0...10 */
    type = arrow->type;
    /* style = 0 (unfilled) or 1 (filled) */
    style = arrow->style;
    /* index into shape array */
    indx = 2*type + style;

    *npoints = 0;
    *nboundpts = 0;
    dx = x2 - x1;
    dy = y1 - y2;
    if (dx==0 && dy==0)
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */

    tipmv = arrow_shapes[indx].tipmv;
    lpt = 0.0;
    /* lines are made a little thinner in set_linewidth */
    thick = (arrow->thickness <= THICK_SCALE) ? 	
		0.5* arrow->thickness :
		arrow->thickness - THICK_SCALE;
    if (tipmv > 0.0)
        lpt = thick / (2.0 * sin(atan(wd / (tipmv * ht))));
    else if (tipmv == 0.0)
	lpt = thick / 3.0;	 /* types which have blunt end */
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

    /* (xa,ya) is the rotated endpoint */
    xa =  xb * cosa + yb * sina + 0.5;
    ya = -xb * sina + yb * cosa + 0.5;

    /*
     * We approximate circles with an octagon since, at small sizes,
     * this is sufficient.  I haven't bothered to alter the bounding
     * box calculations.
     */
    miny =  10000000.0;
    maxy = -10000000.0;
    if (type == 5 || type == 6) {	/* also include half circle */
	double rmag;
	double angle, init_angle, rads;
	double fix_x, fix_y;

	/* get angle of line */
	init_angle = compute_angle(dx,dy);

	/* (xs,ys) is a point the length (height) of the arrowhead BACK from 
	   the end of the shaft */
	/* for the half circle, use 0.0 */
	xs =  (xb-(type==5? ht: 0.0)) * cosa + yb * sina + 0.5;
	ys = -(xb-(type==5? ht: 0.0)) * sina + yb * cosa + 0.5;

	/* calc new (dx, dy) from moved endpoint to (xs, ys) */
	dx = mx - xs;
	dy = my - ys;
	/* radius */
	rmag = ht/2.0;
	fix_x = xs + (dx / (double) 2.0);
	fix_y = ys + (dy / (double) 2.0);
	/* choose number of points for circle - 20+mag/4 points */
	np = round(mag/4.0) + 20;
	/* full or half circle? */
	rads = (type==5? M_2PI: M_PI);

	if (type == 5) {
	    init_angle = 5.0*M_PI_2 - init_angle;
	    /* np/2 points in the forward part of the circle for the line clip area */
	    *nboundpts = np/2;
	    /* full circle */
	    rads = M_2PI;
	} else {
	    init_angle = 3.0*M_PI_2 - init_angle;
	    /* no points in the line clip area */
	    *nboundpts = 0;
	    /* half circle */
	    rads = M_PI;
	}
	/* draw the half or full circle */
	for (i = 0; i < np; i++) {
	    if (type == 5)
		angle = init_angle - (rads * (double) i / (double) np);
	    else
		angle = init_angle - (rads * (double) i / (double) (np-1));
	    x = fix_x + round(rmag * cos(angle));
	    points[*npoints].x = x;
	    y = fix_y + round(rmag * sin(angle));
	    points[*npoints].y = y;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    (*npoints)++;
	}
	x = 2.0*THICK_SCALE;
	y = rmag;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c1x = xt;
	*c1y = yt;
	y = -rmag;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c2x = xt;
	*c2y = yt;
    } else {
	/* 3 points in the arrowhead that define the line clip part */
	*nboundpts = 3;
	np = arrow_shapes[indx].numpts;
	for (i=0; i<np; i++) {
	    x = arrow_shapes[indx].points[i].x * ht;
	    y = arrow_shapes[indx].points[i].y * wd;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    xt =  x*cosa + y*sina + xa;
	    yt = -x*sina + y*cosa + ya;
	    points[*npoints].x = xt;
	    points[*npoints].y = yt;
	    (*npoints)++;
	}
	x = arrow_shapes[indx].points[arrow_shapes[indx].tipno].x * ht + THICK_SCALE;
	y = maxy;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c1x = xt;
	*c1y = yt;
	y = miny;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c2x = xt;
	*c2y = yt;
    }
}

/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1995 C. Blanc and C. Schlick
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
#include "alloc.h"
#include "object.h"
#include "free.h"
#include "trans_spline.h"


struct zXPoint
{
  int x,y;
}; 

typedef struct zXPoint zXPoint;



/* declarations for splines */

#define MIN_NUMPOINTS_FOR_QUICK_REDRAW     5
#define MAX_SPLINE_STEP                    0.2

#define COPY_CONTROL_POINT(P0, S0, P1, S1) \
      P0 = P1; \
      S0 = S1

#define NEXT_CONTROL_POINTS(P0, S0, P1, S1, P2, S2, P3, S3) \
      COPY_CONTROL_POINT(P0, S0, P1, S1); \
      COPY_CONTROL_POINT(P1, S1, P2, S2); \
      COPY_CONTROL_POINT(P2, S2, P3, S3); \
      COPY_CONTROL_POINT(P3, S3, P3->next, S3->next)

#define INIT_CONTROL_POINTS(SPLINE, P0, S0, P1, S1, P2, S2, P3, S3) \
      COPY_CONTROL_POINT(P0, S0, SPLINE->points, SPLINE->controls); \
      COPY_CONTROL_POINT(P1, S1, P0->next, S0->next);               \
      COPY_CONTROL_POINT(P2, S2, P1->next, S1->next);               \
      COPY_CONTROL_POINT(P3, S3, P2->next, S2->next)

#define SPLINE_SEGMENT_LOOP(K, P0, P1, P2, P3, S1, S2, PREC) \
      step = step_computing(K, P0, P1, P2, P3, S1, S2, PREC);    \
      spline_segment_computing(step, K, P0, P1, P2, P3, S1, S2)



static void          spline_segment_computing();
static float         step_computing();
static INLINE void   point_adding();
static INLINE void   point_computing();
static INLINE void   negative_s1_influence();
static INLINE void   negative_s2_influence();
static INLINE void   positive_s1_influence();
static INLINE void   positive_s2_influence();
static INLINE double f_blend();
static INLINE double g_blend();
static INLINE double h_blend();
static void          free_point_array();
static int           num_points();
static void          too_many_points();
static F_line	     *create_line();
static F_point	     *create_point();
static F_control     *create_cpoint();

/************** CURVE DRAWING FACILITIES ****************/

static int	npoints;
static zXPoint *points;
static int	max_points;
static int	allocstep;


static void
free_point_array(pts)
     zXPoint *pts;
{
  free(pts);
}



static		Boolean
init_point_array(init_size, step_size)
    int		    init_size, step_size;
{
    npoints = 0;
    max_points = init_size;
    allocstep = step_size;
    if (max_points > MAXNUMPTS) {
	max_points = MAXNUMPTS;
    }
    if ((points = (zXPoint *) malloc(max_points * sizeof(zXPoint))) == 0) {
	fprintf(stderr, "xfig: insufficient memory to allocate point array\n");
	return False;
    }
    return True;
}



static void
too_many_points()
{
  fprintf(stderr,
	  "Too many points, recompile with MAXNUMPTS > %d in trans_spline.h\n",
	  MAXNUMPTS);
}


static		Boolean
add_point(x, y)
    int		    x, y;
{
    if (npoints >= max_points) {
	zXPoint	       *tmp_p;

	if (max_points >= MAXNUMPTS) {
	    max_points = MAXNUMPTS;
	    return False;		/* stop; it is not closing */
	}
	max_points += allocstep;
	if (max_points >= MAXNUMPTS)
	    max_points = MAXNUMPTS;

	if ((tmp_p = (zXPoint *) realloc(points,
					max_points * sizeof(zXPoint))) == 0) {
	    fprintf(stderr,
		    "xfig: insufficient memory to reallocate point array\n");
	    return False;
	}
	points = tmp_p;
    }
    /* ignore identical points */
    if (npoints > 0 &&
	points[npoints-1].x == x && points[npoints-1].y == y)
		return True;
    points[npoints].x = x;
    points[npoints].y = y;
    npoints++;
    return True;
}




/********************* CURVES FOR SPLINES *****************************

 The following spline drawing routines are from

    "X-splines : A Spline Model Designed for the End User"

    by Carole BLANC and Christophe SCHLICK, Proceedings of SIGGRAPH'95

***********************************************************************/


zXPoint *
compute_open_spline(spline, precision)
     F_spline	   *spline;
     float         precision;
{
  int       k;
  float     step;
  F_point   *p0, *p1, *p2, *p3;
  F_control *s0, *s1, *s2, *s3;

  if (!init_point_array(300, 200))
      return NULL;

  p1 = spline->points;
  for (k=0; p1->next; k++) {
      p0 = p1;
      p1 = p1->next;
  }
  /* special case - two point spline is just straight line */
  if (k==1) {
      if (!add_point(p0->x,p0->y) ||
	  !add_point(p1->x,p1->y))
		too_many_points();
      return points;
  }

  COPY_CONTROL_POINT(p0, s0, spline->points, spline->controls);
  COPY_CONTROL_POINT(p1, s1, p0, s0);
  /* first control point is needed twice for the first segment */
  COPY_CONTROL_POINT(p2, s2, p1->next, s1->next);
  if (p2->next == NULL) {
      COPY_CONTROL_POINT(p3, s3, p2, s2);
  } else {
      COPY_CONTROL_POINT(p3, s3, p2->next, s2->next);
  }

  for (k = 0 ;  ; k++) {
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
      if (p3->next == NULL)
	break;
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
  }
  /* last control point is needed twice for the last segment */
  COPY_CONTROL_POINT(p0, s0, p1, s1);
  COPY_CONTROL_POINT(p1, s1, p2, s2);
  COPY_CONTROL_POINT(p2, s2, p3, s3);
  SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
  
  if (!add_point(p3->x, p3->y))
    too_many_points();
  
  return points;
}


zXPoint *
compute_closed_spline(spline, precision)
     F_spline	   *spline;
     float         precision;
{
  int k, i;
  float     step;
  F_point   *p0, *p1, *p2, *p3, *first;
  F_control *s0, *s1, *s2, *s3, *s_first;

  if (!init_point_array(300, 200))
      return NULL;

  INIT_CONTROL_POINTS(spline, p0, s0, p1, s1, p2, s2, p3, s3);
  COPY_CONTROL_POINT(first, s_first, p0, s0); 

  for (k = 0 ; p3 != NULL ; k++) {
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
  }
  /* when we are at the end, join to the beginning */
  COPY_CONTROL_POINT(p3, s3, first, s_first);
  SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);

  for (i = 0; i < 2; i++) {
      k++;
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
  }

  if (!add_point(points[0].x,points[0].y))
    too_many_points();

  return points;
}



#define Q(s)  (-(s))
#define EQN_NUMERATOR(dim) \
  (A_blend[0]*p0->dim+A_blend[1]*p1->dim+A_blend[2]*p2->dim+A_blend[3]*p3->dim)

static INLINE double
f_blend(numerator, denominator)
     double numerator, denominator;
{
  double p = 2 * denominator * denominator;
  double u = numerator / denominator;
  double u2 = u * u;

  return (u * u2 * (10 - p + (2*p - 15)*u + (6 - p)*u2));
}

static INLINE double
g_blend(u, q)             /* p equals 2 */
     double u, q;
{
  return(u*(q + u*(2*q + u*(8 - 12*q + u*(14*q - 11 + u*(4 - 5*q))))));
}

static INLINE double
h_blend(u, q)
     double u, q;
{
  double u2 = u*u;
   return (u * (q + u * (2 * q + u2 * (-2*q - u*q))));
}

static INLINE void
negative_s1_influence(t, s1, A0, A2)
     double       t, s1, *A0 ,*A2;
{
  *A0 = h_blend(-t, Q(s1));
  *A2 = g_blend(t, Q(s1));
}

static INLINE void
negative_s2_influence(t, s2, A1, A3)
     double       t, s2, *A1 ,*A3;
{
  *A1 = g_blend(1-t, Q(s2));
  *A3 = h_blend(t-1, Q(s2));
}

static INLINE void
positive_s1_influence(k, t, s1, A0, A2)
     int          k;
     double       t, s1, *A0 ,*A2;
{
  double Tk;
  
  Tk = k+1+s1;
  *A0 = (t+k+1<Tk) ? f_blend(t+k+1-Tk, k-Tk) : 0.0;
  
  Tk = k+1-s1;
  *A2 = f_blend(t+k+1-Tk, k+2-Tk);
}

static INLINE void
positive_s2_influence(k, t, s2, A1, A3)
     int          k;
     double       t, s2, *A1 ,*A3;
{
  double Tk;

  Tk = k+2+s2; 
  *A1 = f_blend(t+k+1-Tk, k+1-Tk);
  
  Tk = k+2-s2;
  *A3 = (t+k+1>Tk) ? f_blend(t+k+1-Tk, k+3-Tk) : 0.0;
}

static INLINE void
point_adding(A_blend, p0, p1, p2, p3)
     F_point     *p0, *p1, *p2, *p3;
     double      *A_blend;
{
  double weights_sum;

  weights_sum = A_blend[0] + A_blend[1] + A_blend[2] + A_blend[3];
  if (!add_point(round(EQN_NUMERATOR(x) / (weights_sum)),
		 round(EQN_NUMERATOR(y) / (weights_sum))))
      too_many_points();
}

static INLINE void
point_computing(A_blend, p0, p1, p2, p3, x, y)
     F_point     *p0, *p1, *p2, *p3;
     double      *A_blend;
     int         *x, *y;
{
  double weights_sum;

  weights_sum = A_blend[0] + A_blend[1] + A_blend[2] + A_blend[3];

  *x = round(EQN_NUMERATOR(x) / (weights_sum));
  *y = round(EQN_NUMERATOR(y) / (weights_sum));
}

static float
step_computing(k, p0, p1, p2, p3, s1, s2, precision)
     int     k;
     F_point *p0, *p1, *p2, *p3;
     double  s1, s2;
     float   precision;
{
  double A_blend[4];
  int    xstart, ystart, xend, yend, xmid, ymid, xlength, ylength;
  int    start_to_end_dist, number_of_steps;
  float  step, angle_cos, scal_prod, xv1, xv2, yv1, yv2, sides_length_prod;
  
  /* This function computes the step used to draw the segment (p1, p2)
     (xv1, yv1) : coordinates of the vector from middle to origin
     (xv2, yv2) : coordinates of the vector from middle to extremity */

  /* compute coordinates of the origin */
  if (s1>0) {
      if (s2<0) {
	  positive_s1_influence(k, 0.0, s1, &A_blend[0], &A_blend[2]);
	  negative_s2_influence(0.0, s2, &A_blend[1], &A_blend[3]); 
      } else {
	  positive_s1_influence(k, 0.0, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 0.0, s2, &A_blend[1], &A_blend[3]); 
      }
      point_computing(A_blend, p0, p1, p2, p3, &xstart, &ystart);
  } else {
      xstart = p1->x;
      ystart = p1->y;
  }
  
  /* compute coordinates  of the extremity */
  if (s2>0) {
      if (s1<0) {
	  negative_s1_influence(1.0, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 1.0, s2, &A_blend[1], &A_blend[3]);
      } else {
	  positive_s1_influence(k, 1.0, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 1.0, s2, &A_blend[1], &A_blend[3]); 
      }
      point_computing(A_blend, p0, p1, p2, p3, &xend, &yend);
  } else {
      xend = p2->x;
      yend = p2->y;
  }

  /* compute coordinates  of the middle */
  if (s2>0) {
      if (s1<0) {
	  negative_s1_influence(0.5, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 0.5, s2, &A_blend[1], &A_blend[3]);
      } else {
	  positive_s1_influence(k, 0.5, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 0.5, s2, &A_blend[1], &A_blend[3]); 
      }
  } else if (s1<0) {
      negative_s1_influence(0.5, s1, &A_blend[0], &A_blend[2]);
      negative_s2_influence(0.5, s2, &A_blend[1], &A_blend[3]);
  } else {
      positive_s1_influence(k, 0.5, s1, &A_blend[0], &A_blend[2]);
      negative_s2_influence(0.5, s2, &A_blend[1], &A_blend[3]);
  }
  point_computing(A_blend, p0, p1, p2, p3, &xmid, &ymid);

  xv1 = xstart - xmid;
  yv1 = ystart - ymid;
  xv2 = xend - xmid;
  yv2 = yend - ymid;

  scal_prod = xv1*xv2 + yv1*yv2;
  
  sides_length_prod = sqrt((xv1*xv1 + yv1*yv1)*(xv2*xv2 + yv2*yv2));

  /* compute cosinus of origin-middle-extremity angle, which approximates the
     curve of the spline segment */

/* IG: Wed Oct  1 13:16:46 BST 1997: Fix floating-point exception bug */

   if (sides_length_prod == 0.0)
     angle_cos = 0.0;
   else
     angle_cos = scal_prod/sides_length_prod; 

  xlength = xend - xstart;
  ylength = yend - ystart;

  start_to_end_dist = (int)sqrt((double)xlength*(double)xlength + (double)ylength*(double)ylength);

  /* more steps if segment's origin and extremity are remote */
  number_of_steps = (int)sqrt((double)start_to_end_dist)/2;

  /* more steps if the curve is high */
  number_of_steps += (int)((1.0 + angle_cos)*10.0);

  if (number_of_steps == 0 || number_of_steps > 999)
    step = 1.0;
  else
    step = precision/number_of_steps;
  
  if ((step > MAX_SPLINE_STEP) || (step == 0))
    step = MAX_SPLINE_STEP;
  return step;
}

static void
spline_segment_computing(step, k, p0, p1, p2, p3, s1, s2)
     float   step;
     F_point *p0, *p1, *p2, *p3;
     int     k;
     double  s1, s2;
{
  double A_blend[4];
  double t;
  
  if (s1<0) {  
     if (s2<0) {
	 for (t=0.0 ; t<1 ; t+=step) {
	     negative_s1_influence(t, s1, &A_blend[0], &A_blend[2]);
	     negative_s2_influence(t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
	 }
     } else {
	 for (t = 0.0 ; t<1 ; t+=step) {
	     negative_s1_influence(t, s1, &A_blend[0], &A_blend[2]);
	     positive_s2_influence(k, t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
	 }
     }
  } else if (s2<0) {
      for (t = 0.0 ; t<1 ; t+=step) {
	     positive_s1_influence(k, t, s1, &A_blend[0], &A_blend[2]);
	     negative_s2_influence(t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
	   }
  } else {
      for (t = 0.0 ; t<1 ; t+=step) {
	     positive_s1_influence(k, t, s1, &A_blend[0], &A_blend[2]);
	     positive_s2_influence(k, t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
      } 
  }
}


F_line *
create_line_with_spline(s)
    F_spline	   *s;
{
  zXPoint  *points;
  F_line   *line;
  int      i = 0;
  int      start = 0;
  F_point  *ptr, *pt;
  F_comment *lcomm, *scomm;
  
  points = open_spline(s) ? compute_open_spline(s, HIGH_PRECISION)
                          : compute_closed_spline(s, HIGH_PRECISION);
  if (points==NULL)  
    return NULL;


  if ((line = create_line()) == NULL) {
    free_point_array(points);
    return NULL;
  }
  line->style      = s->style;  
  line->thickness  = s->thickness;
  line->pen_color  = s->pen_color;
  line->depth      = s->depth;
  line->pen        = s->pen;
  line->fill_color = s->fill_color;
  line->fill_style = s->fill_style;
  line->style_val  = s->style_val;
  line->join_style = 2;			/* prevents spikes in tight splines */
  line->cap_style  = s->cap_style;
  line->for_arrow  = s->for_arrow;
  line->back_arrow = s->back_arrow;
  /* copy the comments */
  if (s->comments) {
    scomm = s->comments;
    line->comments = lcomm = (F_comment *) malloc(COMMENT_SIZE);
    while (scomm) {
	lcomm->comment = malloc(strlen(scomm->comment)+1);
	strcpy(lcomm->comment, scomm->comment);
	if (scomm->next)
	    lcomm->next = (F_comment *) malloc(COMMENT_SIZE);
	else
	    lcomm->next = NULL;
	scomm = scomm->next;
	lcomm = lcomm->next;
    }
  }
  
  if (s->for_arrow) {
    s->for_arrow = NULL;
    if (npoints > ARROW_START) {
	points[npoints - ARROW_START] = points[npoints - 1];
	npoints -= (ARROW_START-1);          /* avoid some points to have good 
					    orientation for arrow */
    }
  }
  if (s->back_arrow) {
    s->back_arrow = NULL;
    if (npoints > ARROW_START) {
	points[ARROW_START - 1] = points[0];   /* avoid some points to have good 
					      orientation for arrow */
	npoints -= (ARROW_START - 1);
	start = ARROW_START - 1;
    } else {
	start = 0;
    }
  }
  
  line->type = open_spline(s) ? T_POLYLINE : T_POLYGON;
  line->radius = 0;
  line->next = NULL;
  line->pic = NULL;  
  ptr = NULL;
  for (i = start; i<npoints+start; i++)
    {
      if ((pt = create_point()) == NULL)
	{
	  free(points);
	  free_line(&line);
	  return NULL;
	}
      pt->x = points[i].x;
      pt->y = points[i].y;
      pt->next = NULL;

      if (ptr == NULL)
	ptr = line->points = pt;
      else 
	{
	  ptr->next = pt;
	  ptr = ptr->next;
	}
    }

  free_point_array(points);
  npoints = 0;
  return line;
}



int
make_control_factors(spl)
     F_spline *spl;
{
  F_point   *p = spl->points;
  F_control *cp, *cur_cp;
  int       type_s = approx_spline(spl) ? S_SPLINE_APPROX : S_SPLINE_INTERP;
  
  spl->controls = NULL;
  if ((cp = create_cpoint()) == NULL)
    return 0;
  spl->controls = cur_cp = cp;
  cp->s = closed_spline(spl) ? type_s : S_SPLINE_ANGULAR;
  p = p->next;
  
  for(; p != NULL ; p = p->next)
    {
      if ((cp = create_cpoint()) == NULL)
	return 0;

      cur_cp->next = cp;
      cp->s = type_s;
      cur_cp = cur_cp->next;
    }
  
  cur_cp->s = spl->controls->s;
  cur_cp->next = NULL;
  return 1;
}



static F_control      *
create_cpoint()
{
    F_control	   *cp;

    if ((cp = (F_control *) malloc(CONTROL_SIZE)) == NULL)
	fprintf(stderr,Err_mem);
    return cp;
}


static F_line	       *
create_line()
{
    F_line	   *l;

    if ((l = (F_line *) malloc(LINOBJ_SIZE)) == NULL)
	fprintf(stderr,Err_mem);
    l->pic = NULL;
    l->next = NULL;
    l->for_arrow = NULL;
    l->back_arrow = NULL;
    l->points = NULL;
    l->radius = DEFAULT;
    l->comments = NULL;
    return l;
}

static F_point	       *
create_point()
{
    F_point	   *p;

    if ((p = (F_point *) malloc(POINT_SIZE)) == NULL)
	put_msg(Err_mem);
    return p;
}



static int
num_points(points)
    F_point	   *points;
{
    int		    n;
    F_point	   *p;

    for (p = points, n = 0; p != NULL; p = p->next, n++);
    return n;
}

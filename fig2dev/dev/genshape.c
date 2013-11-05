/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 2002 by Christian Gollwitzer (auriocus)
 * Parts Copyright (c) 1999 by T. Sato
 * Parts Copyright (c) 1989-1999 by Brian V. Smith
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

/* genshape.c: LaTeX shapepar driver for fig2dev
 * derived from genmap.c
 *
 * "Christian Gollwitzer" <auriocus@web.de>
 * if an object has a comment starting with '+', it becomes
 * a filled shape. If the comment starts with '-'
 * then it is a hole. The objects are composed
 * in the order of their depths. A word following the initial
 * '+' or '-' is considered to be an identifier that groups
 * different shapes to one text segment. The order of filling these
 * segments is determined by the alphabetic sort order of the identifiers.
 * The special comments "center", "width" and "height" define
 * position and dimension.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fig2dev.h"
#include "object.h"

static void die(const char * msg) {
 fprintf(stderr, "fig2dev(shape): %s\n", msg);
 exit(-1);
} 
#ifndef STRDUP
#define STRDUP(out,in) { out=malloc(strlen(in)+1); \
                       strcpy(out,in); }

#define STRNDUP(out, in, n) { out=malloc(n+1); \
                              strncpy(out,in, n); \
                              out[n]=0; }
#endif
 
static char*     macroname=NULL;

static int MAX_POINTS=100;
#define POINT_INC 100
static int MAX_SHAPES=20;
#define SHAPE_INC  20
static int  MAX_SHAPEGROUPS=5;
#define SHAPEGROUP_INC 5
#define bool int
#define true 1
#define false 0

static int      num_points = 0;
struct lineseg {
  int x;
  int y;
  bool intersect;
  bool intersect_hit;
  int shapenr;
  int x2;
  int y2;
};
typedef struct lineseg lineseg;

static lineseg *points;

static int num_shapes=0;
struct shape {
  bool ispositiv;
  bool inside;
  int linestart;
  int lineend;
  int depth;
  char * groupname;
};

typedef struct shape shape;
static shape *shapes;

struct shapegroup {
  int shapestart;
  int shapeend;
}; 

typedef struct shapegroup shapegroup;

static shapegroup *shapegroups;
static num_shapegroups=0;


static void alloc_arrays() {
 /* reserve initial space for data structures */
 points=malloc(sizeof(points[0])*MAX_POINTS);
 shapes=malloc(sizeof(shapes[0])*MAX_SHAPES);
 shapegroups=malloc(sizeof(shapegroups[0])*MAX_SHAPEGROUPS);
 
}

static void cleanup_memory() {
/* I'd like to have C++, because it has destructors ... */
  int i;
  free(points);
  for (i=0; i<num_shapes; i++) free(shapes[i].groupname);
  free(shapes);
  free(shapegroups);
}



/* adding shapes */
static void add_object(char *groupid, int lstart, int depth) {
  char *groupname=NULL;
  bool ispositiv=false;
  if (num_shapes>=MAX_SHAPES) {
    MAX_SHAPES+=SHAPE_INC;
    shapes=realloc(shapes, sizeof(shapes[0])*MAX_SHAPES);
   /**debugging stuff fprintf(stderr, "Inc to %d shapes\n", MAX_SHAPES); **/
  }  
  if ((groupid==NULL) || (strlen(groupid)==0)) die("Programming Error: no groupname specified");
  STRDUP(groupname, groupid+1); /*remove the first character from groupid*/
  
  switch (groupid[0]) {
    case '+': ispositiv=true;
              break;
    case '-': ispositiv=false;
              break;
    default:  die("Ill-formed shape comment - start with '+' or '-'");
              break;
  }
  
  shapes[num_shapes].ispositiv=ispositiv;
  shapes[num_shapes].groupname=groupname;
  shapes[num_shapes].linestart=lstart;
  shapes[num_shapes].depth=depth;

  shapes[num_shapes].inside=false;
}

static void finish_object(int lend) {
  shapes[num_shapes].lineend=lend;
  num_shapes++;
}

int shapecomp(const void* s1, const void * s2) {
  /* sorting the shapes in groups (same group identifier */
  return strcmp(((const shape*)s1)->groupname,((const shape*)s2)->groupname);
}

/* Adding shapegroups */
static void add_shapegroup(int shapestart, int shapeend) {
  if (num_shapegroups>=MAX_SHAPEGROUPS) {
     /* maybe later dynamically realloc */
     MAX_SHAPEGROUPS+=SHAPEGROUP_INC;
     shapegroups=realloc(shapegroups, sizeof(shapegroups[0])*MAX_SHAPEGROUPS);
  } 
  shapegroups[num_shapegroups].shapestart=shapestart;
  shapegroups[num_shapegroups].shapeend=shapeend;
  num_shapegroups++;
  
}  

/** debugging stuff **/
static void print_shapegroups() {
  int i;
  for (i=0; i<num_shapegroups; i++) 
    fprintf(stderr, "#%d from %d to %d, id %s \n", 
      i, shapegroups[i].shapestart, shapegroups[i].shapeend,
      shapes[shapegroups[i].shapestart].groupname);
}    

#define fscale 10

#define XZOOM(x)   round(mag * ((x) - llx) / fscale )
#define YZOOM(y)   round(mag * ((y) - lly) / fscale )
#define ZZOOM(z)   round(mag * ((z)/ fscale)) 

#define MIN_DIST    5  /* ignore closer points when processing polyline */

#define ARC_STEP    (M_PI / 23) /* Make a circle to be a 46-gon */

void
genshape_option(opt, optarg)
     char opt;
     char *optarg;
{
  switch (opt) {
  case 'n': /* macro name */
    STRDUP(macroname,optarg);
    break;
  case 'L': /* ignore language and magnif. */
  case 'm':
    break;
  default:
    put_msg(Err_badarg, opt, "shape");
    exit(1);
  }
}

static bool scaleset=false;
static bool centerset=false;
static double centerpos=0.0;
static double scaledim=10.0;

static bool widthref(char *comm) {
  /* return true if this object is the reference for the width */
  return (strncmp(comm, "width", 5)==0);
}

static bool heightref(char *comm) {
  /* return true if this object is the reference for the height */
  return (strncmp(comm, "height", 6)==0);
}

static bool centerref(char *comm) {
  /* return true if this object is the reference for the center */
  return (strncmp(comm, "center", 6)==0);
}


/* reads the first comment of an object, if valid */
static char *get_comment(comment) 
     F_comment  *comment;
{ char *retval=NULL;
  int maxlen=0;
  if (comment==NULL) return NULL;
  if (comment->comment==NULL) return NULL;
#ifdef COM
#undef COM
#endif
#define COM comment->comment  
  /* if we are behind this point, there is some comment to examine 
     we get everything up to the first illegal character (should be whitespace) */
  if ((COM[0]=='+') || (COM[0]=='-')) maxlen=1; /* ignore the first character, if it is + or - */
  while (isalnum(COM[maxlen])) maxlen++;
  /* look for the first non-alphanumeric character, maybe the terminating 0 */
  STRNDUP(retval, COM, maxlen);
  if (strcmp(retval,"")==0) {
    /* no shape defining comment - free mem and return */
    free(retval);
    return NULL;
  }  
  /* it is a shape defining comment
     reject everything not starting with + or - or being special comment */
  if ((retval[0]!='+') && (retval[0]!='-') && (!widthref(retval)) && (!heightref(retval)) && (!centerref(retval))) {
     fprintf(stderr, "Comment \"%s\":\n", COM);
     die("Illegal shape specification, must start with '+' or '-' or be 'center', 'width' or height'\n"
         "Use blank to start real comment. See documentation for explanation");
  }       
  return retval;

#undef COM
}

/* Adding lines */
static bool was_horizontal=false;
static void add_point(int x, int y, int x2, int y2, bool intersect) {
  if (num_points>=MAX_POINTS) {
    /* realloc the linesegments */
    MAX_POINTS+=POINT_INC;
    points=realloc(points, sizeof(points[0])*MAX_POINTS);
  }  
  /* look, if this line continues the last one horizontally */
  if (was_horizontal && (y==y2)) {
    if (points[num_points-1].x2==x && points[num_points-1].y2==y && 
        points[num_points-1].shapenr==num_shapes) {
          /* only continue the line, don't append a new one */
          points[num_points-1].x2=x2;
          points[num_points-1].y2=y2;
          return;
    }
  }   

          
  /* construct a new point */    
  points[num_points].x=x;
  points[num_points].y=y;
  
  points[num_points].x2=x2;
  
  points[num_points].y2=y2;
  
  points[num_points].intersect=intersect;
  points[num_points].intersect_hit=false;

  points[num_points].shapenr=num_shapes;
  num_points++;
  
  was_horizontal=(y==y2);
}

  

static int lastx=0;
static int lasty=0;
static int startx=0;
static int starty=0;
static bool line_start=true;


static void start_line(char *groupid, int depth) {
  add_object(groupid, num_points, depth);
  line_start=true;
}

static void line_to(int x, int y) {
  if (line_start) {
    startx=lastx=x;
    starty=lasty=y;
    line_start=false;
  } else  {
    
    add_point(lastx, lasty, x, y,false);
    
    lastx=x;
    lasty=y;
      
    }  
}

static void finish_line(void) {
    if ((lastx!=startx) || (lasty!=starty)) add_point(lastx,lasty,startx,starty,false);
    finish_object(num_points-1);
    line_start=true;
}  

/** End of data structures **/



int floatcomp(const void* y1, const void * y2) {
  if (*((float*)y1)<*((float*)y2)) return -1;
  if (*((float*)y1)>*((float*)y2)) return +1;
  return 0;
}  

/** begin of the outer layer, derived from HTML map driver **/
void
genshape_start(objects)
     F_compound *objects;
 
{  F_comment* comment=objects->comments;
  alloc_arrays(); 
  /* Arrays for holding lines, shapes and groups */
  if (comment==NULL) return;
  if (comment->comment==NULL) return;
  if (macroname==NULL) STRDUP(macroname,comment->comment);
/* if the user has set a drawing name, use this
   for the macro name, if not overridden from the commandline */    
  
 }


void
genshape_arc(a)
     F_arc      *a;
{
  char *comm;
  int cx, cy, sx, sy, ex, ey;
  double r;
  double sa, ea;
  double alpha;

  comm = get_comment(a->comments);
  if (comm!=NULL) {
    cx = XZOOM(a->center.x);
    cy = YZOOM(a->center.y);
    sx = XZOOM(a->point[0].x);
    sy = YZOOM(a->point[0].y);
    ex = XZOOM(a->point[2].x);
    ey = YZOOM(a->point[2].y);

    if (a->direction == 0) {
      sa = atan2((double)(sy - cy), (double)(sx - cx));
      ea = atan2((double)(ey - cy), (double)(ex - cx));
    } else {
      ea = atan2((double)(sy - cy), (double)(sx - cx));
      sa = atan2((double)(ey - cy), (double)(ex - cx));
    }
    if (ea < sa) ea = ea + 2 * M_PI;

    r = sqrt((double)((sx - cx) * (sx - cx)
                      + (sy - cy) * (sy - cy)));
                      
    start_line(comm, a->depth); 
    if (a->type == T_PIE_WEDGE_ARC)
      line_to(cx, cy);
    for (alpha = sa; alpha < (ea - ARC_STEP / 4); alpha += ARC_STEP) {
      line_to(round(cx + r * cos(alpha)), round(cy + r * sin(alpha)));
    }
       line_to(round(cx + r * cos(ea)), round(cy + r * sin(ea)));
    finish_line();
    free(comm);
  }
}

void
genshape_ellipse(e)
     F_ellipse  *e;
{
  char *comm;
  int x0, y0;
  double rx, ry;
  double angle, theta;

  comm = get_comment(e->comments);
  if (comm!=NULL) {
    x0 = XZOOM(e->center.x);
    y0 = YZOOM(e->center.y);
    rx = mag * e->radiuses.x / fscale;
    ry = mag * e->radiuses.y / fscale;
    angle = - e->angle;
      start_line(comm, e->depth);
      for (theta = ARC_STEP/2; theta < 2.0 * M_PI+ARC_STEP/2; theta += ARC_STEP) {
        line_to(
                round(x0 + cos(angle) * rx * cos(theta)
                      - sin(angle) * ry * sin(theta)),
                round(y0 + sin(angle) * rx * cos(theta)
                      + cos(angle) * ry * sin(theta)));
      }
      finish_line();
      free(comm);
  }
}

void
genshape_line(l)
     F_line     *l;
{
  char *comm;
  int xmin, xmax, ymin, ymax;
  int x, y, r, last_x, last_y;
  F_point *p;
  double theta;

  comm = get_comment(l->comments);
  if (comm!=NULL) {
    if (widthref(comm)) {
      /* The width of this line will be the reference */
      xmin=xmax=XZOOM(l->points->x);
      for (p = l->points->next; p != NULL; p = p->next) {
        x=XZOOM(p->x);
        if (x<xmin) xmin=x;
        if (x>xmax) xmax=x;
      }
      if (xmin!=xmax) {
        scaleset=true;
        scaledim=xmax-xmin;
      } else {
         fprintf(stderr, "fig2dev(shape) Warning: Width of reference object=0 - ignored \n");
      }
      return;
      /* reference objects are *no* shapes */
    } 
    if (heightref(comm)) {
      /* The height of this line will be the reference */
      ymin=ymax=YZOOM(l->points->y);
      for (p = l->points->next; p != NULL; p = p->next) {
        y=YZOOM(p->y);
        if (y<ymin) ymin=y;
        if (y>ymax) ymax=y;
      }
      if (ymin!=ymax) {
        scaleset=true;
        scaledim=ymax-ymin;
      } else {
         fprintf(stderr, "fig2dev(shape) Warning: Height of reference object=0 - ignored \n");
      }
      return;
      /* reference objects are *no* shapes */
    }
    if (centerref(comm)) {
      /* The center of this line will be the reference */
      xmin=xmax=XZOOM(l->points->x);
      for (p = l->points->next; p != NULL; p = p->next) {
        x=XZOOM(p->x);
        if (x<xmin) xmin=x;
        if (x>xmax) xmax=x;
      }
      /* this cannot fail */
      centerset=true;
      centerpos=(xmin+xmax)/2;
      return;
      /* reference objects are *no* shapes */
    } 
    switch (l->type) {
    case T_BOX:
    case T_PIC_BOX:
      start_line(comm, l->depth);
      line_to(XZOOM(l->points->x),YZOOM(l->points->y));
      for (p = l->points->next; p != NULL; p = p->next) {
        line_to(XZOOM(p->x),YZOOM(p->y));
      }
            
      finish_line();
      /* rectangle */
      break;
    case T_ARC_BOX:
      start_line(comm, l->depth);
      xmin = xmax = l->points->x;
      ymin = ymax = l->points->y;
      for (p = l->points->next; p != NULL; p = p->next) {
        if (p->x < xmin) xmin = p->x;
        if (xmax < p->x) xmax = p->x;
        if (p->y < ymin) ymin = p->y;
        if (ymax < p->y) ymax = p->y;
      }
      
      r=ZZOOM((l->radius));
      xmin=XZOOM(xmin);
      xmax=XZOOM(xmax);
      ymin=YZOOM(ymin);
      ymax=YZOOM(ymax);
      
      r=MIN(r,(xmax-xmin)/2);
      r=MIN(r,(ymax-ymin)/2);
      /* corner radius cannot be more than half of one side */
#define ARCCORNER(FIN_ANGLE) while(theta<(FIN_ANGLE)) { \
                             line_to(round(x+r*cos(theta)), round(y+r*sin(theta)));\
                             theta+=ARC_STEP; }
/* define macro for the corner arc */                        
             
      x=xmax-r;
      y=ymax-r;
      theta=0;
      ARCCORNER(M_PI/2);
      /* lower right corner */
      
      x=xmin+r;
      ARCCORNER(M_PI);
      /* lower left corner */
      
      y=ymin+r;
      ARCCORNER(3*M_PI/2);
      /* upper left corner */

      x=xmax-r;
      ARCCORNER(2*M_PI);
      /* upper right corner */
      
      finish_line();
      break;
 
    case T_POLYLINE:
    case T_POLYGON:
      start_line(comm, l->depth);
      for (p = l->points; (l->type==T_POLYLINE? p: p->next) != NULL; p = p->next) {
        x = XZOOM(p->x);
        y = YZOOM(p->y);
        if (p == l->points
            || MIN_DIST < abs(last_x - x) || MIN_DIST < abs(last_y - y)) {
         
            line_to(x, y);
            last_x = x;
            last_y = y;
          }
        }
      
      finish_line();
      break;
    }
    free(comm);
  }
}

void
genshape_text(t)
     F_text     *t;
{
  char *comm;

  comm = get_comment(t->comments);
  if (comm != NULL) {
    fprintf(stderr, "fig2dev(shape): TEXT can't be used as shape\n");
  }
}
/** and of the outer layer **/


bool between(float b1, float b2, float test) {
  return (((b1<=test)  && (test<=b2)) || ((b2<=test)  && (test<=b1))) ;
}

bool between_exclude(float b1, float b2, float test) {
  return (((b1<test)  && (test<b2)) || ((b2<test)  && (test<b1))) ;
}

bool between_int(int b1, int b2, int test) {
  return (((b1<=test)  && (test<=b2)) || ((b2<=test)  && (test<=b1))) ;
}




struct intersect_point {
  float x; float y;
  float x2; float y2;
  int shapenr;
  bool is_endpoint;
  bool is_horizontal;
};  
typedef struct intersect_point intersect_point;
static intersect_point *intersect_points=NULL;
static int MAX_INTERSECTPOINTS=0;
#define INTERSECTPOINT_INC 100

static realloc_intersects(int minimum) {
  while (minimum>=MAX_INTERSECTPOINTS) {
    MAX_INTERSECTPOINTS+=INTERSECTPOINT_INC;
    intersect_points=realloc(intersect_points, sizeof(intersect_points[0])*MAX_INTERSECTPOINTS);
  }  
}

static void 
intersect(float y_val, lineseg* lseg, int * num_xvalues) {
  /* compute the intersection between horizontal line at y_val
    and the linesegment lseg, store it in intersect_points */
#define ip intersect_points[*num_xvalues]
/* for readability define this abbreviation */
  /* fprintf(stderr, "Intersecting at %f with lineseg in shape %d\n", y_val, lseg->shapenr); */
  if (lseg->intersect_hit) return;
  /* this line has been handled by find_intersects */
  if (lseg->y==y_val) {
    /* it hits the endpoint */
    realloc_intersects((*num_xvalues)+1);
    ip.x=lseg->x;   ip.y=lseg->y;
    ip.x2=lseg->x2; ip.y2=lseg->y2;
    ip.shapenr=lseg->shapenr;
    ip.is_endpoint=!lseg->intersect;
    if (lseg->y2==y_val) {
      /* it is horizontal */
      ip.is_horizontal=true;
      (*num_xvalues)++;
      
     } else {
     
         ip.is_horizontal=false;
         (*num_xvalues)++;
         return;
     }   
    /* if it is horizontal, hits the second endpoint too*/
  }
  
  if (lseg->y2==y_val) {
    /* it hits the second endpoint */
    realloc_intersects((*num_xvalues)+1);
    ip.x=lseg->x2;   ip.y=lseg->y2;
    ip.x2=lseg->x; ip.y2=lseg->y;
    ip.shapenr=lseg->shapenr;
    ip.is_endpoint=!lseg->intersect;
    ip.is_horizontal=(ip.y2==ip.y);
    (*num_xvalues)++;
    return;
  }
  
  if (between(lseg->y, lseg->y2, y_val)) {
    /* only then there is an intersection */
    realloc_intersects((*num_xvalues)+1);
    ip.y=y_val;
    ip.x=lseg->x2+ ((float)(lseg->x-lseg->x2)*(float)(y_val-lseg->y2))/(float)(lseg->y-lseg->y2);
    ip.x2=lseg->x2;
    ip.y2=lseg->y2; 
    ip.is_endpoint=false;
    ip.is_horizontal=false;
    ip.shapenr=lseg->shapenr;
    (*num_xvalues)++;
    return;
  }
  
  /* no intersection, don`t increment num_xvalues */   
}
#undef ip  

#define IP1 ((intersect_point*)ip1)
#define IP2 ((intersect_point*)ip2)

static int intersect_comp_above(const void* ip1, const void * ip2) {
  /* first compare the x-values */
  if (IP1->x < IP2->x) return -1;
  if (IP1->x > IP2->x) return +1;
  /* for real intersections, falling line must before rising */
  if ((IP1->y != IP1->y2) && (IP2->y != IP2->y2)) {
    if ((IP1->x2 - IP1->x)/(IP1->y2 - IP1->y) > (IP2->x2 - IP2->x)/(IP2->y2 - IP2->y)) return -1;
    if ((IP1->x2 - IP1->x)/(IP1->y2 - IP1->y) < (IP2->x2 - IP2->x)/(IP2->y2 - IP2->y)) return +1;
    

  }
  if (IP1->shapenr < IP2->shapenr) return -1;
  if (IP1->shapenr > IP2->shapenr) return +1;
  return 0;
}  

static int intersect_comp_below(const void* ip1, const void * ip2) {
  /* first compare the x-values */
  if (IP1->x < IP2->x) return -1;
  if (IP1->x > IP2->x) return +1;
  /* for real intersections, falling line must before rising */
  if ((IP1->y != IP1->y2) && (IP2->y != IP2->y2)) {
    if ((IP1->x2 - IP1->x)/(IP1->y2 - IP1->y) > (IP2->x2 - IP2->x)/(IP2->y2 - IP2->y)) return +1;
    if ((IP1->x2 - IP1->x)/(IP1->y2 - IP1->y) < (IP2->x2 - IP2->x)/(IP2->y2 - IP2->y)) return -1;
    

  }
  if (IP1->shapenr < IP2->shapenr) return -1;
  if (IP1->shapenr > IP2->shapenr) return +1;
  return 0;
}

#undef IP1
#undef IP2

struct fullintersect_point {
  float x; float y;
  int nr1, nr2;
};

typedef struct fullintersect_point fullintersect_point;
static fullintersect_point* fips;
static int num_fips=0;
static int MAX_FIPS=20;
#define FIPS_INC 20
  
static void 
full_intersect(lineseg* lines, int nr1, int nr2, int * num_fip) {
  long D, f;
  int dx, dy, dxs, dys;
  float xs, ys;
  lineseg *ls1, *ls2;
  ls1=(&lines[nr1]);
  ls2=(&lines[nr2]);
  
  dx=ls1->x-ls1->x2;
  dy=ls1->y-ls1->y2;
  dxs=ls2->x-ls2->x2;
  dys=ls2->y-ls2->y2;
  
  if ((dy==0) || (dys==0)) return;
  /* intersection with horizontal line is *not* needed, instead harmful */
  D=dx*(long)dys-dxs*(long)dy;
  /* determinant of the linear equation system */
  if (D==0) return; 
  /* the lines are parallel */
  f=(ls2->x-ls1->x)*(long)dys-(ls2->y-ls1->y)*(long)dxs;
  /* avoid floating point arithmetic till the last step */
  xs=ls1->x+((float)(dx*f))/D;
  ys=ls1->y+((float)(dy*f))/D;
  /* test if the intersection point is inside both lines */
  if ((between_exclude(ls2->x, ls2->x2, xs) || between_exclude(ls2->y, ls2->y2, ys))
     && (between_exclude(ls1->x, ls1->x2, xs) || between_exclude(ls1->y, ls1->y2, ys))) {
    if (*num_fip>=MAX_FIPS) {
      /* realloc the fips */
      MAX_FIPS+=FIPS_INC;
      fips=realloc(fips, sizeof(fips[0])*MAX_FIPS);
    }  
    fips[*num_fip].x=xs;
    fips[*num_fip].y=ys;
    fips[*num_fip].nr1=nr1;
    fips[*num_fip].nr2=nr2;
    (*num_fip)++;
    /* fprintf(stderr, "(%d,%d)->(%d,%d) / (%d,%d)->(%d,%d) = (%g,%g) \n",
                   ls1->x, ls1->y, ls1->x2, ls1->y2, ls2->x, ls2->y, ls2->x2, ls2->y2, xs, ys); */


  }
    
}

static void clear_intersects(lineseg *lseg) {
 int i;
 for (i=0; i<num_fips; i++) {
   lseg[fips[i].nr1].intersect_hit=false;
   lseg[fips[i].nr2].intersect_hit=false;
 }
}

static void find_intersects(float y_val, lineseg *lseg, int *num_xvalues) {
 int i;
 for (i=0; i<num_fips; i++) {
   if (fips[i].y==y_val) {
     /* yeah, we found one */
#define ip intersect_points     
     realloc_intersects((*num_xvalues)+2);
     ip[*num_xvalues].x=fips[i].x;
     ip[*num_xvalues].y=fips[i].y;
     ip[*num_xvalues].is_endpoint=false;
     ip[*num_xvalues].x2=lseg[fips[i].nr1].x2;
     ip[*num_xvalues].y2=lseg[fips[i].nr1].y2;
     ip[*num_xvalues].shapenr=lseg[fips[i].nr1].shapenr;
     ip[*num_xvalues].is_horizontal=(ip[*num_xvalues].y==ip[*num_xvalues].y2);
     lseg[fips[i].nr1].intersect_hit=true;
     (*num_xvalues)++;
     
     ip[*num_xvalues].x=fips[i].x;
     ip[*num_xvalues].y=fips[i].y;
     ip[*num_xvalues].is_endpoint=false;
     ip[*num_xvalues].x2=lseg[fips[i].nr2].x2;
     ip[*num_xvalues].y2=lseg[fips[i].nr2].y2;
     ip[*num_xvalues].shapenr=lseg[fips[i].nr2].shapenr;
     ip[*num_xvalues].is_horizontal=(ip[*num_xvalues].y==ip[*num_xvalues].y2);
     lseg[fips[i].nr2].intersect_hit=true;
     (*num_xvalues)++;
   }
 }  
#undef ip 
}

typedef struct Textblock {
  float x; float len;
  char special;
  
  struct Textblock *next;
}  textblock;

typedef struct Scanline {
  float y;
  bool contains_horizontal;
  textblock* blocks[2];
}

scanline;

bool total_inside() {
 /* compute if we are in the shape totally
    according to what individual shapes we are in */
 
 int takethis=-1;
 int lastdepth=-1;
 int i;
 for (i=0; i<num_shapes; i++) {
   if (shapes[i].inside) {
     if ((lastdepth==-1) || (shapes[i].depth<lastdepth)) {
       takethis=i;
       lastdepth=shapes[i].depth;
     }
    }
 }
 if (takethis==-1) return false;
 return (shapes[takethis].ispositiv==1)?true:false;
} 

static textblock *blockptr=NULL, *oldblockptr=NULL;
static bool inblock;
static float block_xval;

static void start_blocks() {
 blockptr=oldblockptr=NULL;
 inblock=false;
}

static void begin_block(float atx) {
  inblock=true;
  block_xval=atx;
}

static void end_block(float atx) {
 if (!inblock) die("End block outside of a block ???");
 inblock=false;
 if (oldblockptr==NULL) {
   oldblockptr=malloc(sizeof(textblock));
   blockptr=oldblockptr;
   blockptr->x=block_xval;
   blockptr->len=atx-block_xval;
   blockptr->special=0;
   blockptr->next=NULL;
 } else {
   blockptr->next=malloc(sizeof(textblock));
   blockptr=blockptr->next;
   blockptr->x=block_xval;
   blockptr->len=atx-block_xval;
   blockptr->special=0;
   blockptr->next=NULL;
 }
}

static void special_block(float atx, char special) {
  if (inblock) {
     /* end block here */
     end_block(atx);
     blockptr->next=malloc(sizeof(textblock));
     blockptr=blockptr->next;
     blockptr->x=atx;
     blockptr->special=special;
     blockptr->len=0;
     blockptr->next=NULL;
     begin_block(atx);
  } else {
     if (oldblockptr==NULL) {
       /* the first block is special */
       oldblockptr=malloc(sizeof(textblock));
       blockptr=oldblockptr;
       blockptr->x=atx;
       blockptr->special=special;
       blockptr->len=0;
       blockptr->next=NULL;
     } else {
       /* only insert the special block */
       blockptr->next=malloc(sizeof(textblock));
       blockptr=blockptr->next;
       blockptr->x=atx;
       blockptr->special=special;
       blockptr->len=0;
       blockptr->next=NULL;
     }
  }
}

static void destroy_blocks(textblock* blockptr) {
  textblock *oldblockptr;
  oldblockptr=blockptr;
  while (blockptr!=NULL) {
    blockptr=blockptr->next;
    free(oldblockptr);
    oldblockptr=blockptr;
  }
}  

/* I'd really like to have C++ here, because of the destructors */
static void destroy_scanlines(scanline* scanlines, int num_yvalues) { 
 int i;
 /*free memory*/
  for (i=0; i<num_yvalues; i++) {
    destroy_blocks(scanlines[i].blocks[0]);
    if (scanlines[i].contains_horizontal) destroy_blocks(scanlines[i].blocks[1]);
  }
  free(scanlines);
}  

/* this is debugging stuff */
void print_inside(float last_xvalue) {
  int i;
  fprintf(stderr, "%g: ",last_xvalue);
  for (i=0; i<num_shapes; i++) fprintf (stderr, "%d(%d) ", shapes[i].inside, shapes[i].depth);
  fprintf(stderr,"\n");
}  

static void print_shape(int num_yvalues, scanline* scanlines) {
  int y_nr,i;
  float lastscale=0.1;
  float xmin, xmax;
  bool unset=true;
  bool firstline=true;
  
  if (macroname==NULL) macroname="newshape";
  /* write the definition */
  fprintf(tfp, "\\def\\%spar#1{\\shapepar{\\%sshape}#1\\par}\n\\def\\%sshape{%%\n", macroname, macroname, macroname);
  /* compute the center and the scale */
  for (y_nr=0; y_nr<num_yvalues; y_nr++) {
    blockptr=scanlines[y_nr].blocks[0];
    while (blockptr!=NULL) {
      if (unset) {
        xmax=xmin=blockptr->x;
        unset=false;
      } else {
        if (xmin>blockptr->x) xmin=blockptr->x;
        if (xmax<(blockptr->x+blockptr->len)) xmax=blockptr->x+blockptr->len;
      }
      blockptr=blockptr->next;
    }
  }
  if (xmax!=xmin) lastscale=50.0/(xmax-xmin);
  if (scaleset)  lastscale=10.0/scaledim;
  if (!centerset) centerpos=(xmax+xmin)/2;
  fprintf(tfp, "{%g}%%\n", centerpos*lastscale);
  /* center the shape */
  for (y_nr=0; y_nr<num_yvalues; y_nr++) {
   for (i=0;i<(scanlines[y_nr].contains_horizontal?2:1); i++) {
     blockptr=scanlines[y_nr].blocks[i];
     if (blockptr==NULL) continue;
     
     if (firstline) firstline=false;
     else fprintf(tfp, "\\\\%%\n");
     
     if (blockptr!=NULL) fprintf(tfp, "{%g}", scanlines[y_nr].y*lastscale);
     while (blockptr!=NULL) {
       switch (blockptr->special) {
        case 's':
        case 'j':
          fprintf(tfp, "%c",blockptr->special);
          break;
        case 'e':
        case 'b':
          fprintf(tfp, "%c{%g}",blockptr->special, blockptr->x*lastscale);
          break;
        case 0:
           fprintf(tfp, "t{%g}{%g}", blockptr->x*lastscale, blockptr->len*lastscale);
           break;
        default:
           die("Unknown blocktype ???");
        }
        blockptr=blockptr->next;
        
     }  
      
    }  
  }

  fprintf(tfp, "%%\n}\n");
}


int
genshape_end()
{ /* everything is done here */
  int	 i, shapenr,shapenr2, shapestart, shapeend, shapegroupnr;
  float	 y0, y_val;
  int	 num_yvalues, num_rawyvalues, num_xvalues;
  int	 y_nr=0;
  bool	 inside=false, inside_now;
  scanline *scanlines;
  float	*y_values;
  float	*above_block_borders, *below_block_borders;
  int	 num_above_borders, num_below_borders;
  int	 snr, aind, bind;
  bool	 equal;
  char	*oldgroupname;
  int	 oldscanlines=0;

  
  if (num_points==0) die("No shape found - have you forgotten to set comments '+'?");

  intersect_points=malloc(sizeof(intersect_point)*num_points);
  MAX_INTERSECTPOINTS=num_points;
  fips=malloc(sizeof(fullintersect_point)*num_points);
  MAX_FIPS=num_points;

  qsort(shapes,num_shapes, sizeof(shapes[0]), shapecomp); 
  /* sort the shapes by their groupname */
  /* now the shapenrs has changed, to which the points belong
     but we need them for quick reverse lookup 
     solve this by rewriting the shapenrs */
  
  for (shapenr=0; shapenr<num_shapes; shapenr++) {
    for (i=shapes[shapenr].linestart; i<=shapes[shapenr].lineend; i++)
      points[i].shapenr=shapenr;
  }    

  
  oldgroupname=shapes[0].groupname;
  /* now find the borders in between the shapegroups */
  shapestart=0;
  for (i=0; i<num_shapes; i++) {
    if (strcmp(shapes[shapestart].groupname, shapes[i].groupname)!=0) {
      /* here two consecutive shapes have different groupnames*/
      add_shapegroup(shapestart, i-1);
      shapestart=i;
    }
  }
  /* add the last shapegroup till the end of the shapes */
  add_shapegroup(shapestart, num_shapes-1);
 
  /* print_shapegroups(); */
  /* now run for each shapegroup inidvidual */
  for (shapegroupnr=0; shapegroupnr<num_shapegroups; shapegroupnr++) {
    shapestart=shapegroups[shapegroupnr].shapestart;
    shapeend=shapegroups[shapegroupnr].shapeend;
    
    /* intersect each polygonline with each other in this shapegroup*/
    num_fips=0;
    for (shapenr=shapestart; shapenr<=shapeend; shapenr++) {
      for (shapenr2=shapenr+1; shapenr2<=shapeend; shapenr2++) {

       int nr1, nr2;
       for (nr1=shapes[shapenr].linestart; nr1<=shapes[shapenr].lineend; nr1++) {
         /* all lines from shape shapenr */
         for (nr2=shapes[shapenr2].linestart; nr2<=shapes[shapenr2].lineend; nr2++) {
          /* have to be compared with all lines from shape shapenr2 */
          /* avoid intersecting the lines of one polygon
             can cause confusion, but has the effect that the polygons *may not*
             intersect itself like an '8' for example */
           full_intersect(points, nr1, nr2, &num_fips);
          /* fprintf(stderr, "Intersecting %d/%d \n", nr1, nr2); */
         }  /* lines from shape shapenr2 */
       } /* lines from shape shapenr */
      }  /* shapenr2 */
    } /* shapenr */  

    /* Now sort all existing y-Coordinates */
    /* allocate memory */
    y_values=malloc(sizeof(y_values[0])*(num_points+num_fips)); 
    num_rawyvalues=0;
    for (i=0; i<num_points; i++) { 
      if (between_int(shapestart, shapeend, points[i].shapenr)) y_values[num_rawyvalues++]=points[i].y;
    } 
    /* fprintf(stderr, "Points in shapegroup %d: %d\n", shapegroupnr, num_rawyvalues); */
    for (i=0; i<num_fips; i++) y_values[num_rawyvalues++]=fips[i].y;
    
    qsort(y_values, num_rawyvalues, sizeof(y_values[0]), floatcomp);
    /* make them unique */
    y0=y_values[0];
    for (i=num_yvalues=1; i<num_rawyvalues; i++ ) {
      if (y_values[i]!=y0) {
        y_values[num_yvalues]=y_values[i];
        num_yvalues++;
        y0=y_values[i];
      }
    } 
    /* allocate memory for the scanlines */
    if (oldscanlines==0) scanlines=malloc(sizeof(scanline)*num_yvalues);
    else scanlines=realloc(scanlines, sizeof(scanline)*(num_yvalues+oldscanlines));
    /* make it bigger, if this a second pass */
    
    for (y_nr=0; y_nr<num_yvalues; y_nr++) {
      y_val=y_values[y_nr];
  
      scanlines[oldscanlines+y_nr].y=y_val;
      scanlines[oldscanlines+y_nr].contains_horizontal=false;
  
      
      /* compute the intersection of the horizontal scan lines
         with all defined lines in this shapegroup */
      num_xvalues=0;
      /* first look for intersection points of polygons */
      clear_intersects(points);
      find_intersects(y_val, points, &num_xvalues);
      /* then compute horizontal intersections, leave out the found intersect-points */
      for (i=0; i<num_points; i++) {
        if (between_int(shapestart, shapeend, points[i].shapenr)) {
           /* only in this shapegroup */
           intersect(y_val, &points[i], &num_xvalues);
           
        }
      }
      
      /* allocate memory for the blocks */
      above_block_borders=malloc(sizeof(float)*num_xvalues*2);
      num_above_borders=0;
      
  
     /* sort the intersections with the horizontal scan line 
        in ascending x- order  for intersection above the line */
      qsort(intersect_points, num_xvalues, sizeof(intersect_point), intersect_comp_above);
      for (i=shapestart; i<=shapeend; i++) {
       shapes[i].inside=false;
      }
      
      inside=false;
      for (i=0; i<num_xvalues;i++) {
        /* the point is hit either if the second is situated above the current line,
           or if it is a real intersection (not endpoint) */
        if (!intersect_points[i].is_endpoint || intersect_points[i].y2<y_val) {
          snr=intersect_points[i].shapenr;
          shapes[snr].inside=!shapes[snr].inside;
          inside_now=total_inside();
          if (inside!= inside_now) {
            /* we crossed the border of the figure */
            above_block_borders[num_above_borders++]=intersect_points[i].x;
            inside=inside_now;
          }  
        }
      }
      if (inside) die("Outside all polygons (above) inside the shape ???");
  
      /* allocate memory for the blocks */
      below_block_borders=malloc(sizeof(float)*num_xvalues*2);
      num_below_borders=0;
      
  
     /* sort the intersections with the horizontal scan line 
        in ascending x- order  for intersection above the line */
      qsort(intersect_points, num_xvalues, sizeof(intersect_point), intersect_comp_below);
      for (i=shapestart; i<=shapeend; i++) {
       shapes[i].inside=false;
      }
      
      inside=false;
      for (i=0; i<num_xvalues;i++) {
        /* the point is hit either if the second is situated below the current line,
           or if it is a real intersection (not endpoint) */
        if (!intersect_points[i].is_endpoint || intersect_points[i].y2>y_val) {
          snr=intersect_points[i].shapenr;
        shapes[snr].inside=!shapes[snr].inside;
        inside_now=total_inside();
        if (inside!= inside_now) {
          /* we crossed the border of the figure */
          below_block_borders[num_below_borders++]=intersect_points[i].x;
          inside=inside_now;
        }  
        }
      }
      if (inside) die("Outside all polygons (below) inside the shape ???");
  
  
      /* comparison algorithm 
         look, if the blocks are identical if zero-lenghts blocks and gaps are neglected
         then it is an ordinary line without horizontals */
      
      start_blocks();
      /* start the textblock engine */
      equal=true;
      aind=0; bind=0;
      while (equal && (aind<num_above_borders) && (bind<num_below_borders)) {
       if (above_block_borders[aind]==below_block_borders[bind]) {
        /* normal begin or end */
        if ((aind % 2)==0)
          begin_block(above_block_borders[aind]);
        else
          end_block(above_block_borders[aind]);
        
        aind++;
        bind++;
        continue;
       }
       if (above_block_borders[aind]<below_block_borders[bind]) {
         /* look for a zero-length difference */
         if (((aind+1)<num_above_borders) && 
            (above_block_borders[aind]==above_block_borders[aind+1])) {
           /* join or end */
           if ((aind % 2)==0)
             special_block(above_block_borders[aind],'e');
           else
             special_block(above_block_borders[aind],'j');
           aind+=2; /* skip both */
           continue;
          } else {
           /* difference more than a 0-gap */
           equal=false;
           break;
           
         }
       }
       
       if (above_block_borders[aind]>below_block_borders[bind]) {
         /* look for a zero-length difference */
         if (((bind+1)<num_below_borders) && 
            (below_block_borders[bind]==below_block_borders[bind+1])) {
           /* begin or split */
           if ((bind % 2)==0)
             special_block(below_block_borders[bind],'b');
           else
             special_block(below_block_borders[bind],'s');
           bind+=2; /* skip both */
           continue;
        } else {
           /* difference more than a 0-gap */
           equal=false;
           break;
          
         }
       }  
      }
  
      while (equal && aind<num_above_borders) {
        /* zero blocks above left ? */
        if ((aind+1)<num_above_borders) {
           if (above_block_borders[aind]==above_block_borders[aind+1]) {
           /* join or end */
           if ((aind % 2)==0)
             special_block(above_block_borders[aind],'e');
           else
             special_block(above_block_borders[aind],'j');
  
             aind+=2; /* skip both */
           continue;
         } else {
           /* difference more than a 0-gap */
           equal=false;
           break;
         }  
         }
      }
  
  
      while (equal && bind<num_below_borders) {
        /* zero blocks below left ? */
        if ((bind+1)<num_below_borders) {
           if (below_block_borders[bind]==below_block_borders[bind+1]) {
           /* begin or split */
           if ((bind % 2)==0)
             special_block(below_block_borders[bind],'b');
           else
             special_block(below_block_borders[bind],'s');
           bind+=2; /* skip both */
           continue;
         } else {
           /* difference more than a 0-gap */
           equal=false;
           break;
         }  
         }
      }
       
      if (equal) {
        /* one line is enough */
        /* now set the textblockchain into the scanline */
        scanlines[oldscanlines+y_nr].blocks[0]=oldblockptr;
        scanlines[oldscanlines+y_nr].blocks[1]=NULL;
        scanlines[oldscanlines+y_nr].contains_horizontal=false;
      } else {
        int aind, bind;
        float ax, bx, last_ax, last_bx;
        scanlines[oldscanlines+y_nr].contains_horizontal=true;
        destroy_blocks(oldblockptr);
        
        /* look deeper, horizontal lines must be handled */
  
        /* start textblock engine for 'above' lines */
        start_blocks();
        bind=1;
        last_ax=MIN(above_block_borders[0], below_block_borders[0])-1;
        /* start left from all */
        for (aind=0; aind<num_above_borders; aind++, last_ax=ax) {
          ax=above_block_borders[aind];
          if (ax==last_ax) {
            /* found a cute special */
            special_block(ax, (aind%2==1)?'e':'j');
            continue;
          }
    
          /* normal textblock ? */
          if (aind%2==1) begin_block(last_ax);
          while ((bind<num_below_borders) && (below_block_borders[bind]<ax)) {
            last_bx=below_block_borders[bind-1];
            bx=below_block_borders[bind];
            if (between(last_ax,ax,last_bx) && between(last_ax,ax,bx)
                 && (last_bx!=bx)) {
              /* complete nonzero element inside a block or hole */
              if ((bind%2==1) && (aind%2==0)) special_block(last_bx,'b');
                /* block inside a hole */
              if ((bind%2==0) && (aind%2==1)) special_block(bx,'s');
                  /* hole inside a block */
            }     
            bind++;
          }
          if (aind%2==1) end_block(ax);
        }
        /* something left below? */
        while ((bind<num_below_borders)) {
          last_bx=below_block_borders[bind-1];
          bx=below_block_borders[bind];
          if ((last_bx!=bx) && ((last_bx>ax) || (num_above_borders==0))) {
            if (bind%2==1) special_block(last_bx,'b');
            /* block outside all */
          }     
          bind++;
        }
  
  
        scanlines[oldscanlines+y_nr].blocks[0]=oldblockptr;
  
        /* now the same for 'below' lines */
        start_blocks();
        aind=1;
        last_bx=MIN(above_block_borders[0], below_block_borders[0])-1;
        /* start left from all */
        for (bind=0; bind<num_below_borders; bind++, last_bx=bx) {
          bx=below_block_borders[bind];
          if (bx==last_bx) {
            /* found a cute special */
            special_block(bx, (bind%2==1)?'b':'s');
            continue;
          }
    
          /* normal textblock ? */
          if (bind%2==1) begin_block(last_bx);
          while ((aind<num_above_borders) && (above_block_borders[aind]<bx)) {
            last_ax=above_block_borders[aind-1];
            ax=above_block_borders[aind];
            if (between(last_bx,bx,last_ax) && between(last_bx,bx,ax)
                 && (last_ax!=ax)) {
              /* complete nonzero element inside a block or hole */
              if ((aind%2==1) && (bind%2==0)) special_block(ax,'e');
                /* block inside a hole */
              if ((aind%2==0) && (bind%2==1)) special_block(last_ax,'j');
                  /* hole inside a block */
            }     
            aind++;
          }
          if (bind%2==1) end_block(bx);
        }
        /* something left above? */
        while ((aind<num_above_borders)) {
          last_ax=above_block_borders[aind-1];
          ax=above_block_borders[aind];
          if ((last_ax!=ax) && ((last_ax>bx) || (num_below_borders==0))) {
            /* complete nonzero element inside a block or hole */
            if (aind%2==1) special_block(ax,'e');
              /* block outside all  */
            }     
          aind++;
        }
  
        scanlines[oldscanlines+y_nr].blocks[1]=oldblockptr;
  
  
  
      }
      /* free the memory for the block borders */
      free(above_block_borders);
      free(below_block_borders);
  
      
  
        
    } /* for all y_values */
    free(y_values);
    oldscanlines+=num_yvalues;
  } /* for all shapegroups */  
  
  /* now print the result */
  print_shape(oldscanlines, scanlines);

  destroy_scanlines(scanlines, num_yvalues);
  free(intersect_points);
  free(fips);
  cleanup_memory();   
  /* all ok */
  return 0;
}


struct driver dev_shape = {
        genshape_option,
        genshape_start,
        gendev_null,
        genshape_arc,
        genshape_ellipse,
        genshape_line,
        gendev_null,
        genshape_text,
        genshape_end,
        INCLUDE_TEXT
};


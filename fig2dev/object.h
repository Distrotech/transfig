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

/* for the xpm package */
#ifdef USE_XPM
#include <xpm.h>
#endif

#define	BLACK_COLOR	0
#define	WHITE_COLOR	7

#define		DEFAULT				(-1)

typedef		struct f_point {
			int			x, y;
			struct f_point		*next;
			}
		F_point;

typedef		struct f_pos {
			int			x, y;
			}
		F_pos;

typedef		struct f_arrow {
			int			type;
			int			style;
			double			thickness;
			double			wid;
			double			ht;
			}
		F_arrow;

typedef		struct f_comment {
			char			*comment;
			struct f_comment	*next;
			}
		F_comment;

typedef		struct f_ellipse {
			int			type;
#define					T_ELLIPSE_BY_RAD	1
#define					T_ELLIPSE_BY_DIA	2
#define					T_CIRCLE_BY_RAD		3
#define					T_CIRCLE_BY_DIA		4
			int			style;
			int			thickness;
			int			pen_color;
			int			fill_color;
			int			depth;
			int			pen;
			int			fill_style;
			double			style_val;
			int			direction;
			double			angle;
#define		       			UNFILLED	-1
#define		       			WHITE_FILL	0
#define		       			BLACK_FILL	20
			struct f_pos		center;
			struct f_pos		radiuses;
			struct f_pos		start;
			struct f_pos		end;
			struct f_comment	*comments;
			struct f_ellipse	*next;
			}
		F_ellipse;

typedef		struct f_arc {
			int			type;
#define					T_OPEN_ARC		1
#define					T_PIE_WEDGE_ARC		2
			int			style;
			int			thickness;
			int			pen_color;
			int			fill_color;
			int			depth;
			int			pen;
			int			fill_style;
			double			style_val;
			struct f_arrow		*for_arrow;
			struct f_arrow		*back_arrow;
			int			cap_style;
/* IMPORTANT: everything above this point must be in the same order 
	      for ARC, LINE and SPLINE (LINE has join_style following cap_style */
			int			direction;
			struct {double x, y;}	center;
			struct f_pos		point[3];
			struct f_comment	*comments;
			struct f_arc		*next;
			}
		F_arc;

typedef		struct f_line {
			int			type;
#define					T_POLYLINE	1
#define					T_BOX		2
#define					T_POLYGON	3
#define	                                T_ARC_BOX       4
#define	                                T_PIC_BOX       5 

			int			style;
			int			thickness;
			int			pen_color;
			int			fill_color;
			int			depth;
			int			pen;
 			int			fill_style;
			double			style_val;
			struct f_arrow		*for_arrow;
			struct f_arrow		*back_arrow;
			int			cap_style;
			struct f_point		*points;
/* IMPORTANT: everything above this point must be in the same order 
	      for ARC, LINE and SPLINE (LINE has join_style following cap_style */
 			int			join_style;
			int			radius;	/* for T_ARC_BOX */
		    	struct f_pic   		*pic;
			struct f_comment	*comments;
			struct f_line		*next;
			}
		F_line;

/* for colormap */

#define RED 0
#define GREEN 1
#define BLUE 2

typedef struct f_pic {
    int		    subtype;
#define	P_EPS	0	/* EPS picture type */
#define	P_XBM	1	/* X11 bitmap picture type */
#define	P_XPM	2	/* X11 pixmap (XPM) picture type */
#define	P_GIF	3	/* GIF picture type */
#define	P_JPEG	4	/* JPEG picture type */
#define	P_PCX	5	/* PCX picture type */
#define	P_PPM	6	/* PPM picture type */
#define	P_TIF	7	/* TIFF picture type */
#define	P_PNG	8	/* PNG picture type */
    char            file[256];
    int             flipped;
    unsigned char  *bitmap;
#ifdef USE_XPM
    XpmImage	    xpmimage;		/* for Xpm images */
#endif /* USE_XPM */
    unsigned char   cmap[3][MAXCOLORMAPSIZE]; /* for color files */
    int		    numcols;		/* number of colors in cmap */
    int		    transp;		/* transparent color (-1 if none) for GIFs */
    float	    hw_ratio;
    struct f_pos    bit_size;
#ifdef V4_0
    struct f_compound *figure; /*ggstemme*/
#endif /* V4_0 */
}
		F_pic;

extern char EMPTY_PIC[];

typedef		struct f_text {
			int			type;
#define					T_LEFT_JUSTIFIED	0
#define					T_CENTER_JUSTIFIED	1
#define					T_RIGHT_JUSTIFIED	2
			int			font;
#define					DEFAULT_FONT		0
#define					ROMAN_FONT		1
#define					BOLD_FONT		2
#define					ITALIC_FONT		3
#define					MODERN_FONT		4
#define					TYPEWRITER_FONT		5
#define					MAX_FONT		5
			double			size;	/* point size */
			int			color;
			int			depth;
			double			angle;	/* in radian */
			int			flags;
#define					RIGID_TEXT	1	
#define					SPECIAL_TEXT	2
#define					PSFONT_TEXT	4
#define					HIDDEN_TEXT	8
			double			height;	/* pixels */
			double			length;	/* pixels */
			int			base_x;
			int			base_y;
			int			pen;
			char			*cstring;
			struct f_comment	*comments;
			struct f_text		*next;
			}
		F_text;

#define MAX_PSFONT	34
#define MAXFONT(T) (psfont_text(T) ? MAX_PSFONT : MAX_FONT)

#define		rigid_text(t) \
			(t->flags == DEFAULT \
				|| (t->flags & RIGID_TEXT))

#define		special_text(t) \
			((t->flags != DEFAULT \
				&& (t->flags & SPECIAL_TEXT)))

#define		psfont_text(t) \
			(t->flags != DEFAULT \
				&& (t->flags & PSFONT_TEXT))

#define		hidden_text(t) \
			(t->flags != DEFAULT \
				&& (t->flags & HIDDEN_TEXT))

typedef		struct f_control {
			double			lx, ly, rx, ry;
			          /* used by older versions*/
			struct f_control	*next;
			double                  s;
			         /* used by 3.2 version */
			}
		F_control;

#define		int_spline(s)		(s->type & 0x2)
#define         x_spline(s)	        (s->type & 0x4)
#define		approx_spline(s)	(!(int_spline(s)|x_spline(s)))
#define		closed_spline(s)	(s->type & 0x1)
#define		open_spline(s)		(!(s->type & 0x1))


#define S_SPLINE_ANGULAR 0.0
#define S_SPLINE_APPROX 1.0
#define S_SPLINE_INTERP (-1.0)


typedef		struct f_spline {
			int			type;
#define					T_OPEN_APPROX		0
#define					T_CLOSED_APPROX		1
#define					T_OPEN_INTERPOLATED	2
#define					T_CLOSED_INTERPOLATED	3
#define                                 T_OPEN_XSPLINE          4
#define                                 T_CLOSED_XSPLINE        5

			int			style;
			int			thickness;
			int			pen_color;
			int			fill_color;
			int			depth;
			int			pen;
			int			fill_style;
			double			style_val;
			struct f_arrow		*for_arrow;
			struct f_arrow		*back_arrow;
			int			cap_style;
			struct f_point		*points;
/* IMPORTANT: everything above this point must be in the same order 
	      for ARC, LINE and SPLINE (LINE has join_style following cap_style */

			struct f_control	*controls;
			struct f_comment	*comments;
			struct f_spline		*next;
			}
		F_spline;

typedef		struct f_compound {
			struct f_pos		nwcorner;
			struct f_pos		secorner;
			struct f_line		*lines;
			struct f_ellipse	*ellipses;
			struct f_spline		*splines;
			struct f_text		*texts;
			struct f_arc		*arcs;
			struct f_compound	*compounds;
			struct f_comment	*comments;
			struct f_compound	*next;
			}
		F_compound;

#define		ARROW_SIZE		sizeof(struct f_arrow)
#define		POINT_SIZE		sizeof(struct f_point)
#define		CONTROL_SIZE		sizeof(struct f_control)
#define		ELLOBJ_SIZE		sizeof(struct f_ellipse)
#define		ARCOBJ_SIZE		sizeof(struct f_arc)
#define		LINOBJ_SIZE		sizeof(struct f_line)
#define		PIC_SIZE		sizeof(struct f_pic)
#define		TEXOBJ_SIZE		sizeof(struct f_text)
#define		SPLOBJ_SIZE		sizeof(struct f_spline)
#define		COMOBJ_SIZE		sizeof(struct f_compound)
#define		COMMENT_SIZE		sizeof(struct f_comment)

/**********************  object codes  **********************/

#define		O_COLOR_DEF		0
#define		O_ELLIPSE		1
#define		O_POLYLINE		2
#define		O_SPLINE		3
#define		O_TEXT			4
#define		O_ARC			5
#define		O_COMPOUND		6
#define		O_END_COMPOUND		(-O_COMPOUND)
#define		O_ALL_OBJECT		99

/************  object styles (except for f_text)  ************/

#define		SOLID_LINE		0
#define		DASH_LINE		1
#define		DOTTED_LINE		2
#define		DASH_DOT_LINE		3
#define		DASH_2_DOTS_LINE	4
#define		DASH_3_DOTS_LINE	5

#define		CLOSED_PATH		0
#define		OPEN_PATH		1

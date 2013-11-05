/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
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

#define		Line_malloc(z)		z = (F_line*)malloc(LINOBJ_SIZE)
#define		Pic_malloc(z)		z = (F_pic*)malloc(PIC_SIZE)
#define		Spline_malloc(z)	z = (F_spline*)malloc(SPLOBJ_SIZE)
#define		Ellipse_malloc(z)	z = (F_ellipse*)malloc(ELLOBJ_SIZE)
#define		Arc_malloc(z)		z = (F_arc*)malloc(ARCOBJ_SIZE)
#define		Compound_malloc(z)	z = (F_compound*)malloc(COMOBJ_SIZE)
#define		Text_malloc(z)		z = (F_text*)malloc(TEXOBJ_SIZE)
#define		Point_malloc(z)		z = (F_point*)malloc(POINT_SIZE)
#define		Control_malloc(z)	z = (F_control*)malloc(CONTROL_SIZE)
#define		Arrow_malloc(z)		z = (F_arrow*)malloc(ARROW_SIZE)

extern char	Err_mem[];

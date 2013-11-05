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

struct color_db {
    char *name;
    int   red, green, blue;
};

static 	int	  read_colordb();
struct	color_db *Xcolors;
int	numXcolors, maxcolors;
Boolean	have_read_X_colors = False;

int
lookup_X_color(name, rgb)
  char *name;
  RGB  *rgb;
{
    int		i, n;
    int		r, g, b;
    struct color_db *col;

    /* read the X color database file if we haven't done that yet */
    if (!have_read_X_colors) {
	if (read_colordb() != 0) {
	    /* error reading color database, return black */
	    rgb->red = rgb->green = rgb->blue = 0;
	    return -1;
	}
	have_read_X_colors = True;
    }

    if (name[0] == '#') {			/* hex color parse it now */
	if (strlen(name) == 4) {		/* #rgb */
		n = sscanf(name,"#%1x%1x%1x",&r,&g,&b);
		rgb->red   = ((r << 4) + r) << 8;
		rgb->green = ((g << 4) + g) << 8;
		rgb->blue  = ((b << 4) + b) << 8;
	} else if (strlen(name) == 7) {		/* #rrggbb */
		n = sscanf(name,"#%2x%2x%2x",&r,&g,&b);
		rgb->red   = r << 8;
		rgb->green = g << 8;
		rgb->blue  = b << 8;
	} else if (strlen(name) == 10) {	/* #rrrgggbbb */
		n = sscanf(name,"#%3x%3x%3x",&r,&g,&b);
		rgb->red   = r << 4;
		rgb->green = g << 4;
		rgb->blue  = b << 4;
	} else if (strlen(name) == 13) {	/* #rrrrggggbbbb */
		n = sscanf(name,"#%4x%4x%4x",&r,&g,&b);
		rgb->red   = r;
		rgb->green = g;
		rgb->blue  = b;
	}
	if (n == 3) {
	    /* ok */
	    return 0;
	}
    } else {
	/* named color, look in the database we read in */
	for (col = Xcolors, i=0; i<numXcolors; col++, i++) {
	    if (strcasecmp(col->name, name) == 0) {
		/* found it */
		rgb->red = col->red;
		rgb->green = col->green;
		rgb->blue = col->blue;
		return 0;
	    }
	}
    }
    /* not found or bad #rgb spec, set to black */
    rgb->red = rgb->green = rgb->blue = 0;
    return -1;
}

/* read the X11 RGB color database (ASCII .txt) file */

int
read_colordb()
{
    FILE	*fp;
    char	s[100], s1[100], *c1, *c2;
    int		r,g,b;
    struct color_db *col;

    fp = fopen(RGB_FILE, "r");
    if (fp == NULL) {
      fprintf(stderr,"Couldn't open the RGB database file '%s'\n", RGB_FILE);
      return -1;
    }
    numXcolors = 0;
    maxcolors = 400;
    if ((Xcolors = (struct color_db*) malloc(maxcolors*sizeof(struct color_db))) == NULL) {
      fprintf(stderr,"Couldn't allocate space for the RGB database file\n");
      return -1;
    }

    while (fgets(s, sizeof(s), fp)) {
	if (numXcolors >= maxcolors) {
	    maxcolors += 500;
	    if ((Xcolors = (struct color_db*) realloc(Xcolors, maxcolors*sizeof(struct color_db))) == NULL) {
	      fprintf(stderr,"Couldn't allocate space for the RGB database file\n");
	      return -1;
	    }
	}
	if (sscanf(s, "%d %d %d %[^\n]", &r, &g, &b, s1) == 4) {
	    /* remove any white space from the color name */
	    for (c1=s1, c2=s1; *c2; c2++) {
		if (*c2 != ' ' && *c2 != '\t') {
		   *c1 = *c2;
		   c1++;
		}
	    }
	    *c1 = '\0';
	    col = (Xcolors+numXcolors);
	    col->red = r << 8;
	    col->green = g << 8;
	    col->blue = b << 8;
	    col->name = malloc(strlen(s1)+1);
	    strcpy(col->name, s1);
	    numXcolors++;
	}
    }
    fclose(fp);
    return 0;
}
	
/* convert rgb to gray scale using the classic luminance conversion factors */

float
rgb2luminance (r, g, b)
     float r;
     float g;
     float b;
{
  return 0.3*r + 0.59*g + 0.11*b;
}



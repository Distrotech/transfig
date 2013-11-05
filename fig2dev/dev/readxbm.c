/*
 * TransFig: Facility for Translating Fig code
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

/*
   This is a modified version of the XReadBitmapFromFile() routine from
   the X11R5 distribution.  This version reads the XBM file into a (char*)
   array rather than creating the pixmap directly.
*/

/* $XConsortium: XRdBitF.c,v 1.15 91/02/01 16:34:46 gildea Exp $ */
/* Copyright, 1987, Massachusetts Institute of Technology */

/*
   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation, and that the name of M.I.T. not be used in advertising or
   publicity pertaining to distribution of the software without specific,
   written prior permission.  M.I.T. makes no representations about the
   suitability of this software for any purpose.  It is provided "as is"
   without express or implied warranty.
*/

/*
 *	Code to read bitmaps from disk files. Interprets
 *	data from X10 and X11 bitmap files.
 *
 *	Modified for speedup by Jim Becker, changed image
 *	data parsing logic (removed some fscanf()s).
 *	Aug 5, 1988
 *
 * Note that this file and ../Xmu/RdBitF.c look very similar....  Keep them
 * that way (but don't use common source code so that people can have one
 * without the other).
 */

#include "fig2dev.h"
#include "object.h"
#include "readxbm.h"

#define MAX_SIZE 255

/* attempt to read a bitmap file */

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_xbm(file,filetype,pic,llx,lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
    int status;
    unsigned int x, y;

    *llx = *lly = 0;
    /* first try for a X Bitmap file format */
    status = ReadFromBitmapFile(file, &x, &y, &pic->bitmap);
    if (status == 1) {
	pic->subtype = P_XBM;
	pic->hw_ratio = (float) y / x;
	pic->numcols = 0;
	pic->bit_size.x = x;
	pic->bit_size.y = y;
	/* output PostScript comment */
	fprintf(tfp, "%% Begin Imported X11 Bitmap File: %s\n\n", pic->file);
	return 1;
    }
    /* Non Bitmap file */
    return 0;
}

/* shared data for the image read/parse logic */
static short hexTable[256];		/* conversion value */
static int initialized = 0;		/* easier to fill in at run time */


/*
 *	Table index for the hex values. Initialized once, first time.
 *	Used for translation value or delimiter significance lookup.
 */
static void initHexTable()
{
    /*
     * We build the table at run time for several reasons:
     *
     *     1.  portable to non-ASCII machines.
     *     2.  still reentrant since we set the init flag after setting table.
     *     3.  easier to extend.
     *     4.  less prone to bugs.
     */
    hexTable['0'] = 0;	hexTable['1'] = 1;
    hexTable['2'] = 2;	hexTable['3'] = 3;
    hexTable['4'] = 4;	hexTable['5'] = 5;
    hexTable['6'] = 6;	hexTable['7'] = 7;
    hexTable['8'] = 8;	hexTable['9'] = 9;
    hexTable['A'] = 10;	hexTable['B'] = 11;
    hexTable['C'] = 12;	hexTable['D'] = 13;
    hexTable['E'] = 14;	hexTable['F'] = 15;
    hexTable['a'] = 10;	hexTable['b'] = 11;
    hexTable['c'] = 12;	hexTable['d'] = 13;
    hexTable['e'] = 14;	hexTable['f'] = 15;

    /* delimiters of significance are flagged w/ negative value */
    hexTable[' '] = -1;	hexTable[','] = -1;
    hexTable['}'] = -1;	hexTable['\n'] = -1;
    hexTable['\t'] = -1;
	
    initialized = 1;
}

/*
 *	read next hex value in the input stream, return -1 if EOF
 */

static int
NextInt (file)
    FILE *file;
{
    int	ch;
    int	value = 0;
    int	ret_value;
    int gotone = 0;
    int done = 0;

    /* loop, accumulate hex value until find delimiter  */
    /* skip any initial delimiters found in read stream */

    while (!done) {
	ch = getc(file);
	if (ch == EOF) {
	    value	= -1;
	    done++;
	} else {
	    /* trim high bits, check type and accumulate */
	    ch &= 0xff;
	    if (isascii(ch) && isxdigit(ch)) {
		value = (value << 4) + hexTable[ch];
		gotone++;
	    } else if ((hexTable[ch]) < 0 && gotone)
	      done++;
	}
    }

    ret_value = 0;
    if (value & 0x80)
	ret_value |= 0x01;
    if (value & 0x40)
	ret_value |= 0x02;
    if (value & 0x20)
	ret_value |= 0x04;
    if (value & 0x10)
	ret_value |= 0x08;
    if (value & 0x08)
	ret_value |= 0x10;
    if (value & 0x04)
	ret_value |= 0x20;
    if (value & 0x02)
	ret_value |= 0x40;
    if (value & 0x01)
	ret_value |= 0x80;
    return ret_value;

}

int
ReadFromBitmapFile (file, width, height, data_ret)
    FILE	*file;			/* handle on file  */
    unsigned int *width, *height;       /* RETURNED */
    unsigned char **data_ret;           /* RETURNED */
{
    unsigned	char *data = NULL;	/* working variable */
    char	line[MAX_SIZE];		/* input line from file */
    int		size;			/* number of bytes of data */
    char	name_and_type[MAX_SIZE]; /* an input line */
    char	*type;			/* for parsing */
    int		value;			/* from an input line */
    int		version10p;		/* boolean, old format */
    int		padding;		/* to handle alignment */
    int		bytes_per_line;		/* per scanline of data */
    unsigned	int ww = 0;		/* width */
    unsigned	int hh = 0;		/* height */

    /* first time initialization */
    if (initialized == 0)
	initHexTable();

    /* error cleanup and return macro	*/
#define	RETURN(code) { if (data) free (data);  return code; }

    while (fgets(line, MAX_SIZE, file)) {
	if (strlen(line) == MAX_SIZE-1) {
	    RETURN (0);
	}
	if (sscanf(line,"#define %s %d",name_and_type,&value) == 2) {
	    if (!(type = strrchr(name_and_type, '_')))
	      type = name_and_type;
	    else
	      type++;

	    if (!strcmp("width", type))
	      ww = (unsigned int) value;
	    if (!strcmp("height", type))
	      hh = (unsigned int) value;
	    continue;
	}

	if (sscanf(line, "static short %s = {", name_and_type) == 1)
	  version10p = 1;
	else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
	  version10p = 0;
	else if (sscanf(line, "static char %s = {", name_and_type) == 1)
	  version10p = 0;
	else
	  continue;

	if (!(type = strrchr(name_and_type, '_')))
	  type = name_and_type;
	else
	  type++;

	if (strcmp("bits[]", type))
	  continue;

	if (!ww || !hh)
	  RETURN (0);

	if ((ww % 16) && ((ww % 16) < 9) && version10p)
	  padding = 1;
	else
	  padding = 0;

	bytes_per_line = (ww+7)/8 + padding;

	size = bytes_per_line * hh;
	data = (unsigned char *) malloc ((unsigned int) size);
	if (!data)
	  RETURN (0);

	if (version10p) {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=data; bytes<size; (bytes += 2)) {
		if ((value = NextInt(file)) < 0)
		  RETURN (0);
		*(ptr++) = value;
		if (!padding || ((bytes+2) % bytes_per_line))
		  *(ptr++) = value >> 8;
	    }
	} else {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=data; bytes<size; bytes++, ptr++) {
		if ((value = NextInt(file)) < 0)
		  RETURN (0);
		*ptr=value;
	    }
	}
    }					/* end while */

    if (data == NULL) {
	RETURN (0);
    }

    *data_ret = data;
    *width = ww;
    *height = hh;

    RETURN (1);
}

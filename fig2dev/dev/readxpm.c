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

#include "fig2dev.h"
#include "object.h"

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_xpm(filename,filetype,pic,llx,lly)
    char	   *filename;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	int	    status;

	*llx = *lly = 0;
	status = XpmReadFileToXpmImage(filename, &pic->xpmimage, NULL);
	pic->subtype = P_XPM;
	pic->numcols = pic->xpmimage.ncolors;
	pic->bit_size.x = pic->xpmimage.width;
	pic->bit_size.y = pic->xpmimage.height;

	/* output PostScript comment */
	fprintf(tfp, "%% Begin Imported XPM File: %s\n\n", pic->file);
	return 1;
}

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

extern	int	_read_pcx();

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_tif(filename,filetype,pic,llx,lly)
    char	   *filename;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	char	 buf[2*PATH_MAX+40],pcxname[PATH_MAX];
	FILE	*tiftopcx;
	int	 stat;

	*llx = *lly = 0;
	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a TIFF File: %s\n\n", pic->file);

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());
	/* make command to convert tif to pnm then to pcx into temp file */
	sprintf(buf, "tifftopnm %s 2> /dev/null | ppmtopcx > %s 2> /dev/null",
		filename, pcxname);
	if ((tiftopcx = popen(buf,"w" )) == 0) {
	    fprintf(stderr,"Cannot open pipe to tifftopnm or ppmtopcx\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return 0;
	}
	/* close pipe */
	pclose(tiftopcx);
	if ((tiftopcx = fopen(pcxname, "rb")) == NULL) {
	    fprintf(stderr,"Can't open temp output file\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return 0;
	}
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(tiftopcx, pic);
	pic->transp = -1;
	/* close the temp file */
	fclose(tiftopcx);
	/* and remove it */
	unlink(pcxname);

	return stat;
}


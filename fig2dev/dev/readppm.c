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
		  0 : failure
*/

int
read_ppm(file,filetype,pic,llx,lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	char	 buf[512],pcxname[PATH_MAX];
	FILE	*giftopcx;
	int	 stat, size;

	*llx = *lly = 0;
	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a PPM File: %s\n\n", pic->file);

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());
	/* make command to convert gif to pcx into temp file */
	sprintf(buf, "ppmtopcx > %s 2> /dev/null", pcxname);
	if ((giftopcx = popen(buf,"w" )) == 0) {
	    fprintf(stderr,"Cannot open pipe to giftoppm\n");
	    return 0;
	}
	while ((size=fread(buf, 1, 512, file)) != 0) {
	    fwrite(buf, size, 1, giftopcx);
	}
	/* close pipe */
	pclose(giftopcx);
	if ((giftopcx = fopen(pcxname, "rb")) == NULL) {
	    fprintf(stderr,"Can't open temp output file\n");
	    return 0;
	}
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(giftopcx, pic);
	pic->transp = -1;
	/* close file */
	fclose(giftopcx);
	/* remove temp file */
	unlink(pcxname);

	return stat;
}

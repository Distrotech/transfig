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

/* for both procedures:
     return codes:  1 : success
		    0 : failure
*/

static int	read_eps_pdf();

/* read a PDF file */

int
read_pdf(file, filetype, pic, llx, lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
    return read_eps_pdf(file,filetype,pic,llx,lly,True);
}

/* read an EPS file */

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/

int
read_eps(file, filetype, pic, llx, lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
    return read_eps_pdf(file,filetype,pic,llx,lly,False);
}

static int
read_eps_pdf(file, filetype, pic, llx, lly, pdf_flag)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
    Boolean	    pdf_flag;
{
	char	    buf[512];
	double	    fllx, flly, furx, fury;
	int	    nested;
	char	   *c;

	pic->bit_size.x = pic->bit_size.y = 0;
	pic->subtype = P_EPS;

	/* give some initial values for bounding in case none is found */
	*llx = 0;
	*lly = 0;
	pic->bit_size.x = 10;
	pic->bit_size.y = 10;
	nested = 0;

	while (fgets(buf, 512, file) != NULL) {
	    /* look for /MediaBox for pdf file */
	    if (pdf_flag) {
		if (!strncmp(buf, "/MediaBox", 8)) {	/* look for the MediaBox spec */
		    c = strchr(buf,'[')+1;
		    if (c && sscanf(c,"%d %d %d %d",llx,lly,&urx,&ury) < 4) {
			*llx = *lly = 0;
			urx = paperdef[0].width*72;
			ury = paperdef[0].height*72;
			put_msg("Bad MediaBox in imported PDF file %s, assuming %s size", 
				pic->file, metric? "A4" : "Letter" );
		    }
		}
	    /* look for bounding box for EPS file */
	    } else if (!nested && !strncmp(buf, "%%BoundingBox:", 14)) {
		c=buf+14;
		/* skip past white space */
		while (*c == ' ' || *c == '\t') 
		    c++;
		if (strncmp(c,"(atend)",7)) {	/* make sure not an (atend) */
		    if (sscanf(c, "%lf %lf %lf %lf",
				&fllx, &flly, &furx, &fury) < 4) {
			fprintf(stderr,"Bad EPS bitmap file: %s\n", pic->file);
			return 0;
		    }
		    *llx = (int) floor(fllx);
		    *lly = (int) floor(flly);
		    pic->bit_size.x = (int) (furx-fllx);
		    pic->bit_size.y = (int) (fury-flly);
		    break;
		}
	    } else if (!strncmp(buf, "%%Begin", 7)) {
		++nested;
	    } else if (nested && !strncmp(buf, "%%End", 5)) {
		--nested;
	    }
	}
	fprintf(tfp, "%% Begin Imported %s File: %s\n", 
				pdf_flag? "PDF" : "EPS", pic->file);
	fprintf(tfp, "%%%%BeginDocument: %s\n", pic->file);
	fprintf(tfp, "%%\n");
	return 1;
}

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
#include <png.h>

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_png(file,filetype,pic,llx,lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
    register int    i, j;
    png_structp	    png_ptr;
    png_infop	    info_ptr;
    png_infop	    end_info;
    png_uint_32	    w, h, rowsize;
    int		    bit_depth, color_type, interlace_type;
    int		    compression_type, filter_type;
    png_bytep	   *row_pointers;
    unsigned char  *ptr;
    int		    num_palette;
    png_colorp	    palette;
    double          gamma;
    png_color_16p   file_background;
    png_color_16    png_background;

    *llx = *lly = 0;

    /* read the png file here */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		(png_voidp) NULL, NULL, NULL);
    if (!png_ptr)
	return 0;
		
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
	return 0;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
	return 0;
    }

    /* set long jump here */
    if (setjmp(png_jmpbuf(png_ptr))) {
	/* if we get here there was a problem reading the file */
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	return 0;
    }
	
    /* set up the input code */
    png_init_io(png_ptr, file);

    /* now read the file info */
    png_read_info(png_ptr, info_ptr);

    /* get width, height etc */
    png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
	&interlace_type, &compression_type, &filter_type);

    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
	png_set_gamma(png_ptr, 2.2, gamma);
    else
	png_set_gamma(png_ptr, 2.2, 0.45);

    if (png_get_bKGD(png_ptr, info_ptr, &file_background))
	/* set the background to the one supplied */
	png_set_background(png_ptr, file_background,
		PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
    else {
	/* blend the canvas background using the alpha channel */
	if (bgspec) {
	    png_background.red   = background.red >> 8;
	    png_background.green = background.green >> 8;
	    png_background.blue  = background.blue >> 8;
	    png_background.gray  = 0;
	} else {
	    /* no background specified by user, use white */
	    png_background.red = png_background.green =
		png_background.blue = png_background.gray  = 255;
	}
	png_set_background(png_ptr, &png_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 2.2);
    }

    /* set order to BGR (default is RGB) */
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_bgr(png_ptr);

    /* if user wants grayscale (-N) then map to gray */
    if (grayonly &&
	(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA))
	    png_set_rgb_to_gray(png_ptr, 1, 0.3, 0.59);

    /* strip 16-bit RGB values down to 8-bit */
    if (bit_depth == 16)
	png_set_strip_16(png_ptr);

    /* force to 8-bits per pixel if less than 8 */
    if (bit_depth < 8)
	png_set_packing(png_ptr);

    /* dither rgb files down to 8 bit palette */
    num_palette = 0;
    pic->transp = -1;
    if (color_type & PNG_COLOR_MASK_COLOR) {
	png_uint_16p	histogram;

	if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
	    png_get_hIST(png_ptr, info_ptr, &histogram);
#if (PNG_LIBPNG_VER_MAJOR > 1 || (PNG_LIBPNG_VER_MAJOR == 1 && (PNG_LIBPNG_VER_MINOR > 4))) || defined(png_set_quantize)
	    png_set_quantize(png_ptr, palette, num_palette, 256, histogram, 0);
#else
	    png_set_dither(png_ptr, palette, num_palette, 256, histogram, 0);
#endif
	}
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
	/* expand to full range */
	png_set_expand(png_ptr);
	/* make a gray colormap */
	num_palette = 256;
	for (i = 0; i < num_palette; i++)
	    pic->cmap[RED][i] = pic->cmap[GREEN][i] = pic->cmap[BLUE][i] = i;
    } else {
	/* transfer the palette to the object's colormap */
	for (i=0; i<num_palette; i++) {
	    pic->cmap[RED][i]   = palette[i].red;
	    pic->cmap[GREEN][i] = palette[i].green;
	    pic->cmap[BLUE][i]  = palette[i].blue;
	    /* if user wants grayscale (-N) then map to gray */
	    if (grayonly)
		pic->cmap[RED][i] = pic->cmap[GREEN][i] = pic->cmap[BLUE][i] = 
		    (int) (rgb2luminance(pic->cmap[RED][i]/255.0, 
					pic->cmap[GREEN][i]/255.0, 
					pic->cmap[BLUE][i]/255.0)*255.0);
	}
    }
    rowsize = w;
    if (color_type == PNG_COLOR_TYPE_RGB ||
	color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	    rowsize = w*3;

    /* allocate the row pointers and rows */
    row_pointers = (png_bytep *) malloc(h*sizeof(png_bytep));
    for (i=0; i<h; i++) {
	if ((row_pointers[i] = malloc(rowsize)) == NULL) {
	    for (j=0; j<i; j++)
		free(row_pointers[j]);
	    return 0;
	}
    }

    /* finally, read the file */
    png_read_image(png_ptr, row_pointers);

    /* allocate the bitmap */
    if ((pic->bitmap=malloc(rowsize*h))==NULL)
	    return 0;

    /* copy it to our bitmap */
    ptr = pic->bitmap;
    for (i=0; i<h; i++) {
	bcopy(row_pointers[i], ptr, rowsize);
	ptr += rowsize;
    }
    /* put in width, height */
    pic->bit_size.x = w;
    pic->bit_size.y = h;

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
	/* no palette */
	pic->numcols = 2<<16;
    } else {
	pic->numcols = num_palette;
    }

    /* clean up */
    png_read_end(png_ptr, end_info);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    for (i=0; i<h; i++)
	free(row_pointers[i]);

    pic->subtype = P_PNG;
    pic->hw_ratio = (float) pic->bit_size.y / pic->bit_size.x;

    /* success */
    return 1;
}

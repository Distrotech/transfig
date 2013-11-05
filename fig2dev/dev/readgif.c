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

/* The following code is extracted from giftoppm.c, from the pbmplus package */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

#include "fig2dev.h"
#include "object.h"

extern	FILE	*open_picfile();
extern	int	 _read_pcx();

#define BUFLEN 1024

/* Some of the following code is extracted from giftopnm.c, from the netpbm package */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */


static Boolean	 ReadColorMap();
static Boolean	 DoGIFextension();
static int	 GetDataBlock();

#define LOCALCOLORMAP		0x80
#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) != 0)
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))

#define LM_to_uint(a,b)			(((b)<<8)|(a))

struct {
	unsigned int	Width;
	unsigned int	Height;
	unsigned int	BitPixel;
	unsigned char	ColorMap[3][MAXCOLORMAPSIZE];
	unsigned int	ColorResolution;
	unsigned int	Background;
	unsigned int	AspectRatio;
} GifScreen;

struct {
	int	transparent;
	int	delayTime;
	int	inputFlag;
	int	disposal;
} Gif89 = { -1, -1, -1, 0 };

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_gif(filename,filetype,pic,llx,lly)
    char	   *filename;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	char		 buf[BUFLEN],pcxname[PATH_MAX];
	char		 realname[PATH_MAX];
	FILE		*file, *giftopcx;
	int		 i, stat, size;
	int		 useGlobalColormap;
	unsigned int	 bitPixel;
	unsigned char	 c;
	char		 version[4];
	unsigned char    transp[3]; /* RGB of transparent color (if any) */

	/* open the file */
	if ((file=open_picfile(filename, &filetype, False, realname)) == NULL) {
		fprintf(stderr,"No such GIF file: %s\n",realname);
		return 0;
	}

	/* first read header to look for any transparent color extension */

	*llx = *lly = 0;

	if (! ReadOK(file,buf,6)) {
		return 0;
	}

	if (strncmp((char*)buf,"GIF",3) != 0) {
		return 0;
	}

	strncpy(version, (char*)(buf + 3), 3);
	version[3] = '\0';

	if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
		fprintf(stderr,"Unknown GIF version %s\n",version);
		return 0;
	}

	if (! ReadOK(file,buf,7)) {
		return 0;		/* failed to read screen descriptor */
	}

	GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
	GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
	GifScreen.BitPixel        = 2<<(buf[4]&0x07);
	GifScreen.ColorResolution = (((((int)buf[4])&0x70)>>3)+1);
	GifScreen.Background      = (unsigned int) buf[5];
	GifScreen.AspectRatio     = (unsigned int) buf[6];

	if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
		if (!ReadColorMap(file,GifScreen.BitPixel,pic->cmap)) {
			return 0;	/* error reading global colormap */
		}
	}

	/* assume no transparent color for now */
	Gif89.transparent =  -1;

	for (;;) {
		if (! ReadOK(file,&c,1)) {
			return 0;	/* EOF / read error on image data */
		}

		if (c == ';') {			/* GIF terminator, finish up */
			break;			/* all done */
		}

		if (c == '!') { 		/* Extension */
		    if (! ReadOK(file,&c,1))
			fprintf(stderr,"GIF read error on extention function code\n");
		    (void) DoGIFextension(file, c);
		    continue;
		}

		if (c != ',') {			/* Not a valid start character */
			continue;
		}

		if (! ReadOK(file,buf,9)) {
			return 1;	/* couldn't read left/top/width/height */
		}

		useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

		bitPixel = 1<<((buf[8]&0x07)+1);

		if (! useGlobalColormap) {
		    if (!ReadColorMap(file, bitPixel, pic->cmap)) {
			fprintf(stderr,"error reading local GIF colormap\n" );
			return 1;
		    }
		}
		break;				/* image starts here, header is done */
	}

	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a GIF File: %s\n\n", pic->file);

	/* save transparent indicator */
	pic->transp = Gif89.transparent;
	/* and RGB values */
	if (pic->transp != -1) {
	    transp[RED]   = pic->cmap[RED][pic->transp];
	    transp[GREEN] = pic->cmap[GREEN][pic->transp];
	    transp[BLUE]  = pic->cmap[BLUE][pic->transp];
	}

	/* reposition the file at the beginning */
	fseek(file, 0, SEEK_SET);
	
	/* now call giftopnm and ppmtopcx */

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());
	/* make command to convert gif to pcx into temp file */
	sprintf(buf, "giftopnm -quiet | ppmtopcx -quiet > %s 2> /dev/null", pcxname);
	if ((giftopcx = popen(buf,"w" )) == 0) {
	    fprintf(stderr,"Cannot open pipe to giftoppm\n");
	    return 0;
	}
	while ((size=fread(buf, 1, BUFLEN, file)) != 0) {
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
	/* close file */
	fclose(giftopcx);

	/* remove temp file */
	unlink(pcxname);

	/* now match original transparent colortable index with possibly new 
	   colortable from ppmtopcx */
	if (pic->transp != -1) {
	    for (i=0; i<pic->numcols; i++) {
		if (pic->cmap[RED][i]   == transp[RED] &&
		    pic->cmap[GREEN][i] == transp[GREEN] &&
		    pic->cmap[BLUE][i]  == transp[BLUE])
			break;
	    }
	    if (i < pic->numcols)
		pic->transp = i;
	}

	return stat;
}

static Boolean
ReadColorMap(fd,number,cmap)
FILE	*fd;
unsigned int	number;
unsigned char cmap[3][MAXCOLORMAPSIZE];
{
	int		i;
	unsigned char	rgb[3];

	for (i = 0; i < number; ++i) {
	    if (! ReadOK(fd, rgb, sizeof(rgb))) {
		fprintf(stderr,"bad GIF colormap\n" );
		return False;
	    }
	    cmap[RED][i]   = rgb[RED];
	    cmap[GREEN][i] = rgb[GREEN];
	    cmap[BLUE][i]  = rgb[BLUE];
	}
	return True;
}

static Boolean
DoGIFextension(fd, label)
FILE	*fd;
int	label;
{
	static unsigned char buf[256];
	char	    *str;

	switch (label) {
	    case 0x01:		/* Plain Text Extension */
		str = "Plain Text Extension";
		break;
	    case 0xff:		/* Application Extension */
		str = "Application Extension";
		break;
	    case 0xfe:		/* Comment Extension */
		str = "Comment Extension";
		while (GetDataBlock(fd, buf) != 0) {
			; /* GIF comment */
		}
		return False;
	    case 0xf9:		/* Graphic Control Extension */
		str = "Graphic Control Extension";
		(void) GetDataBlock(fd, (unsigned char*) buf);
		Gif89.disposal    = (buf[0] >> 2) & 0x7;
		Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
		Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
		if ((buf[0] & 0x1) != 0)
			Gif89.transparent = buf[3];

		while (GetDataBlock(fd, buf) != 0)
			;
		return False;
	    default:
		str = (char *) buf;
		sprintf(str, "UNKNOWN (0x%02x)", label);
		break;
	}

	while (GetDataBlock(fd, buf) != 0)
		;

	return False;
}

int	ZeroDataBlock = False;

static int
GetDataBlock(fd, buf)
FILE		*fd;
unsigned char 	*buf;
{
	unsigned char	count;

	/* error in getting DataBlock size */
	if (! ReadOK(fd,&count,1)) {
		return -1;
	}

	ZeroDataBlock = count == 0;

	/* error in reading DataBlock */
	if ((count != 0) && (! ReadOK(fd, buf, count))) {
		return -1;
	}

	return count;
}

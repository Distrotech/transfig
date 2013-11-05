/*
 * TransFig: Facility for Translating Fig code
 *
 * (C) Thomas Merz 1994-2002
 * Used with permission 19-03-2002
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

/* --------------------------------------------------------------------
 * jpeg2ps
 * convert JPEG files to compressed PostScript Level 2 or 3 EPS
 *
 * (C) Thomas Merz 1994-2002
 * Adapted from Version 1.9 for fig2dev by Brian V. Smith 19-03-2002
 *
 * ------------------------------------------------------------------*/

#include "fig2dev.h"
#include "object.h"
#include "psimage.h"

extern void	 close_picfile();
extern FILE	*open_picfile();

#ifdef REQUIRES_GETOPT
int      getopt(int nargc, char **nargv, char *ostr);
#endif

int Margin     	= 20;           /* safety margin */
Boolean quiet	= False;	/* suppress informational messages */
Boolean autorotate = False;	/* disable automatic rotation */

extern	Boolean	AnalyzeJPEG(imagedata * image);
extern	int	ASCII85Encode(FILE *in, FILE *out);
extern	void    ASCIIHexEncode(FILE *in, FILE *out);

#define BUFFERSIZE 1024
static char buffer[BUFFERSIZE];
static char *ColorSpaceNames[] = {"", "Gray", "", "RGB", "CMYK" };

static imagedata image;

/* only read the jpeg file header to get the pertinent info and put in pic struct */

int
read_jpg(file, filetype, pic, llx, lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{

	/* setup internal header */
	image.mode	= ASCII85;	/* we won't use the BINARY mode */
	image.dpi	= DPI_IGNORE;	/* ignore any DPI info from JPEG file */
	image.adobe	= False;
	image.fp	= file;
	image.filename	= pic->file;

	/* read image parameters and fill image struct */
	if (!AnalyzeJPEG(&image)) {
	    fprintf(stderr, "Error: '%s' is not a proper JPEG file!\n", image.filename);
	    return 0;
	}

	*llx = *lly = 0;
	pic->transp = -1;
	pic->bit_size.x = image.width;
	pic->bit_size.y = image.height;

	/* output PostScript comment */
	fprintf(tfp, "%% Begin Imported JPEG File: %s\n\n", pic->file);

	/* number of colors, size and bitmap is put in by read_JPEG_file() */
	pic->subtype = P_JPEG;
	return 1;			/* all ok */
}

/* here's where we read the rest of the jpeg file and format for PS */

void 
JPEGtoPS(char *jpegfile, FILE *PSfile) {
  imagedata	*JPEG;
  size_t	 n;
  time_t	 t;
  int		 i, filtype;
  char		 realname[PATH_MAX];

  JPEG = &image;

  /* reopen the file */
  JPEG->fp = open_picfile(jpegfile, &filtype, True, realname);

  time(&t);

  /* produce EPS header comments */
  fprintf(PSfile, "%%!PS-Adobe-3.0 EPSF-3.0\n");
  fprintf(PSfile, "%%%%Creator: jpeg2ps %s by Thomas Merz\n", VERSION);
  fprintf(PSfile, "%%%%Title: %s\n", JPEG->filename);
  fprintf(PSfile, "%%%%CreationDate: %s", ctime(&t));
  fprintf(PSfile, "%%%%BoundingBox: %d %d %d %d\n", 
                   0, 0, JPEG->width, JPEG->height);
  fprintf(PSfile, "%%%%DocumentData: %s\n", 
                  JPEG->mode == BINARY ? "Binary" : "Clean7Bit");
  fprintf(PSfile, "%%%%LanguageLevel: 2\n");
  fprintf(PSfile, "%%%%EndComments\n");
  fprintf(PSfile, "%%%%BeginProlog\n");
  fprintf(PSfile, "%%%%EndProlog\n");

  fprintf(PSfile, "/languagelevel where {pop languagelevel 2 lt}");
  fprintf(PSfile, "{true} ifelse {\n");
  fprintf(PSfile, "  (JPEG file '%s' needs PostScript Level 2!",
                  JPEG->filename);
  fprintf(PSfile, "\\n) dup print flush\n");
  fprintf(PSfile, "  /Helvetica findfont 20 scalefont setfont ");
  fprintf(PSfile, "100 100 moveto show showpage stop\n");
  fprintf(PSfile, "} if\n");

  fprintf(PSfile, "save\n");
  fprintf(PSfile, "/RawData currentfile ");

  if (JPEG->mode == ASCIIHEX)            /* hex representation... */
    fprintf(PSfile, "/ASCIIHexDecode filter ");
  else if (JPEG->mode == ASCII85)        /* ...or ASCII85         */
    fprintf(PSfile, "/ASCII85Decode filter ");
  /* else binary mode: don't use any additional filter! */

  fprintf(PSfile, "def\n");

  fprintf(PSfile, "/Data RawData << ");
  fprintf(PSfile, ">> /DCTDecode filter def\n");

  fprintf(PSfile, "/Device%s setcolorspace\n", 
                  ColorSpaceNames[JPEG->components]);
  fprintf(PSfile, "{ << /ImageType 1\n");
  fprintf(PSfile, "     /Width %d\n", JPEG->width);
  fprintf(PSfile, "     /Height %d\n", JPEG->height);
  fprintf(PSfile, "     /ImageMatrix [ %d 0 0 %d 0 %d ]\n",
                  JPEG->width, -JPEG->height, JPEG->height);
  fprintf(PSfile, "     /DataSource Data\n");
  fprintf(PSfile, "     /BitsPerComponent %d\n", 
                  JPEG->bits_per_component);

  /* workaround for color-inverted CMYK files produced by Adobe Photoshop:
   * compensate for the color inversion in the PostScript code
   */
  if (JPEG->adobe && JPEG->components == 4) {
    if (!quiet)
	fprintf(stderr, "Note: Adobe-conforming CMYK file - applying workaround for color inversion.\n");
    fprintf(PSfile, "     /Decode [1 0 1 0 1 0 1 0]\n");
  }else {
    fprintf(PSfile, "     /Decode [0 1");
    for (i = 1; i < JPEG->components; i++) 
      fprintf(PSfile," 0 1");
    fprintf(PSfile, "]\n");
  }

  fprintf(PSfile, "  >> image\n");
  fprintf(PSfile, "  Data closefile\n");
  fprintf(PSfile, "  RawData flushfile\n");
  fprintf(PSfile, "  showpage\n");
  fprintf(PSfile, "  restore\n");
  fprintf(PSfile, "} exec");

  switch (JPEG->mode) {
	case BINARY:
	    /* important: ONE blank and NO newline */
	    fprintf(PSfile, " ");
	#if defined(DOS)
	    fflush(PSfile);         	  /* up to now we have CR/NL mapping */
	    setmode(fileno(PSfile), O_BINARY);    /* continue in binary mode */
	#endif
	    /* copy data without change */
	    while ((n = fread(buffer, 1, sizeof(buffer), JPEG->fp)) != 0)
	      fwrite(buffer, 1, n, PSfile);
	#if defined(DOS)
	    fflush(PSfile);                  	/* binary yet */
	    setmode(fileno(PSfile), O_TEXT);    /* text mode */
	#endif
	    break;

	case ASCII85:
	    fprintf(PSfile, "\n");

	    /* ASCII85 representation of image data */
	    if (ASCII85Encode(JPEG->fp, PSfile)) {
	      fprintf(stderr, "Error: internal problems with ASCII85Encode!\n");
	      exit(1);
	    }
	    break;

	case ASCIIHEX:
	    /* hex representation of image data (useful for buggy dvips) */
	    ASCIIHexEncode(JPEG->fp, PSfile);
	    break;
    }

    /* close the jpeg file */
    close_picfile(JPEG->fp, filtype);
}

/* The following enum is stolen from the IJG JPEG library
 * Comments added by tm
 * This table contains far too many names since jpeg2ps
 * is rather simple-minded about markers
 */

extern Boolean quiet;

typedef enum {		/* JPEG marker codes			*/
  M_SOF0  = 0xc0,	/* baseline DCT				*/
  M_SOF1  = 0xc1,	/* extended sequential DCT		*/
  M_SOF2  = 0xc2,	/* progressive DCT			*/
  M_SOF3  = 0xc3,	/* lossless (sequential)		*/
  
  M_SOF5  = 0xc5,	/* differential sequential DCT		*/
  M_SOF6  = 0xc6,	/* differential progressive DCT		*/
  M_SOF7  = 0xc7,	/* differential lossless		*/
  
  M_JPG   = 0xc8,	/* JPEG extensions			*/
  M_SOF9  = 0xc9,	/* extended sequential DCT		*/
  M_SOF10 = 0xca,	/* progressive DCT			*/
  M_SOF11 = 0xcb,	/* lossless (sequential)		*/
  
  M_SOF13 = 0xcd,	/* differential sequential DCT		*/
  M_SOF14 = 0xce,	/* differential progressive DCT		*/
  M_SOF15 = 0xcf,	/* differential lossless		*/
  
  M_DHT   = 0xc4,	/* define Huffman tables		*/
  
  M_DAC   = 0xcc,	/* define arithmetic conditioning table	*/
  
  M_RST0  = 0xd0,	/* restart				*/
  M_RST1  = 0xd1,	/* restart				*/
  M_RST2  = 0xd2,	/* restart				*/
  M_RST3  = 0xd3,	/* restart				*/
  M_RST4  = 0xd4,	/* restart				*/
  M_RST5  = 0xd5,	/* restart				*/
  M_RST6  = 0xd6,	/* restart				*/
  M_RST7  = 0xd7,	/* restart				*/
  
  M_SOI   = 0xd8,	/* start of image			*/
  M_EOI   = 0xd9,	/* end of image				*/
  M_SOS   = 0xda,	/* start of scan			*/
  M_DQT   = 0xdb,	/* define quantization tables		*/
  M_DNL   = 0xdc,	/* define number of lines		*/
  M_DRI   = 0xdd,	/* define restart interval		*/
  M_DHP   = 0xde,	/* define hierarchical progression	*/
  M_EXP   = 0xdf,	/* expand reference image(s)		*/
  
  M_APP0  = 0xe0,	/* application marker, used for JFIF	*/
  M_APP1  = 0xe1,	/* application marker			*/
  M_APP2  = 0xe2,	/* application marker			*/
  M_APP3  = 0xe3,	/* application marker			*/
  M_APP4  = 0xe4,	/* application marker			*/
  M_APP5  = 0xe5,	/* application marker			*/
  M_APP6  = 0xe6,	/* application marker			*/
  M_APP7  = 0xe7,	/* application marker			*/
  M_APP8  = 0xe8,	/* application marker			*/
  M_APP9  = 0xe9,	/* application marker			*/
  M_APP10 = 0xea,	/* application marker			*/
  M_APP11 = 0xeb,	/* application marker			*/
  M_APP12 = 0xec,	/* application marker			*/
  M_APP13 = 0xed,	/* application marker			*/
  M_APP14 = 0xee,	/* application marker, used by Adobe	*/
  M_APP15 = 0xef,	/* application marker			*/
  
  M_JPG0  = 0xf0,	/* reserved for JPEG extensions		*/
  M_JPG13 = 0xfd,	/* reserved for JPEG extensions		*/
  M_COM   = 0xfe,	/* comment				*/
  
  M_TEM   = 0x01,	/* temporary use			*/

  M_ERROR = 0x100	/* dummy marker, internal use only	*/
} JPEG_MARKER;

/*
 * The following routine used to be a macro in its first incarnation:
 *  #define get_2bytes(fp) ((unsigned int) (getc(fp) << 8) + getc(fp))
 * However, this is bad programming since C doesn't guarantee
 * the evaluation order of the getc() calls! As suggested by
 * Murphy's law, there are indeed compilers which produce the wrong
 * order of the getc() calls, e.g. the Metrowerks C compilers for BeOS
 * and Macintosh.
 * Since there are only few calls we don't care about the performance 
 * penalty and use a simplistic C function.
 */

/* read two byte parameter, MSB first */
static unsigned int
get_2bytes(FILE *fp) {
    unsigned int val;
    val = (unsigned int) (getc(fp) << 8);
    val += (unsigned int) (getc(fp));
    return val;
}

static int 
next_marker(FILE *fp) { /* look for next JPEG Marker  */
  int c, nbytes = 0;

  if (feof(fp))
    return M_ERROR;                 /* dummy marker               */

  do {
    do {                            /* skip to FF 		  */
      nbytes++;
      c = getc(fp);
    } while (c != 0xFF);
    do {                            /* skip repeated FFs  	  */
      c = getc(fp);
    } while (c == 0xFF);
  } while (c == 0);                 /* repeat if FF/00 	      	  */

  return c;
}

/* analyze JPEG marker */

Boolean
AnalyzeJPEG(imagedata *image) {
  int b, c, unit;
  unsigned long i, length = 0;
#define APP_MAX 255
  unsigned char appstring[APP_MAX];
  Boolean SOF_done = False;

  /* process JPEG markers */
  while (!SOF_done && (c = next_marker(image->fp)) != M_EOI) {
    switch (c) {
      case M_ERROR:
	fprintf(stderr, "Error: unexpected end of JPEG file!\n");
	return False;

      /* The following are not officially supported in PostScript level 2 */
      case M_SOF2:
      case M_SOF3:
      case M_SOF5:
      case M_SOF6:
      case M_SOF7:
      case M_SOF9:
      case M_SOF10:
      case M_SOF11:
      case M_SOF13:
      case M_SOF14:
      case M_SOF15:
	fprintf(stderr, 
         "Warning: JPEG file uses compression method %X - proceeding anyway.\n", c);
        fprintf(stderr, "PostScript output does not work on all PS interpreters!\n");
	/* FALLTHROUGH */

      case M_SOF0:
      case M_SOF1:
	length = get_2bytes(image->fp);    /* read segment length  */

	image->bits_per_component = getc(image->fp);
	image->height             = (int) get_2bytes(image->fp);
	image->width              = (int) get_2bytes(image->fp);
	image->components         = getc(image->fp);

	SOF_done = True;
	break;

      case M_APP0:		/* check for JFIF marker with resolution */
	length = get_2bytes(image->fp);

	for (i = 0; i < length-2; i++) {	/* get contents of marker */
	  b = getc(image->fp);
	  if (i < APP_MAX)			/* store marker in appstring */
	    appstring[i] = (unsigned char) b;
	}

	/* Check for JFIF application marker and read density values
	 * per JFIF spec version 1.02.
	 * We only check X resolution, assuming X and Y resolution are equal.
	 * Use values only if resolution not preset by user or to be ignored.
	 */

#define ASPECT_RATIO	0	/* JFIF unit byte: aspect ratio only */
#define DOTS_PER_INCH	1	/* JFIF unit byte: dots per inch     */
#define DOTS_PER_CM	2	/* JFIF unit byte: dots per cm       */

	if (image->dpi == DPI_USE_FILE && length >= 14 &&
	    !strncmp((const char *)appstring, "JFIF", 4)) {
	  unit = appstring[7];		        /* resolution unit */
	  					/* resolution value */
	  image->dpi = (float) ((appstring[8]<<8) + appstring[9]);

	  if (image->dpi == 0.0) {
	    image->dpi = DPI_USE_FILE;
	    break;
	  }

	  switch (unit) {
	    /* tell the caller we didn't find a resolution value */
	    case ASPECT_RATIO:
	      image->dpi = DPI_USE_FILE;
	      break;

	    case DOTS_PER_INCH:
	      break;

	    case DOTS_PER_CM:
	      image->dpi *= (float) 2.54;
	      break;

	    default:				/* unknown ==> ignore */
	      fprintf(stderr, 
		"Warning: JPEG file contains unknown JFIF resolution unit - ignored!\n");
	      image->dpi = DPI_IGNORE;
	      break;
	  }
	}
        break;

      case M_APP14:				/* check for Adobe marker */
	length = get_2bytes(image->fp);

	for (i = 0; i < length-2; i++) {	/* get contents of marker */
	  b = getc(image->fp);
	  if (i < APP_MAX)			/* store marker in appstring */
	    appstring[i] = (unsigned char) b;
	}

	/* Check for Adobe application marker. It is known (per Adobe's TN5116)
	 * to contain the string "Adobe" at the start of the APP14 marker.
	 */
	if (length >= 12 && !strncmp((const char *) appstring, "Adobe", 5))
	  image->adobe = True;			/* set Adobe flag */

	break;

      case M_SOI:		/* ignore markers without parameters */
      case M_EOI:
      case M_TEM:
      case M_RST0:
      case M_RST1:
      case M_RST2:
      case M_RST3:
      case M_RST4:
      case M_RST5:
      case M_RST6:
      case M_RST7:
	break;

      default:			/* skip variable length markers */
	length = get_2bytes(image->fp);
	for (length -= 2; length > 0; length--)
	  (void) getc(image->fp);
	break;
    }
  }

  /* do some sanity checks with the parameters */
  if (image->height <= 0 || image->width <= 0 || image->components <= 0) {
    fprintf(stderr, "Error: DNL marker not supported in PostScript Level 2!\n");
    return False;
  }

  /* some broken JPEG files have this but they print anyway... */
  if (length != (unsigned int) (image->components * 3 + 8))
    fprintf(stderr, "Warning: SOF marker has incorrect length - ignored!\n");

  if (image->bits_per_component != 8) {
    fprintf(stderr, "Error: %d bits per color component ", image->bits_per_component);
    fprintf(stderr, "not supported in PostScript level 2!\n");
    return False;
  }

  if (image->components!=1 && image->components!=3 && image->components!=4) {
    fprintf(stderr, "Error: unknown color space (%d components)!\n", image->components);
    return False;
  }

  return True;
}

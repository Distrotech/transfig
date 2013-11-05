/*
 * TransFig: Facility for Translating Fig code
 *
 * (C) Thomas Merz 1994-2002
 * Used with permission 19-03-2002
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

/* data output mode: binary, ascii85, hex-ascii */
typedef enum { BINARY, ASCII85, ASCIIHEX } DATAMODE;

typedef struct {
  FILE     *fp;                   /* file pointer for jpeg file		 */
  char     *filename;             /* name of image file			 */
  int      width;                 /* pixels per line			 */
  int      height;                /* rows				 */
  int      components;            /* number of color components		 */
  int      bits_per_component;    /* bits per color component		 */
  float    dpi;                   /* image resolution in dots per inch   */
  DATAMODE mode;                  /* output mode: 8bit, ascii, ascii85	 */
  Boolean  adobe;                 /* image includes Adobe comment marker */
} imagedata;

#define	DPI_IGNORE (float) (-1.0) /* dummy value for imagedata.dpi       */
#define DPI_USE_FILE ((float) 0.0)/* dummy value for imagedata.dpi       */

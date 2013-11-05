/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1999 by Philippe Bekaert
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
 * genemf.h -- convert fig to Enhanced MetaFile header file
 *
 * Written by Michael Schrick (2001-03-04)
 *
 */

#ifndef GENEMF_H
#define GENEMF_H

typedef unsigned char  uchar;
typedef unsigned short TCHAR;

/* 16bit value */
typedef short		EMFshort;
typedef unsigned short	EMFushort;

/* 32bit value (use int, not long, for LP64 platforms) */
typedef int		EMFlong;
typedef unsigned int	EMFulong;

/* 32bit floating value */
typedef union {
	EMFulong	bit_uint;	/* bit pattern */
	/* float	val_float; */	/* floating value (not portable) */
} EMFfloat;


/*
 * Macros for conversion between host and file byte order.
 */

#ifndef BYTE_ORDER
# define BIG_ENDIAN	4321
# define LITTLE_ENDIAN	1234

# if defined(__BIG_ENDIAN__) || defined(m68k) || defined(__m68k__) || defined(sparc) || defined(__sparc) || defined(__sparc__) || defined(hppa) || defined(__hppa) || defined(MIPSEB) || defined(__ARMEB__)
#  define BYTE_ORDER	BIG_ENDIAN

# else

#  if defined(__LITTLE_ENDIAN__) || defined(vax) || defined(__vax__) || defined(__alpha) || defined(__alpha__) || defined(MIPSEL) || defined(ns32k) || defined(__ARMEL__) || defined(i386) || defined(__i386__)
#   define BYTE_ORDER	LITTLE_ENDIAN
#  endif

# endif
#endif

#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)

# define htofs(x) (x)
# define htofl(x) (x)

# define ftohs(x) (x)
# define ftohl(x) (x)

#else
# if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)

#  define htofs(x) ((EMFushort)( \
		(((EMFushort)(x) & (EMFushort)0x00ffU) << 8) | \
		(((EMFushort)(x) & (EMFushort)0xff00U) >> 8) ))
#  define htofl(x) ((EMFulong)( \
		(((EMFulong)(x) & (EMFulong)0x000000ffUL) << 24) | \
		(((EMFulong)(x) & (EMFulong)0x0000ff00UL) <<  8) | \
		(((EMFulong)(x) & (EMFulong)0x00ff0000UL) >>  8) | \
		(((EMFulong)(x) & (EMFulong)0xff000000UL) >> 24) ))

#  define ftohs(x) (htofs(x))
#  define ftohl(x) (htofl(x))

# else

/* Unknwon endian.  This is not efficient but should work. */
#  include <netinet/in.h>

#  define htofs(x) ((EMFushort)htons( \
		(((EMFushort)(x) & (EMFushort)0x00ffU) << 8) | \
		(((EMFushort)(x) & (EMFushort)0xff00U) >> 8) ))
#  define htofl(x) ((EMFulong)htonl( \
		(((EMFulong)(x) & (EMFulong)0x000000ffUL) << 24) | \
		(((EMFulong)(x) & (EMFulong)0x0000ff00UL) <<  8) | \
		(((EMFulong)(x) & (EMFulong)0x00ff0000UL) >>  8) | \
		(((EMFulong)(x) & (EMFulong)0xff000000UL) >> 24) ))

#  define ftohs(x) ((EMFushort)( \
		(((EMFushort)ntohs(x) & (EMFushort)0x00ffU) << 8) | \
		(((EMFushort)ntohs(x) & (EMFushort)0xff00U) >> 8) ))
#  define ftohl(x) ((EMFulong)( \
		(((EMFulong)ntohl(x) & (EMFulong)0x000000ffUL) << 24) | \
		(((EMFulong)ntohl(x) & (EMFulong)0x0000ff00UL) <<  8) | \
		(((EMFulong)ntohl(x) & (EMFulong)0x00ff0000UL) >>  8) | \
		(((EMFulong)ntohl(x) & (EMFulong)0xff000000UL) >> 24) ))

# endif
#endif

/* for efficiency (evaluate src only once) */
#define HTOFS(dst, src) \
	do { EMFushort htofstmp = (src); dst = htofs(htofstmp); } while (0)
#define HTOFL(dst, src) \
	do { EMFulong htofltmp = (src); dst = htofl(htofltmp); } while (0)


#define LATEX_FONT_BASE	2	/* Index of first LaTeX-like text font */
#define NUM_LATEX_FONTS	5	/* Number of LaTeX like text fonts */
#define PS_FONT_BASE	7	/* Index of first PostScript text font */
#define NUM_PS_FONTS	35	/* Number of PostScript fonts */

#define PIXEL_01MM	(2.11666666667)	/* Converts from pixels to 0.01 mm */
#define PIXEL_01MM_BITPATTERN	(0x40077777)	/* as bit pattern */

#define RAD_01DEG (572.95780)		/* Converts from radians to 0.1 deg */

#define OUT_DEFAULT_PRECIS	0
#define OUT_STRING_PRECIS	1
#define OUT_CHARACTER_PRECIS	2
#define OUT_STROKE_PRECIS	3
#define OUT_TT_PRECIS		4
#define OUT_DEVICE_PRECIS	5
#define OUT_RASTER_PRECIS	6
#define OUT_TT_ONLY_PRECIS	7
#define OUT_OUTLINE_PRECIS	8

#define CLIP_DEFAULT_PRECIS	0
#define CLIP_CHARACTER_PRECIS	1
#define CLIP_STROKE_PRECIS	2
#define CLIP_MASK		0xf
#define CLIP_LH_ANGLES		(1<<4)
#define CLIP_TT_ALWAYS		(2<<4)
#define CLIP_EMBEDDED		(8<<4)

#define DEFAULT_QUALITY		0
#define DRAFT_QUALITY		1
#define PROOF_QUALITY		2

#define DEFAULT_PITCH		0
#define FIXED_PITCH		1
#define VARIABLE_PITCH		2

#define ANSI_CHARSET		0
#define DEFAULT_CHARSET		1
#define SYMBOL_CHARSET		2
#define SHIFTJIS_CHARSET	128
#define HANGUL_CHARSET		129
#define GB2312_CHARSET		134
#define CHINESEBIG5_CHARSET	136
#define OEM_CHARSET		255

/* Font Families */
#define FF_DONTCARE	(0<<4)	/* Don't care or don't know. */
#define FF_ROMAN	(1<<4)	/* Variable stroke width, serifed. */
				/* Times Roman, Century Schoolbook, etc. */
#define FF_SWISS	(2<<4)	/* Variable stroke width, sans-serifed. */
				/* Helvetica, Swiss, etc. */
#define FF_MODERN	(3<<4)	/* Constant stroke width, serifed or */
				/* sans-serifed. Pica, Elite, Courier, etc. */
#define FF_SCRIPT	(4<<4)	/* Cursive, etc. */
#define FF_DECORATIVE	(5<<4)	/* Old English, etc. */

/* Font Weights */
#define FW_DONTCARE		0
#define FW_THIN			100
#define FW_EXTRALIGHT		200
#define FW_LIGHT		300
#define FW_NORMAL		400
#define FW_MEDIUM		500
#define FW_SEMIBOLD		600
#define FW_BOLD			700
#define FW_EXTRABOLD		800
#define FW_HEAVY		900

#define FW_ULTRALIGHT		FW_EXTRALIGHT
#define FW_REGULAR		FW_NORMAL
#define FW_DEMIBOLD		FW_SEMIBOLD
#define FW_ULTRABOLD		FW_EXTRABOLD
#define FW_BLACK		FW_HEAVY

#define PANOSE_COUNT		       10
#define PAN_FAMILYTYPE_INDEX		0
#define PAN_SERIFSTYLE_INDEX		1
#define PAN_WEIGHT_INDEX		2
#define PAN_PROPORTION_INDEX		3
#define PAN_CONTRAST_INDEX		4
#define PAN_STROKEVARIATION_INDEX	5
#define PAN_ARMSTYLE_INDEX		6
#define PAN_LETTERFORM_INDEX		7
#define PAN_MIDLINE_INDEX		8
#define PAN_XHEIGHT_INDEX		9

#define PAN_CULTURE_LATIN		0

/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* Binary raster ops */
#define R2_BLACK	1	/*  0       */
#define R2_NOTMERGEPEN	2	/* DPon     */
#define R2_MASKNOTPEN	3	/* DPna     */
#define R2_NOTCOPYPEN	4	/* PN       */
#define R2_MASKPENNOT	5	/* PDna     */
#define R2_NOT		6	/* Dn       */
#define R2_XORPEN	7	/* DPx      */
#define R2_NOTMASKPEN	8	/* DPan     */
#define R2_MASKPEN	9	/* DPa      */
#define R2_NOTXORPEN	10	/* DPxn     */
#define R2_NOP		11	/* D        */
#define R2_MERGENOTPEN	12	/* DPno     */
#define R2_COPYPEN	13	/* P        */
#define R2_MERGEPENNOT	14	/* PDno     */
#define R2_MERGEPEN	15	/* DPo      */
#define R2_WHITE	16	/*  1       */
#define R2_LAST		16


/* palette entry flags */

#define RGB(r,g,b)		( (EMFulong)( (uchar)(r) | \
				  (((uchar)(g))<<8) | \
				  (((uchar)(b))<<16) ) )
#define PALETTERGB(r,g,b)	(0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i)		((EMFulong)(0x01000000 | (EMFushort)(i)))

#define PC_RESERVED	0x01	/* Palette index used for animation */
#define PC_EXPLICIT	0x02	/* Palette index is explicit to device */
#define PC_NOCOLLAPSE	0x04	/* Do not match color to system palette */

#define GetRValue(rgb)	((uchar)(rgb))
#define GetGValue(rgb)	((uchar)((rgb) >> 8))
#define GetBValue(rgb)	((uchar)((rgb) >> 16))

/* Background Modes */
#define TRANSPARENT	1
#define OPAQUE		2
#define BKMODE_LAST	2

/* Graphics Modes */

#define GM_COMPATIBLE	1
#define GM_ADVANCED	2
#define GM_LAST		2

/* PolyDraw and GetPath point types */
#define PT_CLOSEFIGURE		0x01
#define PT_LINETO		0x02
#define PT_BEZIERTO		0x04
#define PT_MOVETO		0x06

/* Mapping Modes */
#define MM_TEXT			1
#define MM_LOMETRIC		2
#define MM_HIMETRIC		3
#define MM_LOENGLISH		4
#define MM_HIENGLISH		5
#define MM_TWIPS		6
#define MM_ISOTROPIC		7
#define MM_ANISOTROPIC		8

/* Min and Max Mapping Mode values */
#define MM_MIN			MM_TEXT
#define MM_MAX			MM_ANISOTROPIC
#define MM_MAX_FIXEDSCALE	MM_TWIPS

/* Coordinate Modes */
#define ABSOLUTE		1
#define RELATIVE		2

/* Stock Logical Objects */
#define WHITE_BRUSH		0
#define LTGRAY_BRUSH		1
#define GRAY_BRUSH		2
#define DKGRAY_BRUSH		3
#define BLACK_BRUSH		4
#define NULL_BRUSH		5
#define HOLLOW_BRUSH		NULL_BRUSH
#define WHITE_PEN		6
#define BLACK_PEN		7
#define NULL_PEN		8
#define OEM_FIXED_FONT		10
#define ANSI_FIXED_FONT		11
#define ANSI_VAR_FONT		12
#define SYSTEM_FONT		13
#define DEVICE_DEFAULT_FONT	14
#define DEFAULT_PALETTE		15
#define SYSTEM_FIXED_FONT	16
#define DEFAULT_GUI_FONT	17

#define STOCK_LAST		19

#define CLR_INVALID	0xFFFFFFFF

/* Brush Styles */
#define BS_SOLID		0
#define BS_NULL			1
#define BS_HOLLOW		BS_NULL
#define BS_HATCHED		2
#define BS_PATTERN		3
#define BS_INDEXED		4
#define BS_DIBPATTERN		5
#define BS_DIBPATTERNPT		6
#define BS_PATTERN8X8		7
#define BS_DIBPATTERN8X8	8

/* Hatch Styles */
#define HS_HORIZONTAL		0	/* ----- */
#define HS_VERTICAL		1	/* ||||| */
#define HS_FDIAGONAL		2	/* \\\\\ */
#define HS_BDIAGONAL		3	/* ///// */
#define HS_CROSS		4	/* +++++ */
#define HS_DIAGCROSS		5	/* xxxxx */

/* Pen Styles */
#define PS_SOLID		0
#define PS_DASH			1	/* -------  */
#define PS_DOT			2	/* .......  */
#define PS_DASHDOT		3	/* _._._._  */
#define PS_DASHDOTDOT		4	/* _.._.._  */
#define PS_NULL			5
#define PS_INSIDEFRAME		6
#define PS_USERSTYLE		7
#define PS_ALTERNATE		8
#define PS_STYLE_MASK		0x0000000F

#define PS_ENDCAP_ROUND		0x00000000
#define PS_ENDCAP_SQUARE	0x00000100
#define PS_ENDCAP_FLAT		0x00000200
#define PS_ENDCAP_MASK		0x00000F00

#define PS_JOIN_ROUND		0x00000000
#define PS_JOIN_BEVEL		0x00001000
#define PS_JOIN_MITER		0x00002000
#define PS_JOIN_MASK		0x0000F000

#define PS_COSMETIC		0x00000000
#define PS_GEOMETRIC		0x00010000
#define PS_TYPE_MASK		0x000F0000

#define AD_COUNTERCLOCKWISE	1
#define AD_CLOCKWISE		2

/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

typedef struct tagSIZEL {
    EMFlong	cx;
    EMFlong	cy;
} SIZEL;

typedef struct tagPOINTS {
    EMFshort	x;
    EMFshort	y;
} POINTS;

typedef struct tagPOINTL {
    EMFlong	x;
    EMFlong	y;
} POINTL;

typedef struct tagRECTL {
    EMFlong	left;
    EMFlong	top;
    EMFlong	right;
    EMFlong	bottom;
} RECTL;

typedef struct tagXFORM {
    EMFfloat	eM11;
    EMFfloat	eM12;
    EMFfloat	eM21;
    EMFfloat	eM22;
    EMFfloat	eDx;
    EMFfloat	eDy;
} XFORM;

/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* Enhanced Metafile structures */
typedef struct tagENHMETARECORD {
    EMFulong	iType;		/* Record type EMR_XXX */
    EMFulong	nSize;		/* Record size in bytes */
    EMFulong	dParm[1];	/* Parameters */
} ENHMETARECORD;

typedef struct tagENHMETAHEADER {
    EMFulong	iType;		/* Record type EMR_HEADER */
    EMFulong	nSize;		/* Record size in bytes.  This may be greater */
				/*  than the sizeof(ENHMETAHEADER). */
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    RECTL	rclFrame;	/* Inclusive-inclusive Picture Frame of */
				/*  metafile in .01 mm units */
    EMFulong	dSignature;	/* Signature.  Must be ENHMETA_SIGNATURE. */
    EMFulong	nVersion;	/* Version number */
    EMFulong	nBytes;		/* Size of the metafile in bytes */
    EMFulong	nRecords;	/* Number of records in the metafile */
    EMFushort	nHandles;	/* Number of handles in the handle table */
				/* Handle index zero is reserved. */
    EMFushort	sReserved;	/* Reserved.  Must be zero. */
    EMFulong	nDescription;	/* Number of chars in the unicode */
				/*  description string */
			/* This is 0 if there is no description string */
    EMFulong	offDescription;	/* Offset to the metafile description record. */
			/* This is 0 if there is no description string */
    EMFulong	nPalEntries;	/* Number of entries in the metafile palette. */
    SIZEL	szlDevice;	/* Size of the reference device in pels */
    SIZEL	szlMillimeters;	/* Size of the reference device in millimeters */
} ENHMETAHEADER;

/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
 * The next structures the logical color space. Unlike pens and brushes,
 * but like palettes, there is only one way to create a LogColorSpace.
 * A pointer to it must be passed, its elements can't be pushed as
 * arguments.
 */
#define MAX_PATH	260

typedef EMFlong LCSCSTYPE;
typedef EMFlong LCSGAMUTMATCH;
typedef EMFlong FXPT2DOT30;

typedef struct tagCIEXYZ {
    FXPT2DOT30	ciexyzX;
    FXPT2DOT30	ciexyzY;
    FXPT2DOT30	ciexyzZ;
} CIEXYZ;

typedef struct tagCIEXYZTRIPLE {
    CIEXYZ	ciexyzRed;
    CIEXYZ	ciexyzGreen;
    CIEXYZ	ciexyzBlue;
} CIEXYZTRIPLE;

typedef struct tagLOGCOLORSPACEA {
    EMFulong	lcsSignature;
    EMFulong	lcsVersion;
    EMFulong	lcsSize;
    LCSCSTYPE	lcsCSType;
    LCSGAMUTMATCH lcsIntent;
    CIEXYZTRIPLE lcsEndpoints;
    EMFulong	lcsGammaRed;
    EMFulong	lcsGammaGreen;
    EMFulong	lcsGammaBlue;
    char	lcsFilename[MAX_PATH];
} LOGCOLORSPACEA, *LPLOGCOLORSPACEA;

typedef struct tagLOGCOLORSPACEW {
    EMFulong	lcsSignature;
    EMFulong	lcsVersion;
    EMFulong	lcsSize;
    LCSCSTYPE	lcsCSType;
    LCSGAMUTMATCH lcsIntent;
    CIEXYZTRIPLE lcsEndpoints;
    EMFulong	lcsGammaRed;
    EMFulong	lcsGammaGreen;
    EMFulong	lcsGammaBlue;
    TCHAR	lcsFilename[MAX_PATH];
} LOGCOLORSPACEW;

/* Logical Font */
#define LF_FACESIZE		32
#define LF_FULLFACESIZE		64
#define ELF_VENDOR_SIZE		4

typedef struct tagLOGFONTW
{
    EMFlong	lfHeight;
    EMFlong	lfWidth;
    EMFlong	lfEscapement;
    EMFlong	lfOrientation;
    EMFlong	lfWeight;
    uchar	lfItalic;
    uchar	lfUnderline;
    uchar	lfStrikeOut;
    uchar	lfCharSet;
    uchar	lfOutPrecision;
    uchar	lfClipPrecision;
    uchar	lfQuality;
    uchar	lfPitchAndFamily;
    TCHAR	lfFaceName[LF_FACESIZE];
} LOGFONTW;

typedef struct tagPANOSE
{
    uchar	bFamilyType;
    uchar	bSerifStyle;
    uchar	bWeight;
    uchar	bProportion;
    uchar	bContrast;
    uchar	bStrokeVariation;
    uchar	bArmStyle;
    uchar	bLetterform;
    uchar	bMidline;
    uchar	bXHeight;
    uchar	pad[2];		/* Padding to ensure data alignment */
} PANOSE;

typedef struct tagEXTLOGFONTW {
    LOGFONTW	elfLogFont;
    TCHAR	elfFullName[LF_FULLFACESIZE];
    TCHAR	elfStyle[LF_FACESIZE];
    EMFulong	elfVersion;	/* 0 for the first release */
    EMFulong	elfStyleSize;
    EMFulong	elfMatch;
    EMFulong	elfReserved;
    uchar	elfVendorId[ELF_VENDOR_SIZE];
    EMFulong	elfCulture;	/* 0 for Latin             */
    PANOSE	elfPanose;
} EXTLOGFONTW;

typedef struct tagEXTLOGPEN {
    EMFulong	elpPenStyle;
    EMFulong	elpWidth;
    EMFulong	elpBrushStyle;
    EMFulong	elpColor;
    EMFlong	elpHatch;
    EMFulong	elpNumEntries;
/*  EMFulong	elpStyleEntry[1]; */
} EXTLOGPEN;

/* Logical Brush (or Pattern) */
typedef struct tagLOGBRUSH
{
    EMFulong	lbStyle;
    EMFulong	lbColor;
    EMFlong	lbHatch;
} LOGBRUSH;

typedef struct tagPALETTEENTRY {
    uchar	peRed;
    uchar	peGreen;
    uchar	peBlue;
    uchar	peFlags;
} PALETTEENTRY;

/* Logical Palette */
typedef struct tagLOGPALETTE {
    EMFushort		palVersion;
    EMFushort		palNumEntries;
    PALETTEENTRY	palPalEntry[1];
} LOGPALETTE;

/* Logical Pen */
typedef struct tagLOGPEN
{
  EMFulong	lopnStyle;
  POINTL	lopnWidth;
  EMFulong	lopnColor;
} LOGPEN;

/* DIB color information */
#define BI_RGB		0	/* uncompressed */
#define BI_RLE8		1	/* 8 bpp run-length encoding */
#define BI_RLE4		2	/* 4 bpp run-length encoding */
#define BI_BITFIELDS	3	/* uncompressed, color mask */

typedef struct tagBITMAPINFOHEADER {
    EMFulong	biSize;
    EMFlong	biWidth;
    EMFlong	biHeight;
    EMFushort	biPlanes;
    EMFushort	biBitCount;
    EMFulong	biCompression;
    EMFulong	biSizeImage;
    EMFlong	biXPelsPerMeter;
    EMFlong	biYPelsPerMeter;
    EMFulong	biClrUsed;
    EMFulong	biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    uchar	rgbBlue;
    uchar	rgbGreen;
    uchar	rgbRed;
    uchar	rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER	bmiHeader;
/*  RGBQUAD		bmiColors[1];*/
} BITMAPINFO;

/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
 * Enhanced metafile constants.
 */
#define ENHMETA_SIGNATURE	0x464D4520
#define ENHMETA_VERSION		0x10000
/*
 * Stock object flag used in the object handle index in the enhanced
 * metafile records.
 * E.g. The object handle index (ENHMETA_STOCK_OBJECT | BLACK_BRUSH)
 * represents the stock object BLACK_BRUSH.
 */
#define ENHMETA_STOCK_OBJECT	0x80000000
/*
 * Enhanced metafile record types.
 */
#define EMR_ABORTPATH			68
#define EMR_ANGLEARC			41
#define EMR_ARC				45
#define EMR_ARCTO			55
#define EMR_BEGINPATH			59
#define EMR_BITBLT			76
#define EMR_CHORD			46
#define EMR_CLOSEFIGURE			61
#define EMR_CREATEBRUSHINDIRECT		39
#define EMR_CREATECOLORSPACE		99
#define EMR_CREATEDIBPATTERNBRUSHPT	94
#define EMR_CREATEMONOBRUSH		93
#define EMR_CREATEPALETTE		49
#define EMR_CREATEPEN			38
#define EMR_DELETECOLORSPACE	       101
#define EMR_DELETEOBJECT		40
#define EMR_ELLIPSE			42
#define EMR_ENDPATH			60
#define EMR_EOF				14
#define EMR_EXCLUDECLIPRECT		29
#define EMR_EXTCREATEFONTINDIRECTW	82
#define EMR_EXTCREATEPEN		95
#define EMR_EXTFLOODFILL		53
#define EMR_EXTSELECTCLIPRGN		75
#define EMR_EXTTEXTOUTA			83
#define EMR_EXTTEXTOUTW			84
#define EMR_FILLPATH			62
#define EMR_FILLRGN			71
#define EMR_FLATTENPATH			65
#define EMR_FRAMERGN			72
#define EMR_GDICOMMENT			70
#define EMR_HEADER			 1
#define EMR_INTERSECTCLIPRECT		30
#define EMR_INVERTRGN			73
#define EMR_LINETO			54
#define EMR_MASKBLT			78
#define EMR_MODIFYWORLDTRANSFORM	36
#define EMR_MOVETOEX			27
#define EMR_OFFSETCLIPRGN		26
#define EMR_PAINTRGN			74
#define EMR_PIE				47
#define EMR_PLGBLT			79
#define EMR_POLYBEZIER			 2
#define EMR_POLYBEZIER16		85
#define EMR_POLYBEZIERTO		 5
#define EMR_POLYBEZIERTO16		88
#define EMR_POLYDRAW			56
#define EMR_POLYDRAW16			92
#define EMR_POLYGON			 3
#define EMR_POLYGON16			86
#define EMR_POLYLINE			 4
#define EMR_POLYLINE16			87
#define EMR_POLYLINETO			 6
#define EMR_POLYLINETO16		89
#define EMR_POLYPOLYGON			 8
#define EMR_POLYPOLYGON16		91
#define EMR_POLYPOLYLINE		 7
#define EMR_POLYPOLYLINE16		90
#define EMR_POLYTEXTOUTA		96
#define EMR_POLYTEXTOUTW		97
#define EMR_REALIZEPALETTE		52
#define EMR_RECTANGLE			43
#define EMR_RESIZEPALETTE		51
#define EMR_RESTOREDC			34
#define EMR_ROUNDRECT			44
#define EMR_SAVEDC			33
#define EMR_SCALEVIEWPORTEXTEX		31
#define EMR_SCALEWINDOWEXTEX		32
#define EMR_SELECTCLIPPATH		67
#define EMR_SELECTOBJECT		37
#define EMR_SELECTPALETTE		48
#define EMR_SETARCDIRECTION		57
#define EMR_SETBKCOLOR			25
#define EMR_SETBKMODE			18
#define EMR_SETBRUSHORGEX		13
#define EMR_SETCOLORADJUSTMENT		23
#define EMR_SETCOLORSPACE	       100
#define EMR_SETDIBITSTODEVICE		80
#define EMR_SETICMMODE			98
#define EMR_SETMAPMODE			17
#define EMR_SETMAPPERFLAGS		16
#define EMR_SETMETARGN			28
#define EMR_SETMITERLIMIT		58
#define EMR_SETPALETTEENTRIES		50
#define EMR_SETPIXELV			15
#define EMR_SETPOLYFILLMODE		19
#define EMR_SETROP2			20
#define EMR_SETSTRETCHBLTMODE		21
#define EMR_SETTEXTALIGN		22
#define EMR_SETTEXTCOLOR		24
#define EMR_SETVIEWPORTEXTEX		11
#define EMR_SETVIEWPORTORGEX		12
#define EMR_SETWINDOWEXTEX		 9
#define EMR_SETWINDOWORGEX		10
#define EMR_SETWORLDTRANSFORM		35
#define EMR_STRETCHBLT			77
#define EMR_STRETCHDIBITS		81
#define EMR_STROKEANDFILLPATH		63
#define EMR_STROKEPATH			64
#define EMR_WIDENPATH			66

#define EMR_MIN				 1
#define EMR_MAX			       101


/* Text Alignment Options */
#define TA_NOUPDATECP		0
#define TA_UPDATECP		1

#define TA_LEFT			0
#define TA_RIGHT		2
#define TA_CENTER		6

#define TA_TOP			0
#define TA_BOTTOM		8
#define TA_BASELINE		24
#define TA_RTLREADING		256
#define TA_MASK		(TA_BASELINE+TA_CENTER+TA_UPDATECP+TA_RTLREADING)

#define VTA_BASELINE	TA_BASELINE
#define VTA_LEFT	TA_BOTTOM
#define VTA_RIGHT	TA_TOP
#define VTA_CENTER	TA_CENTER
#define VTA_BOTTOM	TA_RIGHT
#define VTA_TOP		TA_LEFT

#define ETO_OPAQUE		0x0002
#define ETO_CLIPPED		0x0004
#define ETO_GLYPH_INDEX		0x0010
#define ETO_RTLREADING		0x0080

#define ASPECT_FILTERING	0x0001

/* DIB color id */
#define DIB_RGB_COLORS	0
#define DIB_PAL_COLORS	1

/* raster operation codes (most common ones) */
#define SRCCOPY		0x00CC0020	/* Destination = Source */
#define SRCPAINT	0x00EE0086	/* Destination |= Source */
#define SRCAND		0x008800C6	/* Destination &= Source */
#define SRCINVERT	0x00660046	/* Destination ^= Source */

/* Base record type for the enhanced metafile. */

typedef struct tagEMR
{
    EMFulong	iType;		/* Enhanced metafile record type */
    EMFulong	nSize;		/* Length of the record in bytes */
				/* This must be a multiple of 4 */
} EMR;

/* Base text record type for the enhanced metafile. */

typedef struct tagEMRTEXT
{
    POINTL	ptlReference;
    EMFulong	nChars;
    EMFulong	offString;	/* Offset to the string */
    EMFulong	fOptions;	/* Options like ETO_OPAQUE and ETO_CLIPPED */
    RECTL	rcl;
    EMFulong	offDx;		/* Offset to the inter-character spacing array*/
				/* This is always given */
} EMRTEXT;

/* Record structures for the enhanced metafile. */

typedef struct tagABORTPATH
{
    EMR		emr;
} EMRABORTPATH, EMRBEGINPATH, EMRENDPATH, EMRCLOSEFIGURE, EMRFLATTENPATH,
  EMRWIDENPATH, EMRSETMETARGN, EMRSAVEDC, EMRREALIZEPALETTE;

typedef struct tagEMRSELECTCLIPPATH
{
    EMR		emr;
    EMFulong	iMode;
} EMRSELECTCLIPPATH, EMRSETBKMODE, EMRSETMAPMODE, EMRSETPOLYFILLMODE,
  EMRSETROP2, EMRSETSTRETCHBLTMODE, EMRSETICMMODE, EMRSETTEXTALIGN;

typedef struct tagEMRSETMITERLIMIT
{
    EMR		emr;
    EMFfloat	eMiterLimit;
} EMRSETMITERLIMIT;

typedef struct tagEMRRESTOREDC
{
    EMR		emr;
    EMFlong	iRelative;	/* Specifies a relative instance */
} EMRRESTOREDC;

typedef struct tagEMRSETARCDIRECTION
{
    EMR		emr;
    EMFulong	iArcDirection;	/* Specifies the arc direction in the */
				/* advanced graphics mode. */
} EMRSETARCDIRECTION;

typedef struct tagEMRSETMAPPERFLAGS
{
    EMR		emr;
    EMFulong	dwFlags;
} EMRSETMAPPERFLAGS;

typedef struct tagEMRSETTEXTCOLOR
{
    EMR		emr;
    EMFulong	crColor;
} EMRSETBKCOLOR, EMRSETTEXTCOLOR;

typedef struct tagEMRSELECTOBJECT
{
    EMR		emr;
    EMFulong	ihObject;	/* Object handle index */
} EMRSELECTOBJECT, EMRDELETEOBJECT;

typedef struct tagEMRSELECTCOLORSPACE
{
    EMR		emr;
    EMFulong	ihCS;		/* ColorSpace handle index */
} EMRSELECTCOLORSPACE, EMRDELETECOLORSPACE;

typedef struct tagEMRSELECTPALETTE
{
    EMR		emr;
    EMFulong	ihPal;		/* Palette handle index, background mode only */
} EMRSELECTPALETTE;

typedef struct tagEMRRESIZEPALETTE
{
    EMR		emr;
    EMFulong	ihPal;		/* Palette handle index */
    EMFulong	cEntries;
} EMRRESIZEPALETTE;

typedef struct tagEMRSETPALETTEENTRIES
{
    EMR		emr;
    EMFulong	ihPal;		/* Palette handle index */
    EMFulong	iStart;
    EMFulong	cEntries;
    PALETTEENTRY aPalEntries[1];/* The peFlags fields do not contain any flags */
} EMRSETPALETTEENTRIES;

typedef struct tagEMRSETCOLORADJUSTMENT
{
    EMR			emr;
/*  COLORADJUSTMENT	ColorAdjustment; */
} EMRSETCOLORADJUSTMENT;


typedef struct tagEMRGDICOMMENT
{
    EMR		emr;
    EMFulong	cbData;		/* Size of data in bytes */
} EMRGDICOMMENT;

typedef struct tagEMREOF
{
    EMR		emr;
    EMFulong	nPalEntries;	/* Number of palette entries */
    EMFulong	offPalEntries;	/* Offset to the palette entries */
    EMFulong	nSizeLast;	/* Same as nSize and must be the last ulong */
				/* of the record.  The palette entries, */
				/* if exist, precede this field. */
} EMREOF;

typedef struct tagEMRLINETO
{
    EMR		emr;
    POINTL	ptl;
} EMRLINETO, EMRMOVETOEX;

typedef struct tagEMROFFSETCLIPRGN
{
    EMR		emr;
    POINTL	ptlOffset;
} EMROFFSETCLIPRGN;

typedef struct tagEMRFILLPATH
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
} EMRFILLPATH, EMRSTROKEANDFILLPATH, EMRSTROKEPATH;

typedef struct tagEMREXCLUDECLIPRECT
{
    EMR		emr;
    RECTL	rclClip;
} EMREXCLUDECLIPRECT, EMRINTERSECTCLIPRECT;

typedef struct tagEMRSETVIEWPORTORGEX
{
    EMR		emr;
    POINTL	ptlOrigin;
} EMRSETVIEWPORTORGEX, EMRSETWINDOWORGEX, EMRSETBRUSHORGEX;

typedef struct tagEMRSETVIEWPORTEXTEX
{
    EMR		emr;
    SIZEL	szlExtent;
} EMRSETVIEWPORTEXTEX, EMRSETWINDOWEXTEX;

typedef struct tagEMRSCALEVIEWPORTEXTEX
{
    EMR		emr;
    EMFlong	xNum;
    EMFlong	xDenom;
    EMFlong	yNum;
    EMFlong	yDenom;
} EMRSCALEVIEWPORTEXTEX, EMRSCALEWINDOWEXTEX;

typedef struct tagEMRSETWORLDTRANSFORM
{
    EMR		emr;
    XFORM	xform;
} EMRSETWORLDTRANSFORM;

typedef struct tagEMRMODIFYWORLDTRANSFORM
{
    EMR		emr;
    XFORM	xform;
    EMFulong	iMode;
} EMRMODIFYWORLDTRANSFORM;

typedef struct tagEMRSETPIXELV
{
    EMR		emr;
    POINTL	ptlPixel;
    EMFulong	crColor;
} EMRSETPIXELV;

typedef struct tagEMREXTFLOODFILL
{
    EMR		emr;
    POINTL	ptlStart;
    EMFulong	crColor;
    EMFulong	iMode;
} EMREXTFLOODFILL;

typedef struct tagEMRELLIPSE
{
    EMR		emr;
    RECTL	rclBox;		/* Inclusive-inclusive bounding rectangle */
} EMRELLIPSE, EMRRECTANGLE;

typedef struct tagEMRROUNDRECT
{
    EMR		emr;
    RECTL	rclBox;		/* Inclusive-inclusive bounding rectangle */
    SIZEL	szlCorner;
} EMRROUNDRECT;

typedef struct tagEMRARC
{
    EMR		emr;
    RECTL	rclBox;		/* Inclusive-inclusive bounding rectangle */
    POINTL	ptlStart;
    POINTL	ptlEnd;
} EMRARC, EMRARCTO, EMRCHORD, EMRPIE;

typedef struct tagEMRANGLEARC
{
    EMR		emr;
    POINTL	ptlCenter;
    EMFulong	nRadius;
    EMFfloat	eStartAngle;
    EMFfloat	eSweepAngle;
} EMRANGLEARC;

typedef struct tagEMRPOLYLINE
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cptl;		/* Number of points in the array */
/*  POINTL	aptl[cptl];	(* Array of 32-bit points */
} EMRPOLYLINE, EMRPOLYBEZIER, EMRPOLYGON, EMRPOLYBEZIERTO, EMRPOLYLINETO;

typedef struct tagEMRPOLYLINE16
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cpts;		/* Number of points in the array */
/*  POINTS	apts[cpts];	(* Array of 16-bit points */
} EMRPOLYLINE16, EMRPOLYBEZIER16, EMRPOLYGON16, EMRPOLYBEZIERTO16,
  EMRPOLYLINETO16;

typedef struct tagEMRPOLYDRAW
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cptl;		/* Number of points */
    POINTL	aptl[1];	/* Array of points */
    uchar	abTypes[1];	/* Array of point types */
} EMRPOLYDRAW;

typedef struct tagEMRPOLYDRAW16
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cpts;		/* Number of points */
    POINTS	apts[1];	/* Array of points */
    uchar	abTypes[1];	/* Array of point types */
} EMRPOLYDRAW16;

typedef struct tagEMRPOLYPOLYLINE
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	nPolys;		/* Number of polys */
    EMFulong	cptl;		/* Total number of points in all polys */
    EMFulong	aPolyCounts[1];	/* Array of point counts for each poly */
    POINTL	aptl[1];	/* Array of points */
} EMRPOLYPOLYLINE, EMRPOLYPOLYGON;

typedef struct tagEMRPOLYPOLYLINE16
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	nPolys;		/* Number of polys */
    EMFulong	cpts;		/* Total number of points in all polys */
    EMFulong	aPolyCounts[1];	/* Array of point counts for each poly */
    POINTS	apts[1];	/* Array of points */
} EMRPOLYPOLYLINE16, EMRPOLYPOLYGON16;

typedef struct tagEMRINVERTRGN
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cbRgnData;	/* Size of region data in bytes */
    uchar	RgnData[1];
} EMRINVERTRGN, EMRPAINTRGN;

typedef struct tagEMRFILLRGN
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cbRgnData;	/* Size of region data in bytes */
    EMFulong	ihBrush;	/* Brush handle index */
    uchar	RgnData[1];
} EMRFILLRGN;

typedef struct tagEMRFRAMERGN
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	cbRgnData;	/* Size of region data in bytes */
    EMFulong	ihBrush;	/* Brush handle index */
    SIZEL	szlStroke;
    uchar	RgnData[1];
} EMRFRAMERGN;

typedef struct tagEMREXTSELECTCLIPRGN
{
    EMR		emr;
    EMFulong	cbRgnData;	/* Size of region data in bytes */
    EMFulong	iMode;
    uchar	RgnData[1];
} EMREXTSELECTCLIPRGN;

typedef struct tagEMREXTTEXTOUTA
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	iGraphicsMode;	/* Current graphics mode */
    EMFfloat	exScale;	/* X and Y scales from Page units to .01mm */
    EMFfloat	eyScale;	/*  units if graphics mode is GM_COMPATIBLE. */
    EMRTEXT	emrtext;	/* This is followed by the string and */
				/* spacing array */
} EMREXTTEXTOUTA, EMREXTTEXTOUTW;

typedef struct tagEMRPOLYTEXTOUTA
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFulong	iGraphicsMode;	/* Current graphics mode */
    EMFfloat	exScale;	/* X and Y scales from Page units to .01mm */
    EMFfloat	eyScale;	/*  units if graphics mode is GM_COMPATIBLE. */
    EMFlong	cStrings;
    EMRTEXT	aemrtext[1];	/* Array of EMRTEXT structures.  This is */
				/* followed by the strings and spacing arrays.*/
} EMRPOLYTEXTOUTA, EMRPOLYTEXTOUTW;

typedef struct tagEMRBITBLT
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFlong	xDest;
    EMFlong	yDest;
    EMFlong	cxDest;
    EMFlong	cyDest;
    EMFulong	dwRop;
    EMFlong	 xSrc;
    EMFlong	 ySrc;
    XFORM	xformSrc;	/* Source DC transform */
    EMFulong	crBkColorSrc;	/* Source DC BkColor in RGB */
    EMFulong	iUsageSrc;	/* Source bitmap info color table usage */
				/* (DIB_RGB_COLORS) */
    EMFulong	offBmiSrc;	/* Offset to the source BITMAPINFO structure */
    EMFulong	cbBmiSrc;	/* Size of the source BITMAPINFO structure */
    EMFulong	offBitsSrc;	/* Offset to the source bitmap bits */
    EMFulong	cbBitsSrc;	/* Size of the source bitmap bits */
} EMRBITBLT;

typedef struct tagEMRSTRETCHBLT
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFlong	xDest;
    EMFlong	yDest;
    EMFlong	cxDest;
    EMFlong	cyDest;
    EMFulong	dwRop;
    EMFlong	xSrc;
    EMFlong	ySrc;
    XFORM	xformSrc;	/* Source DC transform */
    EMFulong	crBkColorSrc;	/* Source DC BkColor in RGB */
    EMFulong	iUsageSrc;	/* Source bitmap info color table usage */
				/* (DIB_RGB_COLORS) */
    EMFulong	offBmiSrc;	/* Offset to the source BITMAPINFO structure */
    EMFulong	cbBmiSrc;	/* Size of the source BITMAPINFO structure */
    EMFulong	offBitsSrc;	/* Offset to the source bitmap bits */
    EMFulong	cbBitsSrc;	/* Size of the source bitmap bits */
    EMFlong	cxSrc;
    EMFlong	cySrc;
} EMRSTRETCHBLT;

typedef struct tagEMRMASKBLT
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFlong	xDest;
    EMFlong	yDest;
    EMFlong	cxDest;
    EMFlong	cyDest;
    EMFulong	dwRop;
    EMFlong	xSrc;
    EMFlong	ySrc;
    XFORM	xformSrc;	/* Source DC transform */
    EMFulong	crBkColorSrc;	/* Source DC BkColor in RGB */
    EMFulong	iUsageSrc;	/* Source bitmap info color table usage */
				/* (DIB_RGB_COLORS) */
    EMFulong	offBmiSrc;	/* Offset to the source BITMAPINFO structure */
    EMFulong	cbBmiSrc;	/* Size of the source BITMAPINFO structure */
    EMFulong	offBitsSrc;	/* Offset to the source bitmap bits */
    EMFulong	cbBitsSrc;	/* Size of the source bitmap bits */
    EMFlong	xMask;
    EMFlong	yMask;
    EMFulong	iUsageMask;	/* Mask bitmap info color table usage */
    EMFulong	offBmiMask;	/* Offset to the mask BITMAPINFO structure if any */
    EMFulong	cbBmiMask;	/* Size of the mask BITMAPINFO structure if any */
    EMFulong	offBitsMask;	/* Offset to the mask bitmap bits if any */
    EMFulong	cbBitsMask;	/* Size of the mask bitmap bits if any */
} EMRMASKBLT;

typedef struct tagEMRPLGBLT
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    POINTL	aptlDest[3];
    EMFlong	xSrc;
    EMFlong	ySrc;
    EMFlong	cxSrc;
    EMFlong	cySrc;
    XFORM	xformSrc;	/* Source DC transform */
    EMFulong	crBkColorSrc;	/* Source DC BkColor in RGB */
    EMFulong	iUsageSrc;	/* Source bitmap info color table usage */
				/* (DIB_RGB_COLORS) */
    EMFulong	offBmiSrc;	/* Offset to the source BITMAPINFO structure */
    EMFulong	cbBmiSrc;	/* Size of the source BITMAPINFO structure */
    EMFulong	offBitsSrc;	/* Offset to the source bitmap bits */
    EMFulong	cbBitsSrc;	/* Size of the source bitmap bits */
    EMFlong	xMask;
    EMFlong	yMask;
    EMFulong	iUsageMask;	/* Mask bitmap info color table usage */
    EMFulong	offBmiMask;	/* Offset to the mask BITMAPINFO structure if any */
    EMFulong	cbBmiMask;	/* Size of the mask BITMAPINFO structure if any */
    EMFulong	offBitsMask;	/* Offset to the mask bitmap bits if any */
    EMFulong	cbBitsMask;	/* Size of the mask bitmap bits if any */
} EMRPLGBLT;

typedef struct tagEMRSETDIBITSTODEVICE
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFlong	xDest;
    EMFlong	yDest;
    EMFlong	xSrc;
    EMFlong	ySrc;
    EMFlong	cxSrc;
    EMFlong	cySrc;
    EMFulong	offBmiSrc;	/* Offset to the source BITMAPINFO structure */
    EMFulong	cbBmiSrc;	/* Size of the source BITMAPINFO structure */
    EMFulong	offBitsSrc;	/* Offset to the source bitmap bits */
    EMFulong	cbBitsSrc;	/* Size of the source bitmap bits */
    EMFulong	iUsageSrc;	/* Source bitmap info color table usage */
    EMFulong	iStartScan;
    EMFulong	cScans;
} EMRSETDIBITSTODEVICE;

typedef struct tagEMRSTRETCHDIBITS
{
    EMR		emr;
    RECTL	rclBounds;	/* Inclusive-inclusive bounds in device units */
    EMFlong	xDest;
    EMFlong	yDest;
    EMFlong	xSrc;
    EMFlong	ySrc;
    EMFlong	cxSrc;
    EMFlong	cySrc;
    EMFulong	offBmiSrc;	/* Offset to the source BITMAPINFO structure */
    EMFulong	cbBmiSrc;	/* Size of the source BITMAPINFO structure */
    EMFulong	offBitsSrc;	/* Offset to the source bitmap bits */
    EMFulong	cbBitsSrc;	/* Size of the source bitmap bits */
    EMFulong	iUsageSrc;	/* Source bitmap info color table usage */
    EMFulong	dwRop;
    EMFlong	cxDest;
    EMFlong	cyDest;
} EMRSTRETCHDIBITS;

typedef struct tagEMREXTCREATEFONTINDIRECTW
{
    EMR		emr;
    EMFulong	ihFont;		/* Font handle index */
    EXTLOGFONTW	elfw;
} EMREXTCREATEFONTINDIRECTW;

typedef struct tagEMRCREATEPALETTE
{
    EMR		emr;
    EMFulong	ihPal;		/* Palette handle index */
    LOGPALETTE	lgpl;		/* The peFlags fields in the palette entries */
				/* do not contain any flags */
} EMRCREATEPALETTE;

typedef struct tagEMRCREATECOLORSPACE
{
    EMR			emr;
    EMFulong		ihCS;	/* ColorSpace handle index */
    LOGCOLORSPACEW	lcs;
} EMRCREATECOLORSPACE;

typedef struct tagEMRCREATEPEN
{
    EMR		emr;
    EMFulong	ihPen;		/* Pen handle index */
    LOGPEN	lopn;
} EMRCREATEPEN;

typedef struct tagEMREXTCREATEPEN
{
    EMR		emr;
    EMFulong	ihPen;		/* Pen handle index */
    EMFulong	offBmi;		/* Offset to the BITMAPINFO structure if any */
    EMFulong	cbBmi;		/* Size of the BITMAPINFO structure if any */
				/* The bitmap info is followed by the bitmap */
				/* bits to form a packed DIB. */
    EMFulong	offBits;	/* Offset to the brush bitmap bits if any */
    EMFulong	cbBits;		/* Size of the brush bitmap bits if any */
    EXTLOGPEN	elp;		/* The extended pen with the style array. */
} EMREXTCREATEPEN;

typedef struct tagEMRCREATEBRUSHINDIRECT
{
    EMR		emr;
    EMFulong	ihBrush;	/* Brush handle index */
    LOGBRUSH	lb;		/* The style must be BS_SOLID, BS_HOLLOW, */
				/* BS_NULL or BS_HATCHED. */
} EMRCREATEBRUSHINDIRECT;

typedef struct tagEMRCREATEMONOBRUSH
{
    EMR		emr;
    EMFulong	ihBrush;	/* Brush handle index */
    EMFulong	iUsage;		/* Bitmap info color table usage */
    EMFulong	offBmi;		/* Offset to the BITMAPINFO structure */
    EMFulong	cbBmi;		/* Size of the BITMAPINFO structure */
    EMFulong	offBits;	/* Offset to the bitmap bits */
    EMFulong	cbBits;		/* Size of the bitmap bits */
} EMRCREATEMONOBRUSH;

typedef struct tagEMRCREATEDIBPATTERNBRUSHPT
{
    EMR		emr;
    EMFulong	ihBrush;	/* Brush handle index */
    EMFulong	iUsage;		/* Bitmap info color table usage */
    EMFulong	offBmi;		/* Offset to the BITMAPINFO structure */
    EMFulong	cbBmi;		/* Size of the BITMAPINFO structure */
				/* The bitmap info is followed by the bitmap */
				/* bits to form a packed DIB. */
    EMFulong	offBits;	/* Offset to the bitmap bits */
    EMFulong	cbBits;		/* Size of the bitmap bits */
} EMRCREATEDIBPATTERNBRUSHPT;

typedef struct tagEMRFORMAT
{
    EMFulong	dSignature;	/* Format signature, e.g. ENHMETA_SIGNATURE. */
    EMFulong	nVersion;	/* Format version number. */
    EMFulong	cbData;		/* Size of data in bytes. */
    EMFulong	offData;	/* Offset to data from GDICOMMENT_IDENTIFIER. */
				/* It must begin at a ulong offset. */
} EMRFORMAT;

#endif /* GENEMF_H */

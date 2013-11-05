/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
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
 *	psfont.c : PostScript font mappings
 *
*/
#include "fig2dev.h"
#include "object.h"

char		*PSfontnames[] = {
		"Times-Roman",			/* default */ 
		"Times-Roman",
		"Times-Italic",			/* italic */
		"Times-Bold",			/* bold */
		"Times-BoldItalic",
		"AvantGarde-Book",
		"AvantGarde-BookOblique",
		"AvantGarde-Demi",
		"AvantGarde-DemiOblique",
		"Bookman-Light",
		"Bookman-LightItalic",
		"Bookman-Demi",
		"Bookman-DemiItalic",
		"Courier",	
		"Courier-Oblique",
		"Courier-Bold",
		"Courier-BoldOblique",
		"Helvetica",
		"Helvetica-Oblique",
		"Helvetica-Bold",
		"Helvetica-BoldOblique",
		"Helvetica-Narrow",
		"Helvetica-Narrow-Oblique",
		"Helvetica-Narrow-Bold",
		"Helvetica-Narrow-BoldOblique",
		"NewCenturySchlbk-Roman",
		"NewCenturySchlbk-Italic",
		"NewCenturySchlbk-Bold",
		"NewCenturySchlbk-BoldItalic",
		"Palatino-Roman",
		"Palatino-Italic",
		"Palatino-Bold",
		"Palatino-BoldItalic",
		"Symbol",
		"ZapfChancery-MediumItalic",
		"ZapfDingbats"
		};

static int	PSfontmap[] = {
		ROMAN_FONT, ROMAN_FONT,	/* Times-Roman */
		ITALIC_FONT,		/* Times-Italic */
		BOLD_FONT,		/* Times-Bold */
		BOLD_FONT,		/* Times-BoldItalic */
		ROMAN_FONT,		/* AvantGarde */
		ROMAN_FONT,		/* AvantGarde-BookOblique */
		ROMAN_FONT,		/* AvantGarde-Demi */
		ROMAN_FONT,		/* AvantGarde-DemiOblique */
		ROMAN_FONT,		/* Bookman-Light */
		ITALIC_FONT,		/* Bookman-LightItalic */
		ROMAN_FONT,		/* Bookman-Demi */
		ITALIC_FONT,		/* Bookman-DemiItalic */
		TYPEWRITER_FONT,	/* Courier */
		TYPEWRITER_FONT,	/* Courier-Oblique */
		BOLD_FONT,		/* Courier-Bold */
		BOLD_FONT,		/* Courier-BoldItalic */
		MODERN_FONT,		/* Helvetica */
		MODERN_FONT,		/* Helvetica-Oblique */
		BOLD_FONT,		/* Helvetica-Bold */
		BOLD_FONT,		/* Helvetica-BoldOblique */
		MODERN_FONT,		/* Helvetica-Narrow */
		MODERN_FONT,		/* Helvetica-Narrow-Oblique */
		BOLD_FONT,		/* Helvetica-Narrow-Bold */
		BOLD_FONT,		/* Helvetica-Narrow-BoldOblique */
		ROMAN_FONT,		/* NewCenturySchlbk-Roman */
		ITALIC_FONT,		/* NewCenturySchlbk-Italic */
		BOLD_FONT,		/* NewCenturySchlbk-Bold */
		BOLD_FONT,		/* NewCenturySchlbk-BoldItalic */
		ROMAN_FONT,		/* Palatino-Roman */
		ITALIC_FONT,		/* Palatino-Italic */
		BOLD_FONT,		/* Palatino-Bold */
		BOLD_FONT,		/* Palatino-BoldItalic */
		ROMAN_FONT,		/* Symbol */
		ROMAN_FONT,		/* ZapfChancery-MediumItalic */
		ROMAN_FONT		/* ZapfDingbats */
		};

static int	PSmapwarn[] = {
		False, False,		/* Times-Roman */
		False,			/* Times-Italic */
		False,			/* Times-Bold */
		False,			/* Times-BoldItalic */
		True,			/* AvantGarde */
		True,			/* AvantGarde-BookOblique */
		True,			/* AvantGarde-Demi */
		True,			/* AvantGarde-DemiOblique */
		True,			/* Bookman-Light */
		True,			/* Bookman-LightItalic */
		True,			/* Bookman-Demi */
		True,			/* Bookman-DemiItalic */
		False,			/* Courier */
		True,			/* Courier-Oblique */
		True,			/* Courier-Bold */
		True,			/* Courier-BoldItalic */
		False,			/* Helvetica */
		True,			/* Helvetica-Oblique */
		True,			/* Helvetica-Bold */
		True,			/* Helvetica-BoldOblique */
		True,			/* Helvetica-Narrow */
		True,			/* Helvetica-Narrow-Oblique */
		True,			/* Helvetica-Narrow-Bold */
		True,			/* Helvetica-Narrow-BoldOblique */
		True,			/* NewCenturySchlbk-Roman */
		True,			/* NewCenturySchlbk-Italic */
		True,			/* NewCenturySchlbk-Bold */
		True,			/* NewCenturySchlbk-BoldItalic */
		True,			/* Palatino-Roman */
		True,			/* Palatino-Italic */
		True,			/* Palatino-Bold */
		True,			/* Palatino-BoldItalic */
		True,			/* Symbol */
		True,			/* ZapfChancery-MediumItalic */
		True			/* ZapfDingbats */
		};

int	        PSisomap[] = {
		False, False,		/* Times-Roman */
		False,			/* Times-Italic */
		False,			/* Times-Bold */
		False,			/* Times-BoldItalic */
		False,			/* AvantGarde */
		False,			/* AvantGarde-BookOblique */
		False,			/* AvantGarde-Demi */
		False,			/* AvantGarde-DemiOblique */
		False,			/* Bookman-Light */
		False,			/* Bookman-LightItalic */
		False,			/* Bookman-Demi */
		False,			/* Bookman-DemiItalic */
		False,			/* Courier */
		False,			/* Courier-Oblique */
		False,			/* Courier-Bold */
		False,			/* Courier-BoldItalic */
		False,			/* Helvetica */
		False,			/* Helvetica-Oblique */
		False,			/* Helvetica-Bold */
		False,			/* Helvetica-BoldOblique */
		False,			/* Helvetica-Narrow */
		False,			/* Helvetica-Narrow-Oblique */
		False,			/* Helvetica-Narrow-Bold */
		False,			/* Helvetica-Narrow-BoldOblique */
		False,			/* NewCenturySchlbk-Roman */
		False,			/* NewCenturySchlbk-Italic */
		False,			/* NewCenturySchlbk-Bold */
		False,			/* NewCenturySchlbk-BoldItalic */
		False,			/* Palatino-Roman */
		False,			/* Palatino-Italic */
		False,			/* Palatino-Bold */
		False,			/* Palatino-BoldItalic */
		NO,			/* Symbol */
		False,			/* ZapfChancery-MediumItalic */
		NO			/* ZapfDingbats */
		};

static char *figfontnames[] = {
		"Roman", "Roman",
		"Roman", 
		"Bold",
		"Italic",
		"Modern",
		"Typewriter"
		};

void unpsfont(t)
F_text	*t;
{
	if (!psfont_text(t))
	    return;
	if (PSmapwarn[t->font+1])
	  fprintf(stderr, "PS fonts not supported; substituting %s for %s\n",
		figfontnames[PSfontmap[t->font+1]+1], PSfontnames[t->font+1]);
	if (t->font == -1) /* leave default to be default, but no-ps */
	  t->font = 0;
	else
	  t->font = PSfontmap[t->font+1];
}


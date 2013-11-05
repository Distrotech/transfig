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

#ifdef NFSS
static char		*texfontfamily[] = {
			"\\familydefault",	/* default */
			"\\rmdefault",		/* roman */
			"\\rmdefault",		/* bold */
			"\\rmdefault",		/* italic */
			"\\sfdefault",		/* sans serif */
			"\\ttdefault"		/* typewriter */
		};

static char		*texfontseries[] = {
			"\\mddefault", 		/* default */
			"\\mddefault",		/* roman */
			"\\bfdefault",		/* bold */
			"\\mddefault",		/* italic */
			"\\mddefault", 		/* sans serif */
			"\\mddefault"		/* typewriter */
		};

static char		*texfontshape[] = {
			"\\updefault",		/* default */
			"\\updefault",		/* roman */
			"\\updefault",		/* bold */
			"\\itdefault",		/* italic */
			"\\updefault",		/* sans serif */
			"\\updefault"		/* typewriter */
		};
#endif

static char		*texfontnames[] = {
  			"rm",			/* default */
			"rm",			/* roman */
			"bf",			/* bold */
			"it",			/* italic */
			"sf", 			/* sans serif */
			"tt"			/* typewriter */
		};

/* The selection of font names may be site dependent.
 * Not all fonts are preloaded at all sizes.
 */

static char		texfontsizes[] = {
                       11,            /* default */
                       5, 5, 5, 5,    /* 1-4: small fonts */
                       5,			/* five point font */
                       6, 7, 8,	/* etc */
                       9, 10, 11,
                       12, 12, 14,
                       14, 14, 17,
                       17, 17, 20,
                       20, 20, 20, 20, 25,
                       25, 25, 25, 29,
                       29, 29, 29, 29,
                       34, 34, 34, 34,
                       34, 34, 34, 41,
                       41, 41
  			};

#define MAXFONTSIZE 	42

#ifdef NFSS
#define TEXFAMILY(F)	(texfontfamily[((F) <= MAX_FONT) ? (F) : (MAX_FONT-1)])
#define TEXSERIES(F)	(texfontseries[((F) <= MAX_FONT) ? (F) : (MAX_FONT-1)])
#define TEXSHAPE(F)	(texfontshape[((F) <= MAX_FONT) ? (F) : (MAX_FONT-1)])
#endif
#define TEXFONT(F)	(texfontnames[((F) <= MAX_FONT) ? (F) : (MAX_FONT-1)])


#define TEXFONTSIZE(S)	(texfontsizes[((S) <= MAXFONTSIZE) ? (int)(round(S))\
				      				: (MAXFONTSIZE-1)])
#define TEXFONTMAG(T)	TEXFONTSIZE(T->size*(rigid_text(T) ? 1.0 : fontmag))

void setfigfont( F_text *text );		/* genepic.c */

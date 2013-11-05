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

static char		*psfontnames[] = {
			"Times-Roman", "Times-Roman",	/* default */
			"Times-Roman",			/* roman */
			"Times-Bold",			/* bold */
			"Times-Italic",			/* italic */
			"Helvetica",			/* sans serif */
			"Courier"			/* typewriter */
		};

extern int v2_flag, v21_flag, v30_flag;

#define PS_FONTNAMES(T)	\
  	(((v2_flag&&!(v21_flag||v30_flag)) || \
		psfont_text(T)) ? PSfontnames : psfontnames)

#define PSFONT(T) \
 ((T->font) <= MAXFONT(T) ? PS_FONTNAMES(T)[T->font+1] : PS_FONTNAMES(T)[0])

#define PSFONTMAG(T)  (((T->size) <= ULIMIT_FONT_SIZE ? \
				     T->size :  ULIMIT_FONT_SIZE) \
				       * ppi/(correct_font_size? (metric ? 72*80/76.2 : 72): 80))

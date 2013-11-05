/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1992 Uri Blumenthal, IBM
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

#define              ULIMIT_FONT_SIZE        300
#define	       DEFAULT_PICFONT	     	13
#define PICPSFONT(F)      (PICfontnames[ \
		PICPSfontmap[(((F)->font) <= MAXFONT(F)) ? \
		((F)->font)+1 : \
		DEFAULT_PICFONT]])
extern int v2_flag, v21_flag;
#define ROMAN_DEFAULT 0
#define ROMAN 	1
#define ITALIC 	2
#define BOLD    3
#define ITABOL  4
#define HELVET  5
#define HELBOL  6
#define HELOBL  7
#define HELBOB  8
#define COUR    9
#define COURBL  10
#define COUROB  11
#define COURBO  12
#define SYMBOL  13
#define BRAKET  14
int    PICPSfontmap[] = {
			ROMAN_DEFAULT, ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			COUR,
			COUROB,
			COURBL,
			COURBO,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			SYMBOL,
			ITALIC,
			BRAKET
		};
char		*PICfontnames[] = {
		"R", "R",     /* default */
		"I",
		"B",
		"BI",
		"H",
		"HB",
		"HO",
		"HX",
		"C",
		"CB",
		"CO",
		"CX",
		"S",
		"S2"
		};

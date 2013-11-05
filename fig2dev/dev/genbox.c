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
 *	genbox : Empty box driver for fig2dev translator
 *
*/
#include "fig2dev.h"
#include "object.h"

void genbox_option(opt, optarg)
char opt, *optarg;
{
  	switch (opt) {

	case 's':
	case 'f':
	case 'm':
	case 'L':
		break;

 	default:
		put_msg(Err_badarg, opt, "box");
		exit(1);
	}
}

void genbox_start(objects)
F_compound	*objects;
{
	/* draw box */
        fprintf(tfp, "\\makebox[%.3fin]{\\rule{0in}{%.3fin}}\n",
		(urx-llx)*mag/ppi, (ury-lly)*mag/ppi);
	}

int
genbox_end()
{
	/* all OK */
	return 0;
}

struct driver dev_box = {
	genbox_option,
	genbox_start,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	genbox_end,
	INCLUDE_TEXT
};

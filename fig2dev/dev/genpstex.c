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
 *	genpstex.c : psTeX and psTeX_t drivers for fig2dev
 *
 *	Author: Jose Alberto Fernandez R /Maryland CP 9/90
 * 	It uses the LaTeX and PostScript drivers to generate 
 *      LaTeX processed text for a Postscript figure.
 *
 * The pstex_t driver is like a latex driver that only translates 
 * text defined in the default font.
 *
 * The pstex driver is like a PostScript driver that translates 
 * everything except for text in the default font.
 *
 * The pdftex_t and pdftex are drivers for combined PDF/LaTeX.
 *
 * The option '-p file' added to the pstex_t translator specifies
 * the name of the PostScript file to be called in the psfig macro.
 * If not set or its value is null then no PS file will be inserted.
 *
 * Jose Alberto.
 */

#include "fig2dev.h"
#include "genps.h"
#include "genpdf.h"
#include "object.h"
#include "texfonts.h"

extern double rad2deg;

#ifdef hpux
#define rint(a) floor((a)+0.5)     /* close enough? */
#endif

#ifdef gould
#define rint(a) floor((a)+0.5)     /* close enough? */
#endif

extern void
	genlatex_start (),
	gendev_null (),
     	geneps_option (),
	genps_start (),
	genps_arc (),
	genps_ellipse (),
	genps_line (),
	genps_spline (),
        genlatex_option (),
        genlatex_text (),
        genps_text ();
extern int
	genlatex_end (),
	genps_end ();

static char pstex_file[1000] = "";

void genpstex_t_option(opt, optarg)
char opt, *optarg;
{
       if (opt == 'p') 
	   strcpy(pstex_file, optarg);
       else
	   genlatex_option(opt, optarg);
}


void genpstex_t_start(objects)
F_compound	*objects;
{
	/* Put PostScript Image if any*/
        if (pstex_file[0] != '\0') {
		fprintf(tfp, "\\begin{picture}(0,0)%%\n");
/* newer includegraphics directive suggested by Stephen Harker 1/13/99 */
#if defined(LATEX2E_GRAPHICS)
#  if defined(EPSFIG)
		fprintf(tfp, "\\epsfig{file=%s}%%\n",pstex_file); 
#  else
		fprintf(tfp, "\\includegraphics{%s}%%\n",pstex_file);
#  endif
#else
		fprintf(tfp, "\\special{psfile=%s}%%\n",pstex_file);
#endif
		fprintf(tfp, "\\end{picture}%%\n");
	}
        genlatex_start(objects);

}

void genpstex_t_text(t)
F_text	*t;
{

	if (!special_text(t))
	  gendev_null();
	else genlatex_text(t);
}

void genpstex_text(t)
F_text	*t;
{

	if (!special_text(t))
	  genps_text(t);
	else gendev_null();
}

void genpstex_option(opt, optarg)
char opt, *optarg;
{
       if (opt != 'p')
	   genlatex_option(opt, optarg);
}

struct driver dev_pstex_t = {
  	genpstex_t_option,
	genpstex_t_start,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	genpstex_t_text,
	genlatex_end,
	INCLUDE_TEXT
};

struct driver dev_pdftex_t = {
  	genpstex_t_option,
	genpstex_t_start,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	genpstex_t_text,
	genlatex_end,
	INCLUDE_TEXT
};

struct driver dev_pstex = {
  	geneps_option, 	/* use eps so always exported in Portrait mode */
	genps_start,
	genps_grid,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genpstex_text,
	genps_end,
	INCLUDE_TEXT
};

extern void     genpdf_option();
extern void     genpdf_start();
extern int      genpdf_end();

struct driver dev_pdftex = {
  	genpdf_option,
	genpdf_start,
	genps_grid,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genpstex_text,
	genpdf_end,
	INCLUDE_TEXT
};



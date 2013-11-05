/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
 */

#include <stdio.h>
#include "transfig.h"

void puttarget();
void putfig();
void putoptions();
void putclean();

/*
 * create an appropriate makefile
 */

void
makefile(mk, altfonts, arg_list)
    FILE	*mk;
    int		 altfonts;
    argument	*arg_list;
{
  argument *a;
  char *i;
  enum language to;
  int needps, needeps, needpic, needfig;
  
  fprintf(mk, "#\n# TransFig makefile\n#\n");

  fprintf(mk, "\nall: ");
  for (a = arglist; a; a = a->next)
    	fprintf(mk, "%s.%s ", a->name, lname[a->tolang]);
  fprintf(mk, "\n");

  for (a = arglist; a; a = a->next)
  {
	i = a->name;
	to = a->tolang;

	needps = needeps = needpic = needfig = 0;

	fprintf(mk, "\n# translation into %s\n\n", lname[(int)to]);

	switch (to) {
	  case box:
		putfig(mk, box, altfonts, NULL, NULL, a->m, NULL, i, "tex");
		needfig = 1;
		break;

	  case cgm:
	  case emf:
	  case gif:
	  case ibmgl:
	  case jpeg:
	  case map:
	  case mf:
	  case mmp:
	  case mp:
	  case pcx:
	  case pdf:
	  case pic:
	  case png:
	  case ppm:
	  case ptk:
	  case sld:
	  case textyl:
	  case tiff:
	  case tk:
	  case xbm:
	  case xpm:

		putfig(mk, to, altfonts, a->f, a->s, a->m, a->o, i, lname[(int)to]);
		needfig = 1;
                break;

	  case eepicemu:
	  case epic:
	  case eepic:
		putfig(mk, to, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	  case latex:
		putfig(mk, latex, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	  case pdftex:

		/*
		 * The makefile for the pdftex need to update two files.
		 * file.pdf with is created using fig2dev -L pdftex file.fig
		 * and
		 * file.tex with fig2dev -L pdftex_t -p file.pdf file.fig
		 * 
		 */
		puttarget(mk, i, "tex", "pdf");
		fprintf(mk, "\tfig2dev -L pdftex_t -p %s.pdf ", i);
		putoptions(mk, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		break;

	  case pictex:
		putfig(mk, pictex, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	  case ps:
                puttarget(mk, i, "tex", "ps");
                fprintf(mk, "\tfig2ps2tex %s.ps >%s.tex\n", i, i);
		needps = 1;
                break;

	  case eps:
                puttarget(mk, i, "tex", "eps");
                fprintf(mk, "\tfig2ps2tex %s.eps >%s.tex\n", i, i);
		needeps = 1;
                break;

	  case psfig:
                puttarget(mk, i, "tex", "ps");
		fprintf(mk,"\techo '\\strut\\psfig{figure=%s.ps}' >%s.tex\n",
                          i, i);
		needps = 1;
                break;

	  case pstex:

		/*
		 * The makefile for the pstex need to update two files.
		 * file.eps with is created using fig2dev -L pstex file.fig
		 * and
		 * file.tex with fig2dev -L pstex_t -p file.eps file.fig
		 * 
		 */
		puttarget(mk, i, "tex", "eps");
		fprintf(mk, "\tfig2dev -L pstex_t -p %s.eps ", i);
		putoptions(mk, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needeps = 1;
		break;


	  case tpic:
		puttarget(mk, i, "tex", "pic");
		/* fprintf(mk, "\ttpic %s.pic\n", i); */
		fprintf(mk, "\tpic2tpic %s.pic | tpic > %s.tex\n", i, i);
		fprintf(mk, "\techo \'\\strut\\box\\graph\' >>%s.tex\n", i);
		needpic = 1;
		break;

	}

	putclean(mk, i, lname[a->tolang] );

	/* conversion to postscript */
	if (needps && a->type != i_ps) {
		if ( a->tops ) {
		    puttarget(mk, i, "ps", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.ps\n", a->tops, i, iname[(int)a->type], i);
		}
		else {
                    putfig(mk, (to == pstex ? pstex : ps), 
			   altfonts, a->f, a->s, a->m, a->o, i, "ps");
                    a->interm = mksuff(i, ".ps");
		    needfig = 1;
		}
		putclean(mk, i, "ps" );
	}

	/* conversion to eps */
	if (needeps && a->type != i_eps) {
		if ( a->tops ) {
		    puttarget(mk, i, "eps", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.eps\n", a->tops, i, iname[(int)a->type], i);
		}
		else {
                    putfig(mk, (to == pstex ? pstex : eps), 
			   altfonts, a->f, a->s, a->m, a->o, i, "eps");
                    a->interm = mksuff(i, ".eps");
		    needfig = 1;
		}
		putclean(mk, i, "eps" );
	}

	/* conversion to pic */
	if (needpic && a->type != i_pic) {
		if ( a->topic ) {
		    puttarget(mk, i, "pic", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.pic\n", a->topic, i, iname[(int)a->type],i);
		}
		else {
		    putfig(mk, tpic, altfonts, a->f, a->s, a->m, a->o, i, "pic");

		    needfig = 1;
		}
		putclean(mk, i, "pic" );
	}

	/* conversion to fig */
	if (needfig && a->type != i_fig) {
		if ( a->tofig ) {
		    puttarget(mk, i, "fig", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.fig\n", a->tofig, i, iname[(int)a->type],i);
		    a->interm = mksuff(i, ".fig");
		}
		else {
		    fprintf(stderr, "transfig: warning: don't now how to make %s\n", mksuff(i, ".fig") );
		}
		putclean(mk, i, "fig" );
	}
  }
}

void
puttarget(mk, i, suf1, suf2)
    FILE	*mk;
    char	*i, *suf1, *suf2;
{
    fprintf(mk, "%s.%s: %s.%s %s\n", i, suf1, i, suf2, mkfile);
}

void
putfig(mk, to, altfonts, f, s, m, o, i, suf)
    FILE	*mk;
    enum	 language to;
    int		 altfonts;
    char	 *f, *s, *m, *o, *i, *suf;
{
  fprintf(mk, "%s%s%s: %s.fig %s\n",
	       i, (suf ? "." : ""), (suf ? suf : ""), i, mkfile);

  if ( to == tpic )
	  fprintf(mk, "\tfig2dev -L pic ");
  else
	  fprintf(mk, "\tfig2dev -L %s ", lname[(int)to]);

  putoptions(mk, altfonts, f, s, m, o, i, suf);
}

void
putoptions(mk, altfonts, f, s, m, o, i, suf)
    FILE	*mk;
    int		 altfonts;
    char	*f, *s, *m, *o, *i, *suf;
{
  if (altfonts==1) fprintf(mk, "-a ");
  if (f && *f) fprintf(mk, "-f %s ", f);
  if (s && *s) fprintf(mk, "-s %s ", s);
  if (m && *m) fprintf(mk, "-m %s ", m);
  if (o && *o) fprintf(mk, "%s ", o);

  fprintf(mk, "%s.fig %s%s%s\n", i, i, (suf ? "." : ""), (suf ? suf : ""));
}

void
putclean(mk, i, suf)
    FILE	*mk;
    char	*i, *suf;
{
   fprintf(mk, "clean::\n");
   fprintf(mk, "\trm -f %s.%s\n", i, suf);
   fprintf(mk, "\n");
}

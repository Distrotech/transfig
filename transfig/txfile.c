/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
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

#include <stdio.h>
#include <stdlib.h>
#include "transfig.h"

/*
 * create appropriate .tex file
 */
void
texfile(tx, in, arg_list)
    FILE	*tx;
    char	*in;
    argument	*arg_list;
{
  enum language to;
  argument *a, *arg_l;
  int texfonts = 1;  /* do we use TeX fonts for output? */

  for (a = arglist; a; a = a->next) {
     to = a->tolang;

     /* see if we already have this language */
     for (arg_l = arglist; arg_l != a; arg_l = arg_l->next)
	if ( arg_l->tolang == to ) break;
	
     if ( arg_l == a )
	switch (to) {
	  case box:
		fprintf(tx, "\\typeout{TransFig: null figures.}\n");
		texfonts = 0;
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
		fprintf(tx, "\\typeout{TransFig: figures in %s.}\n", lname[(int)to]);
		texfonts = 0;
		break;

	  case eepicemu:
		to = eepicemu;

	  case eepic:
#ifdef eemulation
		to = eepicemu;
#endif

	  case epic:
		fprintf(tx, "\\typeout{TransFig: figures in %s.}\n",
							lname[(int)to]);
		if (to == eepicemu || to == eepic)
			fprintf(tx, "\\%s{epic}",INCLFIG);
		fprintf(tx, "\\%s{%s}\n",INCLFIG, lname[(int)to]);
		break;

	  case latex:
		fprintf(tx, "\\typeout{TransFig: figures in LaTeX.}\n");
		break;

	  case pictex:
		fprintf(tx, "\\typeout{TransFig: figures in PiCTeX.}\n");
		fprintf(tx, "\
\\ifx\\fivrm\\undefined\n\
  \\font\\fivrm=cmr5\\relax\n\
\\fi\n\
\\input{prepictex}\n\
\\input{pictex}\n\
\\input{postpictex}\n");
		break;

	  case ps:
		fprintf(tx, "\\typeout{TransFig: figures in PostScript.}\n");
		texfonts = 0;
		break;

	  case eps:
		fprintf(tx, "\\typeout{TransFig: figures in EPS.}\n");
		texfonts = 0;
		break;

	  case pstex: 
		fprintf(tx, "\\typeout{TransFig: figure text in LaTeX.}\n");
		fprintf(tx, "\\typeout{TransFig: figures in PostScript.}\n");
		break;

	  case psfig:
		fprintf(tx, "\\typeout{TransFig: figures in PostScript w/psfig.}\n");
		fprintf(tx, "\\%s{psfig}\n",INCLFIG);
		texfonts = 0;
		break;

	  case tpic:
		fprintf(tx, "\\typeout{TransFig: figures in tpic.}\n");
		texfonts = 0;
		break;

	  default:
		fprintf(stderr, "Unknown graphics language %s\n", lname[(int)to]);
		exit(1);
		break;

	}
  }

  if (*in) fprintf(tx, "\n\\input{%s}\n", in);

  fprintf(tx, "\n\\endinput\n");
}

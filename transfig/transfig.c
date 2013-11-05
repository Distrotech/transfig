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

/*
 * transfig: 	figure translation setup program
 *		creates TeX macro file and makefile
 *
 * usage: transfig <option> ... [[<flag> ... ] [<figure>] ... ] ...
 *
 * where:	<option> = -L <language> | -M <makefile> | -T <texfile>
 *		<flag>	 = -f <font> | -s <size> | -m <scale>
 */

#include <stdio.h>
#include <stdlib.h>
#include "patchlevel.h"
#include "transfig.h"


argument *parse_arg(), *arglist = NULL, *lastarg = NULL;
char *strip();

char *mkfile = "Makefile";
char *txfile = "transfig.tex";
char *input = "";
int  altfonts = 0;

/* output languages */
/* Warning - if this list is changed the enum language list in transfig.h must be changed too */

char *lname[] = {
		"box",
		"cgm",
		"eepic",
		"eepicemu",
                "emf",
		"epic",
		"eps",
		"gbx",
		"gif",
		"ibmgl",
		"dxf",
		"jpeg",
		"latex",
		"map",
		"mf",
                "mmp",
                "mp",
		"pcx",
		"pdf",
		"pdftex",
		"pic",
		"pictex",
		"png",
		"ppm",
		"ps",
		"psfig",
		"pstex",
		"ptk",
		"sld",
		"svg",
		"textyl",
		"tiff",
		"tk",
		"tpic",
		"xbm",
		"xpm"};

/* enum input {apg, fig, pic, ps}; */
char *iname[] = {
	"apg",
	"fig",
  	"pic",
	"ps",
	"eps"};
 
main(argc, argv)
int argc;
char *argv[];
{
  FILE *mk, *tx;
  enum language tolang = epic;
  argument *a;
  char *cp; 
  char *arg_f = NULL, *arg_s = NULL, *arg_m = NULL, *arg_o = NULL, *argbuf;

  for ( optind = 1; optind < argc; optind++ ) {
    cp = argv[optind];
    if (*cp == '-') {
  	if (!cp[1]) {
		fprintf(stderr, "transfig: bad option format '-'\n");
		exit(1);
	}
	if (cp[1] == 'V') {
		fprintf(stderr, "TransFig Version %s Patchlevel %s\n",
							VERSION, PATCHLEVEL);
		exit(0);
	} else if (cp[1] == 'h') {
		fprintf(stderr,"usage: transfig <option> ... [[<flag> ... ] [<figure>] ... ] ...\n");
		fprintf(stderr,"where:	<option> = -L <language> | -M <makefile> | -T <texfile>\n");
		fprintf(stderr,"	<flag>	 = -f <font> | -s <size> | -m <scale>\n");
		exit(0);
	}

	if (cp[2]) {
		optarg = &cp[2];
	} else {
	    if (cp[1] != 'a') {
		optind += 1;
		if (optind == argc) {
		    fprintf(stderr, "transfig: no value for '%c' arg\n", cp[1]);
		    exit(1);
		}
		optarg = argv[optind];
	    }
	}
 	switch (cp[1]) {

	    case 'I':
		input = optarg;
		break;

  	    case 'L':
		tolang = str2lang(optarg);
		break;
  	    case 'M':
		mkfile = optarg;
		break;
  	    case 'T':
		txfile = optarg;
		break;
	    case 'a':
		altfonts = 1;
		break;
	    case 'f':
		arg_f = optarg;
		break;
	    case 's':
		arg_s = optarg;	
		break;
	    case 'm':
		arg_m = optarg;	
		break;

	    case 'o':
		arg_o = optarg;
		break;

  	default:
		fprintf(stderr, "transfig: illegal option -- '%c'\n", cp[1]);
		exit(1);
  	}
    } else
    {
	a = parse_arg(tolang, arg_f, arg_s, arg_m, arg_o, argv[optind]);

	if ( !lastarg )
		arglist = a;
	else
		lastarg->next = a; 
	lastarg = a;
    }
  }

  /* no files specified -> all files */
  if (!arglist)
  {
	argbuf = sysls();
	while (cp = strchr(argbuf, '\n'))
	{
		*cp = '\0';
		a = parse_arg(tolang, arg_f, arg_s, arg_m, arg_o, argbuf);
		if ( !lastarg )
			arglist = a;
		else
			lastarg->next = a; 
		lastarg = a;
		argbuf = cp+1;
	}
  }

  /* remove any backup and move existing texfile to texfile~ */
  sysmv(txfile);
  if (strcmp(txfile,"-")==0)
	tx = stdout;
  else if ((tx = fopen(txfile, "w")) == NULL) {
	fprintf(stderr,"transfig: can't open Texfile '%s'\n",txfile);
	exit(1);
  }
  texfile(tx, input, altfonts, arglist);

  /* remove any backup and move existing makefile to makefile~ */
  sysmv(mkfile);
  if (strcmp(mkfile,"-")==0)
	mk = stdout;
  else if ((mk = fopen(mkfile, "w")) == NULL) {
	fprintf(stderr,"transfig: can't open Makefile '%s'\n",mkfile);
	exit(1);
  }
  makefile(mk, altfonts, arglist);
  exit(0);
}

enum language str2lang(s)
char *s;
{
  int i;

  /* aliases */
  if (!strcmp(s, "pic")) return tpic;
  if (!strcmp(s, "postscript")) return ps;
  if (!strcmp(s, "latexps")) return pstex;
  if (!strcmp(s, "null")) return box;

  /* real names*/
  for (i = 0; i <= (int)MAXLANG; i++)
	if (!strcmp(lname[i], s)) return (enum language)i;

  /* other strings */
  fprintf(stderr, "Unknown output language \"%s\"\n", s);
  exit(1);
}

argument *parse_arg(tolang, arg_f, arg_s, arg_m, arg_o, arg)
enum language tolang;
char *arg_f, *arg_s, *arg_m, *arg_o, *arg;
{
  argument *a;

  a = (argument *)malloc(sizeof(argument));
  a->f = arg_f;
  a->s = arg_s;
  a->m = arg_m;
  a->o = arg_o;
  a->next = NULL;
  a->tofig = NULL;
  a->topic = NULL;
  a->tops = NULL;
  a->tolang = tolang;
  
  /* PIC */
  if (strip(arg, ".pic"))
  {
  	a->name = mksuff(arg, "");
  	a->type = i_pic;
	a->tofig = PIC2FIG;
	return a;
  }

  /* PS format */
  if (strip(arg, ".ps"))
  {
  	a->name = mksuff(arg, "");
  	a->type = i_ps;
 	return a;
  }

  /* EPS format */
  if (strip(arg, ".eps"))
  {
  	a->name = mksuff(arg, "");
  	a->type = i_eps;
 	return a;
  }

  /* ApGraph format */
  if (strip(arg, ".apg"))
  {
  	a->name = mksuff(arg, "");
  	a->type = i_apg;
	a->tofig = APG2FIG;
 	return a;
  }

  /* Fig format */
  strip(arg, ".fig");
  a->name = mksuff(arg, "");
  a->type = i_fig;
  return a;
}

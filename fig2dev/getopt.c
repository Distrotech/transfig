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
	I got this off net.sources from Henry Spencer.
	It is a public domain getopt(3) like in System V.
	I have made the following modifications:

	index(s,c) was added because too many people could
	not compile getopt without it.

*/
#include <stdio.h>

#ifndef lint
static	char	sccsfid[] = "@(#) getopt.c 5.0 (UTZoo) 1985";
#endif

#define	ARGCH    (int)':'
#define BADCH	 (int)'?'
#define EMSG	 ""
#define	ENDARGS  "--"

/* this is included because index is not on some UNIX systems */
static char *
index (s, c)
  register  char *s;
  register  int   c;
{
	while (*s)
		if (c == *s) return (s);
		else s++;
	return (NULL);
}

/*
 * get option letter from argument vector
 */
int	opterr = 1,		/* useless, never set or used */
	optind = 1,		/* index into parent argv vector */
	optopt;			/* character checked for validity */
char	*optarg;		/* argument associated with option */

#define tell(s)	fputs(*nargv,stderr);fputs(s,stderr); \
		fputc(optopt,stderr);fputc('\n',stderr);return(BADCH);

int
fig_getopt(nargc,nargv,ostr)
  int	nargc;
  char	**nargv,
	*ostr;
{
	static char	*place = EMSG;	/* option letter processing */
	register char	*oli;		/* option letter list index */
	char	*index();

	if(!*place) {			/* update scanning pointer */
		if(optind >= nargc || *(place = nargv[optind]) != '-' || !*++place) return(EOF);
		if (*place == '-') {	/* found "--" */
			++optind;
			return(EOF);
		}
	}				/* option letter okay? */
	if ((optopt = (int)*place++) == ARGCH || !(oli = index(ostr,optopt))) {
		if(!*place) ++optind;
		tell(": illegal option -- ");
	}
	if (*++oli != ARGCH) {		/* don't need argument */
		optarg = NULL;
		if (!*place) ++optind;
	}
	else {				/* need an argument */
		if (*place) optarg = place;	/* no white space */
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			tell(": option requires an argument -- ");
		}
	 	else optarg = nargv[optind];	/* white space */
		place = EMSG;
		++optind;
	}
	return(optopt);			/* dump back option letter */
}


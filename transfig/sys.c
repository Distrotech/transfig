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
#include <string.h>
#include "transfig.h"

#define MAXSYS 10000
static char sysbuf[MAXSYS];

char *sysls()
{
  FILE *ls;
  int i;
  int c;

  ls = popen("/bin/ls *.fig", "r");
  i = 0;
  c = fgetc(ls);
  while (c != EOF & i < MAXSYS-1)
  {
	sysbuf[i] = (char) c;
	i += 1;
	c = fgetc(ls);
  }
  sysbuf[i] = '\0';
  return sysbuf;
}

sysmv(file)
char *file;
{
  sprintf(sysbuf, "%s~", file);
  unlink(sysbuf);
  if (!link(file, sysbuf)) {
	fprintf(stderr, "Renaming %s to %s~\n", file, file);
	unlink(file);
  }
}

char *strip(str, suf)
char *str, *suf;
{
  char *p1, *p2;

  for (p1 = &str[strlen(str)-1], p2 = &suf[strlen(suf)-1];
	(p1 >= str && p2 >= suf) && (*p1 == *p2);
	--p1, --p2);

  if (p2 < suf)
  {
	*(p1+1) = '\0';
	return str;
  } else
	return NULL;
}

char *mksuff(name, suff)
char *name, *suff;
{
  char *temp;

  temp = (char *)malloc(strlen(name)+strlen(suff)+1);
  strcpy(temp, name);
  strcat(temp, suff);
  return temp;
}

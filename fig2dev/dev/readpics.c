/*
 * TransFig: Facility for Translating Fig code
 * Various copyrights in this file follow
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

#include <sys/types.h>
#include <sys/stat.h>
#include "fig2dev.h"

char * xf_basename();

/* 
   Open the file 'name' and return its type (real file=0, pipe=1) in 'type'.
   Return the full name in 'retname'.  This will have a .gz or .Z if the file is
   zipped/compressed.
   The return value is the FILE stream.
*/

FILE *
open_picfile(name, type, pipeok, retname)
    char	*name;
    int		*type;
    Boolean	 pipeok;
    char	*retname;
{
    char	 unc[PATH_MAX+20];	/* temp buffer for gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	 status;
    char	*gzoption;

    *type = 0;
    *retname = '\0';
    if (pipeok)
	gzoption = "-c";		/* tell gunzip to output to stdout */
    else
	gzoption = "";

    /* see if the filename ends with .Z or .z or .gz */
    if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	      ((strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2))))) {
	/* yes, make command to uncompress it */
	sprintf(unc,"gunzip -q %s %s", gzoption, name);
	*type = 1;
    /* no, see if the file with .gz, .z or .Z appended exists */
    /* failing that, if there is an absolute path, strip it and look in current directory */
    } else {
	/* check for straight name first */
	if (!stat(name, &status)) {
	    /* found it, skip other checks */
	    *type = 0;
	} else {
	    /* no, check for .gz */
	    strcpy(retname, name);
	    strcat(retname, ".gz");
	    if (!stat(retname, &status)) {
		/* yes, found with .gz */
		sprintf(unc, "gunzip %s %s", gzoption, retname);
		*type = 1;
		name = retname;
	    } else {
		/* no, check for .z */
		strcpy(retname, name);
		strcat(retname, ".z");
		if (!stat(retname, &status)) {
		    /* yes, found with .z */
		    sprintf(unc, "gunzip %s %s", gzoption, retname);
		    *type = 1;
		    name = retname;
		} else {
		    /* no, check for .Z */
		    strcpy(retname, name);
		    strcat(retname, ".Z");
		    if (!stat(retname, &status)) {
			/* yes, found with .Z */
			sprintf(unc, "gunzip %s %s", gzoption, retname);
			*type = 1;
			name = retname;
		    } else {
			/* can't find it, if there is an absolute path, strip it and look in
			   current directory */
			if (strchr(name,'/')) {
			    /* yes, strip it off */
			    strcpy(retname, xf_basename(name));
			    if (!stat(retname, &status)) {
				*type = 0;
				strcpy(name, retname);
			    }
			}
		    }
		}
	    }
	}
    }
    /* if a pipe, but the caller needs a file, uncompress the file now */
    if (*type == 1 && !pipeok) {
	char *p;
	system(unc);
	if (p=strrchr(name,'.')) {
	    *p = '\0';		/* terminate name before last .gz, .z or .Z */
	}
	strcpy(retname, name);
	/* force to plain file now */
	*type = 0;
    }

    /* no appendages, just see if it exists */
    /* and restore the original name */
    strcpy(retname, name);
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "rb");
	    break;
	  case 1:
	    fstream = popen(unc,"r");
	    break;
	}
    }
    return fstream;
}

void
close_picfile(file,type)
    FILE	*file;
    int		type;
{
    if (type == 0)
	fclose(file);
    else
	pclose(file);
}

/* for systems without basename() (e.g. SunOS 4.1.3) */
/* strip any path from filename */

char *
xf_basename(filename)
    char	   *filename;
{
    char	   *p;
    if (filename == NULL || *filename == '\0')
	return filename;
    if (p=strrchr(filename,'/')) {
	return ++p;
    } else {
	return filename;
    }
}

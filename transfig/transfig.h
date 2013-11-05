#if defined(SYSV) || defined(SVR4)
#include <string.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

/*
 * converters program names
 */
#define FIG2DEV	"fig2dev"
#define PIC2FIG "pic2fig"
#define APG2FIG "apgto f"

/*
 * filename defaults
 */
#define MK "Makefile"
#define TX "transfig.tex"

/* if using LaTeX209, use "documentstyle", if using LaTeX2e use "usepackage" */
#ifdef LATEX2E
#define INCLFIG "usepackage"
#else
#define INCLFIG "documentstyle"
#endif

/* Warning - if this list is changed the lname[] array in transfig.c must be changed too */
/* Also, be sure to change MAXLANG if the list extends beyond the current lang */

enum language  {box, cgm, eepic, eepicemu, emf, epic, eps, gbx, gif,
	ibmgl, dxf, jpeg, latex, map, mf, mmp, mp, pcx, pdf, pdftex,
	pic, pictex, png, ppm, ps, psfig, pstex, ptk, sld, svg,
	textyl, tiff, tk, tpic, xbm, xpm};
#define MAXLANG xpm

enum input {i_apg, i_fig, i_pic, i_ps, i_eps};
#define MAXINPUT xps

typedef struct argument{
	char *name, *interm, *f, *s, *m, *o, *tofig, *topic, *tops;
	enum language tolang;
	enum input type;
	struct argument *next;
} argument ;

extern enum language str2lang();
extern char *lname[];
extern char *iname[];

extern char *sysls(), *mksuff();
extern argument *arglist;
extern char *txfile, *mkfile;

extern char *optarg;
extern int optind;


/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
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

/* Add a (La)TeX macro, if TeX fonts are used. */

/* This macro is called with three (five with NFSS support) arguments:
 *    #1   fontsize (without `pt')
 *    #2   baselineskip (without `pt')
 *    #3   font  (without escape character)
 *  And, for NFSS support:
 *    #4   font series
 *    #5   font shape
 */

#include <fig2dev.h>

Boolean FontSizeOnly = False;

void
define_setfigfont(tfp)
     FILE *tfp;
{
#ifdef NFSS

    if ( FontSizeOnly )
    	fprintf(tfp, "%%\n\
\\begingroup\\makeatletter\\ifx\\SetFigFont\\undefined%%\n\
\\gdef\\SetFigFont#1#2{%%\n\
  \\fontsize{#1}{#2pt}%%\n\
  \\selectfont}%%\n\
\\fi\\endgroup%%\n");
    else
	fprintf(tfp, "%%\n\
\\begingroup\\makeatletter\\ifx\\SetFigFont\\undefined%%\n\
\\gdef\\SetFigFont#1#2#3#4#5{%%\n\
  \\reset@font\\fontsize{#1}{#2pt}%%\n\
  \\fontfamily{#3}\\fontseries{#4}\\fontshape{#5}%%\n\
  \\selectfont}%%\n\
\\fi\\endgroup%%\n");
#else /* NFSS */
  fprintf(tfp, "%%\n\
\\begingroup\\makeatletter\\ifx\\SetFigFont\\undefined\n\
%% extract first six characters in \\fmtname\n\
\\def\\x#1#2#3#4#5#6#7\\relax{\\def\\x{#1#2#3#4#5#6}}%%\n\
\\expandafter\\x\\fmtname xxxxxx\\relax \\def\\y{splain}%%\n\
\\ifx\\x\\y   %% LaTeX or SliTeX?\n\
\\gdef\\SetFigFont#1#2#3{%%\n\
  \\ifnum #1<17\\tiny\\else \\ifnum #1<20\\small\\else\n\
  \\ifnum #1<24\\normalsize\\else \\ifnum #1<29\\large\\else\n\
  \\ifnum #1<34\\Large\\else \\ifnum #1<41\\LARGE\\else\n\
     \\huge\\fi\\fi\\fi\\fi\\fi\\fi\n\
  \\csname #3\\endcsname}%%\n\
\\else\n\
\\gdef\\SetFigFont#1#2#3{\\begingroup\n\
  \\count@#1\\relax \\ifnum 25<\\count@\\count@25\\fi\n\
  \\def\\x{\\endgroup\\@setsize\\SetFigFont{#2pt}}%%\n\
  \\expandafter\\x\n\
    \\csname \\romannumeral\\the\\count@ pt\\expandafter\\endcsname\n\
    \\csname @\\romannumeral\\the\\count@ pt\\endcsname\n\
  \\csname #3\\endcsname}%%\n\
\\fi\n\
\\fi\\endgroup\n");
#endif /* NFSS */
}

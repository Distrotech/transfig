/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1992 Herbert Bauer and  B. Raichle
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


/* map ISO-Font Symbols to appropriate sequences in TeX */
/* Herbert Bauer 22.11.1991 */

/* B.Raichle 12.10.92, changed some of the definitions */


/* B.Raichle 12.10.92, changed some of the definitions */

/* T.Kopal 22.3.02, added ISO-8859-2 table */

/* ISO-8859-1 encoding table */
char *ISO1toTeX[] =   /* starts at octal 240 */
{
  "{}",
  "{!`}",	/* inverse ! */
  "{}",		/* cent sign (?) */
  "\\pounds{}",
  "{}",		/* circle with x mark */
  "{}",		/* Yen */
  "{}",		/* some sort of space - doen't work under mwm */
  "\\S{}",	/* paragraph sign */
  "\\\"{}",		/* diaresis points */
  "\\copyright{}",
  "\\b{a}",
  "\\mbox{$\\ll$}",		/* << */
  "{--}", 	/* longer dash - doesn't work with mwm */
  "{-}",		/* short dash */
  "{}",		/* trademark */
  "{}",		/* overscore */
/* 0xb0 */
  "{\\lower.2ex\\hbox{\\char\\'27}}",		/* degree */
  "\\mbox{$\\pm$}",	/* plus minus - math mode */
  "\\mbox{$\\mathsurround 0pt{}^2$}",		/* squared  - math mode */
  "\\mbox{$\\mathsurround 0pt{}^3$}",		/* cubed  - math mode */
  "\\'{}",		/* accent egue */
  "\\mbox{$\\mu$}",	/* greek letter mu - math mode */
  "\\P{}",	/* paragraph */
  "\\mbox{$\\cdot$}",	/* centered dot  - math mode */
  "",
  "\\mbox{$\\mathsurround 0pt{}^1$}",		/* superscript 1  - math mode */
  "\\b{o}",
  "\\mbox{$\\gg$}",		/* >> */
  "\\mbox{$1\\over 4$}",	/* 1/4 - math mode */
  "\\mbox{$1\\over 2$}",	/* 1/2 - math mode */
  "\\mbox{$3\\over 4$}",	/* 3/4 - math mode */
  "{?`}",		/* inverse ? */
/* 0xc0 */
  "\\`A",
  "\\'A",
  "\\^A",
  "\\~A",
  "\\\"A",
  "\\AA{}",
  "\\AE{}",
  "\\c C",
  "\\`E",
  "\\'E",
  "\\^E",
  "\\\"E",
  "\\`I",
  "\\'I",
  "\\^I",
  "\\\"I",
/* 0xd0 */
  "{\\rlap{\\raise.3ex\\hbox{--}}D}", /* Eth */
  "\\~N",
  "\\`O",
  "\\'O",
  "\\^O",
  "\\~O",
  "\\\"O",
  "\\mbox{$\\times$}",	/* math mode */
  "\\O{}",
  "\\`U",
  "\\'U",
  "\\^U",
  "\\\"U",
  "\\'Y",
  "{}",		/* letter P wide-spaced */
  "\\ss{}",
/* 0xe0 */
  "\\`a",
  "\\'a",
  "\\^a",
  "\\~a",
  "\\\"a",
  "\\aa{}",
  "\\ae{}",
  "\\c c",
  "\\`e",
  "\\'e",
  "\\^e",
  "\\\"e",
  "\\`\\i{}",
  "\\'\\i{}",
  "\\^\\i{}",
  "\\\"\\i{}",
/* 0xf0 */
  "\\mbox{$\\partial$}",	/* correct?  - math mode */
  "\\~n",
  "\\`o",
  "\\'o",
  "\\^o",
  "\\~o",
  "\\\"o",
  "\\mbox{$\\div$}",	/* math mode */
  "\\o{}",
  "\\`u",
  "\\'u",
  "\\^u",
  "\\\"u",
  "\\'y",
  "{}",		/* letter p wide-spaced */
  "\\\"y"
};

/* ISO-8859-2 encoding table */
char *ISO2toTeX[] =   /* starts at octal 240 */
{
  "{}",		/* no-break space */
  "\\k{A}",	/* latin capital letter A with ogonek */
  "\\u{}",	/* breve */
  "{\\L}",	/* latin capital letter L with stroke */
  "{}",		/* currency sign */
  "\\v{L}",	/* latin capital letter L with caron */
  "\\'S",	/* latin capital letter S with acute */
  "\\S{}",	/* section sign */
  "\\\"{}",	/* diaeresis */
  "\v{S}",	/* latin capital letter S with caron */
  "\\c{S}",	/* latin capital letter S with cedilla */
  "\\v{T}",	/* latin capital letter T with caron */
  "\\'Z", 	/* latin capital letter Z with acute */
  "{-}",	/* soft hyphen */
  "\\v{Z}",	/* latin capital letter Z with caron */
  "\\.Z",	/* latin capital letter Z with dot above */
/* 0xb0 */
  "{\\lower.2ex\\hbox{\\char\\'27}}",		/* degree sign */
  "\\k{a}",	/* latin small letter A with ogonek */
  "\\k{}",	/* ogonek */
  "{\\l}",	/* latin small letter L with stroke */
  "\\'{}",	/* acute accent */
  "\\v{l}",	/* latin small letter L with caron */
  "\\'s",	/* latin small letter S with acute */
  "\\v{}",	/* caron */
  "\\o{}",	/* cedilla */
  "\\v{s}",	/* latin small letter S with caron */
  "\\c{s}",	/* latin small letter S with cedilla */
  "\\v{t}",	/* latin small letter T with caron */
  "\\'z",	/* latin small letter Z with acute */
  "\\H{}",	/* double acute accent */
  "\\v{z}",	/* latin small letter Z with caron */
  "\\.z",	/* latin small letter Z with dot above */
/* 0xc0 */
  "\\'R",	/* latin capital letter R with acute */
  "\\'A",	/* latin capital letter A with acute */
  "\\^A",	/* latin capital letter A with circumflex */
  "\\u{A}",	/* latin capital letter A with breve */
  "\\\"A",	/* latin capital letter A with diaeresis */
  "\\'L",	/* latin capital letter L with acute */
  "\\'C",	/* latin capital letter C with acute */
  "\\c{C}",	/* latin capital letter C with cedilla */
  "\\v{C}",	/* latin capital letter C with caron */
  "\\'E",	/* latin capital letter E with acute */
  "\\k{E}",	/* latin capital letter E with ogonek */
  "\\\"E",	/* latin capital letter E with diaeresis */
  "\\v{E}",	/* latin capital letter E with caron */
  "\\'I",	/* latin capital letter I with acute */
  "\\^I",	/* latin capital letter I with circumflex */
  "\\v{D}",	/* latin capital letter D with caron */
/* 0xd0 */
  "D",		/* latin capital letter D with stroke */
  "\\'N",	/* latin capital letter N with acute */
  "\\v{N}",	/* latin capital letter N with caron */
  "\\'O",	/* latin capital letter O with acute */
  "\\^O",	/* latin capital letter O with circumflex */
  "\\H{O}",	/* latin capital letter O with double acute */
  "\\\"O",	/* latin capital letter O with diaeresis */
  "\\mbox{$\\times$}",	/* multiplication sign */
  "\\v{R}",	/* latin capital letter R with caron */
  "\\v{U}",	/* latin capital letter U with ring above */
  "\\'U",	/* latin capital letter U with acute */
  "\\H{U}",	/* latin capital letter U with double acute */
  "\\\"U",	/* latin capital letter U with diaeresis */
  "\\'Y",	/* latin capital letter Y with acute */
  "\\c{T}",	/* latin capital letter T with cedilla */
  "\\ss{}",	/* latin small letter sharp S */
/* 0xe0 */
  "\\'r",	/* latin small letter R with acute */
  "\\'a",	/* latin small letter A with acute */
  "\\^a",	/* latin small letter A with circumflex */
  "\\u{a}",	/* latin small letter A with breve */
  "\\\"a",	/* latin small letter A with diaeresis */
  "\\'l",	/* latin small letter L with acute */
  "\\'c",	/* latin small letter C with acute */
  "\\c{c}",	/* latin small letter C with cedilla */
  "\\v{c}",	/* latin small letter C with caron */
  "\\'e",	/* latin small letter E with acute */
  "\\k{e}",	/* latin small letter E with ogonek */
  "\\\"e",	/* latin small letter E with diaeresis */
  "\\v{e}",	/* latin small letter E with caron */
  "\\'\\i{}",	/* latin small letter I with acute */
  "\\^\\i{}",	/* latin small letter I with circumflex */
  "\\v{d}",	/* latin small letter D with caron */
/* 0xf0 */
  "d",		/* latin small letter D with stroke */
  "\\'n",	/* latin small letter N with acute */
  "\\v{n}",	/* latin small letter N with caron */
  "\\'o",	/* latin small letter O with acute */
  "\\^o",	/* latin small letter O with circumflex */
  "\\H{o}",	/* latin small letter O with double acute */
  "\\\"o",	/* latin small letter O with diaeresis */
  "\\mbox{$\\div$}",	/* division sign */
  "\\v{r}",	/* latin small letter R with caron */
  "\\v{u}",	/* latin small letter U with ring above */
  "\\'u",	/* latin small letter U with acute */
  "\\H{u}",	/* latin small letter U with double acute */
  "\\\"u",	/* latin small letter U with diaeresis */
  "\\'y",	/* latin small letter Y with acute */
  "\\c{t}",	/* latin small letter T with cedilla */
  "\\.{}"	/* dot above */
};


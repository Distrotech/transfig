/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Copyright (c) 1988 by Conrad Kwok
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
 * genpstricks.c: pstricks driver for fig2dev
 * Copyright (c) 2004, 2005, 2006, 2007, 2008 by Eugene Ressler
 * Acknowledgement: code for font manipulation is modeled closely after
 *   the epic driver by Conrad Kwok.
 *
 * Changes:
 *   V1.2:
 *   27 May 2010 - Added new arrowhead styles and various gcc warning fixes.
 */
 
#define PST_DRIVER_VERSION 1.2
 
#include "fig2dev.h"
#include "object.h"
#include "texfonts.h"
 
/**********************************************************************/
/* utility functions                                                  */
/**********************************************************************/
 
#define _STRINGIFY(X) #X
#define STRINGIFY(S) _STRINGIFY(S)
#define ARRAY_SIZE(A) ((int)(sizeof (A) / sizeof (A)[0]))
#define bit(N) (1 << (N))
#define PI (3.1415926535897932384626433832795)
 
/* format used for double coordinates */
#define DBL "%.4lf"
 
/* return (double) centimeters for xfig integer distances */
#define cm_from_80ths(Val) \
  ((double)((Val) * (2.54 / 80.0)))
 
#define cm_from_1200ths(Val)  \
  ((double)((Val) / ppi * 2.54))
 
/* compute min/max of double pair */
static double
min_dbl(a, b)
     double a, b;
{
  return a < b ? a : b;
}
 
static double
max_dbl(a, b)
     double a, b;
{
  return a > b ? a : b;
}
 
static double
sqr_dbl(x)
     double x;
{
  return x * x;
}
 
/* return absolute value of int */
static int
abs_int(x)
     int x;
{
  return x < 0 ? -x : x;
}
 
/* compute min/max of int pair */
static int
min_int(a, b)
     int a, b;
{
  return a < b ? a : b;
}
 
static int
max_int(a, b)
     int a, b;
{
  return a > b ? a : b;
}
 
/* fold a new value into an accumulated integer min/max */
static void
fold_min_int(acc, val)
     int *acc, val;
{
  if (val < *acc)
    *acc = val;
}
 
static void
fold_max_int(acc, val)
     int *acc, val;
{
  if (val > *acc)
    *acc = val;
}
 
/* return a roman numeral for a given integer */
static char*
roman_numeral_from_int(char *rn, unsigned n)
{
  static char rn_digits[][4] = {
    "M", "CM", "D", "CD",
    "C", "XC", "L", "XL",
    "X", "IX", "V", "IV",
    "m", "cm", "d", "cd",
    "c", "xc", "l", "xl",
    "x", "ix", "v", "iv",
    "i"
  };
  static unsigned rn_divs[] = {
    1000000, 900000, 500000, 400000,
    100000,  90000,  50000,  40000,
    10000,   9000,   5000,   4000,
    1000,    900,    500,    400,
    100,     90,     50,     40,
    10,      9,      5,      4,
    1
  };
  unsigned i;
 
  rn[0] = '\0';
  for (i = 0; i < ARRAY_SIZE(rn_divs); i++) 
    while (n >= rn_divs[i]) {
      n -= rn_divs[i];
      strcat(rn, rn_digits[i]);
    }
  return rn;
}
 
void *
xmalloc(n)
     unsigned n;
{
  void *p = (void*)malloc(n);
  if (!p) {
    fprintf(stderr, "out of memory.\n");
    exit(1);
  }
  return p;
}
 
char *
xstrdup(s)
     char *s;
{
  char *c = (char*)xmalloc(strlen(s) + 1);
  strcpy(c, s);
  return c;
}
 
/* traverse an xfig point list and return its bounding box */
static void
find_bounding_box_of_points(llx, lly, urx, ury, pts)
     int *llx, *lly, *urx, *ury;
     F_point *pts;
{
  F_point *p;
 
  if (!pts) {
    *llx = *lly = *urx = *ury = 0;
    return;
  }
 
  *llx = *urx = pts->x;
  *lly = *ury = pts->y;
  for (p = pts->next; p; p = p->next) {
    fold_min_int(llx, p->x);
    fold_min_int(lly, p->y);
    fold_max_int(urx, p->x);
    fold_max_int(ury, p->y);
  }
}
 
/* Xfig's coordinate system has y=0 at the top. These conversions make
   coordinates bounding-box relative, which is good for generating
   figures and full-page printouts. These functions fix that by
   computing coordinates relative to the entire image bounding box. 
   They also incorporate user-specified margins.
 
   The llx and lly here are the global bounding box set in fig2dev.
   They are misleadingly labeled as to which is bigger and smaller,
   so the absolute values and min/maxes here are used to remove all 
   doubt. */
 
static void
convert_xy_to_image_bb(x_bb, y_bb, x, y)
     int *x_bb, *y_bb, x, y;
{
  /* llx and lly are global bounding box set by the reader */
  *x_bb = x - min_int(llx, urx);
  *y_bb = max_int(lly, ury) - y;
}
 
static void
convert_xy_dbl_to_image_bb(x_bb, y_bb, x, y)
     double *x_bb, *y_bb, x, y;
{
  *x_bb = x - min_int(llx, urx);
  *y_bb = max_int(lly, ury) - y;
}
 
static void
convert_pt_to_image_bb(pt_bb, pt)
     F_point *pt_bb, *pt;
{
  convert_xy_to_image_bb(&pt_bb->x, &pt_bb->y, pt->x, pt->y);
}
 
static void
convert_pt_to_image_bb_in_place(pt)
     F_point *pt;
{
  convert_pt_to_image_bb(pt, pt);
}
 
/* append a new string value onto a comma-separated list */
static void
append_to_comma_list(lst, val)
     char **lst, *val;
{
  if (!val || !val[0])
    return;
  if (**lst) {
    *lst += strlen(*lst);
    sprintf(*lst, ",%s", val);
  }
  else 
    strcpy(*lst, val);
}
 
/* character predicates; string.h is too non-compatible */
int
is_digit (int ch)
{
  return '0' <= ch && ch <= '9';
}
 
int
lower (int ch)
{
  return ('A' <= ch && ch <= 'Z') ? ch + ('a' - 'A') : ch;
}
 
int
is_alpha(int ch)
{
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}
 
/**********************************************************************/
/* warning system                                                     */
/**********************************************************************/
 
#define W_DASH_DOT         0
#define W_DASH_DOTS        1
#define W_HOLLOW_ARROW     2
#define W_HATCH_PATTERN    3
#define W_OLD_SPLNE        4
#define W_PIC              5
#define W_GRID             6
#define W_ELLIPSE_HATCH    7
#define W_FN_CHARS         8
#define W_SLASH_CONVERT    9
#define W_UNK_PIC_TYPE     10
#define W_SYSTEM_EXEC      11
#define W_BMEPS_ERR        12
#define W_EPS_NEEDED       13
#define W_PIC_CONVERT      14
#define W_PS_FONT          15
#define W_PAGESCALE        16
#define W_UNIT             17
#define W_LINEJOIN         18
#define W_UNKNOWN_ARROW    19
#define W_NEW_ARROW        20
#define W_TERMINATOR_ARROW 21
 
#define W_RTN_ERROR_FLAGS (bit(W_UNK_PIC_TYPE) | bit(W_SYSTEM_EXEC) | bit(W_BMEPS_ERR))
 
static char *warnings[] = {
  /* 0 */ "Dash-dot lines require pstricks patchlevel 15 of June 2004 or later.",
  /* 1 */ "Dash-dot-dot lines require \\usepackage{pstricks-add}.",
  /* 2 */ "Hollow arrows required \\usepackage{pstricks-add}.",
  /* 3 */ "A hatch pattern not supported by pstricks has been converted to crosshatch.",
  /* 4 */ "Driver never tested with old splines. Send examples to ressler@usma.edu.",
  /* 5 */ "Picture requires \\usepackage{graphicx}",
  /* 6 */ "Grid sizes are ignored by the pstricks driver. Default \\psgrid issued.",
  /* 7 */ "Hatch pattern in rotated ellipse may be imprecisely converted.",
  /* 8 */ "Spaces and special chars in picture file path are likely to cause problems.",
  /* 9 */ "Backslashes were converted to slashes in picture file path.",
  /*10 */ "Couldn't convert unknown picture type to eps. Known: eps, png, jpg, pnm, tif (exit code 17).",  
  /*11 */ "Could not execute bmeps to convert picture to eps (exit code 17).",
  /*12 */ "BMEPS returned non-0 exit code; picture conversion to eps probably failed (exit code 17).",
  /*13 */ "Non-EPS picture paths were converted to the eps image directory.",
  /*14 */ "Pictures were converted and placed in the eps image directory.",
  /*15 */ "Subsituted LaTeX font for PS.  Use -v to see details.",
  /*16 */ "Image scaled to page.",
  /*17 */ "Setting unit requires \\psset{unit=X}\\newlength\\unit\\setlength\\unit{X}.",
  /*18 */ "Linejoins depend on PSTricks version. Use -t option to ensure compatibility.",
  /*19 */ "Unknown arrow type. Simple (stick) arrow substituted.",
  /*20 */ "New arrow type requires xfig 3.2.5a or newer.",
  /*21 */ "One or more arrows have been approximated with a PSTricks style. Try -R 0.",
};
 
/* set of warnings that will be printed once at the end of the run */
static unsigned warn_flags = 0;
static int rtn_val = 0;
 
void warn(flag)
     int flag;
{
  warn_flags |= bit(flag);
  if (bit(flag) & W_RTN_ERROR_FLAGS)
    rtn_val = 17;
}
 
void issue_warnings(f)
     FILE *f;
{
  unsigned i, mask;
 
  if (warn_flags) {
    fprintf(f, "IMPORTANT notes from pstricks driver:\n");
    for (i = 0, mask = 1; mask; i++, mask <<= 1)
      if (mask & warn_flags) 
        fprintf(f, "  %s\n", warnings[i]);
  }
}
 
/**********************************************************************/
/* rudimentary string tables                                          */
/**********************************************************************/
 
typedef struct string_table_node_t {
  struct string_table_node_t *next;
  int n_refs;
  union {
    int i;
    double dbl;
    void *vp;
  } val;
  char str[1];
} STRING_TABLE_NODE;
 
typedef struct string_table_t {
  STRING_TABLE_NODE *head;
  int n_str;
} STRING_TABLE;
 
/* static tables do not need to be initialized */
 
#ifdef UNUSED
 
static void
init_string_table(tbl)
     STRING_TABLE *tbl;
{
  tbl->head = 0;
  tbl->n_str = 0;
}
 
#endif
 
static STRING_TABLE_NODE*
string_lookup_val(tbl, str)
     STRING_TABLE *tbl;
     char *str;
{
  STRING_TABLE_NODE *stn;
 
  for (stn = tbl->head; stn; stn = stn->next) {
    if (strcmp(stn->str, str) == 0)
      return stn;
  }
  return 0;
}
 
static STRING_TABLE_NODE*
add_string(tbl, str)
     char *str;
     STRING_TABLE *tbl;
{
  STRING_TABLE_NODE *stn;
 
  stn = string_lookup_val(tbl, str);
  if (stn) 
    stn->n_refs++;
  else {
    stn = (STRING_TABLE_NODE*)xmalloc(sizeof(STRING_TABLE_NODE) + strlen(str));
    stn->next = tbl->head;
    stn->n_refs = 1;
    memset(&stn->val, 0, sizeof stn->val);
    strcpy(stn->str, str);
    tbl->head = stn;
    tbl->n_str++;
  }
  return stn;
}
 
/**********************************************************************/
/* user options                                                       */
/**********************************************************************/
 
/* configurable parameters */
static int Verbose = False;
 
#define PM_PICTURE     0
#define PM_BARE        1
#define PM_PAGE        2
#define PM_SCALED_PAGE 3
static int Page_mode = PM_PICTURE;
 
#define A_XFIG      0
#define A_PSTRICKS  1
#define A_DEFAULT   2
static int Arrows = A_XFIG;
 
#define FH_FULL       0
#define FH_SIZE_ONLY  1
#define FH_NONE       2
static int Font_handling = FH_FULL;
 
static int Pic_convert_p = 0;
static char Pic_convert_dir[260] = "";
static double X_margin = 0, Y_margin = 0;
static double Line_weight = 0.5;
 
static struct {
  double mag;
  int size;
} ScaleTbl[5] = {
  { 0.6667, 8 },
  { 0.75  , 9 },
  { 0.8333, 10 },
  { 0.9167, 11 },
  { 1.0   , 12 }
};
 
#define MAX_PST_VERSION_STRING_SIZE 10
 
typedef struct pst_version_t {
  char str[MAX_PST_VERSION_STRING_SIZE];
  char key[MAX_PST_VERSION_STRING_SIZE * 2];
  unsigned n_updates;
} PST_VERSION;
 
/* Version 1.20 added linejoin option, where pstverb was 
   needed previously. */
#define PST_LINEJOIN_VERSION 1.20
 
/* This should be set to the latest version that 
   makes a difference for Sketch output. */
#define ASSUMED_PST_VERSION PST_LINEJOIN_VERSION 
 
PST_VERSION Pst_version[1];
#define LJ_PSTVERB    0
#define LJ_PSTOPTION  1
int Linejoin = LJ_PSTOPTION;
 
/* Fill a version struct given a version string.  This is
   a little fsm that recognizes digit+.digit+[suffix], 
   where suffix is a single letter and digit is 0-9. Keys
   are zero-padded and .-justified so a simple strcmp
   lets us compare versions for relative order. Zero 
   return means parse is successful.  Other values are
   error codes. */
int
parse_pst_version (PST_VERSION *v, char *str)
{
#define M (sizeof v->key / 2)
  int i = 0, iv = 0, i_minor = -1;
 
  v->n_updates++;
  memset(v->key, '0', sizeof v->key);
  memset(v->str, '\0', sizeof v->str);
 
  if (strlen(str) > sizeof v->str - 1) {
    fprintf(stderr, "PSTricks version string too long");
    return 1;
  }
 
  if ( is_digit (str[i]) ) {
    v->str[iv++] = str[i++];
    goto B;
  }
  else {
    fprintf(stderr, "bad character '%c' in PSTricks version", str[i]);
    return 2;
  }
 B:
  if ( is_digit (str[i]) ) {
    v->str[iv++] = str[i++];
    goto B;
  }
  else if ( str[i] == '.' ) {
    memcpy (&v->key[M - i], v->str, i); /* save major in key */
    v->str[iv++] = str[i++];
    i_minor = iv; /* remember where minor version starts */
    goto C;
  }
  else {
    fprintf(stderr, "expected dot in PSTricks version");
    return 3;
  }
 C: 
  if ( is_digit (str[i]) ) {
    v->str[iv++] = str[i++];
    goto D;
  }
  else {
    fprintf(stderr, "expected digit after dot in PSTricks version");
    return 4;
  }
 D: 
  if ( is_digit (str[i]) ) {
    v->str[iv++] = str[i++];
    goto D;
  }
  else if ( is_alpha(str[i]) ) {
    v->str[iv++] = lower (str[i++]);
    goto F;
  }
  else if ( str[i] == '\0' ) {
    memcpy (&v->key[M], &v->str[i_minor], i - i_minor); /* save minor in key */
    return 0; /* accept */
  }
  else {
    fprintf(stderr, "expected digit or subversion letter in PSTricks version");
    return 5;
  }
 F: 
  if ( str[i] == '\0' ) {
    memcpy (&v->key[M], &v->str[i_minor], i - i_minor);
    return 0; /* accept */
  }
  else {
    fprintf(stderr, "expected end of PSTricks version, found '%c'", str[i]);
    return 6;
  }
  return -1;
}
 
void 
init_pst_version(PST_VERSION *v)
{
  v->n_updates = 0;
  parse_pst_version(v, STRINGIFY(ASSUMED_PST_VERSION));
  v->n_updates = 0;
}
 
void
pst_version_cpy(PST_VERSION *dst, PST_VERSION *src)
{
  unsigned n_dst_updates = dst->n_updates;
  *dst = *src;
  dst->n_updates = n_dst_updates + 1;
}
 
int
pst_version_cmp(PST_VERSION *a, PST_VERSION *b)
{
  return strncmp(a->key, b->key, sizeof a->key);
}
 
void genpstrx_option(opt, optarg)
	char opt;
	char *optarg;
{
  int i, tmp_int;
  double tmp_dbl;
  char fn[256];
  FILE *f;
  PST_VERSION tmp_pst_version[1];
 
  /* set up default version and a temp */
  init_pst_version(Pst_version);
  init_pst_version(tmp_pst_version);
 
  switch (opt) {
 
  case 'f':
    for (i = 1; i <= MAX_FONT; i++)
      if (strcmp(optarg, texfontnames[i]) == 0) 
        break;
    if (i > MAX_FONT)
      fprintf(stderr, "warning: non-standard font name %s ignored\n", optarg);
    else {
      texfontnames[0] = texfontnames[i];
#ifdef NFSS
      texfontfamily[0] = texfontfamily[i];
      texfontseries[0] = texfontseries[i];
      texfontshape[0]  = texfontshape[i];
#endif
    }
    break;
 
  case 'G':            /* grid spec; just warn that we don't pay attention to spacing */
    warn(W_GRID);
    break;
 
  case 'L':            /* language spec; no further work required */
    break;
 
  case 'l':
    if (sscanf(optarg, "%lf", &tmp_dbl) == 1 && 0 <= tmp_dbl && tmp_dbl <= 2.0)
      Line_weight = tmp_dbl;
    else
      fprintf(stderr, "warning: bad line weight %s, expected 0 to 2.0\n", optarg);
    break;
 
  case 'n':
    if (sscanf(optarg, "%d", &tmp_int) == 1 && 0 <= tmp_int && tmp_int <= 3)
      Page_mode = tmp_int;
    else
      fprintf(stderr, "warning: bad page mode (0, 1, 2, or 3 expected)\n");
    break;
 
  case 'P':
    Page_mode = PM_SCALED_PAGE;
    break;
 
  case 'p':
    Pic_convert_p = 1;
    if (strcmp(optarg, "-") != 0) {
      /* copy while converting slashes */
      for (i = 0; optarg[i] && i < ARRAY_SIZE(Pic_convert_dir) - 2; i++)
        Pic_convert_dir[i] = (optarg[i] == '\\') ? '/' : optarg[i];
 
      /* append slash if not there already */
      if (i > 0 && Pic_convert_dir[i - 1] != '/')
        Pic_convert_dir[i++] = '/';
      Pic_convert_dir[i] = '\0';
    }
    sprintf(fn, "%sreadme.txt", Pic_convert_dir);
    f = fopen(fn, "w");
    if (!f) {
      fprintf(stderr, "can't write the eps conversion directory %s.\n", Pic_convert_dir);
      exit(1);
    }
    fprintf(f, 
	    "This directory has been used by the fig2dev pstricks driver.\n"
	    "Any '.eps' file here may be overwritten.\n");
    fclose(f);
    break;
 
  case 'R':
    if (sscanf(optarg, "%d", &tmp_int) == 1 && 0 <= tmp_int && tmp_int <= 2)
      Arrows = tmp_int;
    else
      fprintf(stderr, "warning: bad arrow spec (0, 1, or 2 expected)\n");
    break;
 
  case 'S':
    if (optarg && sscanf(optarg, "%d", &tmp_int) == 1 && (tmp_int < 8 || tmp_int > 12)) {
      fprintf(stderr, "Scale must be between 8 and 12 inclusively\n");
      exit(1);
    }
    mag = ScaleTbl[tmp_int - 8].mag;
    font_size = (double) ScaleTbl[tmp_int - 8].size;
    break;
 
  case 't':
    if (parse_pst_version(tmp_pst_version, optarg) == 0) {
      pst_version_cpy(Pst_version, tmp_pst_version);
      /* compare user's version with first version that had linejoin to see
	 which style of linejoin command to emit */
      parse_pst_version(tmp_pst_version, STRINGIFY(PST_LINEJOIN_VERSION));
      Linejoin = (pst_version_cmp(Pst_version, tmp_pst_version) < 0) ? LJ_PSTVERB : LJ_PSTOPTION;
    }
    else 
      fprintf(stderr, "warning: bad PSTricks version '%s' was ignored\n", optarg);
    break;
 
  case 'v':
    Verbose = True;    /* put extra info in output LaTex file */
    break;
 
  case 'x':
    if (sscanf(optarg, "%lf", &tmp_dbl) == 1 && 0 <= tmp_dbl && tmp_dbl <= 100.0)
      X_margin = 1200/2.54 * tmp_dbl;
    else
      fprintf(stderr, "warning: bad x margin setting ignored\n");
    break;
 
  case 'y':
    if (sscanf(optarg, "%lf", &tmp_dbl) == 1 && 0 <= tmp_dbl && tmp_dbl <= 100.0) 
      Y_margin = 1200/2.54 * tmp_dbl;
    else
      fprintf(stderr, "warning: bad y margin setting ignored\n");
    break;
 
  case 'z':
    if (sscanf(optarg, "%d", &tmp_int) == 1 && 0 <= tmp_int && tmp_int <= 2)
      Font_handling = tmp_int;
    else
      fprintf(stderr, "warning: bad font spec (0, 1, or 2 expected)\n");
    break;
 
  default:
    put_msg(Err_badarg, opt, "pstricks");
    exit(1);
  }
}
 
/**********************************************************************/
/* start/end                                                          */
/**********************************************************************/
 
/* possible extra packages needed */
#define EP_GRAPHICX     0
#define EP_PSTRICKS_ADD 1
 
static char *extra_package_names[] = {
  "graphicx",
  "pstricks-add",
};
 
static struct {
  unsigned extra_package_mask;
  int has_text_p;
} Preprocessed_data[1];
 
static void
pp_check_arrow_style(arrow)
     F_arrow *arrow;
{
  if (!arrow)
    return;
  if (arrow->style == 0) /* hollow arrow */
    Preprocessed_data->extra_package_mask |= bit(EP_PSTRICKS_ADD);
}
 
static void
pp_check_line_style(style)
     int style;
{
  if (style == DASH_2_DOTS_LINE || style == DASH_3_DOTS_LINE)
    Preprocessed_data->extra_package_mask |= bit(EP_PSTRICKS_ADD);
}
 
/* This is just so we can emit a minimal 
   \usepackage{} in page mode.
   Much work for a small result! */
static void
pp_find_pstricks_extras(objects)
     F_compound *objects;
{
  F_line *line;
  F_ellipse *ellipse;
  F_spline *spline;
  F_arc *arc;
 
  if (!objects)
    return;
 
  for (line = objects->lines; line; line = line->next) {
    pp_check_line_style(line->style);
    pp_check_arrow_style(line->for_arrow);
    pp_check_arrow_style(line->back_arrow);
    if (line->type == T_PIC_BOX)
      Preprocessed_data->extra_package_mask |= bit(EP_GRAPHICX);
  }
 
  for (ellipse = objects->ellipses; ellipse; ellipse = ellipse->next)
    pp_check_line_style(ellipse->style);
 
  for (spline = objects->splines; spline; spline = spline->next) {
    pp_check_line_style(spline->style);
    pp_check_arrow_style(spline->for_arrow);
    pp_check_arrow_style(spline->back_arrow);
  }
 
  for (arc = objects->arcs; arc; arc = arc->next) {
    pp_check_line_style(arc->style);
    pp_check_arrow_style(arc->for_arrow);
    pp_check_arrow_style(arc->back_arrow);
  }
 
  pp_find_pstricks_extras(objects->compounds);
  pp_find_pstricks_extras(objects->next);
}
 
void
pp_find_text(objects)
     F_compound *objects;
{
  if (!objects)
    return;
 
  if (objects->texts)
    Preprocessed_data->has_text_p = 1;
  if (!Preprocessed_data->has_text_p)
    pp_find_text(objects->compounds);
  if (!Preprocessed_data->has_text_p)
    pp_find_text(objects->next);
}
 
/* Make preprocessing passes over the object tree. */
static void
preprocess(objects)
     F_compound *objects;
{
  pp_find_pstricks_extras(objects);
  pp_find_text(objects);
}
 
static void
define_setfont(tfp)
     FILE *tfp;
{
  if (!Preprocessed_data->has_text_p)
    return;
 
#ifdef NFSS
  if (Font_handling == FH_SIZE_ONLY)
    fprintf(tfp,
	    "%%\n"
	    "\\begingroup\\makeatletter%%\n"
	    "\\ifx\\SetFigFontSizeOnly\\undefined%%\n"
	    "  \\gdef\\SetFigFont#1#2{%%\n"
	    "    \\fontsize{#1}{#2pt}%%\n"
	    "    \\selectfont}%%\n"
	    "\\fi\n"
	    "\\endgroup%%\n");
  else if (Font_handling == FH_FULL)
    fprintf(tfp,
	    "%%\n"
	    "\\begingroup\\makeatletter%%\n"
	    "\\ifx\\SetFigFont\\undefined%%\n"
	    "  \\gdef\\SetFigFont#1#2#3#4#5{%%\n"
	    "    \\reset@font\\fontsize{#1}{#2pt}%%\n"
	    "    \\fontfamily{#3}\\fontseries{#4}\\fontshape{#5}%%\n"
	    "    \\selectfont}%%\n"
	    "\\fi\n"
	    "\\endgroup%%\n");
#else
  if (Font_handling != FH_NONE)
    fprintf(tfp, 
	    "%%\n"
	    "\\begingroup\\makeatletter%%\n"
	    "\\ifx\\SetFigFont\\undefined%%\n"
	    "%%   extract first six characters in \\fmtname\n"
	    "\\def\\x#1#2#3#4#5#6#7\\relax{\\def\\x{#1#2#3#4#5#6}}%%\n"
	    "\\expandafter\\x\\fmtname xxxxxx\\relax \\def\\y{splain}%%\n"
	    "\\ifx\\x\\y   %% LaTeX or SliTeX?\n"
	    "\\gdef\\SetFigFont#1#2#3{%%\n"
	    "  \\ifnum #1<17\\tiny\\else \\ifnum #1<20\\small\\else\n"
	    "  \\ifnum #1<24\\normalsize\\else \\ifnum #1<29\\large\\else\n"
	    "  \\ifnum #1<34\\Large\\else \\ifnum #1<41\\LARGE\\else\n"
	    "     \\huge\\fi\\fi\\fi\\fi\\fi\\fi\n"
	    "  \\csname #3\\endcsname}%%\n"
	    "\\else\n"
	    "\\gdef\\SetFigFont#1#2#3{\\begingroup\n"
	    "  \\count@#1\\relax \\ifnum 25<\\count@\\count@25\\fi\n"
	    "  \\def\\x{\\endgroup\\@setsize\\SetFigFont{#2pt}}%%\n"
	    "  \\expandafter\\x\n"
	    "    \\csname \\romannumeral\\the\\count@ pt\\expandafter\\endcsname\n"
	    "    \\csname @\\romannumeral\\the\\count@ pt\\endcsname\n"
	    "  \\csname #3\\endcsname}%%\n"
	    "\\fi\n"
	    "\\fi\\endgroup\n");
#endif
}
 
void
genpstrx_start(objects)
     F_compound *objects;
{
  int i;
  double unit, pllx, plly, purx, pury;
 
  /* Run the preprocessor. */
  preprocess(objects);
 
  /* If user gave font size, set up tables */
  texfontsizes[0] = texfontsizes[1] =
    TEXFONTSIZE(font_size != 0.0 ? font_size : DEFAULT_FONT_SIZE);
  define_setfont(tfp);
 
  /* print header information */
  if (Verbose) {
    fprintf(tfp, 
      "%% PSTricks driver for Fig Version " 
      STRINGIFY(PST_DRIVER_VERSION) 
      " by Gene Ressler.\n");
    fprintf(tfp, 
      "%% PSTricks version %s is assumed in Sketch output (change with -t).\n", 
      Pst_version->str);
  }
 
  /* print any whole-figure comments prefixed with "%" */
  if (objects->comments) {
    fprintf(tfp,"%%\n");
    print_comments("% ",objects->comments,"");
    fprintf(tfp,"%%\n");
  }
 
  /* print preamble if user asked for complete tex file */
  if (Page_mode >= PM_PAGE) {
    fprintf(tfp, 
	    "\\documentclass{article}\n"
	    "\\pagestyle{empty}\n"
	    "\\setlength\\topmargin{-.5in}\n"
	    "\\setlength\\textheight{10in}\n"
	    "\\setlength\\oddsidemargin{-.5in}\n"
	    "\\setlength\\evensidemargin\\oddsidemargin\n"
	    "\\setlength\\textwidth{7.5in}\n"
	    "\\usepackage{pstricks");
    /* mask was set up by the preprocessor */
    for (i = 0; i < 31; i++) {
      if (Preprocessed_data->extra_package_mask & bit(i))
        fprintf(tfp, ",%s", extra_package_names[i]);
    }
    fprintf(tfp, 
	    "}\n"
	    "\\begin{document}\n"
	    "\\begin{center}\n");
  }
 
  pllx = cm_from_1200ths(-X_margin);
  plly = cm_from_1200ths(-Y_margin);
  purx = cm_from_1200ths(1 + abs_int(urx - llx) + X_margin);
  pury = cm_from_1200ths(1 + abs_int(ury - lly) + Y_margin);
 
  /* default unit is 1cm */
  unit = 1.0;
  if (Page_mode == PM_SCALED_PAGE) {
    /* allow a small epsilon for page size to avoid spurious blank page */
#define PAGE_EPS .01
    unit = min_dbl(1.0, (7.5 * 2.54 - PAGE_EPS) / (purx - pllx));
    unit = min_dbl(unit, (10 * 2.54 - PAGE_EPS) / (pury - plly));
    if (unit < 1.0) {
      fprintf(tfp, "\\psset{unit="DBL"}\n", unit);
      warn(W_PAGESCALE);
    }
    /* this works only roughly */
    fontmag *= unit;
  }
  /* Set up unit length for pictures if there are any.  We can
     get that from the preprocessor pass flags. */
  if (Preprocessed_data->extra_package_mask & bit(EP_GRAPHICX)) {
    fprintf(tfp,
	    "%% Following sets up unit length for picture dimensions. If you \n"
	    "%% do \\psset{unit=LEN} to scale the picture, you must also\n"
	    "%% \\newlength\\unit\\setlength\\unit{LEN} for the bitmaps to match.\n"
	    "\\makeatletter%%\n"
	    "\\@ifundefined{unit}{\\newlength\\unit\\setlength\\unit{%.3lfcm}}%%\n"
	    "\\makeatother%%\n",
	    unit);
    warn(W_UNIT);
  }
  if (Page_mode != PM_BARE) {
    fprintf(tfp,
	    "\\begin{pspicture}("DBL","DBL")("DBL","DBL")\n", 
	    pllx, plly, purx, pury);
  }
}
 
int
genpstrx_end()
{
  if (Page_mode != PM_BARE)
    fprintf(tfp, "\\end{pspicture}\n");
 
  if (Page_mode >= PM_PAGE) {
    fprintf(tfp, 
	    "\\end{center}\n"
	    "\\end{document}\n");
  }
  issue_warnings(stderr);
 
  /* return is determined by warning mechanism above */
  return rtn_val;
}
 
/**********************************************************************/
/* general purpose drawing                                            */
/**********************************************************************/
 
/* color table -- 
   1 in the "defined" field means this color is already declared by pstricks;
   shades and tints are boolean bit fields that denote whether a declaration
   for the corresponding-numbered shade or tint has already been emitted
   (initially all _not_) bits 1-19 are shades 1-19 and tints 21-39 respectively */
static
struct color_table_entry_t {
  char name[24];
  Boolean defined_p;
  double r, g, b;
  unsigned shades, tints;
} color_table[544] = {
  {"black",     1, 0.00, 0.00, 0.00, 0u, 0u},	/* black */
  {"blue",      1, 0.00, 0.00, 1.00, 0u, 0u},	/* blue */
  {"green",     1, 0.00, 1.00, 0.00, 0u, 0u},	/* green */
  {"cyan",      1, 0.00, 1.00, 1.00, 0u, 0u},	/* cyan */
  {"red",       1, 1.00, 0.00, 0.00, 0u, 0u},	/* red */
  {"magenta",   1, 1.00, 0.00, 1.00, 0u, 0u},	/* magenta */
  {"yellow",    1, 1.00, 1.00, 0.00, 0u, 0u},	/* yellow */
  {"white",     1, 1.00, 1.00, 1.00, 0u, 0u},	/* white */
  {"bluei",     0, 0.00, 0.00, 0.56, 0u, 0u},	/* blue1 */
  {"blueii",    0, 0.00, 0.00, 0.69, 0u, 0u},	/* blue2 */
  {"blueiii",   0, 0.00, 0.00, 0.82, 0u, 0u},	/* blue3 */
  {"blueiv",    0, 0.53, 0.81, 1.00, 0u, 0u},	/* blue4 */
  {"greeni",    0, 0.00, 0.56, 0.00, 0u, 0u},	/* green1 */
  {"greenii",   0, 0.00, 0.69, 0.00, 0u, 0u},	/* green2 */
  {"greeniii",  0, 0.00, 0.82, 0.00, 0u, 0u},	/* green3 */
  {"cyani",     0, 0.00, 0.56, 0.56, 0u, 0u},	/* cyan1 */
  {"cyanii",    0, 0.00, 0.69, 0.69, 0u, 0u},	/* cyan2 */
  {"cyaniii",   0, 0.00, 0.82, 0.82, 0u, 0u},	/* cyan3 */
  {"redi",      0, 0.56, 0.00, 0.00, 0u, 0u},	/* red1 */
  {"redii",     0, 0.69, 0.00, 0.00, 0u, 0u},	/* red2 */
  {"rediii",    0, 0.82, 0.00, 0.00, 0u, 0u},	/* red3 */
  {"magentai",  0, 0.56, 0.00, 0.56, 0u, 0u},	/* magenta1 */
  {"magentaii", 0, 0.69, 0.00, 0.69, 0u, 0u},	/* magenta2 */
  {"magentaiii",0, 0.82, 0.00, 0.82, 0u, 0u},	/* magenta3 */
  {"browni",    0, 0.50, 0.19, 0.00, 0u, 0u},	/* brown1 */
  {"brownii",   0, 0.63, 0.25, 0.00, 0u, 0u},	/* brown2 */
  {"browniii",  0, 0.75, 0.38, 0.00, 0u, 0u},	/* brown3 */
  {"pinki",     0, 1.00, 0.50, 0.50, 0u, 0u},	/* pink1 */
  {"pinkii",    0, 1.00, 0.63, 0.63, 0u, 0u},	/* pink2 */
  {"pinkiii",   0, 1.00, 0.75, 0.75, 0u, 0u},	/* pink3 */
  {"pinkiv",    0, 1.00, 0.88, 0.88, 0u, 0u},	/* pink4 */
  {"gold",      0, 1.00, 0.84, 0.00, 0u, 0u}	/* gold */
};
 
/* certain indices we need */
#define CT_BLACK  0
#define CT_WHITE  7
 
static void
setup_color_table()
{
  int i;
  char rn[100];
  static int done_p = 0;
 
  if (done_p) return;
 
  for (i = 0; i < num_usr_cols; i++) {
    int iuc = i + NUM_STD_COLS;
    sprintf(color_table[iuc].name, "usrclr%s", roman_numeral_from_int(rn, i));
    color_table[iuc].r = user_colors[i].r / 256.0;
    color_table[iuc].g = user_colors[i].g / 256.0;
    color_table[iuc].b = user_colors[i].b / 256.0;
  }
  done_p = 1;
}
 
/* die if a bad color index is seen */
static void
check_color_index(ic)
     int ic;
{
  if (ic < 0 || ic >= NUM_STD_COLS + num_usr_cols) {
    fprintf(stderr, "bad color index (%d; max=%d)\n", ic, NUM_STD_COLS - 1 + num_usr_cols);
    exit(1);
  }
}
 
/* interpolate two color values given indices for them in the table
   and the interpolation parameter; used for tints and shades */
static void
interpolate_colors(r, g, b, ic0, ic1, t)
     double *r, *g, *b;
     int ic0, ic1;
     double t;
{
  if (t < 0 || t > 1) {
    fprintf(stderr, "bad color interpolation parameter ("DBL")\n", t);
    exit(1);
  }
  check_color_index(ic0);
  check_color_index(ic1);
  setup_color_table();
 
  *r = color_table[ic0].r + t * (color_table[ic1].r - color_table[ic0].r);
  *g = color_table[ic0].g + t * (color_table[ic1].g - color_table[ic0].g);
  *b = color_table[ic0].b + t * (color_table[ic1].b - color_table[ic0].b);
}
 
/* return the name of a color given its index; may 
   emit a color declaration if it's needed */
static char *
color_name_after_declare_color(int ic)
{
  setup_color_table();
  check_color_index(ic);
  /* check if color declaration is needed */
  if (!color_table[ic].defined_p) {
    if (Verbose)
      fprintf(tfp, "%% declare color %d\n", ic);
    fprintf(tfp, "\\newrgbcolor{%s}{"DBL" "DBL" "DBL"}%%\n", 
	    color_table[ic].name,
	    color_table[ic].r,
	    color_table[ic].g,
	    color_table[ic].b);
    color_table[ic].defined_p = 1;
  }
  return color_table[ic].name;
}
 
/* return the name of a tint or shade given its index;
   may emit a color declaration if it's needed; we make
   the user specify a string buffer for the name because
   a static table would be huge */
static char *
shade_or_tint_name_after_declare_color(name, ist, ic)
     char *name;
     int ist, ic;
{
  char *special, rn[100];
  double r, g, b;
 
  if (ist < 0 || ist > 40) {
    fprintf(stderr, "bad shade/tint index (%d)\n", ist);
    exit(1);
  }
 
  /* black, white, and full saturation are special cases */
  if (ist == 0)
    special = color_name_after_declare_color(CT_BLACK);
  else if (ist == 20)
    special = color_name_after_declare_color(ic);
  else if (ist == 40)
    special = color_name_after_declare_color(CT_WHITE);
  else
    special = 0;
  if (special) {
    strcpy(name, special);
    return name;
  }
 
  /* make sure name and rgb value of full sat color are available in table */
  setup_color_table();
 
  /* name is color with shade/tint index appended */
  sprintf(name, "%sst%s", color_table[ic].name, roman_numeral_from_int(rn, ist)); 
  if (ist < 20 && (color_table[ic].shades & bit(ist)) == 0) { 
    /* this is a shade needing a declaration */
    if (Verbose)
      fprintf(tfp, "%% declare shade %d of %s\n", ist, color_table[ic].name);
    interpolate_colors(&r, &g, &b, CT_BLACK, ic, (double)ist/20.0);
    fprintf(tfp, "\\newrgbcolor{%s}{"DBL" "DBL" "DBL"}%%\n", name, r, g, b);
    color_table[ic].shades |= bit(ist);
  }
  else if (ist > 20 && (color_table[ic].tints & bit(ist - 20)) == 0) {   
    /* this is a tint needing a declaration */
    if (Verbose)
      fprintf(tfp, "%% declare tint %d of %s\n", ist, color_table[ic].name);
    interpolate_colors(&r, &g, &b, ic, CT_WHITE, (double)(ist - 20)/20.0);
    fprintf(tfp, "\\newrgbcolor{%s}{"DBL" "DBL" "DBL"}%%\n", name, r, g, b);
    color_table[ic].tints |= bit(ist - 20);
  }
  return name;
}
 
/* return the name of a gray value given its intensity as a
   the numerator over 20 - 0 is black and 20 is white; may
   emit a gray declaration if it's needed */
static char *
gray_name_after_declare_gray(int ig)
{
  static char names[20][16]; /* grayxviii */
  char rn[100];
 
  if (ig < 0 || ig > 20) {
    fprintf(stderr, "bad gray value (%d)\n", ig);
    exit(1);
  }
  /* black and white are special cases */
  if (ig == 0)
    return color_name_after_declare_color(CT_BLACK);
  if (ig == 20)
    return color_name_after_declare_color(CT_WHITE);
 
  /* check if gray level declaration is needed */
  if (names[ig][0] == '\0') {
    if (Verbose)
      fprintf(tfp, "%% declare gray %d\n", ig);
    sprintf(names[ig], "gray%s", roman_numeral_from_int(rn, ig));
    fprintf(tfp, "\\newgray{%s}{"DBL"}%%\n", names[ig], (double)ig/20.0);
  }
  return names[ig];
}
 
/* print points, 4 per line */
#define PP_NORMAL    0
#define PP_SKIP_LAST 1
#define PP_REPEAT_2  2
 
static void
put_points(list, flag)
     F_point *list;
     int flag;
{
  F_point *p;
  int n_points = 0;
  int countdown = -1;
 
  for (p = list;;) {
    fprintf(tfp, "("DBL","DBL")", 
	    cm_from_1200ths(p->x), 
	    cm_from_1200ths(p->y));
    ++n_points;
    --countdown;
    p = p->next;
    if (!p && flag == PP_REPEAT_2 && n_points > 2) {
      p = list;
      countdown = 2;
    }
    if (!p || (flag == PP_SKIP_LAST && !p->next) || countdown == 0) {
      fprintf(tfp, "\n");
      return;
    }
    if (n_points % 4 == 0)
      fprintf(tfp, "\n\t");
  }
}
 
/* print line/polygon commands to form arrows as Fig does;
   the alternative is to use pstricks arrow end formatting,
   which is done in format_options() */
#define STICK_ARROW                     0
#define TRIANGLE_ARROW                  1
#define INDENTED_BUTT_ARROW             2
#define POINTED_BUTT_ARROW              3
#define LAST_OLD_ARROW POINTED_BUTT_ARROW
/* These are new arrow types as of xfig 3.2.5a */
#define DIAMOND_ARROW                   4
#define CIRCLE_ARROW                    5
#define SEMI_CIRCLE_ARROW               6
#define RECTANGLE_ARROW                 7
#define REVERSE_TRIANGLE_ARROW          8
#define BISHADE_INDENTED_BUTT_ARROW     9
#define TRIANGLE_HALF_ARROW            10
#define INDENTED_BUTT_HALF_ARROW       11
#define POINTED_BUTT_HALF_ARROW        12
#define PARTIAL_REVERSE_TRIANGLE_ARROW 13
#define PARTIAL_RECTANGLE_ARROW        14
 
#define HOLLOW_ARROW 0
#define FILLED_ARROW 1
 
static void
put_arrowhead(x_hd, y_hd, 
              x_dir, y_dir,
              wid, len, thickness,
              type, style,
              pen_color)
     double x_hd, y_hd, x_dir, y_dir, wid, len, thickness;
     int type, style, pen_color;
{
  double dir_len,
    x_bas, y_bas, /* base point of the arrow */
    x_prp, y_prp, /* unit perp vector wrt arrow direction */
    x_lft, y_lft, /* left base point */
    x_rgt, y_rgt, /* right base point */
    x_lfa, y_lfa, /* left alternate point */
    x_rga, y_rga, /* right alternate point */
    t_lft, t_rgt; /* arc (semicircle) arrowhead thetas */
  char fill_fillcolor_option[50], linecolor_option[50];
  char *pen_color_name, *fillcolor_option, *blank_fillcolor_option;
  
  /* set up the pstricks options to fill and leave blank the arrow center */
  blank_fillcolor_option = ",fillcolor=white";
  if (pen_color == -1) {
    fill_fillcolor_option[0] = '\0';
    linecolor_option[0] = '\0';
  }
  else {
    pen_color_name = color_name_after_declare_color(pen_color);
    sprintf(fill_fillcolor_option, ",fillcolor=%s", pen_color_name);
    sprintf(linecolor_option, ",linecolor=%s", pen_color_name);
  }
 
  /* assign arrow fill options based on style flag */
  fillcolor_option = (style == HOLLOW_ARROW) ? blank_fillcolor_option : fill_fillcolor_option;
 
  /* convert to centimeters */
  x_hd = cm_from_1200ths(x_hd);
  y_hd = cm_from_1200ths(y_hd);
  wid = cm_from_1200ths(wid);
  len = cm_from_1200ths(len);
  thickness = cm_from_1200ths(thickness) * Line_weight;
 
  /* make direction vector unit length */
  dir_len = sqrt(x_dir * x_dir + y_dir * y_dir);
  x_dir /= dir_len;
  y_dir /= dir_len;
 
  /* find base of arrowhead */
  x_bas = x_hd - x_dir * len;
  y_bas = y_hd - y_dir * len;
 
  /* find unit perp from direction */
  x_prp = -y_dir;
  y_prp = x_dir;
 
  /* normal baseline endpoints */
  x_lft = x_bas + x_prp * 0.5 * wid;
  y_lft = y_bas + y_prp * 0.5 * wid;
 
  x_rgt = x_lft - x_prp * wid;
  y_rgt = y_lft - y_prp * wid;
 
  /* alternate baseline endpoints */
  x_lfa = x_hd + x_prp * 0.5 * wid;
  y_lfa = y_hd + y_prp * 0.5 * wid;
 
  x_rga = x_lfa - x_prp * wid;
  y_rga = y_lfa - y_prp * wid;
  
  /* warn user if a new arrow type is being used */
  if (type > LAST_OLD_ARROW) 
    warn(W_NEW_ARROW);
 
  /* draw based on type */
  switch (type) {
  case STICK_ARROW:
  simple_arrow:
    if (Verbose)
      fprintf(tfp, "%% stick arrow\n");
    fprintf(tfp,
	    "\\psline[linewidth="DBL"%s]{c-c}"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option,
	    x_lft, y_lft, x_hd, y_hd, x_rgt, y_rgt);
    break;
 
  case TRIANGLE_ARROW:
    if (Verbose)
      fprintf(tfp, "%% triangle arrow\n");
    fprintf(tfp, 
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option, 
	    x_hd, y_hd, x_lft, y_lft, x_rgt, y_rgt);
    break;
 
  case INDENTED_BUTT_ARROW:
    x_bas += x_dir * len * 0.3;
    y_bas += y_dir * len * 0.3;
    if (Verbose)
      fprintf(tfp, "%% indented butt arrow\n");
    fprintf(tfp, 
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option, 
	    x_hd, y_hd, x_lft, y_lft, x_bas, y_bas, x_rgt, y_rgt);
    break;
 
  case POINTED_BUTT_ARROW:
    /* 
     * old method was consistent with fig2dev calc_arrow; we're 
     * changing to match Andreas
     * x_bas -= x_dir * len * 0.3;
     * y_bas -= y_dir * len * 0.3;
    */
    x_lft += x_dir * len * 0.3;
    y_lft += y_dir * len * 0.3;
    x_rgt += x_dir * len * 0.3;
    y_rgt += y_dir * len * 0.3;
    if (Verbose)
      fprintf(tfp, "%% pointed butt arrow\n");
    fprintf(tfp,
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option,
	    x_hd, y_hd, x_lft, y_lft, x_bas, y_bas, x_rgt, y_rgt);
    break;
 
  case DIAMOND_ARROW:
    x_lft += x_dir * len * 0.5;
    y_lft += y_dir * len * 0.5;
    x_rgt += x_dir * len * 0.5;
    y_rgt += y_dir * len * 0.5;
    if (Verbose)
      fprintf(tfp, "%% diamond arrow\n");
    fprintf(tfp,
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option,
	    x_hd, y_hd, x_lft, y_lft, x_bas, y_bas, x_rgt, y_rgt);
   break;
 
  case CIRCLE_ARROW:
    if (Verbose)
      fprintf(tfp, "%% circle arrow\n");
    fprintf(tfp,
	    "\\pscircle[linewidth="DBL"%s,fillstyle=solid%s]("DBL","DBL"){"DBL"}%%\n",
	    thickness, linecolor_option, fillcolor_option,
	    x_hd - x_dir * len/2, y_hd - y_dir * len/2, len/2);
    break;
 
  case SEMI_CIRCLE_ARROW:
    t_lft = atan2(y_prp, x_prp) * (180.0 / PI);
    t_rgt = t_lft + 180;
    if (Verbose)
      fprintf(tfp, "%% semicircle arrow\n");
    fprintf(tfp,
	    "\\psarc[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL"){"DBL"}{"DBL"}{"DBL"}%%\n",
	    thickness, linecolor_option, fillcolor_option,
	    x_hd, y_hd, len/2, t_lft, t_rgt);    
   break;
 
  case RECTANGLE_ARROW:
    if (Verbose)
      fprintf(tfp, "%% rectangle arrow\n");
    fprintf(tfp,
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option,
	    x_lfa, y_lfa, x_lft, y_lft, x_rgt, y_rgt, x_rga, y_rga);
    break;
 
  case REVERSE_TRIANGLE_ARROW:
    if (Verbose)
      fprintf(tfp, "%% reverse triangle arrow\n");
    fprintf(tfp, 
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option, 
	    x_bas, y_bas, x_rga, y_rga, x_lfa, y_lfa);
    break;
 
  case BISHADE_INDENTED_BUTT_ARROW:
    /* maybe could do this in 2 commands with a clip region */
    x_bas += x_dir * len * 0.3;
    y_bas += y_dir * len * 0.3;
    if (Verbose)
      fprintf(tfp, "%% bishade indented butt arrow\n");
    /* clear the arrowhead area */
    fprintf(tfp, 
	    "\\pspolygon[linestyle=none,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    blank_fillcolor_option,
	    x_hd, y_hd, x_lft, y_lft, x_bas, y_bas, x_rgt, y_rgt);
    /* fill the half that's filled */
    if (style == HOLLOW_ARROW) {
      fprintf(tfp, 
	      "\\pspolygon[linestyle=none,fillstyle=solid%s]"
	      "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	      fill_fillcolor_option,
	      x_hd, y_hd, x_lft, y_lft, x_bas, y_bas);
    }
    else {
      fprintf(tfp, 
	      "\\pspolygon[linestyle=none,fillstyle=solid%s]"
	      "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	      fill_fillcolor_option,
	      x_bas, y_bas, x_rgt, y_rgt, x_hd, y_hd);
    }
    /* rule the outline */
    fprintf(tfp, 
	    "\\pspolygon[linewidth="DBL"%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option,
	    x_hd, y_hd, x_lft, y_lft, x_bas, y_bas, x_rgt, y_rgt);
    break;
 
  case TRIANGLE_HALF_ARROW:
     if (Verbose)
      fprintf(tfp, "%% triangle half-arrow\n");
    fprintf(tfp, 
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option, 
	    x_bas, y_bas, x_rgt, y_rgt, x_hd, y_hd);
   break;
 
  case INDENTED_BUTT_HALF_ARROW:
    x_bas += x_dir * len * 0.3;
    y_bas += y_dir * len * 0.3;
    if (Verbose)
      fprintf(tfp, "%% indented butt half-arrow\n");
    fprintf(tfp, 
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option, 
	    x_bas, y_bas, x_rgt, y_rgt, x_hd, y_hd);
    break;
 
  case POINTED_BUTT_HALF_ARROW:
    x_rgt += x_dir * len * 0.3;
    y_rgt += y_dir * len * 0.3;
    if (Verbose)
      fprintf(tfp, "%% pointed butt half-arrow\n");
    fprintf(tfp,
	    "\\pspolygon[linewidth="DBL"%s,fillstyle=solid%s]"
	    "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	    thickness, linecolor_option, fillcolor_option,
	    x_hd, y_hd, x_bas, y_bas, x_rgt, y_rgt);
    break;
 
  case PARTIAL_REVERSE_TRIANGLE_ARROW:
    if (style == HOLLOW_ARROW) {
      if (Verbose)
	fprintf(tfp, "%% reverse stick arrow\n");
      fprintf(tfp,
	      "\\psline[linewidth="DBL"%s,fillstyle=solid%s]{c-c}"
	      "("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	      thickness, linecolor_option, blank_fillcolor_option,
	      x_rga, y_rga, x_bas, y_bas, x_lfa, y_lfa);
    }
    else {
      if (Verbose)
	fprintf(tfp, "%% T-bar arrow\n");
      fprintf(tfp,
	      "\\psline[linewidth="DBL"%s]{c-c}"
	      "("DBL","DBL")("DBL","DBL")\n",
	      thickness, linecolor_option,
	      x_rga, y_rga, x_lfa, y_lfa);
    }
    break;
 
  case PARTIAL_RECTANGLE_ARROW:
    if (style == HOLLOW_ARROW) {
      if (Verbose)
	fprintf(tfp, "%% cup arrow\n");
      fprintf(tfp,
	      "\\psline[linewidth="DBL"%s,fillstyle=solid%s]{c-c}"
	      "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	      thickness, linecolor_option, blank_fillcolor_option,
	      x_lfa, y_lfa, x_lft, y_lft, x_rgt, y_rgt, x_rga, y_rga);
    }
    else {
      if (Verbose)
	fprintf(tfp, "%% cap arrow\n");
      fprintf(tfp,
	      "\\psline[linewidth="DBL"%s]{c-c}"
	      "("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	      thickness, linecolor_option,
	      x_rgt, y_rgt, x_rga, y_rga, x_lfa, y_lfa, x_lft, y_lft);
    }
    break;
 
  default:
    warn(W_UNKNOWN_ARROW);
    goto simple_arrow;
  }
}
 
/* convenience function to put an arrowhead for a line segment with given end points */
static void
put_arrowhead_on_seg(tl, hd,
                     wid, len, thickness,
                     type, style, pen_color)
     F_point *tl, *hd;
     double wid, len, thickness;
     int type, style, pen_color;
{
  put_arrowhead((double)hd->x, (double)hd->y, 
		(double)(hd->x - tl->x), (double)(hd->y - tl->y), 
		wid, len, thickness, type, style, pen_color);
}
 
/**********************************************************************/
/* general formatting                                                 */
/**********************************************************************/
 
/* format_options flag bits */
#define FO_LINE_STYLE bit(0)
#define FO_PEN_COLOR  bit(1)
#define FO_LINE_TERM  bit(2)
#define FO_ARROWS     bit(3)
#define FO_ARROW_SIZE bit(4)
#define FO_FILL       bit(5)
#define FO_JOIN_STYLE bit(6)
 
/* format terminator option strings, one for square 
   bracket options and one for curly brackets */
static void
format_terminators(opts_sqrb, opts_curb, 
                   flags,
                   ch_arrow, arrow,
                   cap_style)
     char *opts_sqrb, *opts_curb;
     int flags;
     int ch_arrow;
     F_arrow *arrow;
     int cap_style;
{
  double inset, length;
 
  opts_sqrb[0] = opts_curb[0] = '\0';
 
  if ((flags & FO_ARROWS) && arrow) {
 
    /* default will be the given arrow character, no inset, specified length */
    sprintf(opts_curb, "%c", ch_arrow);
    inset = 0.0;
    length = arrow->ht/arrow->wid;
 
    /* draw based on type; those cases that are proper arrows fall
       through to a block on the bottom that sets arrow size options;
       those that are special cases set opts_sqrb and opts_curb and
       return directly */
    switch (arrow->type) {
    case STICK_ARROW:
    simple_arrow:
      inset = 1.0;
      break;
 
    case TRIANGLE_HALF_ARROW:
      /* closest we can get to a half-arrow is a full arrow */
      warn(W_TERMINATOR_ARROW);
    case TRIANGLE_ARROW:
      break;
 
    case BISHADE_INDENTED_BUTT_ARROW:
    case INDENTED_BUTT_HALF_ARROW:
      /* closest we can get to bishade and indented butt arrows is a
	 full indented butt arrow */
      warn(W_TERMINATOR_ARROW);
    case INDENTED_BUTT_ARROW:
      inset = 0.3;
      break;
 
    case POINTED_BUTT_HALF_ARROW:
      /* closest we can get to a pointed butt half arrow is a pointed
	 butt full arrow */
      warn(W_TERMINATOR_ARROW);
    case POINTED_BUTT_ARROW:
      inset = -0.3;
      /* It looks like Andreas is doing this, but original calc_arrow does not. */
      length *= 1.0 / (1.0 - inset);
      break;
 
    case RECTANGLE_ARROW:
      /* closest we can get to a rectangle is a diamond */
      warn(W_TERMINATOR_ARROW);
    case DIAMOND_ARROW:
      inset = -1.0;
      length *= 1.0 / (1.0 - inset);
      break;
 
    case SEMI_CIRCLE_ARROW:
      warn(W_TERMINATOR_ARROW);
      /* closest we can get to semicircle is centered disk */
      if (flags & FO_ARROW_SIZE)
	sprintf(opts_sqrb, "dotsize="DBL" 2", cm_from_1200ths(arrow->ht));
      strcpy(opts_curb, arrow->style == HOLLOW_ARROW ? "o" : "*");
      return;
 
    case CIRCLE_ARROW:
      if (flags & FO_ARROW_SIZE)
	sprintf(opts_sqrb, "dotsize="DBL" 2", cm_from_1200ths(arrow->ht));
      strcpy(opts_curb, arrow->style == HOLLOW_ARROW ? "oo" : "**");
      return;
    
    case PARTIAL_REVERSE_TRIANGLE_ARROW:
      if (arrow->style == HOLLOW_ARROW) {
	/* reverse stick arrow */
	inset = 1;
	sprintf(opts_curb, "%c", (ch_arrow == '<') ? '>' : '<');
      }
      else {
	/* t-bar centered on end */
	if (flags & FO_ARROW_SIZE)
	  sprintf(opts_sqrb, "tbarsize="DBL" 2", cm_from_1200ths(arrow->wid));
	strcpy(opts_curb, "|");
	return;
      }
      break;
 
    case REVERSE_TRIANGLE_ARROW:
      inset = 0;
      sprintf(opts_curb, "%c", (ch_arrow == '<') ? '>' : '<');
      break;
      
    case PARTIAL_RECTANGLE_ARROW:
      /* closest we can get to a cup is a cap */
      if (arrow->style == HOLLOW_ARROW)
	warn(W_TERMINATOR_ARROW);
      if (flags & FO_ARROW_SIZE)
	sprintf(opts_sqrb, "tbarsize="DBL" 2,bracketlength="DBL, 
		cm_from_1200ths(arrow->wid), 
		arrow->ht/arrow->wid);
      sprintf(opts_curb, "%c", (ch_arrow == '<') ? '[' : ']');
      return;
 
    default:
      warn(W_UNKNOWN_ARROW);
      goto simple_arrow;
    }
    if (flags & FO_ARROW_SIZE) {
 
      sprintf(opts_sqrb, "arrowsize="DBL" 2,arrowlength=%.5lf,arrowinset=%.5lf",
	      cm_from_1200ths(arrow->wid), length, inset);
      if (arrow->style == HOLLOW_ARROW) {
        /* warn user hollow arrows require additional package pstricks-add */
        warn(W_HOLLOW_ARROW);
        append_to_comma_list(&opts_sqrb, "ArrowFill=false");
      }
    }
  }
  else {
    strcpy(opts_curb, 
	   cap_style == 1 ? "c" : /* round */
	   cap_style == 2 ? "C" : /* extended square */
	   "");  /* butt (flush square) */
  }
  /* thickness is ignored */
}
 
/* Format an options string that optionally takes care of
 
line style
pen color
line terminators
line join
arrows (only if line terminators are turned on)
arrow size (if user didn't ask for default sizes)
fill style, fill color and/or hatch color
 
This is a little nasty because some of pstricks options go in square brackets and others
in curly brackets, and early versions required \pstverb commands to set line join.
For this purpose, prefix and postfix are used.  So this procedure formats all the
strings and combines the brackets at the end.  The square brackets options can be given 
initial contents by providing sqrb_init.  Prefix and postfix are set and returned 
separately. */
static char*
format_options(options, prefix, postfix,
               sqrb_init, flags, thickness, style, style_val, 
               pen_color, join_style, cap_style, fill_style, fill_color,
               back_arrow, fore_arrow,
               hatch_angle_offset)
     char *options, *prefix, *postfix, *sqrb_init;
     int flags, thickness, style;
     double style_val;
     int pen_color, join_style, cap_style, fill_style, fill_color;
     F_arrow *back_arrow, *fore_arrow;
     double hatch_angle_offset;
{
  char tmps[256], tmps_alt[256],
    tmpc[256],
    opts_sqrb[256], /* square bracket options */ 
    opts_curb[256]; /* curly bracket options */
  char *p_sqrb;
 
  /* set up square bracket option buffer */
  if (sqrb_init)
    strcpy(opts_sqrb, sqrb_init);
  else
    opts_sqrb[0] = '\0';
  p_sqrb = opts_sqrb;
 
  /* set up curly bracket option buffer */
  opts_curb[0] = '\0';
 
  if (flags & FO_LINE_STYLE) {
 
    /* fig allows filled lines where the line has zero width;
       the intent is to draw the filling with no line at all
       pstricks always draws a line even if linewidth is zero
       argh...
       so we will translate to a pseudo-line style -2, meaning
       linestyle=none, which is how PSTricks does filling with
       no line */
    if (thickness == 0) {
      style = -2;
    }
    else {
      sprintf(tmps, "linewidth="DBL"", 
	      cm_from_1200ths(thickness) * Line_weight);
      append_to_comma_list(&p_sqrb, tmps);
    }
 
    switch (style) {
 
    case -2:
      append_to_comma_list(&p_sqrb, "linestyle=none");
      break;
 
    case -1: /* default; use solid */
    case SOLID_LINE:
      break;
 
    case DOTTED_LINE:
      sprintf(tmps, "linestyle=dotted,dotsep="DBL"", 
	      cm_from_80ths(style_val));
      append_to_comma_list(&p_sqrb, tmps);
      break;
 
    case DASH_LINE:
      sprintf(tmps, "linestyle=dashed,dash="DBL" "DBL"", 
	      cm_from_80ths(style_val), 
	      cm_from_80ths(style_val));
      append_to_comma_list(&p_sqrb, tmps);
      break;
 
    case DASH_DOT_LINE:    /* normal dashes with small spaces */
      warn(W_DASH_DOT);
      sprintf(tmps, "linestyle=dashed,dash="DBL" "DBL" "DBL" "DBL"", 
	      cm_from_80ths(style_val), 
	      cm_from_80ths(style_val)/2,
	      2*cm_from_1200ths(thickness) * Line_weight,
	      cm_from_80ths(style_val)/2);
      append_to_comma_list(&p_sqrb, tmps);
      break;
 
    case DASH_2_DOTS_LINE: /* small dashes with normal spaces */
      warn(W_DASH_DOTS);
      sprintf(tmps, "linestyle=dashed,dash="DBL" "DBL" "DBL" "DBL" "DBL" "DBL"", 
	      cm_from_80ths(style_val), 
	      cm_from_80ths(style_val)/3,
	      2*cm_from_1200ths(thickness) * Line_weight,
	      cm_from_80ths(style_val)/3,
	      2*cm_from_1200ths(thickness) * Line_weight,
	      cm_from_80ths(style_val)/3);
      append_to_comma_list(&p_sqrb, tmps);
      break;
 
    case DASH_3_DOTS_LINE: /* small dashes with small spaces */
      warn(W_DASH_DOTS);
      sprintf(tmps, "linestyle=dashed,dash="DBL" "DBL" "DBL" "DBL" "DBL" "DBL" "DBL" "DBL, 
	      cm_from_80ths(style_val), 
	      cm_from_80ths(style_val)/4,
	      2*cm_from_1200ths(thickness) * Line_weight,
	      cm_from_80ths(style_val)/4,
	      2*cm_from_1200ths(thickness) * Line_weight,
	      cm_from_80ths(style_val)/4,
	      2*cm_from_1200ths(thickness) * Line_weight,
	      cm_from_80ths(style_val)/4);
      append_to_comma_list(&p_sqrb, tmps);
      break;
 
    default:
      fprintf(stderr, "bad line style (%d)\n", style);
      exit(1);
    }
  }
 
  /* add linejoin in one of two ways, which depend on PSTricks
     version we're talking to; old way was with \pstverb new is
     with [linejoin=N] */
  if (prefix && postfix) {
    if ((flags & FO_JOIN_STYLE) != 0 && join_style > 0) {
 
      if (Pst_version->n_updates == 0)
        warn(W_LINEJOIN);
 
      switch (Linejoin) {
      case LJ_PSTVERB:
	sprintf(prefix,  "\\pstVerb{%d setlinejoin}%%\n", join_style);
	sprintf(postfix, "\\pstVerb{0 setlinejoin}%%\n");
	break;
      case LJ_PSTOPTION:
	prefix[0] = postfix[0] = '\0';
	sprintf(tmps,  "linejoin=%d", join_style);
	append_to_comma_list(&p_sqrb, tmps);
	break;
      default:
	fprintf(stderr, "bad Linejoin value %d\n", Linejoin);
	break;
      }
    }
    else 
      prefix[0] = postfix[0] = '\0';    
  }
 
  /* add cap style and optionally the arrows */
  if (flags & FO_LINE_TERM) {
    format_terminators(tmps, tmpc, flags, '<', back_arrow, cap_style);
    strcat(opts_curb, tmpc);
    strcat(opts_curb, "-");
    format_terminators(tmps_alt, tmpc, flags, '>', fore_arrow, cap_style);
    strcat(opts_curb, tmpc);
    /* forward arrow parameters take precedence over back; can't have both */
    append_to_comma_list(&p_sqrb, tmps_alt[0] == '\0' ? tmps : tmps_alt);
  }
 
  /* deal with pen color */
  if ((flags & FO_PEN_COLOR) && pen_color != -1) {
    sprintf(tmps, "linecolor=%s", color_name_after_declare_color(pen_color));
    append_to_comma_list(&p_sqrb, tmps);
  }
 
  if ((flags & FO_FILL) && fill_style != -1) {
    if (fill_color == CT_WHITE && fill_style <= 20)
      /* black to white gray fill */
      sprintf(tmps, "fillstyle=solid,fillcolor=%s", gray_name_after_declare_gray(fill_style));
    else if ((fill_color == CT_BLACK || fill_color == -1) && fill_style <= 20)
      /* white to black gray fill */
      sprintf(tmps, "fillstyle=solid,fillcolor=%s", gray_name_after_declare_gray(20 - fill_style));
    else if (fill_style <= 40)
      /* shade or tint fill */
      sprintf(tmps, "fillstyle=solid,fillcolor=%s", 
	      shade_or_tint_name_after_declare_color(tmpc, fill_style, fill_color));
    else {
      char *type = 0, *ps;
      int angle = 0;
 
      /* hatch pattern */
      switch (fill_style) {
      case 41: /* 30 degrees left diagonal */
	type = "hlines";
	angle = 30;
	break;
      case 42: /* 30 degrees right diagonal */
	type = "hlines";
	angle = -30;
	break;
      case 43: /* 30 degree crosshatch */
	type = "crosshatch";
	angle = 30;
	break;
      case 44: /* 45 degrees left diagonal */
	type = "vlines";
	angle = 45;
	break;
      case 45: /* 45 degrees right diagonal */
	type = "hlines";
	angle = -45;
	break;
      case 46: /* 45 degree crosshatch */
	type = "crosshatch";
	angle = 45;
	break;
      case 49: /* horizontal lines */
	type = "hlines";
	angle = 0;
	break;
      case 50: /* vertical lines */
	type = "vlines";
	angle = 0;
	break;
      case 51: /* horizontal/vertical degree crosshatch */
	type = "crosshatch";
	angle = 0;
	break;
      default:
	warn(W_HATCH_PATTERN);
	type = "crosshatch";
	angle = 10;
	break;
      }
      /* build up fill and hatch color strings, accounting for default colors */
      tmps[0] = '\0';
      ps = tmps;
      if (fill_color != -1) {
        sprintf(tmps_alt, "fillcolor=%s", color_name_after_declare_color(fill_color));
        append_to_comma_list(&ps, tmps_alt);
      }
      if (pen_color != -1) {
        sprintf(tmps_alt, "hatchcolor=%s", color_name_after_declare_color(pen_color));
        append_to_comma_list(&ps, tmps_alt);
      }
      /* negative sign on angle for fig's strange clockwise system */
      sprintf(tmps_alt, "fillstyle=%s*,hatchangle=%.2lf", type, -(angle + hatch_angle_offset));
      append_to_comma_list(&ps, tmps_alt);
    }
    append_to_comma_list(&p_sqrb, tmps);
  }
 
  if (opts_curb[0] && opts_sqrb[0])
    sprintf(options, "[%s]{%s}",  opts_sqrb, opts_curb);
  else if (opts_sqrb[0])
    sprintf(options, "[%s]",  opts_sqrb);
  else if (opts_curb[0])
    sprintf(options, "{%s}", opts_curb);
  else
    options[0] = '\0';
 
  return options;
}
 
/* do conversions for non-eps files; may try to
   run bmeps if user asked for it; else we just
   convert file paths to place where the user
   should put the eps-converted files him/herself 
 
   must take some care that the conversion folder
   doesn't have name clashes and that conversion
   for a given source path is done only once */
void do_eps_conversion(eps_path, src)
     char *eps_path, *src;
{
  int i, ibmeps, errval, base_index, islash, iext;
  char buf[256], uniqified_base[256], *base, *ext;
  STRING_TABLE_NODE *stn;
 
  static STRING_TABLE converted[1], base_names[1];
 
#define BMEPS_PNG 0
#define BMEPS_JPG 1
#define BMEPS_PNM 2
#define BMEPS_TIF 3
 
  static char *bmeps_ext_tbl[] = {
    "png",
    "jpg",
    "pnm",
    "tif",
  };
 
  static struct {
    char *ext; 
    int bmeps_type;
  } ext_tbl[] = {
    { "png",  BMEPS_PNG},
    { "jpg",  BMEPS_JPG},
    { "jpeg", BMEPS_JPG},
    { "pnm",  BMEPS_PNM},
    { "tif",  BMEPS_TIF},
    { "tiff", BMEPS_TIF},
  };
 
  /* if we've already converted this one, 
     return the eps path for it */
  stn = string_lookup_val(converted, src);
  if (stn) {
    strcpy(eps_path, (char*)stn->val.vp);
    return;
  }
 
  islash = -1;
  for (i = 0; src[i]; i++) {
    switch (src[i]) {
    case '\\':
      warn(W_SLASH_CONVERT);
      islash = i;
      buf[i] = '/';
      break;
    case '/':
      islash = i;
      buf[i] = src[i];
      break;
    case ' ':
    case '>':
    case '<':
    case '|':
    case '$':
    case '%':
    case '*':
    case '?':
      warn(W_FN_CHARS);
      buf[i] = src[i];
      break;
    default:
      buf[i] = src[i];
      break;
    }
  }
  buf[i] = '\0';
 
  /* get base file name from slash location */
  base = (islash == -1) ? buf : &buf[islash + 1];
 
  /* find and cut off extension */
  iext = strlen(buf);
  for (i = iext - 1; i > islash; i--) {
    if (buf[i] == '.') {
      buf[i] = '\0';
      iext = i + 1;
      break;
    }
  }
  /* lowercase the extension */
  for (i = iext; buf[i]; i++) {
    if ('A' <= buf[i] && buf[i] <= 'Z')
      buf[i] += 'a' - 'A';
  }
 
  /* all done ith extension */
  ext = &buf[iext];
 
  /* build new path to conversion directory 
     if the base is already in the table, then
     the name is already in use for some other
     converted path with the same base, so 
     uniqify the base name for this conversion */
  stn = string_lookup_val(base_names, base);
  if (stn) {
    base_index = ++stn->val.i;
    sprintf(uniqified_base, "%s-%03d", base, base_index);
  }
  else {
    strcpy(uniqified_base, base);
  }
  sprintf(eps_path, "%s%s", Pic_convert_dir, uniqified_base);
 
  /* now we can fill in the tables; we act as if conversion
     succeeded even if it doesn't because the LaTeX file is useless
     unless user can get conversion to eps done him/herself */
  stn = add_string(converted, src);
  stn->val.vp = xstrdup(eps_path);
  add_string(base_names, uniqified_base);
 
  if (Pic_convert_p) {
    /* see if it's a type we know about */
    ibmeps = -1;
    for (i = 0; i < ARRAY_SIZE(ext_tbl); i++) {
      if (strcmp(ext_tbl[i].ext, ext) == 0) {
        ibmeps = i;
        break;
      }
    }
    if (ibmeps == -1)
      warn(W_UNK_PIC_TYPE);
    else {
      char cmd[1024];
      /* try to run bmeps */
      sprintf(cmd, "bmeps -c -t %s \"%s\" \"%s.eps\"", 
	      bmeps_ext_tbl[ibmeps], src, eps_path);
      if (Verbose)
        fprintf(stderr, "running system(%s): ", cmd);
      errval = system(cmd);
      if (Verbose)
        fprintf(stderr, "returned %d\n", errval);
      if (errval == -1)
        warn(W_SYSTEM_EXEC);
      else if (errval != 0)
        warn(W_BMEPS_ERR);
      else
        warn(W_PIC_CONVERT);
    }
  }
  else {
    warn(W_EPS_NEEDED);
  }
}
 
/**********************************************************************/
/* line drawing                                                       */
/**********************************************************************/
 
void
genpstrx_line(line)
     F_line *line;
{
  F_point *p;
  int llx, lly, urx, ury;
  char options[256], file_name[256], prefix[256], postfix[256];
 
  /* print any comments prefixed with "%" */
  print_comments("%% ",line->comments,"");
 
  if (!line->points) 
    return;
 
  /* adjust for bounding box and top<->bottom flip */
  for (p = line->points; p; p = p->next)
    convert_pt_to_image_bb_in_place(p);
 
  /* if only one point, draw a dot */
  if (!line->points->next || 
      /* polygons always have the first point repeated */
      (line->type == T_POLYGON && !line->points->next->next)) {
    if (Verbose)
      fprintf(tfp, "%% line, length zero\n");
    fprintf(tfp, "\\psdots[dotsize="DBL"]("DBL","DBL")\n", 
	    cm_from_1200ths(line->thickness) * Line_weight / 2,
	    cm_from_1200ths(line->points->x), 
	    cm_from_1200ths(line->points->y));
    return;
  }
 
  switch (line->type) {
 
  case T_ARC_BOX: {
    int radius;
 
    if (Verbose)
      fprintf(tfp, "%% arc box\n");
 
    find_bounding_box_of_points(&llx, &lly, &urx, &ury, line->points);
 
    radius = line->radius;		/* radius of the corner */
    fold_min_int(&radius, (urx - llx) / 2);
    fold_min_int(&radius, (ury - lly) / 2);
    sprintf(options, "cornersize=absolute,linearc="DBL"", cm_from_1200ths(radius));
 
    format_options(options, 0, 0, options, 
		   FO_LINE_STYLE | FO_PEN_COLOR | FO_FILL, 
		   line->thickness, line->style, line->style_val,
		   line->pen_color, line->join_style, line->cap_style, 
		   line->fill_style, line->fill_color, 
		   0,0,0.0);
 
    fprintf(tfp, "\\psframe%s("DBL","DBL")("DBL","DBL")\n",
	    options,
	    cm_from_1200ths(llx), cm_from_1200ths(lly), 
	    cm_from_1200ths(urx), cm_from_1200ths(ury));
  } break;
 
  case T_PIC_BOX: {
 
    int rot, x0, y0, wd, ht;
 
    /* rotation angle ends up encoded in diagonal
       x and y order; use these to decode them */
    static int rot_from_signs[2][2] = {
      {  90,   0 },  /* x normal   { y normal, inverted } */
      { 180, -90 }}; /* x inverted { y normal, inverted } */
    static int flipped_rot_from_signs[2][2] = {
      {   0,  90 },  /* x normal   { y normal, inverted } */
      { -90, 180 }}; /* x inverted { y normal, inverted } */
 
    if (Verbose)
      fprintf(tfp, "%% pic box\n");
    warn(W_PIC);
 
    find_bounding_box_of_points(&llx, &lly, &urx, &ury, line->points);
 
    x0 = llx;
    y0 = lly;
    wd = urx - llx;
    ht = ury - lly;
    if (line->pic->flipped) {
      /* a flip is equivalent to a 90d rotation 
	 and a mirror about the x-axis */
      rot = flipped_rot_from_signs
	[line->points->next->next->x < line->points->x]
        [line->points->next->next->y < line->points->y];
      ht = -ht;
    }
    else {
      rot = rot_from_signs
	[line->points->next->next->x < line->points->x]
        [line->points->next->next->y < line->points->y];
    }
 
    /* format the pic file name, but this may also do a conversion to eps! */
    do_eps_conversion(file_name, line->pic->file);
 
    fprintf(tfp, 
	    "\\rput[bl]("DBL","DBL")"
	    "{\\includegraphics[origin=c,angle=%d,width=%.4lf\\unit,totalheight=%.4lf\\unit]{%s}}%%\n", 
	    cm_from_1200ths(x0), cm_from_1200ths(y0),
	    rot,
	    cm_from_1200ths(wd), cm_from_1200ths(ht),
	    file_name);
  } break;
 
  case T_BOX:  
    if (Verbose)
      fprintf(tfp, "%% box\n");
 
    find_bounding_box_of_points(&llx, &lly, &urx, &ury, line->points);
 
    format_options(options, prefix, postfix, 0, 
		   FO_LINE_STYLE | FO_JOIN_STYLE | FO_PEN_COLOR | FO_FILL, 
		   line->thickness, line->style, line->style_val,
		   line->pen_color, line->join_style, line->cap_style, 
		   line->fill_style, line->fill_color, 
		   0,0,0.0);
 
    fprintf(tfp, "%s\\psframe%s("DBL","DBL")("DBL","DBL")\n%s",
	    prefix,
	    options,
	    cm_from_1200ths(llx), cm_from_1200ths(lly), 
	    cm_from_1200ths(urx), cm_from_1200ths(ury),
	    postfix);    
    break;
 
  case T_POLYLINE:
    if (Verbose)
      fprintf(tfp, "%% polyline\n");
 
#define FO_POLYLINE_STD (FO_LINE_STYLE | FO_JOIN_STYLE | FO_PEN_COLOR | FO_FILL | FO_LINE_TERM)
 
    format_options(options, prefix, postfix, 0, 
		   Arrows == A_XFIG      ?  FO_POLYLINE_STD :
		   Arrows == A_PSTRICKS  ? (FO_POLYLINE_STD | FO_ARROWS | FO_ARROW_SIZE) :
		   /* default */           (FO_POLYLINE_STD | FO_ARROWS), 
		   line->thickness, line->style, line->style_val,
		   line->pen_color, line->join_style, line->cap_style, 
		   line->fill_style, line->fill_color,
		   line->back_arrow,line->for_arrow, 0.0);
 
    fprintf(tfp, "%s\\psline%s", prefix, options);
    put_points(line->points, PP_NORMAL);
    fprintf(tfp, "%s", postfix);
 
    if (Arrows == A_XFIG) {
      if (line->back_arrow)
	put_arrowhead_on_seg(line->points->next, line->points,
			     line->back_arrow->wid, line->back_arrow->ht, 
			     max_dbl(line->back_arrow->thickness, (double)line->thickness),
			     line->back_arrow->type, line->back_arrow->style, 
			     line->pen_color);
      if (line->for_arrow) {
	for (p = line->points; p->next->next; p = p->next)
	  /* skip */ ;
	put_arrowhead_on_seg(p, p->next,
			     line->for_arrow->wid, line->for_arrow->ht, 
			     max_dbl(line->for_arrow->thickness, (double)line->thickness),
			     line->for_arrow->type, line->for_arrow->style,
			     line->pen_color);
      }
    }
    break;
 
  case T_POLYGON: 
    if (Verbose)
      fprintf(tfp, "%% polygon\n");
 
    /* if >2 points (3 with repeat at end), draw a polygon, else a line */
    if (line->points->next->next->next) {
      format_options(options, prefix, postfix, 0, 
		     FO_LINE_STYLE | FO_JOIN_STYLE | FO_PEN_COLOR | FO_FILL, 
		     line->thickness, line->style, line->style_val,
		     line->pen_color, line->join_style, line->cap_style, line->fill_style, line->fill_color, 
		     0,0,0.0);
      fprintf(tfp, "%s\\pspolygon%s", prefix, options);
    }
    else {
      format_options(options, prefix, postfix, 0, 
		     FO_LINE_STYLE | FO_PEN_COLOR,  /* no JOIN_STYLE clears postfix */
		     line->thickness, line->style, line->style_val,
		     line->pen_color, line->join_style, line->cap_style, line->fill_style, line->fill_color,
		     0,0, 0.0);
      fprintf(tfp, "\\psline%s", options);
    }
    put_points(line->points, PP_SKIP_LAST);
    fprintf(tfp, "%s", postfix);
    break;
  }
}
 
/**********************************************************************/
/* spline drawing                                                     */
/**********************************************************************/
 
static void
put_bezier(options,
           p1x, p1y, /* 3 user control points */
           p2x, p2y,
           p3x, p3y)
     char *options;
     double p1x, p1y, p2x, p2y, p3x, p3y;
{
  fprintf(tfp, "\\psbezier%s("DBL","DBL")("DBL","DBL")("DBL","DBL")("DBL","DBL")\n",
	  options,
	  p1x, p1y,
	  p1x + 2.0/3.0 * (p2x - p1x), p1y + 2.0/3.0 * (p2y - p1y),
	  p3x + 2.0/3.0 * (p2x - p3x), p3y + 2.0/3.0 * (p2y - p3y),
	  p3x, p3y);
}
 
void
genpstrx_spline(spline)
     F_spline *spline;
{
  char options[256];
  F_point *p, *q, *r;
  int flags;
 
  if (!spline->points)
    return;
 
  /* warn user this code is completely untested */
  warn(W_OLD_SPLNE);
 
  /* adjust for bounding box and top<->bottom flip */
  for (p = spline->points; p; p = p->next)
    convert_pt_to_image_bb_in_place(p);
 
  /* if only one point, draw a dot */
  if (!spline->points->next) {
    if (Verbose)
      fprintf(tfp, "%% spline, length zero\n");
    fprintf(tfp, "\\psdots[dotsize="DBL"]("DBL","DBL")\n", 
	    cm_from_1200ths(spline->thickness) * Line_weight / 2,
	    cm_from_1200ths(spline->points->x), 
	    cm_from_1200ths(spline->points->y));
    return;
  }
 
  /* if two points, draw a line */
  if (!spline->points->next->next) {
 
#define FO_SPLINE_STD (FO_LINE_STYLE | FO_PEN_COLOR | FO_LINE_TERM)
 
    format_options(options, 0, 0, 0, 
		   Arrows == A_XFIG      ?  FO_SPLINE_STD :
		   Arrows == A_PSTRICKS  ? (FO_SPLINE_STD | FO_ARROWS | FO_ARROW_SIZE) :
		   /* default */           (FO_SPLINE_STD | FO_ARROWS), 
		   spline->thickness, spline->style, spline->style_val,
		   spline->pen_color, 0, spline->cap_style, spline->fill_style, spline->fill_color,
		   spline->back_arrow,spline->for_arrow, 0.0);
 
    fprintf(tfp, "\\psline%s", options);
    put_points(spline->points, PP_NORMAL);
 
    if (Arrows == A_XFIG) {
      if (spline->back_arrow)
        put_arrowhead_on_seg(spline->points->next, spline->points,
			     spline->back_arrow->wid, spline->back_arrow->ht, 
			     max_dbl(spline->back_arrow->thickness, (double)spline->thickness),
			     spline->back_arrow->type, spline->back_arrow->style, 
			     spline->pen_color);
      if (spline->for_arrow) {
        for (p = spline->points; p->next->next; p = p->next)
          /* skip */ ;
        put_arrowhead_on_seg(p, p->next,
			     spline->for_arrow->wid, spline->for_arrow->ht, 
			     max_dbl(spline->for_arrow->thickness, (double)spline->thickness),
			     spline->for_arrow->type, spline->for_arrow->style,
			     spline->pen_color);
      }
    }
    return;
  }
 
  /* general case */
  flags = closed_spline(spline) ? (FO_LINE_STYLE | FO_PEN_COLOR | FO_FILL) :
    /* open spline flags */   
    Arrows == A_XFIG      ?  FO_SPLINE_STD :
    Arrows == A_PSTRICKS  ? (FO_SPLINE_STD | FO_ARROWS | FO_ARROW_SIZE) :
    /* default */           (FO_SPLINE_STD | FO_ARROWS), 
 
    format_options(options, 0, 0, 0, flags,
		   spline->thickness, spline->style, spline->style_val,
		   spline->pen_color, 0, spline->cap_style, spline->fill_style, spline->fill_color,
		   spline->back_arrow,spline->for_arrow, 0.0);
 
  if (int_spline(spline)) {
    /* interpolating spline; just use pscurve */
    if (closed_spline(spline)) {
      fprintf(tfp, "\\psecurve%s", options);
      put_points(spline->points, PP_REPEAT_2);
    }
    else {
      fprintf(tfp, "\\pscurve%s", options);
      put_points(spline->points, PP_NORMAL);
    }
  }
  else {
    /* approximating spline
       Output is a set of end-to-end joined bezier curves. Control
       polygons are computed between midpoints of control polygon segments.
       The inner vertices are 2/3 of the way to the user control vertex. */
    if (closed_spline(spline)) {
      /* get last 2 points into q (last) and r (second last) */
      for (r = 0, q = spline->points;
	   q->next; 
	   r = q, q = q->next)
        /* skip */ ;
      /* Points pqr are always a series of three adjacent points on the control poly. 
	 This loop will cover the last two vertices twice, which closes the curve. */
      for (p = spline->points; p != 0; r = q, q = p, p = p->next) {
        put_bezier(options, 
		   1.0/2.0 * cm_from_1200ths(r->x + q->x), 1.0/2.0 * cm_from_1200ths(r->y + q->y), 
		   cm_from_1200ths(q->x),                  cm_from_1200ths(q->y), 
		   1.0/2.0 * cm_from_1200ths(q->x + p->x), 1.0/2.0 * cm_from_1200ths(q->y + p->y));
      }
    }
    else {
      /* make pqr the first three points */
      r = spline->points;
      q = r->next;
      p = q->next;
 
      /* first point and last points handled specially so curve hits them */
      put_bezier(options, 
		 cm_from_1200ths(r->x),                  cm_from_1200ths(r->y), 
		 cm_from_1200ths(q->x),                  cm_from_1200ths(q->y), 
		 1.0/2.0 * cm_from_1200ths(q->x + p->x), 1.0/2.0 * cm_from_1200ths(q->y + p->y));
 
      for (r = q, q = p, p = p->next; p->next != 0; r = q, q = p, p = p->next) {
        put_bezier(options, 
		   1.0/2.0 * cm_from_1200ths(r->x + q->x), 1.0/2.0 * cm_from_1200ths(r->y + q->y), 
		   cm_from_1200ths(q->x),                  cm_from_1200ths(q->y), 
		   1.0/2.0 * cm_from_1200ths(q->x + p->x), 1.0/2.0 * cm_from_1200ths(q->y + p->y));
      }
 
      put_bezier(options, 
		 1.0/2.0 * cm_from_1200ths(r->x + q->x), cm_from_1200ths(r->y + q->y), 
		 cm_from_1200ths(q->x),        cm_from_1200ths(q->y), 
		 cm_from_1200ths(p->x),        cm_from_1200ths(p->y));
    }
  }
}
 
void
genpstrx_ellipse(ellipse)
     F_ellipse *ellipse;
{
  int x, y;
  char options[256];
 
  convert_xy_to_image_bb(&x, &y, ellipse->center.x, ellipse->center.y);
 
  /* note the kludge to compensate for rotation in hatch pattern angle */
  format_options(options, 0, 0, 0, 
		 FO_LINE_STYLE | FO_PEN_COLOR | FO_FILL, 
		 ellipse->thickness, ellipse->style, ellipse->style_val,
		 ellipse->pen_color, 0, 0, ellipse->fill_style, ellipse->fill_color, 
		 0,0, ellipse->angle * (180.0 / PI));
 
  /* cull out circles as a special case */
  if (ellipse->radiuses.x == ellipse->radiuses.y) {
    fprintf(tfp, "\\pscircle%s("DBL","DBL"){"DBL"}%%\n",
	    options,
	    cm_from_1200ths(x), cm_from_1200ths(y), 
	    cm_from_1200ths(ellipse->radiuses.x));
  }
  else if (ellipse->angle == 0) {
    fprintf(tfp, "\\psellipse%s("DBL","DBL")("DBL","DBL")\n",
	    options,
	    cm_from_1200ths(x), cm_from_1200ths(y), 
	    cm_from_1200ths(ellipse->radiuses.x), cm_from_1200ths(ellipse->radiuses.y));
  }
  else {
    if (ellipse->fill_style > 40)
      warn(W_ELLIPSE_HATCH);
    fprintf(tfp, "\\rput{"DBL"}("DBL","DBL"){\\psellipse%s(0,0)("DBL","DBL")}%%\n",
	    ellipse->angle * (180.0 / PI),
	    cm_from_1200ths(x), cm_from_1200ths(y), 
	    options,
	    cm_from_1200ths(ellipse->radiuses.x), cm_from_1200ths(ellipse->radiuses.y));
  }
}
 
/**********************************************************************/
/* text drawing                                                       */
/**********************************************************************/
 
static char *
ref_pt_str_from_text_type(int type)
{
  switch (type) {
  case T_LEFT_JUSTIFIED:
  case DEFAULT:
    return "lb";
  case T_CENTER_JUSTIFIED:
    return "b";
  case T_RIGHT_JUSTIFIED:
    return "rb";
  default:
    fprintf(stderr, "unknown text position (%d)\n", type);
    exit(1);
  }
}
 
static char *
just_str_from_text_type(int type)
{
  switch (type) {
  case T_LEFT_JUSTIFIED:
  case DEFAULT:
    return "l";
  case T_CENTER_JUSTIFIED:
    return "c";
  case T_RIGHT_JUSTIFIED:
    return "r";
  default:
    fprintf(stderr, "unknown text position (%d)\n", type);
    exit(1);
  }
}
 
static char longest_translation[] = "$\\backslash$";
static struct x_t { 
  int ch; 
  char *translation; 
} translation_table[] = {
  {'\\', longest_translation },
  { '{', "$\\{$" },
  { '}', "$\\}$" },
  { '>', "$>$" },
  { '<', "$<$" },
  { '^', "\\^{}" },
  { '~', "\\~{}" },
  { '$', "\\$" },
  { '&', "\\&" },
  { '#', "\\#" },
  { '_', "\\_" },
  { '%', "\\%" },
  {'\n', "\\\\\n" },
  {   0, 0 }
};
 
void 
translate_tex_specials(char *buf, int ch)
{
  struct x_t *p;
 
  for (p = translation_table; p->ch; p++)
    if (p->ch == ch) {
      strcpy(buf, p->translation);
      return;
    }
  buf[0] = ch;
  buf[1] = '\0';
}
 
static char *
fmt_text_str(char *buf, int buf_len, char *str, int type)
{
  char *p = buf;
  int multiline_p = (strchr(str, '\n') != 0);
 
  if (multiline_p) {
    sprintf(p, "\\shortstack[%s]{", just_str_from_text_type(type));
    p += strlen(p);
  }
  else
    p[0] = '\0';
 
  while (*str) {
    if (p - buf >= buf_len - (int)(sizeof longest_translation)) {
      fprintf(stderr, "string too long; truncated ...%s\n", str);
      break;
    }
    translate_tex_specials(p, *str);
    p += strlen(p);
    str++;
  }
 
  if (multiline_p) {
    strcat(p, "}");
    p += strlen(p);
  }
 
  return buf;
}
 
/* could make this very fancy; just a brace balance check here */
static void
check_tex_format(char *buf)
{
  char *p = buf;
  int ch, n_open_braces, n_dollars;
 
  n_open_braces = n_dollars = 0;
  while ((ch = *p++) != '\0') {
    switch (ch) {
    case '{':
      n_open_braces++;
      break;
    case '}':
      n_open_braces--;
      break;
    case '\\':
      if (*p != '\0')
	ch = *p++;
      break;
    case '$':
      n_dollars++;
      break;
    }
  }
  if (n_open_braces != 0) 
    fprintf(stderr, "warning: unbalanced braces in special text '%s'\n", buf);
  if ((n_dollars & 1) != 0)
    fprintf(stderr, "warning: unbalanced $'s in special text '%s'\n", buf);
}
 
/* copied from psfonts */
static int PSfontmap[] = {
  ROMAN_FONT,ROMAN_FONT,/* Times-Roman */
  ITALIC_FONT,		/* Times-Italic */
  BOLD_FONT,		/* Times-Bold */
  BOLD_FONT,		/* Times-BoldItalic */
  ROMAN_FONT,		/* AvantGarde */
  ROMAN_FONT,		/* AvantGarde-BookOblique */
  ROMAN_FONT,		/* AvantGarde-Demi */
  ROMAN_FONT,		/* AvantGarde-DemiOblique */
  ROMAN_FONT,		/* Bookman-Light */
  ITALIC_FONT,		/* Bookman-LightItalic */
  ROMAN_FONT,		/* Bookman-Demi */
  ITALIC_FONT,		/* Bookman-DemiItalic */
  TYPEWRITER_FONT,	/* Courier */
  TYPEWRITER_FONT,	/* Courier-Oblique */
  BOLD_FONT,		/* Courier-Bold */
  BOLD_FONT,		/* Courier-BoldItalic */
  MODERN_FONT,		/* Helvetica */
  MODERN_FONT,		/* Helvetica-Oblique */
  BOLD_FONT,		/* Helvetica-Bold */
  BOLD_FONT,		/* Helvetica-BoldOblique */
  MODERN_FONT,		/* Helvetica-Narrow */
  MODERN_FONT,		/* Helvetica-Narrow-Oblique */
  BOLD_FONT,		/* Helvetica-Narrow-Bold */
  BOLD_FONT,		/* Helvetica-Narrow-BoldOblique */
  ROMAN_FONT,		/* NewCenturySchlbk-Roman */
  ITALIC_FONT,		/* NewCenturySchlbk-Italic */
  BOLD_FONT,		/* NewCenturySchlbk-Bold */
  BOLD_FONT,		/* NewCenturySchlbk-BoldItalic */
  ROMAN_FONT,		/* Palatino-Roman */
  ITALIC_FONT,		/* Palatino-Italic */
  BOLD_FONT,		/* Palatino-Bold */
  BOLD_FONT,		/* Palatino-BoldItalic */
  ROMAN_FONT,		/* Symbol */
  ROMAN_FONT,		/* ZapfChancery-MediumItalic */
  ROMAN_FONT		/* ZapfDingbats */
};
 
static int PSmapwarn[] = {
  False, False,		/* Times-Roman */
  False,		/* Times-Italic */
  False,		/* Times-Bold */
  False,		/* Times-BoldItalic */
  True,			/* AvantGarde */
  True,			/* AvantGarde-BookOblique */
  True,			/* AvantGarde-Demi */
  True,			/* AvantGarde-DemiOblique */
  True,			/* Bookman-Light */
  True,			/* Bookman-LightItalic */
  True,			/* Bookman-Demi */
  True,			/* Bookman-DemiItalic */
  False,		/* Courier */
  True,			/* Courier-Oblique */
  True,			/* Courier-Bold */
  True,			/* Courier-BoldItalic */
  False,		/* Helvetica */
  True,			/* Helvetica-Oblique */
  True,			/* Helvetica-Bold */
  True,			/* Helvetica-BoldOblique */
  True,			/* Helvetica-Narrow */
  True,			/* Helvetica-Narrow-Oblique */
  True,			/* Helvetica-Narrow-Bold */
  True,			/* Helvetica-Narrow-BoldOblique */
  True,			/* NewCenturySchlbk-Roman */
  True,			/* NewCenturySchlbk-Italic */
  True,			/* NewCenturySchlbk-Bold */
  True,			/* NewCenturySchlbk-BoldItalic */
  True,			/* Palatino-Roman */
  True,			/* Palatino-Italic */
  True,			/* Palatino-Bold */
  True,			/* Palatino-BoldItalic */
  True,			/* Symbol */
  True,			/* ZapfChancery-MediumItalic */
  True			/* ZapfDingbats */
};
 
static char *figfontnames[] = {
  "Roman", "Roman",
  "Roman", 
  "Bold",
  "Italic",
  "Modern",
  "Typewriter"
};
 
 
/* taken from the epic driver */
static void 
begin_set_font(text)
     F_text *text;
{
  int texsize;
  double baselineskip;
 
  /* substitute latex for postscript fonts and warn the user (from psfont) */
  if (psfont_text(text)) {
    if (PSmapwarn[text->font+1]) {
      warn(W_PS_FONT);
      if (Verbose)
        fprintf(stderr, "Substituting LaTeX %s for Postscript %s ('%s')\n",
		figfontnames[PSfontmap[text->font+1]+1], PSfontnames[text->font+1],
		text->cstring);
    }
    if (text->font == -1) /* leave default to be default, but no-ps */
      text->font = 0;
    else
      text->font = PSfontmap[text->font+1];
  }
  texsize = TEXFONTMAG(text);
  baselineskip = (texsize * 1.2);
 
#ifdef NFSS
  if (Font_handling == FH_SIZE_ONLY)
    fprintf(tfp, 
	    "\\begingroup\\SetFigFontSizeOnly{%d}{%.1f}%%\n",
	    texsize, baselineskip);
  else if (Font_handling == FH_FULL)
    fprintf(tfp, 
	    "\\begingroup\\SetFigFont{%d}{%.1f}{%s}{%s}{%s}%%\n",
	    texsize, baselineskip,
	    TEXFAMILY(text->font),TEXSERIES(text->font),
	    TEXSHAPE(text->font));
#else
  fprintf(tfp, "\\begingroup\\SetFigFont{%d}{%.1f}{%s}%%\n",
	  texsize, baselineskip, TEXFONT(text->font));
#endif
}
 
static void
end_set_font(text)
     F_text *text;
{
  (void)text; /* avoid unused parameter warnings */
  if (Font_handling == FH_SIZE_ONLY || Font_handling == FH_FULL)
    fprintf(tfp, "\\endgroup%%\n");
}
 
void
genpstrx_text(text)
     F_text *text;
{
  int x, y;
  char formatted_text[64*1024];
  char angle[32];
 
  /* print any comments prefixed with "%" */
  print_comments("% ",text->comments, "");
 
  if (Verbose)
    fprintf(tfp, "%% text - %s\n", special_text(text) ? "special" : "no flags");
 
  begin_set_font(text);
 
  convert_xy_to_image_bb(&x, &y, text->base_x, text->base_y);
 
  if (text->angle == 0)
    angle[0] = '\0';
  else
    sprintf(angle, "{%.2lf}", text->angle * (180.0 / M_PI));
 
  if (special_text(text)) {
    check_tex_format(text->cstring);
    fprintf(tfp, "\\rput[%s]%s("DBL","DBL"){%s}%%\n",
	    ref_pt_str_from_text_type(text->type),
	    angle,
	    cm_from_1200ths(x), cm_from_1200ths(y),
	    text->cstring);
  }
  else {
    fprintf(tfp, "\\rput[%s]%s("DBL","DBL"){%s}%%\n", 
	    ref_pt_str_from_text_type(text->type),
	    angle,
	    cm_from_1200ths(x), cm_from_1200ths(y),
	    fmt_text_str(formatted_text, sizeof formatted_text, 
			 text->cstring, 
			 text->type));
  }
  end_set_font(text);
}
 
void
genpstrx_arc(arc)
     F_arc *arc;
{
  int x_0, y_0, x_1, y_1;
  double x, y, t_0, t_1, r;
  char options[256];
  F_arrow *a_0, *a_1;
 
  /* print any comments prefixed with "%" */
  print_comments("% ",arc->comments,"");
 
  if (Verbose)
    fprintf(tfp, "%% arc\n");
 
  convert_xy_dbl_to_image_bb(&x, &y, arc->center.x, arc->center.y);
 
  if (arc->direction == 1) {
    convert_xy_to_image_bb(&x_0, &y_0, arc->point[0].x, arc->point[0].y);
    convert_xy_to_image_bb(&x_1, &y_1, arc->point[2].x, arc->point[2].y);
  }
  else {
    convert_xy_to_image_bb(&x_0, &y_0, arc->point[2].x, arc->point[2].y);
    convert_xy_to_image_bb(&x_1, &y_1, arc->point[0].x, arc->point[0].y);
  }
  t_0 = atan2(y_0 - y, x_0 - x) * (180.0 / PI);
  t_1 = atan2(y_1 - y, x_1 - x) * (180.0 / PI);
  r = sqrt(sqr_dbl((double)(x_0 - x)) + sqr_dbl((double)(y_0 - y)));
 
 
  switch (arc->type) {
 
  case T_OPEN_ARC:
 
    if (arc->direction == 1) {
      a_0 = arc->back_arrow;
      a_1 = arc->for_arrow;
    }
    else {
      a_0 = arc->for_arrow;
      a_1 = arc->back_arrow;
    }
 
#define FO_ARC_STD (FO_LINE_STYLE | FO_PEN_COLOR | FO_LINE_TERM | FO_FILL)
    format_options(options, 0, 0, 0, 
		   Arrows == A_XFIG     ?  FO_ARC_STD : 
		   Arrows == A_PSTRICKS ? (FO_ARC_STD | FO_ARROWS | FO_ARROW_SIZE) :
		   /* default */          (FO_ARC_STD | FO_ARROWS), 
		   arc->thickness, arc->style, arc->style_val,
		   arc->pen_color, 0, arc->cap_style, arc->fill_style, arc->fill_color,
		   a_0, a_1, 0.0);
 
    fprintf(tfp, "\\psarc%s("DBL","DBL"){"DBL"}{"DBL"}{"DBL"}%%\n", 
	    options,
	    cm_from_1200ths(x), cm_from_1200ths(y), 
	    cm_from_1200ths(r), 
	    t_0, t_1);
    if (Arrows == A_XFIG) {
      if (a_0)
	put_arrowhead((double)x_0, (double)y_0,
		      (double)(y_0 - y), (double)(x - x_0),
		      a_0->wid, a_0->ht, max_dbl(a_0->thickness, (double)arc->thickness),
		      a_0->type, a_0->style,
		      arc->pen_color);
      if (a_1)
	put_arrowhead((double)x_1, (double)y_1,
		      (double)(y - y_1), (double)(x_1 - x), /* perp of radius vector */
		      a_1->wid, a_1->ht, max_dbl(a_1->thickness, (double)arc->thickness),
		      a_1->type, a_1->style,
		      arc->pen_color);
    }
    break;
 
  case T_PIE_WEDGE_ARC:
    format_options(options, 0, 0, 0, 
		   FO_LINE_STYLE | FO_PEN_COLOR | FO_FILL, 
		   arc->thickness, arc->style, arc->style_val,
		   arc->pen_color, 0, arc->cap_style, arc->fill_style, arc->fill_color, 
		   0,0,0.0);
 
    fprintf(tfp, "\\pswedge%s("DBL","DBL"){"DBL"}{"DBL"}{"DBL"}%%\n", 
	    options,
	    cm_from_1200ths(x), cm_from_1200ths(y), 
	    cm_from_1200ths(r), 
	    t_0, t_1);
    break;
  default:
    fprintf(stderr, "unknown arc type (%d)\n", arc->type);
  }
}
 
void genpstrx_grid(major, minor)
     double major, minor;
{
  if (minor == 0.0 && major == 0.0)
    return;
  fprintf(tfp, "\\psgrid[gridcolor=lightgray,subgridcolor=lightgray]\n");
}
 
struct driver dev_pstricks = {
  genpstrx_option,
  genpstrx_start,
  genpstrx_grid,
  genpstrx_arc,
  genpstrx_ellipse,
  genpstrx_line,
  genpstrx_spline,
  genpstrx_text,
  genpstrx_end,
  INCLUDE_TEXT
};


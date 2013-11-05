/*
 * TransFig: Facility for Translating Fig code
 * This routine is from PSencode.c, in the xwpick package by:
 *      E.Chernyaev (IHEP/Protvino)
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

#include "fig2dev.h"

#define MAXWIDTH       16384

#define put_string nc=strlen(s); for(i=0;i<nc;i++) (putc((s[i]),tfp)); Nbyte += nc

typedef unsigned char	byte;
static	char		**str;

/* output PSencode header */

void
PSencode_header()
{
  static char *PSencodeheader[] = {
    "%***********************************************************************",
    "%*                                                                     *",
    "%* Object: Image decoding PS-routine                    Date: 01.02.93 *",
    "%* Author: Evgeni CHERNYAEV (chernaev@vxcern.cern.ch)                  *",
    "%*                                                                     *",
    "%* Function: Display a run-length encoded color image.                 *",
    "%*           The image is displayed in color on viewers and printers   *",
    "%*           that support color Postscript, otherwise it is displayed  *",
    "%*           as grayscale.                                             *",
    "%*                                                                     *",
    "%***********************************************************************",
    "/byte 1 string def",
    "/color 3 string def",
    "systemdict /colorimage known { /cnt 3 def } { /cnt 1 def } ifelse",
    "/String 256 cnt mul string def",
    "%***********************************************************************",
    "/DecodePacket            % Decode color packet                         *",
    "%***********************************************************************",
    "{",
    "  currentfile byte readhexstring pop 0 get",
    "  /Nbyte exch 1 add cnt mul def",
    "  /color ColorMap currentfile byte readhexstring pop 0 get get def",
    "  String dup",
    "  0 cnt Nbyte 1 sub { color putinterval dup } for",
    "  pop 0 Nbyte getinterval",
    "} bind def",
    "%***********************************************************************",
    "/DisplayImage            % Display run-length encoded color image      *",
    "%***********************************************************************",
    "{",
    "  gsave",
    "  currentfile String readline pop",
    "  token { /columns exch def } { } ifelse",
    "  token { /rows exch def pop } { } ifelse",
    "  currentfile String readline pop",
    "  token { /Ncol exch def pop } { } ifelse",
    "  /ColorMap Ncol array def",
    "  systemdict /colorimage known {",
    "    0 1 Ncol 1 sub {",
    "      ColorMap exch",
    "      currentfile 3 string readhexstring pop put",
    "    } for",
    "    columns rows 8",
    "    [ columns 0 0 rows neg 0 rows ]",
    "    { DecodePacket } false 3 colorimage",
    "  }{",
    "    0 1 Ncol 1 sub {",
    "      ColorMap exch",
    "      1 string dup 0",
    "      currentfile color readhexstring pop pop",
    "      color 0 get 0.299 mul",
    "      color 1 get 0.587 mul add",
    "      color 2 get 0.114 mul add",
    "      cvi put put",
    "    } for",
    "    columns rows 8",
    "    [ columns 0 0 rows neg 0 rows ]",
    "    { DecodePacket } image",
    "  } ifelse",
    "  grestore",
    "} bind def",
    NULL
  };

  for (str=PSencodeheader; *str; str++) {
    fprintf(tfp,"%s\n",*str);
  }
  /* set flag saying we've emitted the header */
  psencode_header_done = True;
}

/* output transparentimage header */

void
PStransp_header()
{
  static char *Transpheader[] = {
    "%*************************************",
    "% Transparent image template follows *",
    "%*************************************",
    "",
    "/transparentimage {",
    "  gsave",
    "  32 dict begin",
    "  /olddict <<",
    "      /ImageType 1",
    "      /BitsPerComponent 8",
    "      /Decode [0 255]",
    "  >> def",
    "  /tinteger exch def",
    "  /height exch def",
    "  /width exch def",
    "  /transparent 1 string def",
    "  transparent 0 tinteger put",
    "  olddict /ImageMatrix [width 0 0 height neg 0 height] put",
    "  /newdict olddict maxlength dict def",
    "  olddict newdict copy pop",
    "  newdict /Width width put",
    "  newdict /Height height put",
    "  /w newdict /Width get def",
    "  /str w string def",
    "  /substrlen 2 w log 2 log div floor exp cvi def",
    "  /substrs [",
    "  {",
    "     substrlen string",
    "     0 1 substrlen 1 sub {",
    "       1 index exch tinteger put",
    "     } for",
    "     /substrlen substrlen 2 idiv def",
    "     substrlen 0 eq {exit} if",
    "  } loop",
    "  ] def",
    "  /h newdict /Height get def",
    "  1 w div 1 h div matrix scale",
    "  olddict /ImageMatrix get exch matrix concatmatrix",
    "  matrix invertmatrix concat",
    "  newdict /Height 1 put",
    "  newdict /DataSource str put",
    "  /mat [w 0 0 h 0 0] def",
    "  newdict /ImageMatrix mat put",
    "  0 1 h 1 sub {",
    "    mat 5 3 -1 roll neg put",
    "    % get Width bytes from the image via the RLE decoder",
    "    0 1 w 1 sub { str exch RLEPacket putinterval } for",
    "    /tail str def",
    "    /x 0 def",
    "    {",
    "      tail transparent search dup /done exch not def",
    "      {exch pop exch pop} if",
    "      /w1 1 index length def",
    "      w1 0 ne {",
    "        newdict /DataSource 3 -1 roll put",
    "        newdict /Width w1 put",
    "        mat 4 x neg put",
    "        /x x w1 add def",
    "        newdict image",
    "        /tail tail w1 tail length w1 sub getinterval def",
    "      } {",
    "        pop",
    "      } ifelse",
    "      done {exit} if",
    "      tail substrs {",
    "        anchorsearch {pop} if",
    "      } forall",
    "      /tail exch def",
    "      tail length 0 eq {exit} if",
    "      /x w tail length sub def",
    "    } loop",
    "  } for",
    "  end",
    "  grestore",
    "} bind def",
    "",
    "% number of bytes remaining in current RLE run",
    "/Nbyte 0 def",
    "% color of current RLE run",
    "/color 1 string def",
    "",
    "/byte 1 string def",
    "",
    "%***************************************************",
    "/RLEPacket         % Decode RLE color packet       *",
    "%***************************************************",
    "{",
    "    Nbyte 0 eq {",
    "	  currentfile byte readhexstring pop 0 get",
    "	  /Nbyte exch 1 add def ",
    "	  currentfile color readhexstring pop pop",
    "	} if",
    "    /Nbyte Nbyte 1 sub def",
    "    color ",
    "} bind def",
    NULL
    };

    for (str=Transpheader; *str; str++) {
	fprintf(tfp,"%s\n",*str);
    }
    /* set flag saying we've emitted the header */
    transp_header_done = True;
}

/***********************************************************************
 *                                                                     *
 * Name: PSencode                                    Date:    13.01.93 *
 *                                                                     *
 * Function: Output image in PostScript format (runlength encoding)    *
 *                                                                     *
 * Input: Width      - image width                                     *
 *        Height     - image height                                    *
 *        Transparent - index of transparent color (-1 if none)        *
 *        Ncol       - number of colors                                *
 *        R[]        - red components                                  *
 *        G[]        - green components                                *
 *        B[]        - blue components                                 *
 *        data[]     - array for image data (byte per pixel)           *
 *                                                                     *
 * Return: size of PS                                                  *
 *                                                                     *
 ***********************************************************************/
long
PSencode(Width, Height, Transparent, Ncol, R, G, B, data)
    int		Width, Height, Transparent, Ncol;
    byte	R[], G[], B[];
    unsigned char *data;
{

  long    Nbyte;
  char    s[80];
  byte    *ptr, *end;
  int     i, nc, k, current, previous, run, y;

  static char h[] = "0123456789abcdef";

  /*   CHECK PARAMETERS   */
  
  if (Width <= 0 || Width > MAXWIDTH || Height <= 0 || Height > MAXWIDTH) {
    fprintf(stderr, "\nIncorrect image size: %d x %d\n", Width, Height);
    return 0;
  }

  if (Ncol <= 0 || Ncol > 256) {
    fprintf(stderr,"\nWrong number of colors: %d\n", Ncol);
    return 0;
  }


  if (Transparent != -1) {
	fprintf(tfp,"[/Indexed /DeviceRGB %d <\n",Ncol-1);
  } else {
	fprintf(tfp, "%%***********************************************\n");
	fprintf(tfp, "%%*              Image decoding                 *\n");
	fprintf(tfp, "%%***********************************************\n");
	fprintf(tfp, "DisplayImage\n");
	fprintf(tfp,"%d %d\n", Width, Height);
	fprintf(tfp,"%d\n",Ncol);
  }

  Nbyte = 0;

  /* colormap */
  for (k=0; k<Ncol; k++) {
    sprintf(s,"%02x%02x%02x", R[k], G[k], B[k]);   put_string; 
    if (k % 10 == 9 || k == Ncol-1) { 
      sprintf(s,"\n");                             put_string;
    } else {
      sprintf(s, " "); 	                           put_string;
    }
  }
  if (Transparent != -1) {
	fprintf(tfp,"\n > ] setcolorspace\n");
	fprintf(tfp,"%d %d %d transparentimage\n",Width,Height,Transparent);
  }

  /* RUN-LENGTH COMPRESSION */

  run   = 0;
  nc    = 0;
  s[72] = '\n';
  s[73] = '\0';
  for(y=0; y<Height; y++) {
    ptr = (data+y*Width);
    end = ptr + Width;
    if (y == 0) previous = *ptr++;
    while (ptr < end) {
      current = *ptr++;
      if (current == previous && run < 255) {
        run++;
        continue;
      }
      if (nc == 72) {
        put_string; 
        nc = 0;
      }
      s[nc++] = h[run / 16];
      s[nc++] = h[run % 16];
      s[nc++] = h[previous / 16];
      s[nc++] = h[previous % 16];
      previous = current;
      run = 0;
    }
  }

  if (nc == 72) {
    put_string; 
    nc = 0;
  }
  s[nc++] = h[run / 16];
  s[nc++] = h[run % 16];
  s[nc++] = h[previous / 16];
  s[nc++] = h[previous % 16];
  s[nc++] = '\n';
  s[nc]   = '\0';
  put_string; 
  return Nbyte;
}


/* write 24-bit bitmap as PostScript image (no colortable) */

void
PSrgbimage(file, width, height, data)
	 FILE		*file;
         int		 width, height;
	 unsigned char	*data;
{
    int		 c, h, w, left;
    unsigned char *p;

    fprintf(file,"/picstr 192 string def\n");
    fprintf(file,"%d %d 8\n",width, height);
    fprintf(file,"[%d 0 0 %d 0 %d]\n",width, -height, height);
    fprintf(file,"{currentfile picstr readhexstring pop}\n");
    fprintf(file,"false 3 colorimage\n");

    c = 0;
    p = data;
    for (h=0; h<height; h++) {
    	for (w=0; w<width; w++) {
	    fprintf(file,"%02x%02x%02x", *(p+2), *(p+1), *p);
	    p += 3;
	    c++;
	    if ((c % 15) == 0)
		fprintf(file,"\n");
	}
    }
    /* now output zeroes to pad to 64 triples (length of picstr) */
    left = ((int)((c+63) / 64)) * 64 - c;
    for (c=0; c<left; c++) {
	fprintf(file,"000000");
	if ((c % 12) == 0)
	    fprintf(file,"\n");
    }
    fprintf(file,"\n");
}

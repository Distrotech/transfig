#
# TransFig makefile
#

all: trans.tex 

# translation into latex

trans.tex: trans.fig Makefile
	../../fig2dev/fig2dev -L latex trans.fig > trans.tex
clean::
	rm -f trans.tex

cleandir::

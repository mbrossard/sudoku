CFLAGS=-O3
ifdef VERBOSE
CFLAGS +=-DVERBOSE
endif

default: sudoku

test: sudoku big.txt
	time ./sudoku big.txt

big.txt: top95.txt hardest.txt
	(for i in `cat $^` ; do cat $^ ; done ) > $@

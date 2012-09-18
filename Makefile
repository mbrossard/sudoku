CFLAGS=-O3

test: sudoku big.txt
	time ./sudoku big.txt

big.txt: top95.txt hardest.txt
	(for i in `cat $^` ; do cat $^ ; done ) > $@

CFLAGS = -O3 -march=native
ifdef DEBUG
CFLAGS += -DDEBUG -g -Wall
endif

default: sudoku

test: sudoku big.txt
	time ./sudoku big.txt

big.txt: top95.txt hardest.txt
	(for i in `cat $^` ; do cat $^ ; done ) > $@

profiled: big.txt sudoku.c
	@rm -f sudoku.gc*
	$(CC) -fprofile-generate -fprofile-arcs $(CFLAGS) -o sudoku sudoku.c
	./sudoku big.txt
	$(CC) -fprofile-use -fbranch-probabilities $(CFLAGS) -o sudoku sudoku.c
	@rm -f sudoku.gc*

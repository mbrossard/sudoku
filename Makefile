CFLAGS = -O3
ifdef DEBUG
CFLAGS += -DDEBUG -g -Wall
endif

default: sudoku

test: sudoku big.txt
	time ./sudoku big.txt

big.txt: top95.txt hardest.txt
	(for i in `cat $^` ; do cat $^ ; done ) > $@

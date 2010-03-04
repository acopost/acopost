# git test

CC=gcc

CFLAGS  = -Wall -ansi -O2

# default is double
CFLAGS += -DT3_PROB_IS_FLOAT

.c.o:
	${CC} ${CFLAGS} -c $< -o $*.o

#all: et met t3 tbt ltest TAGS
all: et met t3 tbt

TAGS::
	etags *.[chy] > TAGS

array.o: array.h mem.h Makefile

primes.o: primes.h Makefile

hash.o: hash.h mem.h Makefile

mem.o: mem.h Makefile

util.o: util.h Makefile

met.o: met.c mem.h array.h util.h Makefile

gis.o: gis.h array.h mem.h util.h Makefile

lexicon.o: lexicon.h array.h mem.h hash.h primes.h util.h Makefile

ltest.o: lexicon.h array.h mem.h hash.h primes.h util.h Makefile

ltest: lexicon.o ltest.o array.o mem.o util.o hash.o primes.o Makefile
	${CC} ${CFLAGS} -o ltest -lm ltest.o lexicon.o array.o mem.o util.o hash.o primes.o

et: et.o array.o mem.o util.o hash.o primes.o Makefile
	${CC} ${CFLAGS} -o et -lm et.o array.o mem.o util.o hash.o primes.o

met: met.o gis.o array.o mem.o util.o hash.o primes.o Makefile
	${CC} ${CFLAGS} -o met -lm met.o gis.o array.o mem.o util.o hash.o primes.o

t3: t3.o util.o mem.o array.o hash.o primes.o Makefile
	${CC} ${CFLAGS} -o t3 -lm t3.o util.o mem.o array.o hash.o primes.o 

tbt: tbt.o util.o mem.o array.o hash.o primes.o Makefile
	${CC} ${CFLAGS} -o tbt -lm tbt.o util.o mem.o array.o hash.o primes.o 

install:: met t3 tbt et
	[ -d ../bin ] || mkdir ../bin
	[ -d ../bin ] && cp met t3 tbt et ../bin

clean::
	rm -f *.o met t3 tbt et ltest TAGS *~

CFLAGS=-std=c99 -g -fno-common -Wall -pedantic -Wno-switch

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

# Stage 1

widcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): widcc.h

test/%.exe: widcc test/%.c
	./widcc -Iinclude -Itest -c -o test/$*.o test/$*.c
	$(CC) -std=c11 -pthread -Wno-psabi -o $@ test/$*.o -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	bash test/driver.sh ./widcc $(CC)

test-all: test test-stage2

# Stage 2

stage2/widcc: $(OBJS:%=stage2/%)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stage2/%.o: widcc %.c
	mkdir -p stage2/test
	./widcc -c -o $(@D)/$*.o $*.c

stage2/test/%.exe: stage2/widcc test/%.c
	mkdir -p stage2/test
	./stage2/widcc -Iinclude -Itest -c -o stage2/test/$*.o test/$*.c
	$(CC) -std=c11 -pthread -Wno-psabi -o $@ stage2/test/$*.o -xc test/common

test-stage2: $(TESTS:test/%=stage2/test/%)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	bash test/driver.sh ./stage2/widcc $(CC)

# Misc.

clean:
	rm -rf widcc stage2
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'
	find test/* -type f '(' -name '*~' -o -name '*.exe' ')' -exec rm {} ';'

.PHONY: test clean test-stage2

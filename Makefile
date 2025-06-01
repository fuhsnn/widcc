SRCS=codegen.c hashmap.c main.c parse.c platform.c preprocess.c strings.c tokenize.c type.c unicode.c

TEST_SRCS!=ls test/*.c

TEST_FLAGS=-Itest -std=c23

.SUFFIXES: .exe .stage2.o .stage2.exe .asan.o .asan.exe

# Stage 1

OBJS=$(SRCS:.c=.o)

widcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): widcc.h

TESTS=$(TEST_SRCS:.c=.exe)

test/host/common.o: test/host/common.c
	$(CC) -o $@ -c $<

$(TESTS): widcc test/host/common.o

.c.exe:
	./widcc $(TEST_FLAGS) -o $@ $< test/host/common.o -pthread

test: $(TESTS)
	for i in $(TESTS); do echo $$i; ./$$i >/dev/null || exit 1; echo; done
	$(SHELL) scripts/test_driver.sh $(PWD)/widcc $(CC)
	./widcc -hashmap-test

# Stage 2

OBJS_S2=$(SRCS:.c=.stage2.o)

$(OBJS_S2): widcc

.c.stage2.o:
	./widcc -o $@ -c $<

widcc-stage2: $(OBJS_S2)
	./widcc -o $@ $(OBJS_S2) $(LDFLAGS)

TESTS_S2=$(TEST_SRCS:.c=.stage2.exe)

$(TESTS_S2): widcc-stage2 test/host/common.o

.c.stage2.exe:
	./widcc-stage2 $(TEST_FLAGS) -o $@ $< test/host/common.o -pthread

test-stage2: $(TESTS_S2)
	for i in $(TESTS_S2); do echo $$i; ./$$i >/dev/null || exit 1; echo; done
	$(SHELL) scripts/test_driver.sh $(PWD)/widcc-stage2 $(CC)
	./widcc-stage2 -hashmap-test

# Asan build

OBJS_ASAN=$(SRCS:.c=.asan.o)

$(OBJS_ASAN): widcc.h

.c.asan.o:
	$(CC) $(CFLAGS) -fsanitize=address -g -o $@ -c $<

widcc-asan: $(OBJS_ASAN)
	$(CC) $(CFLAGS) -fsanitize=address -g -o $@ $(OBJS_ASAN) $(LDFLAGS)

TESTS_ASAN=$(TEST_SRCS:.c=.asan.exe)

$(TESTS_ASAN): widcc-asan test/host/common.o

.c.asan.exe:
	./widcc-asan $(TEST_FLAGS) -o $@ $< test/host/common.o -pthread

test-asan: $(TESTS_ASAN)
	for i in $(TESTS_ASAN); do echo $$i; ./$$i >/dev/null || exit 1; echo; done
	$(SHELL) scripts/test_driver.sh $(PWD)/widcc-asan $(CC)
	$(MAKE) widcc CC=./widcc-asan -B
	./widcc-asan -hashmap-test

# Misc.

test-all: test test-stage2

warn: $(SRCS)
	$(CC) -fsyntax-only -Wall -Wpedantic -Wno-switch $(CFLAGS) $(SRCS)

lto: clean
	$(MAKE) CFLAGS="-O2 -flto=auto -Wno-switch"

lto-je: clean
	$(MAKE) CFLAGS="-O2 -flto=auto -Wno-switch" LDFLAGS="-ljemalloc"

lto-mi: clean
	$(MAKE) CFLAGS="-O2 -flto=auto -Wno-switch" LDFLAGS="-lmimalloc"

clean:
	rm -f widcc widcc-stage2 widcc-asan
	rm -f *.o test/*.o test/*.exe test/host/*.o

.PHONY: test clean test-stage2 test-asan

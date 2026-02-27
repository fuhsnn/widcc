SRCS=codegen.c hashmap.c main.c parse.c platform.c preprocess.c strings.c tokenize.c type.c unicode.c

TEST_SRCS!=ls test/*.c

TEST_FLAGS=-Itest -std=gnu11

.SUFFIXES: .exe .stage2.o .stage2.exe .asan.o .asan.exe .filc.o .filc.exe

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
	./widcc-asan scripts/amalgamation.c -c -o/dev/null
	./widcc-asan -hashmap-test

test-misc: widcc-asan test/host/common.o
	$(SHELL) scripts/test_abi.sh $(PWD)/widcc-asan $(CC)
	$(SHELL) scripts/test_abi.sh $(CC) $(PWD)/widcc-asan
	$(SHELL) scripts/test_include_next.sh $(PWD)/widcc-asan
	FILE=file $(SHELL) scripts/test_linker.sh $(PWD)/widcc-asan

# Fil-C build

OBJS_FILC=$(SRCS:.c=.filc.o)

$(OBJS_FILC): widcc.h

.c.filc.o:
	$(FILC) $(CFLAGS) -g -o $@ -c $<

widcc-filc: $(OBJS_FILC)
	$(FILC) $(CFLAGS) -g -o $@ $(OBJS_FILC) $(LDFLAGS)

TESTS_FILC=$(TEST_SRCS:.c=.filc.exe)

$(TESTS_FILC): widcc-filc test/host/common.o

.c.filc.exe:
	./widcc-filc $(TEST_FLAGS) -o $@ $< test/host/common.o -pthread

test-filc: $(TESTS_FILC)
	for i in $(TESTS_FILC); do echo $$i; ./$$i >/dev/null || exit 1; echo; done
	$(SHELL) scripts/test_driver.sh $(PWD)/widcc-filc $(CC)
	./widcc-filc scripts/amalgamation.c -c -o/dev/null
	./widcc-filc -hashmap-test

# Misc.

test-all: test test-stage2

widcc-lto: $(SRCS) widcc.h
	$(CC) -O2 -flto=auto -fvisibility=hidden scripts/amalgamation.c -o $@

widcc-lto-je: $(SRCS) widcc.h
	$(CC) -O2 -flto=auto -fvisibility=hidden scripts/amalgamation.c -o $@ -ljemalloc

widcc-lto-mi: $(SRCS) widcc.h
	$(CC) -O2 -flto=auto -fvisibility=hidden scripts/amalgamation.c -o $@ -lmimalloc

clean:
	rm -f widcc widcc-stage2 widcc-asan widcc-filc widcc-lto widcc-lto-je widcc-lto-mi
	rm -f *.o test/*.o test/*.exe test/host/*.o test/abi/*.o

.PHONY: clean test test-stage2 test-all test-asan test-filc

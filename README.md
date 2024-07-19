`widcc` is a stable branch of [`slimcc`](https://github.com/fuhsnn/slimcc) that
 - don't attempt to optimize codegen 
 - don't introduce new features (unless I really want certain projects to build)
 - would rather remove a feature than to support it incorrectly (not a motto but to avoid giving autoconf the wrong idea)
 
`slimcc` has accomplished a fairly good collection of real world projects it can build and 100% pass tests. `widcc` in turn is to be low-maintenance, simpler-to-port, easier-to-modify, and to serve as stable reference while I make more radical changes to slimcc. 

The following C11 features are removed: `_Alignof` `_Atomic` `_Generic`. These are not seen commonly in use, therefore widcc still declare itself a C11 compiler. 

wid stands for "When in doubt" from the famous quote.

# Building & Running
 - Should just work on recent glibc-based (2.28+) x86-64 Linux.
 - Test script needs `bash`; depending on the distro, `file` and/or `glibc-static` packages may be required.
```
git clone --depth 1 https://github.com/fuhsnn/widcc
cd widcc
make test-stage2 -j
```
Run it in base directory like `CC=~/widcc/widcc`

# Building real-world projects

(package dependencies are assumed to be in place)

`curl 8.8.0`
```
git clone --depth 1 https://github.com/curl/curl --branch curl-8_8_0
mkdir curl/cmakebuild
cd curl/cmakebuild
cmake ../ -DCMAKE_C_COMPILER=/full/path/to/widcc -DCMAKE_C_FLAGS=-fPIC
make -j
make quiet-test -j
```
`git 2.45.2`
```
git clone --depth 1 https://github.com/git/git --branch v2.45.2
cd git
make CC=~/widcc/widcc V=1 test -j
```
`PostgreSQL 15.7`
```
git clone --depth 1 https://github.com/postgres/postgres --branch REL_15_7
cd postgres
CC=~/widcc/widcc ./configure --disable-spinlocks --disable-atomics
make check -j
```
`Python 3.11.9`
```
git clone --depth 1 https://github.com/python/cpython --branch v3.11.9
cd cpython
CC=~/widcc/widcc ./configure
make test -j
```
`sqlite 3.46.0`
```
git clone --depth 1 https://github.com/sqlite/sqlite/ --branch version-3.46.0
cd sqlite
sh ~/widcc/add_wl_pic.sh ./configure
CC=~/widcc/widcc CFLAGS=-D_GNU_SOURCE ./configure
make testrunner
```
`vim trunk`
```
git clone --depth 1 https://github.com/vim/vim
cd vim
CC=~/widcc/widcc ./configure
make -j
make testtiny
```

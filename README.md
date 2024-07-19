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

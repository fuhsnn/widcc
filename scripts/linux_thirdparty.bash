set -eu
set -o pipefail

# utilities

fix_configure() {
 sed -i 's/^\s*lt_prog_compiler_wl=$/lt_prog_compiler_wl=-Wl,/g' "$1"
 sed -i 's/^\s*lt_prog_compiler_pic=$/lt_prog_compiler_pic=-fPIC/g' "$1"
 sed -i 's/^\s*lt_prog_compiler_static=$/lt_prog_compiler_static=-static/g' "$1"
}

replace_line() {
 sed -i s/^"$1"$/"$2"/g "$3"
}

github_tar() {
  mkdir -p "$2"
  curl -fL https://github.com/"$1"/"$2"/archive/refs/tags/"$3".tar.gz | tar xz -C "$2" --strip-components=1
  cd "$2"
}

github_clone() {
  git clone --depth 1 --recurse-submodules --shallow-submodules --branch "$3" https://github.com/"$1"/"$2"
  cd "$2"
}

git_fetch() {
 mkdir -p "$3" && cd "$3"
 git init
 git remote add origin "$1"
 git fetch --depth 1 origin "$2"
 git checkout FETCH_HEAD
}

url_tar() {
  mkdir -p "$2"
  curl -fL "$1" | tar xz -C "$2" --strip-components=1
  cd "$2"
}

install_libtool() {
 url_tar https://ftpmirror.gnu.org/gnu/libtool/libtool-2.5.4.tar.gz __libtool
 fix_configure ./configure
 fix_configure libltdl/configure
 ./configure
 make -j2 install
 cd ../ && rm -rf __libtool
}

# tests

test_bash() {
 url_tar https://ftpmirror.gnu.org/gnu/bash/bash-5.3-rc1.tar.gz bash
 fix_configure ./configure
 CFLAGS=-std=c99 ./configure && make test
}

test_cello() {
 git_fetch https://github.com/orangeduck/Cello 61ee5c3d9bca98fd68af575e9704f5f02533ae26 cello
 make check
}

test_curl() {
 github_tar curl curl curl-8_10_1
 sed -i 's/^if(MSVC OR CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")$/if (TRUE)/g' tests/CMakeLists.txt
 mkdir build && cd build
 cmake ../ -DCMAKE_C_FLAGS=-fPIC
 make -j4 && make test-quiet
}

test_doom() {
 git_fetch https://github.com/Daivuk/PureDOOM 48376ddd6bbdb70085dab91feb1c6ceef80fa9b7 puredoom
 mkdir -p examples/Tests/build && cd "$_"
 replace_line "project(pd_tests)" "project(pd_tests C)" ../CMakeLists.txt
 cmake ../ && make
 cd ../../../ && examples/Tests/build/pd_tests
}

test_git() {
 github_tar git git v2.49.0
 make CC="$CC" test -j2
}

test_glib() {
 github_clone fuhsnn glib main
 libtoolize
 sh autogen.sh
 fix_configure ./configure
 ./configure
 replace_line "#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)" "#if 1" glib/gconstructor.h
 replace_line "#ifdef __GNUC__" "#if 1" glib/gmacros.h
 replace_line "#elif defined(__GNUC__) && (__GNUC__ >= 4)" "#elif 1" gio/tests/modules/symbol-visibility.h
 make -j2 check
}

test_gmake() {
 url_tar https://ftpmirror.gnu.org/gnu/make/make-4.4.1.tar.gz gmake
 fix_configure ./configure
 CFLAGS=-std=c99 ./configure
 make check
}

test_gzip() {
 url_tar https://ftpmirror.gnu.org/gnu/gzip/gzip-1.14.tar.gz gzip
 fix_configure ./configure
 CFLAGS=-std=c99 ./configure
 make check
}

test_imagemagick() {
 github_tar ImageMagick ImageMagick 7.1.0-47
 fix_configure ./configure
 ./configure
 make check
}

test_libpng() {
 github_tar pnggroup libpng v1.6.48
 fix_configure ./configure
 ./configure
 make test
}

test_libressl() {
 url_tar https://github.com/libressl/portable/releases/download/v4.1.0/libressl-4.1.0.tar.gz libressl
 replace_line "#if defined(__GNUC__)" "#if 1" crypto/bn/arch/amd64/bn_arch.h
 fix_configure ./configure
 ./configure
 make check
}

test_libuev() {
 github_tar troglobit libuev v2.4.1
 libtoolize
 autoreconf -fi
 fix_configure ./configure
 ./configure
 make check
}

test_lua() {
 url_tar https://lua.org/ftp/lua-5.4.7.tar.gz lua
 cd src && make CC="$CC" linux-readline
 url_tar https://www.lua.org/tests/lua-5.4.7-tests.tar.gz luatests
 cd libs && make CC="$CC" && cd ../
 ../lua -e"_port=true" all.lua # assertion at files.lua:84 fail on CI
}

test_ocaml() {
 github_tar ocaml ocaml 5.3.0
 fix_configure ./configure
 ./configure --enable-ocamltest --disable-ocamldoc --disable-debugger --disable-native-compiler
 make && make -C testsuite parallel -j 4
}

test_jq() {
 git_fetch https://github.com/jqlang/jq 96e8d893c10ed2f7656ccb8cfa39a9a291663a7e jq
 libtoolize
 autoreconf -fi
 fix_configure ./configure
 ./configure --without-oniguruma
 make check
}

test_openssh() {
 github_tar openssh openssh-portable V_9_8_P1
 ./configure
 make
 make file-tests
 # make t-exec ## "regress/agent-subprocess.sh" fail in CI
 make interop-tests
 make extra-tests
 make unit
}

test_openssl() {
 github_tar openssl openssl openssl-3.4.0
 replace_line "#if !defined(__DJGPP__)" "#if 0" test/rsa_complex.c
 ./Configure
 make test -j4
}

test_perl() {
 github_tar perl perl5 v5.40.2
 # https://github.com/Perl/perl5/blob/80f266d3fc15255d56d2bebebceba52614f04943/.github/workflows/testsuite.yml#L810
 export NO_NETWORK_TESTING=1
 ./Configure -des -Dcc="$CC" -Accflags=-fPIC -Alibs="-lpthread -ldl -lm -lcrypt -lutil -lc" \
   -Alibpth="/usr/local/lib /lib /usr/lib /lib64 /usr/lib64 /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu"
 make -j4 test_prep && HARNESS_OPTIONS=j4 make test_harness
}

test_php() {
 github_tar php php-src php-8.1.32

 export SKIP_IO_CAPTURE_TESTS=1
 rm Zend/tests/zend_signed_multiply-64bit-2.phpt # ZEND_SIGNED_MULTIPLY_LONG fallback has precicion mismatch
 rm ext/fileinfo/tests/cve-2014-3538*.phpt # flaky https://github.com/php/php-src/issues/17600

 # don't work in CI https://github.com/php/php-src/blob/17187c4646f3293e1de8df3f26d56978d45186d6/.github/actions/test-linux/action.yml#L40
 export SKIP_IO_CAPTURE_TESTS=1

 ./buildconf --force
 fix_configure ./configure
 CFLAGS=-std=c99 ./configure --without-pcre-jit --disable-opcache --without-valgrind
 make test NO_INTERACTION=1
}

test_postgres() {
 github_tar postgres postgres REL_17_5
 replace_line "#if (defined(__x86_64__) || defined(_M_AMD64))" "#if 0" src/include/port/simd.h
 ./configure --disable-spinlocks --disable-atomics && make && make check
}

test_python() {
 github_tar python cpython v3.12.10
 ./configure && make

 skip_tests=(
  test_asyncio test_socket # Fail in CI
  test_peg_generator
 )
 ./python -m test -j4 --exclude "${skip_tests[@]}"
}

test_qbe_hare() {
 git_fetch git://c9x.me/qbe.git 8d5b86ac4c24e24802a60e5e9df2dd5902fe0a5c qbe
 make CC="$CC" check
 url_tar https://git.sr.ht/~sircmpwn/harec/archive/0.24.2.tar.gz harec
 mv configs/linux.mk config.mk
 make CC="$CC" QBE=../qbe check
}

test_scrapscript() {
 git_fetch https://github.com/tekknolagi/scrapscript 467a577f1fa389f91b0ecda180b23daaa2b43fa2 scrapscript
 curl -LsSf https://astral.sh/uv/install.sh | sh
 ~/.local/bin/uv python install 3.10
 ~/.local/bin/uv python pin 3.010
 ~/.local/bin/uv run python compiler_tests.py
}

test_sqlite() {
 github_tar sqlite sqlite version-3.49.2
 CC_FOR_BUILD="$CC" CFLAGS=-D_GNU_SOURCE ./configure
 make test
}

test_tinycc() {
 git_fetch https://github.com/TinyCC/tinycc 83de532563c6d922c6262dea757a22cb90d06101 tinycc
 ./configure && make && cd tests/tests2/ && make
}

test_toxcore() {
 github_clone TokTok c-toxcore v0.2.20
 libtoolize
 autoreconf -fi
 fix_configure ./configure
 ./configure
 make check
}

test_toybox() {
 github_tar landley toybox 0.8.12
 replace_line "#define QUIET" "#define QUIET = 0" lib/portability.h
 make CC="$CC" HOSTCC="$CC" defconfig
 make CC="$CC" HOSTCC="$CC"
 make CC="$CC" HOSTCC="$CC" tests
}

test_vim() {
 github_tar vim vim v9.1.1200
 ./configure
 make && make testtiny
}

test_zlib() {
 github_tar madler zlib v1.3.1
 ./configure
 make test
}

test_zsh() {
 github_tar zsh-users zsh zsh-5.9.0.2-test
 libtoolize
 autoreconf -fi
 ./configure
 sed -i 's/stat.mdd link=no/stat.mdd link=static/g' config.modules # Required to pass D07multibyte.ztst
 rm Test/A08time.ztst # Fail in CI

 make && make check
}

build_gcc() {
 url_tar https://ftpmirror.gnu.org/gnu/gcc/gcc-4.7.4/gcc-4.7.4.tar.gz gcc47
 export -f fix_configure
 find . -name 'configure' -exec bash -c 'fix_configure "$0"' {} \;
 sed -i 's/^\s*struct ucontext/ucontext_t/g' ./libgcc/config/i386/linux-unwind.h
 mkdir buildonly && cd "$_"
 export MAKEINFO=missing
 ../configure --enable-languages=c,c++ --disable-multilib --disable-bootstrap
 make
}

build_nano() {
 url_tar https://www.nano-editor.org/dist/v8/nano-8.4.tar.gz nano
 CFLAGS=-std=c99 ./configure && make
}

# run a test

if [[ $(type -t "$1") != function ]]; then
  echo 'expected a test name'
  exit 1
fi

$1

#include "widcc.h"

typedef enum {
  FILE_NONE, FILE_C, FILE_ASM, FILE_OBJ, FILE_AR, FILE_DSO, FILE_PP_ASM
} FileType;

StringArray include_paths;
bool opt_fcommon = true;
bool opt_fpic;
bool opt_g;
bool opt_func_sections;
bool opt_data_sections;
bool opt_cc1_asm_pp;
StdVer opt_std;

static StringArray opt_include;
bool opt_E;
static bool opt_P;
static bool opt_M;
static bool opt_MD;
static bool opt_MMD;
static bool opt_MP;
static bool opt_S;
static bool opt_c;
static bool opt_cc1;
static bool opt_hash_hash_hash;
static bool opt_static;
static bool opt_shared;
static char *opt_MF;
static char *opt_MT;
static char *opt_o;

static StringArray ld_extra_args;
static StringArray std_include_paths;

char *base_file;
static char *output_file;

static StringArray input_paths;
static StringArray tmpfiles;

static void usage(int status) {
  fprintf(stderr, "widcc [ -o <path> ] <file>\n");
  exit(status);
}

static bool take_arg(char *arg) {
  char *x[] = {
    "-o", "-I", "-idirafter", "-include", "-x", "-MF", "-MT", "-Xlinker",
  };

  for (int i = 0; i < sizeof(x) / sizeof(*x); i++)
    if (!strcmp(arg, x[i]))
      return true;
  return false;
}

static void add_include_path(char *p) {
  char *str = strdup(p);

  for (int i = strlen(str) - 1; i >= 0 && str[i] == '/';)
    str[i--] = '\0';

  for (int i = 0; i < include_paths.len; i++) {
    if (!strcmp(include_paths.data[i], str))
      return;
  }
  strarray_push(&include_paths, str);
}

static void add_default_include_paths(char *argv0) {
  // We expect that compiler-provided include files are installed
  // to ./include relative to argv[0].
  strarray_push(&std_include_paths, format("%s/include", dirname(strdup(argv0))));

  // Add standard include paths.
  strarray_push(&std_include_paths, "/usr/local/include");
  strarray_push(&std_include_paths, "/usr/include/x86_64-linux-gnu");
  strarray_push(&std_include_paths, "/usr/include");

  for (int i = 0; i < std_include_paths.len; i++)
    strarray_push(&include_paths, std_include_paths.data[i]);
}

static void define(char *str) {
  char *eq = strchr(str, '=');
  if (eq)
    define_macro(strndup(str, eq - str), eq + 1);
  else
    define_macro(str, "1");
}

static FileType parse_opt_x(char *s) {
  if (!strcmp(s, "c"))
    return FILE_C;
  if (!strcmp(s, "assembler"))
    return FILE_ASM;
  if (!strcmp(s, "assembler-with-cpp"))
    return FILE_PP_ASM;
  if (!strcmp(s, "none"))
    return FILE_NONE;
  error("<command line>: unknown argument for -x: %s", s);
}

static void set_std(int val) {
  if (val == 89 || val == 90)
    opt_std = STD_C89;
  else if (val == 99)
    opt_std = STD_C99;
  else if (val == 11)
    opt_std = STD_C11;
  else if (val == 17 || val == 18)
    opt_std = STD_C17;
  else if (val == 23)
    opt_std = STD_C23;
  else
    error("unknown c standard");
}

static char *quote_makefile(char *s) {
  char *buf = calloc(1, strlen(s) * 2 + 1);

  for (int i = 0, j = 0; s[i]; i++) {
    switch (s[i]) {
    case '$':
      buf[j++] = '$';
      buf[j++] = '$';
      break;
    case '#':
      buf[j++] = '\\';
      buf[j++] = '#';
      break;
    case ' ':
    case '\t':
      for (int k = i - 1; k >= 0 && s[k] == '\\'; k--)
        buf[j++] = '\\';
      buf[j++] = '\\';
      buf[j++] = s[i];
      break;
    default:
      buf[j++] = s[i];
      break;
    }
  }
  return buf;
}

static void parse_args(int argc, char **argv) {
  // Make sure that all command line options that take an argument
  // have an argument.
  for (int i = 1; i < argc; i++)
    if (take_arg(argv[i]))
      if (!argv[++i])
        usage(1);

  StringArray idirafter = {0};

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-###")) {
      opt_hash_hash_hash = true;
      continue;
    }

    if (!strcmp(argv[i], "-cc1")) {
      opt_cc1 = true;
      continue;
    }

    if (!strcmp(argv[i], "--help"))
      usage(0);

    if (!strcmp(argv[i], "-o")) {
      opt_o = argv[++i];
      continue;
    }

    if (!strncmp(argv[i], "-o", 2)) {
      opt_o = argv[i] + 2;
      continue;
    }

    if (!strcmp(argv[i], "-S")) {
      opt_S = true;
      continue;
    }

    if (!strcmp(argv[i], "-fcommon")) {
      opt_fcommon = true;
      continue;
    }

    if (!strcmp(argv[i], "-fno-common")) {
      opt_fcommon = false;
      continue;
    }

    if (!strcmp(argv[i], "-c")) {
      opt_c = true;
      continue;
    }

    if (!strcmp(argv[i], "-E")) {
      opt_E = true;
      continue;
    }

    if (!strcmp(argv[i], "-P")) {
      opt_P = true;
      continue;
    }

    if (!strcmp(argv[i], "-I")) {
      add_include_path(argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-I", 2)) {
      add_include_path(argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-D")) {
      define(argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-D", 2)) {
      define(argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-U")) {
      undef_macro(argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-U", 2)) {
      undef_macro(argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-include")) {
      strarray_push(&opt_include, argv[++i]);
      continue;
    }

    if (!strcmp(argv[i], "-x")) {
      strarray_push(&input_paths, "-x");
      strarray_push(&input_paths, argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-x", 2)) {
      strarray_push(&input_paths, "-x");
      strarray_push(&input_paths, argv[i] + 2);
      continue;
    }

    if (!strncmp(argv[i], "-l", 2) || !strncmp(argv[i], "-Wl,", 4)) {
      strarray_push(&input_paths, argv[i]);
      continue;
    }

    if (!strcmp(argv[i], "-rdynamic")) {
      strarray_push(&input_paths, "-Wl,--export-dynamic");
      continue;
    }

    if (!strcmp(argv[i], "-Xlinker")) {
      strarray_push(&ld_extra_args, argv[++i]);
      continue;
    }

    if (!strcmp(argv[i], "-s")) {
      strarray_push(&ld_extra_args, "-s");
      continue;
    }

    if (!strcmp(argv[i], "-M")) {
      opt_M = true;
      continue;
    }

    if (!strcmp(argv[i], "-MF")) {
      opt_MF = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-MP")) {
      opt_MP = true;
      continue;
    }

    if (!strcmp(argv[i], "-MT")) {
      if (opt_MT == NULL)
        opt_MT = argv[++i];
      else
        opt_MT = format("%s %s", opt_MT, argv[++i]);
      continue;
    }

    if (!strcmp(argv[i], "-MD")) {
      opt_MD = true;
      continue;
    }

    if (!strcmp(argv[i], "-MQ")) {
      if (opt_MT == NULL)
        opt_MT = quote_makefile(argv[++i]);
      else
        opt_MT = format("%s %s", opt_MT, quote_makefile(argv[++i]));
      continue;
    }

    if (!strcmp(argv[i], "-MMD")) {
      opt_MD = opt_MMD = true;
      continue;
    }

    if (!strcmp(argv[i], "-fpic") || !strcmp(argv[i], "-fPIC")) {
      opt_fpic = true;
      continue;
    }

    if (!strcmp(argv[i], "-cc1-input")) {
      base_file = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-cc1-output")) {
      output_file = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-cc1-asm-pp")) {
      opt_E = true;
      opt_cc1_asm_pp = true;
      continue;
    }

    if (!strcmp(argv[i], "-idirafter")) {
      strarray_push(&idirafter, argv[i++]);
      continue;
    }

    if (!strcmp(argv[i], "-pthread")) {
      define("_REENTRANT");
      strarray_push(&input_paths, "-lpthread");
      continue;
    }

    if (!strcmp(argv[i], "-static")) {
      opt_static = true;
      strarray_push(&ld_extra_args, "-static");
      continue;
    }

    if (!strcmp(argv[i], "-shared")) {
      opt_shared = true;
      strarray_push(&ld_extra_args, "-shared");
      continue;
    }

    if (!strcmp(argv[i], "-L")) {
      strarray_push(&ld_extra_args, "-L");
      strarray_push(&ld_extra_args, argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-L", 2)) {
      strarray_push(&ld_extra_args, "-L");
      strarray_push(&ld_extra_args, argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-hashmap-test")) {
      hashmap_test();
      exit(0);
    }

    if (!strncmp(argv[i], "-g", 2)) {
      if (argv[i][2] == '0')
        opt_g = false;
      else
        opt_g = true;
      continue;
    }

    if (!strcmp(argv[i], "-ansi")) {
      set_std(89);
      define("__STRICT_ANSI__");
      continue;
    } else if (!strncmp(argv[i], "-std=c", 6)) {
      set_std(strtoul(argv[i] + 6, NULL, 10));
      continue;
    } else if (!strncmp(argv[i], "--std=c", 7)) {
      set_std(strtoul(argv[i] + 7, NULL, 10));
      continue;
    } else if (!strcmp(argv[i], "--std")) {
      if (*argv[++i] != 'c')
        error("unknown c standard");
      set_std(strtoul(argv[i] + 1, NULL, 10));
      continue;
    }

    if (!strncmp(argv[i], "-fstack-reuse=", 14)) {
      if (strncmp(argv[i] + 14, "all\0", 4))
        dont_reuse_stack = true;
      continue;
    }

    if (!strcmp(argv[i], "-fsigned-char"))
      continue;
    if (!strcmp(argv[i], "-funsigned-char")) {
      ty_pchar->is_unsigned = true;
      continue;
    }

    if (!strcmp(argv[i], "-ffunction-sections")) {
      opt_func_sections = true;
      continue;
    }

    if (!strcmp(argv[i], "-fdata-sections")) {
      opt_data_sections = true;
      continue;
    }

    // These options are ignored for now.
    if (!strncmp(argv[i], "-O", 2) ||
        !strncmp(argv[i], "-W", 2) ||
        !strncmp(argv[i], "-std=", 5) ||
        !strncmp(argv[i], "-march=", 7) ||
        !strcmp(argv[i], "-ffreestanding") ||
        !strcmp(argv[i], "-fno-builtin") ||
        !strcmp(argv[i], "-fno-lto") ||
        !strcmp(argv[i], "-fno-asynchronous-unwind-tables") ||
        !strcmp(argv[i], "-fno-delete-null-pointer-checks") ||
        !strcmp(argv[i], "-fno-omit-frame-pointer") ||
        !strcmp(argv[i], "-fno-stack-protector") ||
        !strcmp(argv[i], "-fno-strict-aliasing") ||
        !strcmp(argv[i], "-fno-strict-overflow") ||
        !strcmp(argv[i], "-fwrapv") ||
        !strcmp(argv[i], "-m64") ||
        !strcmp(argv[i], "-mfpmath=sse") ||
        !strcmp(argv[i], "-mno-red-zone") ||
        !strcmp(argv[i], "-pedantic") ||
        !strcmp(argv[i], "-w"))
      continue;

    if (argv[i][0] == '-' && argv[i][1] != '\0')
      error("unknown argument: %s", argv[i]);

    strarray_push(&input_paths, argv[i]);
  }

  for (int i = 0; i < idirafter.len; i++)
    add_include_path(idirafter.data[i]);

  if (input_paths.len == 0)
    error("no input files");
}

static FILE *open_file(char *path) {
  if (!path || strcmp(path, "-") == 0)
    return stdout;

  FILE *out = fopen(path, "w");
  if (!out)
    error("cannot open output file: %s: %s", path, strerror(errno));
  return out;
}

static bool endswith(char *p, char *q) {
  int len1 = strlen(p);
  int len2 = strlen(q);
  return (len1 >= len2) && !strcmp(p + len1 - len2, q);
}

// Replace file extension
static char *replace_extn(char *tmpl, char *extn) {
  char *filename = basename(strdup(tmpl));
  char *dot = strrchr(filename, '.');
  if (dot)
    *dot = '\0';
  return format("%s%s", filename, extn);
}

static void cleanup(void) {
  for (int i = 0; i < tmpfiles.len; i++)
    unlink(tmpfiles.data[i]);
}

static char *create_tmpfile(void) {
  char *path = strdup("/tmp/widcc-XXXXXX");
  int fd = mkstemp(path);
  if (fd == -1)
    error("mkstemp failed: %s", strerror(errno));
  close(fd);

  strarray_push(&tmpfiles, path);
  return path;
}

static void run_subprocess(char **argv) {
  // If -### is given, dump the subprocess's command line.
  if (opt_hash_hash_hash) {
    fprintf(stderr, "%s", argv[0]);
    for (int i = 1; argv[i]; i++)
      fprintf(stderr, " %s", argv[i]);
    fprintf(stderr, "\n");
  }

  if (fork() == 0) {
    // Child process. Run a new command.
    execvp(argv[0], argv);
    fprintf(stderr, "exec failed: %s: %s\n", argv[0], strerror(errno));
    _exit(1);
  }

  // Wait for the child process to finish.
  int status;
  while (wait(&status) > 0);
  if (status != 0)
    exit(1);
}

static void run_cc1(int argc, char **argv, char *input, char *output, char *option) {
  char **args = calloc(argc + 10, sizeof(char *));
  memcpy(args, argv, argc * sizeof(char *));
  args[argc++] = "-cc1";

  if (input) {
    args[argc++] = "-cc1-input";
    args[argc++] = input;
  }

  if (output) {
    args[argc++] = "-cc1-output";
    args[argc++] = output;
  }

  if (option)
    args[argc++] = option;

  run_subprocess(args);
}

static void print_linemarker(FILE *out, Token *tok) {
  char *name = tok->file->name;
  if (!strcmp(name, "-"))
    name = "<stdin>";
  fprintf(out, "\n# %d \"%s\"\n", tok->line_no, name);
}

// Print tokens to stdout. Used for -E.
static void print_tokens(Token *tok, char *path) {
  FILE *out = open_file(path);

  int line = 0;
  File *markerfile = NULL;
  tok->at_bol = false;

  for (; tok->kind != TK_EOF; tok = tok->next) {
    if (!opt_P) {
      Token *orig = tok->origin ? tok->origin : tok;

      if (markerfile != orig->file) {
        markerfile = orig->file;
        print_linemarker(out, orig);
      } else {
        int diff = orig->line_no - line;
        if (diff > 0 && diff <= 8)
          while (line++ < orig->line_no)
            fprintf(out, "\n");
        else if (diff)
          print_linemarker(out, orig);
      }
      line = orig->line_no;
    } else {
      if (tok->at_bol)
        fprintf(out, "\n");
    }
    if (tok->has_space)
      fprintf(out, " ");

    fprintf(out, "%.*s", tok->len, tok->loc);
  }
  fprintf(out, "\n");

  if (out == stdout)
    fflush(out);
  else
    fclose(out);
}

static bool in_std_include_path(char *path) {
  for (int i = 0; i < std_include_paths.len; i++) {
    char *dir = std_include_paths.data[i];
    int len = strlen(dir);
    if (strncmp(dir, path, len) == 0 && path[len] == '/')
      return true;
  }
  return false;
}

// If -M options is given, the compiler write a list of input files to
// stdout in a format that "make" command can read. This feature is
// used to automate file dependency management.
static void print_dependencies(void) {
  char *path;
  if (opt_MF)
    path = opt_MF;
  else if (opt_MD)
    path = replace_extn(opt_o ? opt_o : base_file, ".d");
  else if (opt_o)
    path = opt_o;
  else
    path = "-";

  FILE *out = open_file(path);
  if (opt_MT)
    fprintf(out, "%s:", opt_MT);
  else
    fprintf(out, "%s:", quote_makefile(replace_extn(base_file, ".o")));

  File **files = get_input_files();

  for (int i = 0; files[i]; i++) {
    char *name = files[i]->name;
    if ((opt_MMD && in_std_include_path(name)) || !files[i]->is_input)
      continue;
    fprintf(out, " \\\n  %s", name);
  }

  fprintf(out, "\n\n");

  if (opt_MP) {
    for (int i = 1; files[i]; i++) {
      char *name = files[i]->name;
      if ((opt_MMD && in_std_include_path(name)) || !files[i]->is_input)
        continue;
      fprintf(out, "%s:\n\n", quote_makefile(name));
    }
  }
}

static Token *must_tokenize_file(char *path, Token **end) {
  int incl_no = -1;
  Token *tok = tokenize_file(path, end, &incl_no);
  if (!tok)
    error("%s: %s", path, strerror(errno));
  return tok;
}

static void cc1(void) {
  Token head = {0};
  Token *cur = &head;

  if (!opt_E) {
    Token *end;
    head.next = tokenize(add_input_file("widcc_builtins",
    "typedef struct {"
    "  unsigned int gp_offset;"
    "  unsigned int fp_offset;"
    "  void *overflow_arg_area;"
    "  void *reg_save_area;"
    "} __builtin_va_list[1];", NULL), &end);
    cur = end;
  }

  // Process -include option
  for (int i = 0; i < opt_include.len; i++) {
    char *incl = opt_include.data[i];

    char *path;
    if (file_exists(incl)) {
      path = incl;
    } else {
      path = search_include_paths(incl);
      if (!path)
        error("-include: %s: %s", incl, strerror(errno));
    }

    Token *end = NULL;
    cur->next = must_tokenize_file(path, &end);
    if (end)
      cur = end;
  }

  // Tokenize and parse.
  cur->next = must_tokenize_file(base_file, NULL);
  Token *tok = preprocess(head.next);

  // If -M or -MD are given, print file dependencies.
  if (opt_M || opt_MD) {
    print_dependencies();
    if (opt_M)
      return;
  }

  // If -E is given, print out preprocessed C code as a result.
  if (opt_E) {
    print_tokens(tok, output_file);
    return;
  }

  Obj *prog = parse(tok);

  // Open a temporary output buffer.
  char *buf;
  size_t buflen;
  FILE *output_buf = open_memstream(&buf, &buflen);

  // Traverse the AST to emit assembly.
  codegen(prog, output_buf);
  fclose(output_buf);

  // Write the asembly text to a file.
  FILE *out = open_file(output_file);
  fwrite(buf, buflen, 1, out);
  fclose(out);
}

static void assemble(char *input, char *output) {
  char *cmd[] = {"as", input, "-o", output, NULL};
  // char *cmd[] = {"clang", "-c", "-xassembler", input, "-o", output, NULL};
  run_subprocess(cmd);
}

static char *find_file(char *pattern) {
  char *path = NULL;
  glob_t buf = {0};
  glob(pattern, 0, NULL, &buf);
  if (buf.gl_pathc > 0)
    path = strdup(buf.gl_pathv[buf.gl_pathc - 1]);
  globfree(&buf);
  return path;
}

// Returns true if a given file exists.
bool file_exists(char *path) {
  struct stat st;
  return !stat(path, &st);
}

static char *find_libpath(void) {
  if (file_exists("/usr/lib/x86_64-linux-gnu/crti.o"))
    return "/usr/lib/x86_64-linux-gnu";
  if (file_exists("/usr/lib64/crti.o"))
    return "/usr/lib64";
  error("library path is not found");
}

static char *find_gcc_libpath(void) {
  char *path = find_file("/usr/lib*/gcc/x86_64*-linux*/*/crtbegin.o");
  if (path)
    return dirname(path);
  error("gcc library path is not found");
}

static void run_linker(StringArray *inputs, char *output) {
  StringArray arr = {0};

  strarray_push(&arr, "ld");
  strarray_push(&arr, "-o");
  strarray_push(&arr, output);
  strarray_push(&arr, "-m");
  strarray_push(&arr, "elf_x86_64");

  char *libpath = find_libpath();
  char *gcc_libpath = find_gcc_libpath();

  if (opt_shared) {
    strarray_push(&arr, format("%s/crti.o", libpath));
    strarray_push(&arr, format("%s/crtbeginS.o", gcc_libpath));
  } else {
    strarray_push(&arr, format("%s/crt1.o", libpath));
    strarray_push(&arr, format("%s/crti.o", libpath));
    strarray_push(&arr, format("%s/crtbegin.o", gcc_libpath));
  }

  strarray_push(&arr, format("-L%s", gcc_libpath));
  strarray_push(&arr, "-L/usr/lib/x86_64-linux-gnu");
  strarray_push(&arr, "-L/usr/lib64");
  strarray_push(&arr, "-L/lib64");
  strarray_push(&arr, "-L/usr/lib/x86_64-linux-gnu");
  strarray_push(&arr, "-L/usr/lib/x86_64-pc-linux-gnu");
  strarray_push(&arr, "-L/usr/lib/x86_64-redhat-linux");
  strarray_push(&arr, "-L/usr/lib");
  strarray_push(&arr, "-L/lib");

  if (!opt_static) {
    strarray_push(&arr, "-dynamic-linker");
    strarray_push(&arr, "/lib64/ld-linux-x86-64.so.2");
  }

  for (int i = 0; i < ld_extra_args.len; i++)
    strarray_push(&arr, ld_extra_args.data[i]);

  for (int i = 0; i < inputs->len; i++)
    strarray_push(&arr, inputs->data[i]);

  if (opt_static) {
    strarray_push(&arr, "--start-group");
    strarray_push(&arr, "-lgcc");
    strarray_push(&arr, "-lgcc_eh");
    strarray_push(&arr, "-lc");
    strarray_push(&arr, "--end-group");
  } else {
    strarray_push(&arr, "-lc");
    strarray_push(&arr, "-lgcc");
    strarray_push(&arr, "--as-needed");
    strarray_push(&arr, "-lgcc_s");
    strarray_push(&arr, "--no-as-needed");
  }

  if (opt_shared)
    strarray_push(&arr, format("%s/crtendS.o", gcc_libpath));
  else
    strarray_push(&arr, format("%s/crtend.o", gcc_libpath));

  strarray_push(&arr, format("%s/crtn.o", libpath));
  strarray_push(&arr, NULL);

  run_subprocess(arr.data);
}

static FileType get_file_type(char *filename) {
  if (endswith(filename, ".a"))
    return FILE_AR;
  if (endswith(filename, ".so"))
    return FILE_DSO;
  if (endswith(filename, ".o") || endswith(filename, ".lo"))
    return FILE_OBJ;
  if (endswith(filename, ".c"))
    return FILE_C;
  if (endswith(filename, ".s"))
    return FILE_ASM;
  if (endswith(filename, ".S"))
    return FILE_PP_ASM;

  if (opt_E && (!strcmp(filename, "-") || endswith(filename, ".h")))
    return FILE_C;

  char *p = strstr(filename, ".so.");
  if (p) {
    p += 3;
    while (isdigit(*p) || (*p == '.' && isdigit(p[1])))
      p++;
    if (!*p)
      return FILE_DSO;
  }

  error("<command line>: unknown file extension: %s", filename);
}

int main(int argc, char **argv) {
  atexit(cleanup);
  init_macros();
  parse_args(argc, argv);

  if (opt_cc1) {
    add_default_include_paths(argv[0]);
    cc1();
    return 0;
  }

  StringArray ld_args = {0};
  int file_count = 0;
  FileType opt_x = FILE_NONE;
  bool run_ld = false;

  for (int i = 0; i < input_paths.len; i++) {
    if (!strcmp(input_paths.data[i], "-x")) {
      opt_x = parse_opt_x(input_paths.data[++i]);
      continue;
    }

    char *input = input_paths.data[i];

    if (!strncmp(input, "-l", 2)) {
      strarray_push(&ld_args, input);
      continue;
    }

    if (!strncmp(input, "-Wl,", 4)) {
      char *s = strdup(input + 4);
      char *arg = strtok(s, ",");
      while (arg) {
        strarray_push(&ld_args, arg);
        arg = strtok(NULL, ",");
      }
      continue;
    }

    if (opt_o && (opt_c || opt_S || opt_E))
      if (++file_count > 1)
        error("cannot specify '-o' with '-c,' '-S' or '-E' with multiple files");

    char *output;
    if (opt_o)
      output = opt_o;
    else if (opt_S)
      output = replace_extn(input, ".s");
    else
      output = replace_extn(input, ".o");

    FileType type;
    if (opt_x != FILE_NONE)
      type = opt_x;
    else
      type = get_file_type(input);

    // Handle .o or .a
    if (type == FILE_OBJ || type == FILE_AR || type == FILE_DSO) {
      strarray_push(&ld_args, input);
      run_ld = true;
      continue;
    }

    // Handle .s
    if (type == FILE_ASM) {
      if (opt_S || opt_E || opt_M)
        continue;

      if (opt_c) {
        assemble(input, output);
        continue;
      }

      char *tmp = create_tmpfile();
      assemble(input, tmp);
      strarray_push(&ld_args, tmp);
      run_ld = true;
      continue;
    }

    // Handle .S
    if (type == FILE_PP_ASM) {
      if (opt_S || opt_E || opt_M) {
        run_cc1(argc, argv, input, (opt_o ? opt_o : "-"), "-cc1-asm-pp");
        continue;
      }
      if (opt_c) {
        char *tmp = create_tmpfile();
        run_cc1(argc, argv, input, tmp, "-cc1-asm-pp");
        assemble(tmp, output);
        continue;
      }
      char *tmp1 = create_tmpfile();
      char *tmp2 = create_tmpfile();
      run_cc1(argc, argv, input, tmp1, "-cc1-asm-pp");
      assemble(tmp1, tmp2);
      strarray_push(&ld_args, tmp2);
      run_ld = true;
      continue;
    }

    assert(type == FILE_C);

    // Just preprocess
    if (opt_E || opt_M) {
      run_cc1(argc, argv, input, (opt_o ? opt_o : "-"), NULL);
      continue;
    }

    // Compile
    if (opt_S) {
      run_cc1(argc, argv, input, output, NULL);
      continue;
    }

    // Compile and assemble
    if (opt_c) {
      char *tmp = create_tmpfile();
      run_cc1(argc, argv, input, tmp, NULL);
      assemble(tmp, output);
      continue;
    }

    // Compile, assemble and link
    char *tmp1 = create_tmpfile();
    char *tmp2 = create_tmpfile();
    run_cc1(argc, argv, input, tmp1, NULL);
    assemble(tmp1, tmp2);
    strarray_push(&ld_args, tmp2);
    run_ld = true;
    continue;
  }

  if (run_ld)
    run_linker(&ld_args, opt_o ? opt_o : "a.out");
  return 0;
}

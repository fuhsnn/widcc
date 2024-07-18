#include "test.h"

#if !defined(__has_attribute)
#error
#endif

#if !__has_attribute(packed)
#error
#endif

#define CAT(x,y) x##y

void has_attr(void) {
  DASSERT(__has_attribute(packed) == 1);
  DASSERT( CAT(__has,_attribute)(packed) == 1);
}

void packed(void) {
  ASSERT(0, offsetof(struct __attribute__((packed)) { char a; int b; }, a));
  ASSERT(1, offsetof(struct __attribute__((__packed__)) { char a; int b; }, b));
  ASSERT(5, ({ struct __attribute__((packed)) { char a; int b; } x; sizeof(x); }));
  ASSERT(9, ({ typedef struct __attribute__((packed)) { char a; int b[2]; } T; sizeof(T); }));
  ASSERT(1, offsetof(struct __attribute__((packed)) T { char a; int b[2]; }, b));
}

int main() {
  has_attr();
  packed();

  printf("OK\n");
  return 0;
}

#include "test.h"
#include <stdarg.h>

typedef struct {
  char g;
} G;

typedef struct {
  float f;
} F;

typedef struct {
  char l;
  float d;
} A;

extern float struct_test101(
  G g0, G g1, G g2, G g3, G g4, G g5,
  F f0, F f1, F f2, F f3, F f4, F f5, F f6, F f7,
  G gs0, F fs0, G gs1, F fs1
);
float struct_test100(
  G g0, G g1, G g2, G g3, G g4, G g5,
  F f0, F f1, F f2, F f3, F f4, F f5, F f6, F f7,
  G gs0, F fs0, G gs1, F fs1
) {
  return g0.g + g1.g + g2.g + g3.g + g4.g + g5.g +
  f0.f + f1.f + f2.f + f3.f + f4.f + f5.f + f6.f + f7.f +
  gs0.g + gs1.g + fs0.f + fs1.f;
}

extern float struct_test111(
  F f0, F f1, F f2, F f3, F f4, F f5, F f6, F f7,
  G g0, G g1, G g2, G g3, G g4, G g5,
  G gs0, F fs0, G gs1, F fs1
);
float struct_test110(
  F f0, F f1, F f2, F f3, F f4, F f5, F f6, F f7,
  G g0, G g1, G g2, G g3, G g4, G g5,
  G gs0, F fs0, G gs1, F fs1
) {
  return
  g0.g + g1.g + g2.g + g3.g + g4.g + g5.g +
  f0.f + f1.f + f2.f + f3.f + f4.f + f5.f + f6.f + f7.f +
  gs0.g + gs1.g + fs0.f + fs1.f;
}

extern float struct_test121(
  A reg0, A reg1, A reg2, A reg3, A reg4, A reg5,
  A s0,
  F f6, F f7,
  A s1);

float struct_test120(
  A reg0, A reg1, A reg2, A reg3, A reg4, A reg5,
  A s0,
  F f6, F f7,
  A s1)
{
  return
  reg0.l + reg0.d + reg1.l + reg1.d + reg2.l + reg2.d +
  reg3.l + reg3.d + reg4.l + reg4.d + reg5.l + reg5.d +
  s0.l + s0.d + s1.l + s1.d + f6.f + f7.f;
}


extern int struct_test131(G g0,G g1,G g2,G g3,G g4,F f0,F f1,F f2,F f3,F f4,F f5,int i0,int i1,...);
int struct_test130(G g0,G g1,G g2,G g3,G g4,F f0,F f1,F f2,F f3,F f4,F f5,int i0,int i1,...) {
  va_list ap;
  va_start(ap, i1);
  long double ret = i0 + i1;
  ret += va_arg(ap, long double);
  ret += va_arg(ap, int);
  ret += va_arg(ap, double);
  ret += va_arg(ap, double);
  ret += va_arg(ap, double);
  ret += va_arg(ap, double);
  ret += va_arg(ap, long double);
  va_end(ap);
  return ret;
}

typedef struct {
  long i,j;
} DI;

int struct_test141(int cnt,...);
int struct_test140(int cnt,...) {
  va_list ap;
  va_start(ap, cnt);
  long ret = 0;
  for (int i = 0; i < cnt; i++) {
    DI s = va_arg(ap, DI);
    ret += s.i + s.j;
  }
  va_end(ap);
  return ret;
}

typedef struct {
  double d,f;
} DD;

int struct_test151(int cnt,...);
int struct_test150(int cnt,...) {
  va_list ap;
  va_start(ap, cnt);
  double ret = 0;
  for (int i = 0; i < cnt; i++) {
    DD s = va_arg(ap, DD);
    ret += s.d + s.f;
  }
  va_end(ap);
  return ret;
}

typedef struct {
    long i;
    double d;
} DM;
int struct_test161(int cnt,...);
int struct_test160(int cnt,...) {
  va_list ap;
  va_start(ap, cnt);
  double ret = 0;
  for (int i = 0; i < cnt; i++) {
    DM s = va_arg(ap, DM);
    ret += s.d + s.i;
  }
  va_end(ap);
  return ret;
}

static int add_all2(int n, ...) {
  va_list ap;
  va_start(ap, n);

  int sum = 0;
  for (int i = 0; i < n; i++)
    sum += ({ va_arg(ap, int); });
  return sum;
}

static void va_fn0(int cnt, ...) {
    va_list ap;
    va_start(ap, cnt);
    for(int i = 0; i < cnt; i++)
        va_arg(ap, void(*)(void))();
    va_end(ap);
}
static int garr[2];
static void va_fn1(void) { garr[0] = 111; }
static void va_fn2(void) { garr[1] = 222; }

int main(void) {
  G g[] = {10,11,12,13,14,15};
  F f[] = {20,21,22,23,24,25,26,27};
  G gs[]  = {30,31};
  F fs[]  = {40,41};

  ASSERT(405, struct_test100(
g[0], g[1], g[2], g[3], g[4], g[5],
f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7],
gs[0], fs[0], gs[1], fs[1]));

  ASSERT(405, struct_test101(
g[0], g[1], g[2], g[3], g[4], g[5],
f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7],
gs[0], fs[0], gs[1], fs[1]));

  ASSERT(405, struct_test110(
f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7],
g[0], g[1], g[2], g[3], g[4], g[5],
gs[0], fs[0], gs[1], fs[1]));

  ASSERT(405, struct_test111(
f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7],
g[0], g[1], g[2], g[3], g[4], g[5],
gs[0], fs[0], gs[1], fs[1]));

  A reg[] = {10,11,20,21,30,31,40,41,50,51,60,61};
  A as[] = {70,71,80,81};

  ASSERT(781, struct_test120(
reg[0], reg[1], reg[2], reg[3], reg[4], reg[5],
as[0],
f[6], f[7],
as[1]));

  ASSERT(781, struct_test121(
reg[0], reg[1], reg[2], reg[3], reg[4], reg[5],
as[0],
f[6], f[7],
as[1]));

  ASSERT(103, struct_test131(
    g[0], g[1], g[2], g[3], g[4],
    f[0], f[1], f[2], f[3], f[4], f[5],
    (int) 11,
    (int) 22,
    (long double) 2.3,
    (int) 33,
    (double) 4.5,
    (double) 5.6,
    (double) 6.7,
    (double) 7.8,
    (long double) 11.1));

  ASSERT(103, struct_test130(
    g[0], g[1], g[2], g[3], g[4],
    f[0], f[1], f[2], f[3], f[4], f[5],
    (int) 11,
    (int) 22,
    (long double) 2.3,
    (int) 33,
    (double) 4.5,
    (double) 5.6,
    (double) 6.7,
    (double) 7.8,
    (long double) 11.1));

    {
      DI s0 = {2,3};
      DI s1 = {5,7};
      DI s2 = {11,13};
      DI s3 = {17,19};
      DI s4 = {23,29};
      DI s5 = {31,37};
      DI s6 = {41,43};
      DI s7 = {47,53};
      DI s8 = {59,61};
      DI s9 = {67,71};
      ASSERT(639, struct_test140(10, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9));
      ASSERT(639, struct_test141(10, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9));
    }
    {
      DD s0 = {2,3};
      DD s1 = {5,7};
      DD s2 = {11,13};
      DD s3 = {17,19};
      DD s4 = {23,29};
      DD s5 = {31,37};
      DD s6 = {41,43};
      DD s7 = {47,53};
      DD s8 = {59,61};
      DD s9 = {67,71};
      ASSERT(639, struct_test150(10, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9));
      ASSERT(639, struct_test151(10, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9));
    }

    {
      DM s0 = {2,3};
      DM s1 = {5,7};
      DM s2 = {11,13};
      DM s3 = {17,19};
      DM s4 = {23,29};
      DM s5 = {31,37};
      DM s6 = {41,43};
      DM s7 = {47,53};
      DM s8 = {59,61};
      DM s9 = {67,71};
      ASSERT(639, struct_test160(10, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9));
      ASSERT(639, struct_test161(10, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9));
    }

  ASSERT(6, add_all2(3,1,2,3));
  ASSERT(5, add_all2(4,1,2,3,-1));

  va_fn0(2, &va_fn1, &va_fn2);
  ASSERT(111, garr[0]);
  ASSERT(222, garr[1]);

  printf("OK\n");

}


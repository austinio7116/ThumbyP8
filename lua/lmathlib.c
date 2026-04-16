/*
** $Id: lmathlib.c,v 1.83.1.1 2013/04/12 18:48:47 roberto Exp $
** Standard mathematical library
** See Copyright Notice in lua.h
**
** ThumbyP8: rewritten for fixed-point lua_Number. Arithmetic stays
** in int32 fixed-point; transcendentals convert to double, compute,
** and convert back. This keeps math.floor / math.abs / math.min
** bit-exact with PICO-8 while using libm for sin/cos/etc.
*/


#include <stdlib.h>
#include <math.h>

#define lmathlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"


#undef PI
#define PI 3.1415926535897932384626433832795

/* Helpers: lua_Number (int32 fixed-point) ↔ double for libm calls. */
static double n2d(lua_Number n) { return (double)n / 65536.0; }
static lua_Number d2n(double d) {
  double s = d * 65536.0;
  if (s >=  2147483647.0) return (lua_Number)0x7fffffff;
  if (s <= -2147483648.0) return (lua_Number)0x80000000;
  return (lua_Number)s;
}


static int math_abs (lua_State *L) {
  lua_Number x = luaL_checknumber(L, 1);
  lua_pushnumber(L, x < 0 ? (lua_Number)(-(uint32_t)x) : x);
  return 1;
}

static int math_sin (lua_State *L) {
  lua_pushnumber(L, d2n(sin(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_sinh (lua_State *L) {
  lua_pushnumber(L, d2n(sinh(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_cos (lua_State *L) {
  lua_pushnumber(L, d2n(cos(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_cosh (lua_State *L) {
  lua_pushnumber(L, d2n(cosh(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_tan (lua_State *L) {
  lua_pushnumber(L, d2n(tan(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_tanh (lua_State *L) {
  lua_pushnumber(L, d2n(tanh(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_asin (lua_State *L) {
  lua_pushnumber(L, d2n(asin(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_acos (lua_State *L) {
  lua_pushnumber(L, d2n(acos(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_atan (lua_State *L) {
  lua_pushnumber(L, d2n(atan(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_atan2 (lua_State *L) {
  lua_pushnumber(L, d2n(atan2(n2d(luaL_checknumber(L, 1)),
                              n2d(luaL_checknumber(L, 2)))));
  return 1;
}

static int math_ceil (lua_State *L) {
  lua_Number x = luaL_checknumber(L, 1);
  /* ceil = -floor(-x). In fixed-point: flip sign, arithmetic-shift
   * down+up (floor), flip sign back. */
  lua_Number neg = (lua_Number)(-(uint32_t)x);
  lua_Number flr = (lua_Number)((neg >> 16) << 16);
  lua_pushnumber(L, (lua_Number)(-(uint32_t)flr));
  return 1;
}

static int math_floor (lua_State *L) {
  lua_Number x = luaL_checknumber(L, 1);
  lua_pushnumber(L, (lua_Number)((x >> 16) << 16));
  return 1;
}

static int math_fmod (lua_State *L) {
  /* C fmod: result has sign of dividend. */
  lua_pushnumber(L, d2n(fmod(n2d(luaL_checknumber(L, 1)),
                             n2d(luaL_checknumber(L, 2)))));
  return 1;
}

static int math_modf (lua_State *L) {
  lua_Number x = luaL_checknumber(L, 1);
  /* Integer part: truncate toward zero (C modf semantics). */
  lua_Number ip;
  if (x >= 0)  ip = (lua_Number)((x >> 16) << 16);
  else         ip = (lua_Number)(-((-(uint32_t)x >> 16) << 16));
  lua_pushnumber(L, ip);
  lua_pushnumber(L, (lua_Number)((uint32_t)x - (uint32_t)ip));
  return 2;
}

static int math_sqrt (lua_State *L) {
  lua_Number x = luaL_checknumber(L, 1);
  lua_pushnumber(L, x < 0 ? 0 : d2n(sqrt(n2d(x))));
  return 1;
}

static int math_pow (lua_State *L) {
  lua_pushnumber(L, d2n(pow(n2d(luaL_checknumber(L, 1)),
                            n2d(luaL_checknumber(L, 2)))));
  return 1;
}

static int math_log (lua_State *L) {
  double x = n2d(luaL_checknumber(L, 1));
  double res;
  if (lua_isnoneornil(L, 2))
    res = log(x);
  else {
    double base = n2d(luaL_checknumber(L, 2));
    if (base == 10.0) res = log10(x);
    else              res = log(x) / log(base);
  }
  lua_pushnumber(L, d2n(res));
  return 1;
}

#if defined(LUA_COMPAT_LOG10)
static int math_log10 (lua_State *L) {
  lua_pushnumber(L, d2n(log10(n2d(luaL_checknumber(L, 1)))));
  return 1;
}
#endif

static int math_exp (lua_State *L) {
  lua_pushnumber(L, d2n(exp(n2d(luaL_checknumber(L, 1)))));
  return 1;
}

static int math_deg (lua_State *L) {
  /* radians → degrees: x * (180/pi) */
  lua_pushnumber(L, d2n(n2d(luaL_checknumber(L, 1)) * (180.0 / PI)));
  return 1;
}

static int math_rad (lua_State *L) {
  lua_pushnumber(L, d2n(n2d(luaL_checknumber(L, 1)) * (PI / 180.0)));
  return 1;
}

static int math_frexp (lua_State *L) {
  int e;
  double m = frexp(n2d(luaL_checknumber(L, 1)), &e);
  lua_pushnumber(L, d2n(m));
  lua_pushinteger(L, e);
  return 2;
}

static int math_ldexp (lua_State *L) {
  double x = n2d(luaL_checknumber(L, 1));
  int ep = luaL_checkint(L, 2);
  lua_pushnumber(L, d2n(ldexp(x, ep)));
  return 1;
}



static int math_min (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  lua_Number dmin = luaL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    lua_Number d = luaL_checknumber(L, i);
    if (d < dmin)
      dmin = d;
  }
  lua_pushnumber(L, dmin);
  return 1;
}


static int math_max (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  lua_Number dmax = luaL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    lua_Number d = luaL_checknumber(L, i);
    if (d > dmax)
      dmax = d;
  }
  lua_pushnumber(L, dmax);
  return 1;
}


static int math_random (lua_State *L) {
  /* r in [0, 1) as fixed-point: rand() % 65536 gives 0..65535. */
  lua_Number r = (lua_Number)(rand() & 0xffff);
  switch (lua_gettop(L)) {
    case 0: {  /* [0, 1) */
      lua_pushnumber(L, r);
      break;
    }
    case 1: {  /* [1, u] integer */
      lua_Number u = luaL_checknumber(L, 1);
      int iu = (int)(u >> 16);
      luaL_argcheck(L, iu >= 1, 1, "interval is empty");
      int v = (rand() % iu) + 1;
      lua_pushnumber(L, p8_fix_from_int(v));
      break;
    }
    case 2: {  /* [l, u] integer */
      lua_Number l = luaL_checknumber(L, 1);
      lua_Number u = luaL_checknumber(L, 2);
      int il = (int)(l >> 16);
      int iu = (int)(u >> 16);
      luaL_argcheck(L, il <= iu, 2, "interval is empty");
      int range = iu - il + 1;
      int v = (rand() % range) + il;
      lua_pushnumber(L, p8_fix_from_int(v));
      break;
    }
    default: return luaL_error(L, "wrong number of arguments");
  }
  return 1;
}


static int math_randomseed (lua_State *L) {
  srand(luaL_checkunsigned(L, 1));
  (void)rand(); /* discard first value to avoid undesirable correlations */
  return 0;
}


static const luaL_Reg mathlib[] = {
  {"abs",   math_abs},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan2", math_atan2},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cosh",   math_cosh},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"exp",   math_exp},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
#if defined(LUA_COMPAT_LOG10)
  {"log10", math_log10},
#endif
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"pow",   math_pow},
  {"rad",   math_rad},
  {"random",     math_random},
  {"randomseed", math_randomseed},
  {"sinh",   math_sinh},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tanh",   math_tanh},
  {"tan",   math_tan},
  {NULL, NULL}
};


/*
** Open math library
*/
LUAMOD_API int luaopen_math (lua_State *L) {
  luaL_newlib(L, mathlib);
  lua_pushnumber(L, d2n(PI));
  lua_setfield(L, -2, "pi");
  lua_pushnumber(L, (lua_Number)0x7fffffff);
  lua_setfield(L, -2, "huge");
  return 1;
}


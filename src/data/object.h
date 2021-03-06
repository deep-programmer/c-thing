#pragma once

#include <stdio.h>

#include "cthing.h"

#include "mem/ref.h"

#ifdef CT_FEATURE_SSE
#include <xmmintrin.h>
#endif

CT_BEGIN_DECLS

typedef enum {
  CT_TYPE_NIL = 0,
  CT_TYPE_I32,
  CT_TYPE_U32,
  CT_TYPE_F32,
  CT_TYPE_VEC4,
  CT_TYPE_PTR,
  CT_TYPE_STR,
  CT_TYPE_CONS
} CT_PrimType;

typedef union {
  uintptr_t u;
  intptr_t i;
  float f;
  void *p;
  struct {
    float x, y, z, w;
  };
  struct {
    float r, g, b, a;
  };
#ifdef CT_FEATURE_SSE
  __m128 vec;
#endif
} CT_Atom;

typedef union {
  size_t tag;
  struct {
    size_t type : 24;
    size_t free : 1;
  };
} CT_Tag;

typedef struct {
  CT_Atom atom;
  CT_Ref rc;
  CT_Tag tag;
} CT_Object;

typedef struct {
  CT_Object *value;
  CT_Object *next;
} CT_ConsRC;

typedef struct { int (*print)(CT_Object *, FILE *); } CT_IPrint;

typedef struct { int (*tostring)(CT_Object *, char *, int); } CT_IToString;

int ct_object_print(CT_Object *o, FILE *out);
int ct_register_print(size_t type, int impl(CT_Object *, FILE *));

int ct_object_tostring(CT_Object *o, char *buf, int bsize);
int ct_register_tostring(size_t type, int impl(CT_Object *, char *, int));

int ct_object_init();
CT_Object *ct_object_raw(size_t type);
CT_Object *ct_object_str(char *x, size_t free);
CT_Object *ct_object_i32(int32_t x);
CT_Object *ct_object_u32(uint32_t x);
CT_Object *ct_object_f32(float x);
CT_Object *ct_object_vec4(float x, float y, float z, float w);

void ct_object_free_nop(const CT_Ref *ref);
void ct_object_free_box(const CT_Object *o);
void ct_object_trace(const CT_Object *o);

ct_inline int ct_object_is(const CT_Object *o, size_t type) {
  return o->tag.type == type;
}

ct_inline CT_Object *ct_object_assign(CT_Object **dest, CT_Object *src) {
  ct_ref_inc(&src->rc);
  *dest = src;
  return *dest;
}

ct_inline void ct_object_unassign(CT_Object **o) {
  ct_ref_dec(&(*o)->rc);
  *o = NULL;
}

CT_END_DECLS

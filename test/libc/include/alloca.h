#ifndef _ALLOCA_H
#define _ALLOCA_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef long size_t;

void *alloca(size_t);

#ifdef __GNUC__
#define alloca __builtin_alloca
#endif

#ifdef __cplusplus
}
#endif

#endif

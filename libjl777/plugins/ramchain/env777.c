//
//  env777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_env777_h
#define crypto777_env777_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#endif
#else
#ifndef crypto777_env777_c
#define crypto777_env777_c

#ifndef crypto777_env777_h
#define DEFINES_ONLY
#include "env777.c"
#undef DEFINES_ONLY
#endif


#endif
#endif

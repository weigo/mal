#ifndef _MAL_CORE_H
#define _MAL_CORE_H

#include "env.h"

MalEnvironment *make_initial_environment();
MalValue *list(MalCell *values);
int64_t _count(MalCell *value);
#endif
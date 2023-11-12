#ifndef _MAL_CORE_H
#define _MAL_CORE_H

#include "env.h"

MalEnvironment *make_initial_environment();
MalValue *list(MalCell *values);

int64_t _count(MalCell *value);

/**
 * Determine length of given list.
 * 
 * @param list the list whose length should be determined
 * @return lenght of list.
 */
int64_t length(MalValue *list);

#endif
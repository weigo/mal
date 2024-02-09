#ifndef _MAL_CORE_H
#define _MAL_CORE_H

#include "env.h"

MalValue *list(MalCell *values);
HashMap *core_namespace();
int64_t _count(MalCell *value);

/**
 * Determine length of given list.
 *
 * @param list the list whose length should be determined
 * @return lenght of list.
 */
int64_t length(MalValue *list);

/**
 * Read a file and return the contents as string.
 */
char *read_file(const char *file_name);
#endif
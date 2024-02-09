#ifndef _MAL_ENV_H
#define _MAL_ENV_H

#include <stdbool.h>
#include <stdlib.h>
#include "types.h"

MalEnvironment *make_environment(MalEnvironment *parent, MalCell *binds, MalCell *exprs, MalValue *rest_symbol);
MalEnvironment *find_environment(MalEnvironment *start, MalValue *symbol);
MalValue *lookup_in_environment(MalEnvironment *environment, MalValue *package, MalValue *symbol);
bool set_in_environment(MalEnvironment *environment, MalValue *symbol, MalValue *value);
void free_environment(MalEnvironment *parent);
#endif
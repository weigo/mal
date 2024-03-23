#include <assert.h>
#include "env.h"
#include "gc.h"
#include "printer.h"

void free_environment(MalEnvironment *environment)
{
    if (environment->map)
    {
        free_hashmap(environment->map);
    }

    environment->parent = NULL;
    environment->map = NULL;

    free(environment);
}

MalEnvironment *find_environment(MalEnvironment *start, const MalValue *symbol)
{
    MalEnvironment *env = start;
    MalValue *value = NULL;
    char *symbol_name = get_symbol_name(symbol);

    while (env != NULL && value == NULL)
    {
        value = hashmap_get(env->map, symbol_name);

        if (value)
        {
            break;
        }

        env = env->parent;
    }

    return env;
}

extern MalValue *get_current_package();
extern MalValue *lookup_in_package(MalValue *package, const MalValue *symbol);

MalValue *lookup_in_environment(MalEnvironment *environment, MalValue *package, const MalValue *symbol)
{
    MalEnvironment *env = environment;
    MalValue *value = NULL;
    char *symbol_name = get_symbol_name(symbol);

    while (env != NULL && value == NULL)
    {
        value = hashmap_get(env->map, symbol_name);
        env = env->parent;
    }

    if (!value && package)
    {
        assert(is_package(package));
        value = lookup_in_package(package, symbol);
    }

    return value;
}

/**
 * Looks up the given symbol in the specified environment or one of the parent environments.
 * The value is then updated in the environment it could be found in. If the symbol did not
 * exist in one of these environments it is put into the one specified in the arguments.
 *
 * @param environment the top level environment the symbol shall be put into
 * @param symbol the symbol that should be looked up in the given environment or one of its parent environments
 * @param value the value that shall be put/update into/in the given environment or one of its parent environments
 *
 * @return {code}false{code} if the symbol did not already exist in the given environment or one of its parent environments, {code}true{code} otherwise.
 */
bool set_in_environment(MalEnvironment *environment, const MalValue *symbol, MalValue *value)
{
    MalEnvironment *env = find_environment(environment, symbol);

    if (!env)
    {
        env = environment;
    }

    char *symbol_name = get_symbol_name(symbol);
    MalValue *oldValue = hashmap_get(env->map, symbol_name);

    hashmap_put(env->map, symbol->valueType, symbol_name, value);

    return oldValue != NULL;
}

MalEnvironment *make_environment(MalEnvironment *parent, MalCell *binds, MalCell *exprs, MalValue *rest_symbol)
{
    MalEnvironment *environment = mal_malloc(sizeof(MalEnvironment));

    if (environment == NULL)
    {
        return NULL;
    }

    HashMap *map = new_hashmap();

    if (map == NULL)
    {
        free_environment(environment);

        return NULL;
    }

    environment->map = map;
    environment->parent = parent;

    MalCell *_binds = binds;
    MalCell *_exprs = exprs;

    while (_binds && _exprs && _binds->value && _exprs && _exprs->value)
    {
        assert(_binds->value->valueType == MAL_SYMBOL);
        set_in_environment(environment, _binds->value, _exprs->value);
        _binds = _binds->cdr;
        _exprs = _exprs->cdr;
    }

    if (rest_symbol)
    {
        set_in_environment(environment, rest_symbol, make_list(_exprs));
    }

    return environment;
}

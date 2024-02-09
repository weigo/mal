#include <assert.h>
#include <stddef.h>

#include "core.h"
#include "gc.h"
#include "package.h"
#include "types.h"
#include "env.h"

static HashMap *packages;
MalEnvironment *global_environment;
static const char *PACKAGE_VARIABLE = "*package*";

MalValue *find_package(MalValue *symbol)
{
    return hashmap_get(packages, symbol->value);
}

MalValue *get_current_package()
{
    return lookup_in_environment(global_environment, NULL, make_symbol(PACKAGE_VARIABLE));
}

void set_current_package(MalValue *package)
{
    set_in_environment(global_environment, make_symbol(PACKAGE_VARIABLE), package);
}

MalValue *make_package(MalValue *name, MalEnvironment *environment, MalCell *used_packages)
{
    assert(name != NULL);
    assert(environment != NULL);
    assert(is_symbol(name));
    MalValue *result = new_value(MAL_PACKAGE);
    MalPackage *package = mal_calloc(1, sizeof(MalPackage));

    package->name = name;
    package->environment = environment;
    package->exported_symbols = new_hashmap();
    package->used_packages = new_hashmap();

    MalCell *used_package = used_packages;

    while (used_package)
    {
        HashMap *package2symbols = new_hashmap();

        // FIXME: Verify: should be a symbol (or maybe a string) or else return an error
        MalValue *p = find_package(used_package->value);

        if (!p)
        {
            return make_error("package '%s' not found!", used_package->value->value);
        }

        hashmap_putall(p->package->exported_symbols, package2symbols);
        hashmap_put(package->used_packages, used_package->value->valueType, used_package->value->value, used_package->value);

        used_package = used_package->cdr;
    }

    result->package = package;

    return result;
}

/**
 * Create an empty package with the given name.
 *
 * The new package will be registered in the global package list.
 *
 * @return the newly created package
 */
MalValue *new_package(char *package_name, MalEnvironment *parent_environment, MalCell *used_packages)
{
    MalValue *symbol = make_symbol(package_name);
    MalEnvironment *environment = make_environment(parent_environment, NULL, NULL, NULL);
    MalValue *package = make_package(symbol, environment, used_packages);
    hashmap_put(packages, symbol->valueType, symbol->value, package);

    return package;
}

MalValue *make_system_package(void (*rep)(const char *input, MalEnvironment *environment, bool print_result))
{
    // initialize system package which contains the functionality implemented in C
    MalValue *package_system = new_package("system", global_environment, NULL);
    HashMap *ns = core_namespace();
    HashMapIterator it = hashmap_iterator(ns);
    MalEnvironment *environment = package_system->package->environment;

    while (hashmap_next(&it))
    {
        set_in_environment(environment, make_symbol(it.key), it.value);
    }

    set_current_package(package_system);

    // FIXME: Make path to 'system.lisp' configurable, e.g. via environment
    char *system_lisp = read_file("system.lisp");
    rep(system_lisp, environment, false);

    // export symbol names from environment
    it = hashmap_iterator(environment->map);

    while (hashmap_next(&it))
    {
        hashmap_put(package_system->package->exported_symbols, it.keyType, it.key, (void *)it.key);
    }

    return package_system;
}

/**
 *
 */
void init_packages(void (*rep)(const char *input, MalEnvironment *environment, bool print_result))
{
    // initialize global environment
    global_environment = make_environment(NULL, NULL, NULL, NULL);
    set_in_environment(global_environment, &MAL_NIL, &MAL_NIL);
    set_in_environment(global_environment, &MAL_TRUE, &MAL_TRUE);
    set_in_environment(global_environment, &MAL_FALSE, &MAL_FALSE);

    packages = new_hashmap();

    MalValue *system_package = make_system_package(rep);
    MalValue *used_packages = make_list(NULL);
    push(used_packages, system_package->package->name);

    set_current_package(new_package("mal-user", global_environment, used_packages->list));
}

MalValue *lookup_in_package(MalValue *package, MalValue *symbol)
{
    assert(is_package(package));
    assert(is_symbol(symbol));

    MalPackage *p = package->package;
    MalValue *result = NULL;
    MalValue *package_name = NULL;
    result = lookup_in_environment(p->environment, NULL, symbol);

    if (!result)
    {
        HashMapIterator used_packages = hashmap_iterator(p->used_packages);
        MalValue *current = NULL;

        while (!result && hashmap_next(&used_packages))
        {
            package_name = (MalValue *)used_packages.value;
            current = find_package(package_name);

            if (!current)
            {
                return make_error("cannot find used package '%s'", package_name->value);
            }

            result = lookup_in_environment(current->package->environment, current, symbol);
        }
    }

    return result;
}
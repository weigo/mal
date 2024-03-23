#include <assert.h>
#include <stddef.h>

#include "core.h"
#include "gc.h"
#include "package.h"
#include "types.h"
#include "env.h"

static HashMap *packages;
MalEnvironment *global_environment;
static const MalSymbol PACKAGE_SYMBOL = {
    .name = "*package*",
    .package = &MAL_NIL};

static const MalValue PACKAGE_VARIABLE = {
    .valueType = MAL_SYMBOL,
    .symbol = &PACKAGE_SYMBOL};

MalValue *find_package(MalValue *symbol)
{
    if (is_string(symbol))
    {
        return hashmap_get(packages, symbol->value);
    }

    return hashmap_get(packages, get_symbol_name(symbol));
}

MalValue *get_current_package()
{
    return lookup_in_environment(global_environment, NULL, &PACKAGE_VARIABLE);
}

void set_current_package(MalValue *package)
{
    set_in_environment(global_environment, &PACKAGE_VARIABLE, package);
}

MalValue *use_package(MalValue *package, MalValue *package_to_use)
{
    MalValue *p = find_package(package_to_use);

    if (!p)
    {
        return make_error("package '%s' not found!", get_symbol_name(package_to_use));
    }

    /*
     * use-package checks for name conflicts between the newly imported symbols and those already
     * accessible in package. A name conflict in use-package between two external symbols inherited
     * by package from packages-to-use may be resolved in favor of either symbol by importing one of
     * them into package and making it a shadowing symbol.
     */
    hashmap_putall(p->package->exported_symbols, package->package->inherited_symbols);
    hashmap_put(package->package->used_packages, p->valueType, p->package->name, p->package->name);
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
    package->internal_symbols = new_hashmap();
    package->inherited_symbols = new_hashmap();
    package->exported_symbols = new_hashmap();
    package->used_packages = new_hashmap();

    MalCell *used_package = used_packages;
    MalValue *tmp;

    while (used_package)
    {
        tmp = use_package(package, used_package->value);

        if (is_error(tmp))
        {
            return tmp;
        }

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
    MalValue *package = find_package(symbol);

    if (package)
    {
        return make_error("A package with name '%s' already exists!", package_name);
    }

    MalEnvironment *environment = make_environment(parent_environment, NULL, NULL, NULL);
    package = make_package(symbol, environment, used_packages);
    hashmap_put(packages, symbol->valueType, symbol->symbol->name, package);

    return package;
}

void intern_symbol(MalValue *package, MalValue *symbol) {
    assert(is_package(package));
    assert(is_string(symbol));
    char *symbol_name = symbol->value;


    if () {

    }

    hashmap_put(package->package->internal_symbols, symbol->valueType, symbol_name, (void *)symbol_name);
}

void export_symbol(MalValue *package, MalValue *symbol)
{
    assert(is_package(package));
    assert(is_symbol(symbol));
    char *symbol_name = get_symbol_name(symbol);
    hashmap_put(package->package->exported_symbols, symbol->valueType, symbol_name, (void *)symbol_name);
}

MalValue *symbol_package(MalValue *symbol)
{
    return symbol->symbol->package;
}

MalValue *intern_symbol(MalValue *package, MalValue *symbol)
{
    assert(is_package(package));
    assert(is_symbol(symbol));

    symbol->symbol->package = package;
}

MalValue *make_system_package(void (*rep)(const char *input, MalEnvironment *environment, bool print_result))
{
    // initialize system package which contains the functionality implemented in C
    MalValue *package_system = new_package("system", global_environment, NULL);
    HashMap *ns = core_namespace();
    HashMapIterator it = hashmap_iterator(ns);
    MalEnvironment *environment = package_system->package->environment;
    MalValue *symbol;

    while (hashmap_next(&it))
    {
        symbol = make_symbol(it.key);
        symbol->symbol->package = package_system;
        set_in_environment(environment, symbol, it.value);
        export_symbol(package_system, symbol);
    }

    set_current_package(package_system);

    // FIXME: Make path to 'system.lisp' configurable, e.g. via environment
    char *system_lisp = read_file("system.lisp");
    rep(system_lisp, environment, true);

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
    set_in_environment(global_environment, &PACKAGE_VARIABLE, NULL);

    packages = new_hashmap();

    MalValue *system_package = make_system_package(rep);
    MalValue *used_packages = make_list(NULL);
    push(used_packages, system_package->package->name);

    set_current_package(new_package("mal-user", make_environment(global_environment, NULL, NULL, NULL), used_packages->list));
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
        result = hashmap_get(p->inherited_symbols, get_symbol_name(symbol));
    }

    return result;
}
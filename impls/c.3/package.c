#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "gc.h"
#include "package.h"
#include "types.h"
#include "env.h"

static HashMap *packages;
MalEnvironment *global_environment;
static const MalSymbol PACKAGE_SYMBOL = {
        .name = "*package*",
        .package = &MAL_NIL
};

static const MalValue PACKAGE_VARIABLE = {
        .valueType = MAL_SYMBOL,
        .symbol = &PACKAGE_SYMBOL
};

static const MalValue KEYWORD_PACKAGE_NAME = {
        .valueType = MAL_STRING,
        .value = "keyword"
};

MalValue *find_package(const MalValue *symbol) {
    if (is_string(symbol)) {
        return hashmap_get(packages, symbol->value);
    }

    return hashmap_get(packages, get_symbol_name(symbol));
}

MalValue *get_current_package() {
    return lookup_in_environment(global_environment, NULL, &PACKAGE_VARIABLE);
}

void set_current_package(MalValue *package) {
    set_in_environment(global_environment, &PACKAGE_VARIABLE, package);
}

MalValue *use_package(MalValue *package, MalValue *package_to_use) {
    MalValue *p = find_package(package_to_use);

    if (!p) {
        return make_error("package '%s' not found!", get_symbol_name(package_to_use));
    }

    /*
     * use-package checks for name conflicts between the newly imported symbols and those already
     * accessible in package. A name conflict in use-package between two external symbols inherited
     * by package from packages-to-use may be resolved in favor of either symbol by importing one of
     * them into package and making it a shadowing symbol.
     */
    hashmap_putall(p->package->exported_symbols, package->package->inherited_symbols);
    hashmap_put(package->package->used_packages, p->valueType, p->package->name->value,
                (void *) p->package->name->value);

    return p;
}

MalValue *make_package(MalValue *name, MalEnvironment *environment, MalCell *used_packages) {
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
    result->package = package;
    MalCell *used_package = used_packages;
    MalValue *tmp;

    while (used_package) {
        tmp = use_package(result, used_package->value);

        if (is_error(tmp)) {
            return tmp;
        }

        used_package = used_package->cdr;
    }

    return result;
}

/**
 * Create an empty package with the given name.
 *
 * The new package will be registered in the global package list.
 *
 * @return the newly created package
 */
MalValue *new_package(char *package_name, MalEnvironment *parent_environment, MalCell *used_packages) {
    MalValue *symbol = make_symbol(package_name);
    MalValue *package = find_package(symbol);

    if (package) {
        return make_error("A package with name '%s' already exists!", package_name);
    }

    MalEnvironment *environment = make_environment(parent_environment, NULL, NULL, NULL);
    package = make_package(symbol, environment, used_packages);
    hashmap_put(packages, symbol->valueType, symbol->symbol->name, package);

    return package;
}

MalValue *intern_symbol(MalValue *package, MalValue *symbol) {
    assert(is_package(package));
    assert(is_string(symbol));
    const char *symbol_name = symbol->value;

    MalValue *actual_symbol = hashmap_get(package->package->internal_symbols, symbol_name);
    MalValue *result = new_value(MAL_MULTI_VALUE);
    MalValue *state = &MAL_NIL;
    MalValue *keyword_package = find_package(&KEYWORD_PACKAGE_NAME);
    // given symbol_name is already registered as an internal symbol, so return this one
    if (actual_symbol) {
        state = find_symbol(":internal", keyword_package);
    }

    //    actual_symbol->package = package;

    hashmap_put(package->package->internal_symbols, symbol->valueType, symbol_name, (void *) actual_symbol);
    push(result, actual_symbol);
    push(result, state);

    return result;
}

void export_symbol(MalValue *package, MalValue *symbol) {
    assert(is_package(package));
    assert(is_symbol(symbol) || is_keyword(symbol));
    char *symbol_name = get_symbol_name(symbol);
    hashmap_delete(package->package->exported_symbols, symbol_name);
    hashmap_put(package->package->exported_symbols, symbol->valueType, symbol_name, (void *) symbol);
}

MalValue *symbol_package(MalValue *symbol) {
    return symbol->symbol->package;
}

MalValue *find_symbol(const char *symbol_name, MalValue *package) {
    MalValue *symbol = hashmap_get(package->package->internal_symbols, symbol_name);
    MalValue *state = &MAL_NIL;
    MalValue *keyword_package = find_package(&KEYWORD_PACKAGE_NAME);

    if (symbol) {
        state = find_symbol(":internal", keyword_package);
    } else {
        symbol = hashmap_get(package->package->exported_symbols, symbol_name);
    }

    if (symbol) {
        state = find_symbol(":external", keyword_package);
    } else {
        symbol = hashmap_get(package->package->exported_symbols, symbol_name);
    }

    if (!symbol) {
        symbol = &MAL_NIL;
    }

    MalValue *values = new_value(MAL_MULTI_VALUE);

    push(values, symbol);
    push(values, state);

    return values;
}

MalValue *make_keyword(const char *keyword_name) {
    MalValue *keyword_package = find_package(&KEYWORD_PACKAGE_NAME);
    MalValue *symbol = hashmap_get(keyword_package->package->exported_symbols, keyword_name);
    char *template = ":%s";

    if (!symbol) {
        size_t len = strlen(keyword_name) + 2;

        if (keyword_name[0] == ':') {
            len--;
            template = "%s";
        }

        char *keyword = mal_calloc(len, sizeof(char));
        snprintf(keyword, len, template, keyword_name);

        symbol = new_value(MAL_KEYWORD);
        symbol->value = keyword;
        hashmap_put(keyword_package->package->exported_symbols, MAL_KEYWORD, keyword, symbol);
    }

    return symbol;
}

MalValue *make_system_package(void (*rep)(const char *input, MalEnvironment *environment, bool print_result)) {
    // initialize system package which contains the functionality implemented in C
    MalValue *package_system = new_package("system", global_environment, NULL);
    HashMap *ns = core_namespace();
    HashMapIterator it = hashmap_iterator(ns);
    MalEnvironment *environment = package_system->package->environment;
    MalValue *symbol;

    while (hashmap_next(&it)) {
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

    while (hashmap_next(&it)) {
        hashmap_put(package_system->package->exported_symbols, it.keyType, it.key, (void *) it.key);
    }

    return package_system;
}

MalValue *make_keyword_package() {
    MalValue *keyword_package = new_package("keyword", global_environment, NULL);

    make_keyword("internal");
    make_keyword("external");
    make_keyword("inherited");

    return keyword_package;
}

/**
 *
 */
void init_packages(void (*rep)(const char *input, MalEnvironment *environment, bool print_result)) {
    // initialize global environment
    global_environment = make_environment(NULL, NULL, NULL, NULL);
    set_in_environment(global_environment, &MAL_NIL, &MAL_NIL);
    set_in_environment(global_environment, &MAL_TRUE, &MAL_TRUE);
    set_in_environment(global_environment, &MAL_FALSE, &MAL_FALSE);
    set_in_environment(global_environment, &PACKAGE_VARIABLE, NULL);

    packages = new_hashmap();
    make_keyword_package();

    MalValue *system_package = make_system_package(rep);
    MalValue *used_packages = make_list(NULL);
    push(used_packages, system_package->package->name);

    set_current_package(
            new_package("mal-user", make_environment(global_environment, NULL, NULL, NULL), used_packages->list));
}

MalValue *lookup_in_package(MalValue *package, MalValue *symbol) {
    assert(is_package(package));
    assert(is_symbol(symbol));

    MalPackage *p = package->package;
    MalValue *result = lookup_in_environment(p->environment, NULL, symbol);

    if (!result) {
        result = hashmap_get(p->inherited_symbols, get_symbol_name(symbol));
    }

    return result;
}
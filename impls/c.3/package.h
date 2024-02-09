#include "types.h"
#include "env.h"

/**
 * Get the current package.
 */
MalValue *get_current_package();

/**
 * Initialize the mal package system using the given environment.
 */
// void rep(const char *input, MalEnvironment *environment, bool print_result)
void init_packages(void (*rep)(const char *input, MalEnvironment *environment, bool print_result));

/**
 * Find the MalPackage associated with the given symbol.
 *
 * @return {code}MalValue{code} when a package with the given name has been registered, {code}NULL{code} otherwise.
 */
MalValue *find_package(MalValue *symbol);

/**
 * Look the given symbol up in the provided package.
 *
 * @return the value found in the given package or {code}NULL{code}.
 */
MalValue *lookup_in_package(MalValue *package, MalValue *symbol);
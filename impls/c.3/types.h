#ifndef _MAL_TYPES_H
#define _MAL_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "error.h"

enum MalValueType
{
    MAL_ATOM,
    MAL_CLOSURE,
    MAL_COMMENT,
    MAL_ERROR,
    MAL_HASHMAP,
    MAL_FIXNUM,
    MAL_FUNCTION,
    MAL_KEYWORD,
    MAL_LIST,
    MAL_NUMBER,
    MAL_PACKAGE,
    MAL_STRING,
    MAL_SYMBOL,
    MAL_VECTOR,
    MAL_TYPE_FALSE,
    MAL_TYPE_NIL,
    MAL_TYPE_TRUE,
};

typedef struct MapEntry
{
    const char *key;
    enum MalValueType keyType;
    void *value;
} MapEntry;

typedef struct HashMap
{
    MapEntry *entries;
    size_t capacity;
    size_t length;
} HashMap;

// Hash table iterator: create with ht_iterator, iterate with ht_next.
typedef struct HashMapIterator
{
    const char *key; // current key
    enum MalValueType keyType;
    void *value; // current value

    // Don't use these fields directly.
    HashMap *_map; // reference to hash table being iterated
    size_t _index; // current index into ht._entries
} HashMapIterator;

HashMap *new_hashmap();
void free_hashmap(HashMap *);
const char *hashmap_put(HashMap *hashMap, enum MalValueType keyType, const char *key, void *value);
void hashmap_putall(HashMap *source, HashMap *target);
void *hashmap_get(const HashMap *hashMap, const char *key);

// Return new hash table iterator (for use with ht_next).
HashMapIterator hashmap_iterator(HashMap *table);

// Move iterator to next item in hash table, update iterator's key
// and value to current item, and return true. If there are no more
// items, return false. Don't call ht_set during iteration.
bool hashmap_next(HashMapIterator *it);

typedef struct MalValue MalValue;
typedef struct MalCell MalCell;

typedef struct MalCell
{
    MalValue *value;
    MalCell *cdr;
} MalCell;

typedef struct MalEnvironment MalEnvironment;
typedef struct MalEnvironment
{
    MalEnvironment *parent;
    HashMap *map;
} MalEnvironment;

typedef struct MalClosure
{
    MalEnvironment *environment;
    MalValue *bindings;
    MalValue *ast;
    MalValue *rest_symbol;
    bool is_macro;
} MalClosure;

/**
 * A MalPackage is {code}name{code}d structure that holds
 * <ul>
 * <li>an {@link}MalEnvironment{@link}
 * <li>a list of used packages (as a {@link}HashMap{link} of symbols) and
 * <li>a list of exported symbols (also as a {@link}HashMap{link} of symbols)
 * </ul>
 */
typedef struct MalPackage
{
    MalValue *name;
    MalEnvironment *environment;
    HashMap *used_packages;
    HashMap *exported_symbols;
} MalPackage;

typedef struct MalValue
{
    enum MalValueType valueType;
    MalValue *metadata;
    union
    {
        const char *value;
        int64_t fixnum;
        MalCell *list;
        MalValue *malValue;
        HashMap *hashMap;
        MalValue *(*function)(MalCell *);
        MalClosure *closure;
        MalPackage *package;
    };
} MalValue;

extern MalValue MAL_NIL;
extern MalValue MAL_FALSE;
extern MalValue MAL_TRUE;
extern MalValue MAL_EOF;

bool is_atom(MalValue *value);
bool is_closure(MalValue *value);
bool is_error(MalValue *value);
bool is_executable(MalValue *value);
bool is_false(MalValue *value);
bool is_fixnum(MalValue *value);
bool is_function(MalValue *value);
bool is_hashmap(MalValue *value);
bool is_keyword(MalValue *value);
bool is_list(MalValue *value);
bool is_macro(MalValue *value);
bool is_named_symbol(MalValue *value, const char *symbol_name);
bool is_nil(MalValue *value);
bool is_number_type(MalValue *value);
bool is_number(MalValue *value);
bool is_self_evaluating(MalValue *value);
bool is_sequence(MalValue *value);
bool is_string_type(MalValue *value);
bool is_string(MalValue *value);
bool is_symbol(MalValue *value);
bool is_true(MalValue *value);
bool is_vector(MalValue *value);
bool is_package(MalValue *value);

MalValue *new_value(enum MalValueType valueType);
MalValue *new_function(MalValue *(*function)(MalCell *args));
MalValue *make_error(char *fmt, ...);
MalValue *wrap_error(MalValue *value);
MalValue *make_symbol(const char *symbol_name);
MalValue *make_value(enum MalValueType valueType, const char *value);
MalValue *make_closure(MalEnvironment *outer, MalCell *context);

/**
 * Create a new MalPackage using {code}name{code}d structure that holds
 * <ul>
 * <li>an {@link}MalEnvironment{@link}
 * <li>a list of used packages (as a {@link}HashMap{link} of symbols) and
 * <li>a list of exported symbols (also as a {@link}HashMap{link} of symbols)
 * </ul>
 */
MalValue *make_package(MalValue *name, MalEnvironment *environment, MalCell *used_packages);
MalValue *mal_clone(MalValue *value);

/**
 * Create a new string value.
 *
 * @param value the string to put into the returned MalValue object.
 * @param unescape indicates whether '\\' should be replaced with a single '\' or the given string shall be placed unchanged in the return value.
 */
MalValue *make_string(const char *value, bool unescape);
MalValue *make_fixnum(int64_t value);

// Sequence related functions.

/**
 * Make a new list using the given values.
 *
 * @param values values to append initially maybe NULL
 * @return the newly created list.
 */
MalValue *make_list(MalCell *values);

/**
 * Make a new vector using the given values.
 *
 * @param values values to append initially maybe NULL
 * @return the newly created vector.
 */
MalValue *make_vector(MalCell *values);

/**
 * Push the given value onto the tail of the given sequence.
 *
 * @param value value to append to the list
 */
void push(MalValue *list, MalValue *value);

/**
 * Append all given values to the given sequence.
 *
 * @param list sequence to append values to.
 * @param values values to append to given sequence.
 */
void push_all(MalValue *list, MalCell *values);

/**
 * Insert the given value at the head of the given sequence.
 *
 * @param list sequence to prepend value to.
 * @param value value to prepend.
 */
void prepend(MalValue *list, MalValue *value);

/**
 * Reverse the given sequence and return a newly created sequence of the same type.
 *
 * @param list sequence to be reversed.
 * @return newly created sequence of same type with its values reversed.
 */
MalValue *reverse(MalValue *list);

// HashMap related functions
MalValue *make_hashmap();
const char *put(MalValue *map, MalValue *key, MalValue *value);
void hashmap_delete(HashMap *map, const char *key);
#endif
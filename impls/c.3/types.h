#ifndef _MAL_TYPES_H
#define _MAL_TYPES_H

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
    MAL_KEYWORD,
    MAL_LIST,
    MAL_NUMBER,
    MAL_STRING,
    MAL_SYMBOL,
    MAL_VECTOR,
    MAL_FUNCTION
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

HashMap *make_hashmap();
void free_hashmap(HashMap *);
const char *hashmap_put(HashMap *hashMap, enum MalValueType keyType, const char *key, void *value);
void *hashmap_get(HashMap *hashMap, const char *key);

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
} MalClosure;

typedef struct MalValue
{
    enum MalValueType valueType;
    HashMap *metadata;
    union
    {
        const char *value;
        int64_t fixnum;
        MalCell *list;
        MalValue *malValue;
        HashMap *hashMap;
        MalValue *(*function)(MalCell *);
        MalClosure *closure;
    };
} MalValue;

extern MalValue MAL_NIL;
extern MalValue MAL_FALSE;
extern MalValue MAL_TRUE;
extern MalValue MAL_EOF;

MalValue *new_value(enum MalValueType valueType);
MalValue *new_function(MalValue *(*function)(MalCell *args));
MalValue *make_error(char *fmt, ...);
MalValue *make_value(enum MalValueType valueType, const char *value);
MalValue *make_closure(MalEnvironment *outer, MalCell *context);

/**
 * Create a new string value.
 *
 * @param value the string to put into the returned MalValue object.
 * @param unescape indicates whether '\\' should be replaced with a single '\' or the given string shall be placed unchanged in the return value.
 */
MalValue *make_string(char *value, bool unescape);
MalValue *make_fixnum(int64_t value);
MalValue *make_list(MalCell *values);

void push(MalValue *list, MalValue *value);
void push_all(MalValue *list, MalCell *values);
const char *put(MalValue *map, MalValue *key, MalValue *value);
void setMetadata(MalValue *value, HashMap *metadata);
#endif
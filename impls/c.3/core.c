#include <assert.h>
#include <string.h>
#include "core.h"
#include "env.h"
#include "printer.h"
#include "reader.h"
#include "symbol.h"
#include "types.h"
#include "gc.h"
#include "libs/readline/readline.h"
#include <sys/time.h>
#include <stdio.h>

extern FILE *output_stream;

bool is_equal_by_type(MalValue *left, MalValue *right);
bool is_equal(MalValue *left, MalValue *right);
bool are_lists_equal(MalCell *left, MalCell *right);

MalEnvironment *global_environment;

MalValue *add(MalCell *values)
{
    MalCell *current = values;
    int64_t result = 0;

    if (current)
    {
        MalCell *cdr = current->cdr;

        if (current->value)
        {
            result = current->value->fixnum;
        }

        while (cdr != NULL && cdr->value != NULL)
        {
            result += cdr->value->fixnum;
            cdr = cdr->cdr;
        }
    }

    return make_fixnum(result);
}

MalValue *subtract(MalCell *values)
{
    MalCell *current = values;

    if (!current)
    {
        return NULL;
    }

    int64_t result = 0;
    MalCell *cdr = current->cdr;

    if (current->value)
    {
        result = current->value->fixnum;
    }

    while (cdr != NULL && cdr->value != NULL)
    {
        result -= cdr->value->fixnum;
        cdr = cdr->cdr;
    }

    return make_fixnum(result);
}

MalValue *multiply(MalCell *values)
{
    MalCell *current = values;
    int64_t result = 1;

    while (current != NULL && current->value != NULL)
    {
        result *= current->value->fixnum;
        current = current->cdr;
    }

    return make_fixnum(result);
}

MalValue *divide(MalCell *values)
{
    MalCell *current = values;
    int64_t result = 1;

    while (current != NULL && current->value != NULL)
    {
        result /= current->value->fixnum;
        current = current->cdr;
    }

    return make_fixnum(result);
}

MalValue *prn(MalCell *values)
{

    print(output_stream, print_values_readably(values), false);
    fprintf(output_stream, "\n");

    return &MAL_NIL;
}

// list functions
/**
 * Create a list from the given values.
 *
 * @param values {code}NULL{code} or list of {code}MalCell{code}s to assign to the new list.
 * @return a new {code}MalValue*{code} of type {code}MAL_LIST{code}
 */
MalValue *list(MalCell *values)
{
    MalValue *result = new_value(MAL_LIST);

    if (values)
    {
        result->list = values;
    }

    return result;
}

MalValue *list_p(MalCell *value)
{
    return (value->value->valueType == MAL_LIST) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *empty_p(MalCell *value)
{
    if (is_sequence(value->value))
    {
        return value->value->list ? &MAL_FALSE : &MAL_TRUE;
    }

    return make_error("'empty': illegal argument, exepcted list or vector");
}

int64_t length(MalValue *list)
{
    assert(is_sequence(list));
    int64_t length = 0;
    MalCell *current = list->list;

    while (current)
    {
        length++;
        current = current->cdr;
    }

    return length;
}

int64_t _count(MalCell *value)
{
    int64_t count = 0;
    MalCell *current = value;

    if (!current || is_nil(current->value))
    {
        return count;
    }

    while (current && current->value)
    {
        count++;
        current = current->cdr;
    }

    return count;
}

/**
 * count: treat the first parameter as a list and return the number of elements that it contains.
 */
MalValue *count(MalCell *value)
{
    return make_fixnum(_count(value));
}

MalValue *nth(MalCell *values)
{
    MalValue *list = values->value;

    if (!is_sequence(list))
    {
        return make_error("'nth': first argument is not a list!");
    }

    if (!values->cdr)
    {
        return make_error("'nth': expected second argument as index into the list!");
    }

    MalValue *index = values->cdr->value;

    if (!is_fixnum(index) || index->fixnum < 0)
    {
        return make_error("'nth': list index must be a fixnum > 0!");
    }

    int64_t idx = 0;
    MalCell *current = list->list;

    while (idx < index->fixnum && current)
    {
        current = current->cdr;
        idx++;
    }

    if (idx <= index->fixnum && !current)
    {
        return make_error("nth: index '%d' out of bounds for '%s'.", index->fixnum, pr_str(list, true));
    }

    return current->value;
}

MalValue *first(MalCell *values)
{
    MalValue *list = values->value;

    if (!list || is_named_symbol(list, SYMBOL_NIL))
    {
        return &MAL_NIL;
    }

    if (empty_p(values) == &MAL_TRUE)
    {
        return &MAL_NIL;
    }

    return list->list->value;
}

MalValue *rest(MalCell *values)
{
    MalValue *list = values->value;

    if (!list || is_named_symbol(list, SYMBOL_NIL))
    {
        return make_list(NULL);
    }

    if (empty_p(values) == &MAL_TRUE)
    {
        return make_list(NULL);
    }

    return make_list(list->list->cdr);
}

MalValue *greater_than(MalCell *values)
{
    if (_count(values) != 2)
    {
        return make_error("Invalid count of arguments. Two arguments expected: '%s'!", print_values_readably(values)->value);
    }

    MalValue *first = values->value;
    MalValue *second = values->cdr->value;

    if (MAL_FIXNUM != first->valueType || MAL_FIXNUM != second->valueType)
    {
        return make_error("Invalid argument. Can only compare fixnums: '%s'!", print_values_readably(values)->value);
    }

    return first->fixnum > second->fixnum ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *less_than(MalCell *values)
{
    if (_count(values) != 2)
    {
        return make_error("Invalid count of arguments. Two arguments expected: '%s'!", print_values_readably(values)->value);
    }

    MalValue *first = values->value;
    MalValue *second = values->cdr->value;

    if (MAL_FIXNUM != first->valueType || MAL_FIXNUM != second->valueType)
    {
        return make_error("Invalid argument. Can only compare fixnums: '%s'!", print_values_readably(values)->value);
    }

    return first->fixnum < second->fixnum ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *less_than_or_equal_to(MalCell *values)
{
    if (_count(values) != 2)
    {
        return make_error("Invalid count of arguments. Two arguments expected: '%s'!", print_values_readably(values)->value);
    }

    MalValue *first = values->value;
    MalValue *second = values->cdr->value;

    if (MAL_FIXNUM != first->valueType || MAL_FIXNUM != second->valueType)
    {
        return make_error("Invalid argument. Can only compare fixnums: '%s'!", print_values_readably(values)->value);
    }

    return first->fixnum <= second->fixnum ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *greater_than_or_equal_to(MalCell *values)
{
    if (_count(values) != 2)
    {
        return make_error("Invalid count of arguments. Two arguments expected: '%s'!", print_values_readably(values)->value);
    }

    MalValue *first = values->value;
    MalValue *second = values->cdr->value;

    if (MAL_FIXNUM != first->valueType || MAL_FIXNUM != second->valueType)
    {
        return make_error("Invalid argument. Can only compare fixnums: '%s'!", print_values_readably(values)->value);
    }

    return first->fixnum >= second->fixnum ? &MAL_TRUE : &MAL_FALSE;
}

bool are_maps_equal(HashMap *left, HashMap *right)
{
    if (left->entries == right->entries)
    {
        return true;
    }

    HashMapIterator it = hashmap_iterator(left);
    MalValue *entry;

    while (hashmap_next(&it))
    {
        entry = hashmap_get(right, it.key);

        if (!entry || !is_equal(it.value, entry))
        {
            return false;
        }
    }

    return true;
}

bool is_equal_by_type(MalValue *left, MalValue *right)
{
    //    assert(left->valueType == right->valueType);

    bool result = false;

    switch (left->valueType)
    {
    case MAL_FIXNUM:
        result = left->fixnum == right->fixnum;
        break;

    case MAL_STRING:
    case MAL_KEYWORD:
    case MAL_SYMBOL:
        result = strcmp(left->value, right->value) == 0;
        break;

    case MAL_LIST:
    case MAL_VECTOR:
        result = are_lists_equal(left->list, right->list);
        break;

    case MAL_HASHMAP:
        result = are_maps_equal(left->hashMap, right->hashMap);
        break;

    default:
        break;
    }

    return result;
}

bool is_comparable(MalValue *left, MalValue *right)
{
    if (is_string_type(left) && is_string_type(right))
    {
        return true;
    }

    if (is_number_type(left) && is_number_type(right))
    {
        return true;
    }

    return left->valueType == right->valueType;
}

bool is_equal(MalValue *left, MalValue *right)
{
    if (is_comparable(left, right))
    {
        return is_equal_by_type(left, right);
    }

    if (((left->valueType == MAL_LIST && right->valueType == MAL_VECTOR) || (left->valueType == MAL_VECTOR && right->valueType == MAL_LIST)))
    {
        return are_lists_equal(left->list, right->list);
    }

    return false;
}

bool are_lists_equal(MalCell *left, MalCell *right)
{
    if ((left && !right) || (!left && right))
    {
        return false;
    }

    bool result = true;

    while (left)
    {
        if (!right || !is_equal(left->value, right->value))
        {
            result = false;
            break;
        }

        left = left->cdr;
        right = right->cdr;
    }

    return result;
}

/**
 * =: compare the first two parameters and return true if they are the same type and contain the same value.
 *  In the case of equal length lists, each element of the list should be compared for equality and if they
 * are the same return true, otherwise false.
 */
MalValue *equals(MalCell *values)
{
    if (!values || !values->value || !values->cdr || !values->cdr->value)
    {
        return make_error("Invalid count of arguments. At least two arguments expected: '%s'!", print_values_readably(values)->value);
    }

    MalCell *left = values;
    MalCell *right = values->cdr;

    if (left->value == right->value)
    {
        return &MAL_TRUE;
    }

    if (is_equal(left->value, right->value))
    {
        return &MAL_TRUE;
    }

    if ((empty_p(left) == &MAL_TRUE && &MAL_NIL == right->value) || (empty_p(right) == &MAL_TRUE && &MAL_NIL == left->value))
    {
        return &MAL_FALSE;
    }

    return &MAL_FALSE;
}

MalValue *read_string(MalCell *values)
{
    if (!values || !is_string(values->value))
    {
        return make_error("Invalid argument!");
    }

    Reader reader = {.input = values->value->value};
    Token token = {};
    reader.token = &token;

    return read_str(&reader);
}

char *read_file(const char *file_name)
{
    FILE *stream = fopen(file_name, "r");
    char *buffer = (char *)NULL;
    int len = 0;
    size_t bytes_read;

    if (stream)
    {
        fseek(stream, 0, SEEK_END);
        len = ftell(stream);
        fseek(stream, 0, SEEK_SET);

        buffer = mal_calloc(len + 1, sizeof(char));
        bytes_read = fread(buffer, sizeof(char), len, stream);
        fclose(stream);

        if (bytes_read != len)
        {
            free(buffer);
            buffer = NULL;
        }
    }

    return buffer;
}

MalValue *slurp(MalCell *values)
{
    if (!values || values->value->valueType != MAL_STRING)
    {
        return make_error("Invalid argument!");
    }

    char *contents = read_file(values->value->value);

    if (!contents)
    {
        return make_error("Could not read '%s'", values->value->value);
    }

    return make_string(contents, true);
}

extern MalValue *EVAL(MalValue *value, MalEnvironment *environment);

MalValue *macro_p(MalCell *values)
{
    if (!values || !values->value)
    {
        return make_error("Illegal number of arguments!");
    }

    return is_macro(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *eval(MalCell *values)
{
    return EVAL(values->value, global_environment);
}

MalValue *atom(MalCell *values)
{
    if (!values || !values->value)
    {
        return make_error("Illegal number of arguments!");
    }

    MalValue *atom = new_value(MAL_ATOM);
    atom->malValue = values->value;

    return atom;
}

MalValue *atom_p(MalCell *values)
{
    if (!values || !values->value)
    {
        return make_error("'atom?': illegal number of arguments!");
    }

    return is_atom(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *number_p(MalCell *values)
{
    if (!values || !values->value || values->cdr)
    {
        return make_error("'number?': illegal number of arguments!");
    }

    return is_number(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *string_p(MalCell *values)
{
    if (!values || !values->value || values->cdr)
    {
        return make_error("'string?': illegal number of arguments!");
    }

    return is_string(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *func_p(MalCell *values)
{
    if (!values || !values->value || values->cdr)
    {
        return make_error("'fn?': illegal number of arguments!");
    }

    return is_executable(values->value) && !is_macro(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *deref(MalCell *values)
{
    MalValue *result = atom_p(values);

    if (is_error(result))
    {
        return result;
    }

    if (is_false(result))
    {
        return make_error("'deref': argument is not an atom");
    }

    return values->value->malValue;
}

MalValue *reset_exclamation_mark(MalCell *values)
{
    MalValue *result = atom_p(values);

    if (result->valueType == MAL_ERROR)
    {
        return result;
    }

    if (result != &MAL_TRUE)
    {
        return make_error("Not an atom!");
    }

    values->value->malValue = values->cdr->value;

    return values->value->malValue;
}

MalValue *swap_exclamation_mark(MalCell *values)
{
    MalValue *result = atom_p(values);

    if (result->valueType == MAL_ERROR)
    {
        return result;
    }

    if (result != &MAL_TRUE)
    {
        return make_error("Not an atom!");
    }

    MalValue *atom = values->value;
    MalCell *first = values->cdr;
    MalCell *rest = first->cdr;
    first->cdr = NULL;

    MalValue *list = make_list(first);
    push(list, atom->malValue);
    push_all(list, rest);

    atom->malValue = EVAL(list, global_environment);

    return atom->malValue;
}

MalValue *cons(MalCell *values)
{
    MalCell *second = values->cdr;

    if (!second || (!is_sequence(second->value)))
    {
        return make_error("Expected a list as second argument!");
    }

    MalValue *result = make_list(NULL);
    push(result, values->value);
    push_all(result, values->cdr->value->list);

    return result;
}

MalValue *concat(MalCell *values)
{
    MalValue *result = make_list(NULL);
    MalCell *current = values;

    while (current)
    {
        if (!is_sequence(current->value))
        {
            return make_error("Expected a list argument!");
        }

        MalCell *nested = current->value->list;

        while (nested)
        {
            push(result, nested->value);
            nested = nested->cdr;
        }

        current = current->cdr;
    }

    return result;
}

MalValue *vec(MalCell *values)
{
    if (!values || values->cdr || !is_sequence(values->value))
    {
        return make_error("'vec': expected a list/vector argument");
    }

    return make_vector(values->value->list);
}

MalValue *vector(MalCell *values)
{
    return make_vector(values);
}

MalValue *vector_p(MalCell *values)
{
    if (!values || values->cdr)
    {
        return make_error("'vector?': expects axactly one argument");
    }

    return is_vector(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *throw(MalCell * values)
{
    if (!values || values->cdr)
    {
        return make_error("'throw': expects axactly one argument");
    }

    if (is_error(values->value))
    {
        return values->value;
    }

    return wrap_error(values->value);
}

MalValue *hashmap(MalCell *values)
{
    int64_t args = _count(values);

    if (args % 2 == 1)
    {
        return make_error("'hash-map': even count of arguments expected, got '%d'", args);
    }

    MalValue *map = make_hashmap();
    MalCell *current = values;

    while (current)
    {
        hashmap_put(map->hashMap, current->value->valueType, current->value->value, current->cdr->value);
        current = current->cdr->cdr;
    }

    return map;
}

MalValue *assoc(MalCell *values)
{
    MalValue *orig = values->value;

    if (!is_hashmap(orig))
    {
        return make_error("'assoc': expected a hasmap as first argument");
    }

    if (_count(values->cdr) % 2 != 0)
    {
        return make_error("'assoc': expected even number of arguments to merge as key/value pairs");
    }

    // FIXME: use # of key/value pairs to merge + capacity of original map as new capacity
    MalValue *map = make_hashmap();
    HashMapIterator it = hashmap_iterator(orig->hashMap);

    while (hashmap_next(&it))
    {
        hashmap_put(map->hashMap, it.keyType, it.key, it.value);
    }

    MalCell *current = values->cdr;

    while (current)
    {
        hashmap_put(map->hashMap, current->value->valueType, current->value->value, current->cdr->value);
        current = current->cdr->cdr;
    }

    return map;
}

MalValue *dissoc(MalCell *values)
{
    MalValue *orig = values->value;

    if (!is_hashmap(orig))
    {
        return make_error("'assoc': expected a hasmap as first argument");
    }

    // FIXME: use # of key/value pairs to merge + capacity of original map as new capacity
    MalValue *map = make_hashmap();
    HashMapIterator it = hashmap_iterator(orig->hashMap);

    while (hashmap_next(&it))
    {
        hashmap_put(map->hashMap, it.keyType, it.key, it.value);
    }

    MalCell *current = values->cdr;

    while (current)
    {
        hashmap_delete(map->hashMap, current->value->value);
        current = current->cdr;
    }

    return map;
}

MalValue *get_from_hashmap(MalCell *values)
{
    if (!values || values->cdr->cdr)
    {
        return make_error("'get': expected exactly two arguments");
    }

    MalValue *map = values->value;

    if (map == &MAL_NIL)
    {
        return &MAL_NIL;
    }

    if (!is_hashmap(map))
    {
        return make_error("'get': expected a hasmap as first argument");
    }

    MalValue *key = values->cdr->value;

    if (!is_string(key) && !is_symbol(key) && !is_keyword(key))
    {
        return make_error("'get?': illegal key type");
    }

    MalValue *result = hashmap_get(map->hashMap, values->cdr->value->value);

    return result == NULL ? &MAL_NIL : result;
}

MalValue *contains_p(MalCell *values)
{
    MalValue *map = values->value;

    if (!is_hashmap(map))
    {
        return make_error("'contains?': expected a hasmap as first argument");
    }

    if (_count(values->cdr) != 1)
    {
        return make_error("'contains?': expected exactly one argument");
    }

    MalValue *key = values->cdr->value;

    if (!is_string(key) && !is_symbol(key) && !is_keyword(key))
    {
        return make_error("'contains?': illegal key type");
    }

    MalValue *result = hashmap_get(map->hashMap, values->cdr->value->value);

    return result == NULL ? &MAL_FALSE : &MAL_TRUE;
}

MalValue *map_p(MalCell *values)
{
    if (!values || values->cdr)
    {
        return make_error("'map?': expected exactly one argument");
    }

    return is_hashmap(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *symbol_p(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'symbol?': exactly one argument expected");
    }

    return is_symbol(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *keyword_p(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'keyword?': exactly one argument expected");
    }

    return is_keyword(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *keyword(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'keyword': exactly one argument expected");
    }

    MalValue *value = values->value;

    if (is_string(value))
    {
        size_t len = strlen(value->value) + 2;
        char *keyword = mal_calloc(len, sizeof(char));
        int written = snprintf(keyword, len, ":%s", value->value);

        assert(written <= len);

        return make_value(MAL_KEYWORD, keyword);
    }
    else if (is_keyword(value))
    {
        return value;
    }

    return make_error("'keyword': expected a string or keyword argument");
}

MalValue *keys(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'keys': exactly one argument expected");
    }

    MalValue *value = values->value;

    if (!is_hashmap(value))
    {
        return make_error("'keys': illegal argument, expected a hashmap");
    }

    MalValue *keys = make_list(NULL);
    HashMapIterator it = hashmap_iterator(value->hashMap);

    while (hashmap_next(&it))
    {
        switch (it.keyType)
        {
        case MAL_STRING:
            push(keys, make_string(it.key, false));
            break;

        case MAL_SYMBOL:
            push(keys, make_symbol(it.key));
            break;

        case MAL_KEYWORD:
        {
            MalValue *key = new_value(MAL_KEYWORD);
            key->value = it.key;
            push(keys, key);
        }
        break;

        default:
            return make_error("'keys': illegal hash map key: '%s'", it.key);
        }
    }

    return keys;
}

MalValue *vals(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'vals': exactly one argument expected");
    }

    MalValue *value = values->value;

    if (!is_hashmap(value))
    {
        return make_error("'vals': illegal argument, expected a hashmap");
    }

    MalValue *vals = make_list(NULL);
    HashMapIterator it = hashmap_iterator(value->hashMap);

    while (hashmap_next(&it))
    {
        push(vals, it.value);
    }

    return vals;
}

MalValue *nil_p(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'nil?': exactly one argument expected");
    }

    MalValue *value = values->value;

    return is_nil(value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *true_p(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'true?': exactly one argument expected");
    }

    MalValue *value = values->value;

    return is_true(value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *false_p(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'false?': exactly one argument expected");
    }

    MalValue *value = values->value;

    return is_false(value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *symbol(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'symbol': exactly one argument expected");
    }

    MalValue *value = values->value;

    if (!is_string(value))
    {
        return make_error("'symbol': can only construct symbols from string");
    }

    return make_symbol(value->value);
}

MalValue *_apply(MalValue *executable, MalCell *params)
{
    MalValue *args = make_list(NULL);
    MalCell *current = params;

    while (current && current->cdr)
    {
        push(args, current->value);
        current = current->cdr;
    }

    if (current)
    {
        if (is_sequence(current->value))

        {
            push_all(args, current->value->list);
        }
        else
        {
            push(args, current->value);
        }
    }

    if (is_function(executable))
    {
        return executable->function(args->list);
    }

    if (is_closure(executable))
    {
        MalClosure *closure = executable->closure;
        int64_t bindings_count = _count(closure->bindings->list);
        int64_t arg_count = _count(args->list);

        if (bindings_count > arg_count)
        {
            return make_error("Expected '%d' arguments, but got '%d'", bindings_count, arg_count);
        }
        else if (arg_count > bindings_count && !closure->rest_symbol)
        {
            return make_error("Too many arguments! Expected '%d', but got '%s'. Perhaps you didn't supply an argument consuming the rest of the argument list?", bindings_count, print_values_readably(args->list)->value);
        }

        MalEnvironment *environment = make_environment(closure->environment, closure->bindings->list, args->list, closure->rest_symbol);

        if (!environment)
        {
            return make_error("Could not allocate environment!");
        }

        return EVAL(closure->ast, environment);
    }

    return make_error("'apply': first argument is not a function/closure/macro");
}

MalValue *apply(MalCell *values)
{
    if (!values->cdr)
    {
        return make_error("'apply': at least two arguments expected");
    }

    // FIXME: Reintegrate _apply????
    return _apply(values->value, values->cdr);
}

MalValue *sequential(MalCell *values)
{
    if (values->cdr)
    {
        return make_error("'sequential?': exactly one argument expected");
    }

    return is_sequence(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *map(MalCell *values)
{
    if (!values || !values->value || !values->cdr)
    {
        return make_error("'map': expected two arguments");
    }

    MalValue *executable = values->value;

    if (!is_executable(executable))
    {
        return make_error("'map': expected function/closure as first argument");
    }

    if (!values->cdr || !is_sequence(values->cdr->value))
    {
        return make_error("'map': illegal argument! expected list/vector");
    }

    if (empty_p(values->cdr) == &MAL_TRUE)
    {
        return values->cdr->value;
    }

    MalCell *current = values->cdr->value->list;
    MalValue *result = make_list(NULL);
    MalCell param = {.value = NULL, .cdr = NULL};
    MalValue *tmp;

    while (current)
    {
        param.value = current->value;
        tmp = _apply(executable, &param);

        if (is_error(tmp))
        {
            return tmp;
        }

        push(result, tmp);
        current = current->cdr;
    }

    return result;
}

MalValue *mal_readline(MalCell *values)
{
    if (!values || values->cdr)
    {
        return make_error("'readline': illegal count of arguments, exactly one argument expected");
    }

    if (!is_string(values->value))
    {
        return make_error("'readline': illegal argument, string expected");
    }

    char *prompt = _readline(values->value->value);

    if (prompt)
    {
        _add_history(prompt);
        return make_string(prompt, false);
    }

    return &MAL_NIL;
}

MalValue *time_ms(MalCell *values)
{
    if (values)
    {
        return make_error("'time-ms': takes no arguments");
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    return make_fixnum(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

MalValue *seq(MalCell *values)
{
    if (!values || !values->value || values->cdr)
    {
        return make_error("'seq': illegal count of arguments, expected a list, vector, string or nil");
    }

    if (is_nil(values->value))
    {
        return values->value;
    }

    if (!is_sequence(values->value) && !is_string(values->value))
    {
        return make_error("'seq': illegal argument, expected a list, vector, string or nil as first argument");
    }

    if (is_string(values->value))
    {

        size_t len = strlen(values->value->value);

        if (len == 0)
        {
            return &MAL_NIL;
        }

        MalValue *result = make_list(NULL);

        for (size_t i = 0; i < len; i++)
        {
            char *s = mal_calloc(2, sizeof(char));
            s[0] = values->value->value[i];
            s[1] = '\0';
            push(result, make_string(s, false));
        }

        return result;
    }

    if (empty_p(values) == &MAL_TRUE)
    {
        return &MAL_NIL;
    }

    if (is_vector(values->value))
    {
        return make_list(values->value->list);
    }

    return values->value;
}

MalValue *meta(MalCell *values)
{
    if (!values || values->cdr)
    {
        return make_error("'meta': illegal argument, expected exactly one argument");
    }

    if (!is_sequence(values->value) && !is_hashmap(values->value) && !is_executable(values->value))
    {
        return make_error("'meta': expected argument of type list/vector/hashmap or function/closure");
    }

    if (!values->value->metadata)
    {
        return &MAL_NIL;
    }

    return values->value->metadata;
}

MalValue *with_meta(MalCell *values)
{
    if (!values || (values->cdr && values->cdr->cdr))
    {
        return make_error("'with-meta': illegal argument, expected exactly two arguments");
    }

    if (!is_sequence(values->value) && !is_hashmap(values->value) && !is_executable(values->value))
    {
        return make_error("'meta': expected argument of type list/vector/hashmap or function/closure");
    }

    MalValue *result = mal_clone(values->value);
    result->metadata = values->cdr->value;

    return result;
}

/**
 * Append values of given cells at end of the given sequence.
 *
 * @param vector to append values to
 * @param values list of MalCells to append to the given sequence
 */
void sequence_append_values(MalValue *vector, MalCell *values)
{
    assert(vector->valueType == MAL_VECTOR || vector->valueType == MAL_LIST);

    if (!values)
    {
        return;
    }

    MalCell *tail = mal_calloc(1, sizeof(MalCell));
    MalCell *head = tail;
    MalCell *current = values;

    while (current)
    {
        tail->value = current->value;
        current = current->cdr;

        if (current)
        {
            tail->cdr = mal_calloc(1, sizeof(MalCell));
            tail = tail->cdr;
        }
    }

    // target vector was empty
    if (!vector->list)
    {
        vector->list = head;
        return;
    }

    // find last element of original vector
    current = vector->list;

    while (current->cdr)
    {
        current = current->cdr;
    }

    // and append new tail
    current->cdr = head;
}

MalValue *vector_clone(MalValue *orig)
{
    MalValue *result = make_vector(NULL);

    sequence_append_values(result, orig->list);

    return result;
}

MalValue *list_clone(MalValue *orig)
{
    MalValue *result = make_list(NULL);

    sequence_append_values(result, orig->list);

    return result;
}

MalValue *_conj(MalCell *values)
{
    if (!values)
    {
        return make_error("'conj': expects at least two arguments");
    }

    if (!is_sequence(values->value))
    {
        return make_error("'conj': expects a list/vector as first argument");
    }

    if (is_vector(values->value))
    {
        MalValue *vector = vector_clone(values->value);
        sequence_append_values(vector, values->cdr);

        return vector;
    }

    if (is_list(values->value))
    {
        MalValue *list = list_clone(values->value);
        MalCell *current = values->cdr;

        while (current)
        {
            prepend(list, current->value);
            current = current->cdr;
        }

        return list;
    }

    assert(false);
}

HashMap *core_namespace()
{
    HashMap *ns = new_hashmap();

    hashmap_put(ns, MAL_SYMBOL, "+", new_function(add));
    hashmap_put(ns, MAL_SYMBOL, "-", new_function(subtract));
    hashmap_put(ns, MAL_SYMBOL, "*", new_function(multiply));
    hashmap_put(ns, MAL_SYMBOL, "/", new_function(divide));
    hashmap_put(ns, MAL_SYMBOL, "prn", new_function(prn));
    hashmap_put(ns, MAL_SYMBOL, "println", new_function(println));
    hashmap_put(ns, MAL_SYMBOL, "pr-str", new_function(print_values_readably));
    hashmap_put(ns, MAL_SYMBOL, "str", new_function(print_values));
    hashmap_put(ns, MAL_SYMBOL, "list", new_function(list));
    hashmap_put(ns, MAL_SYMBOL, "list?", new_function(list_p));
    hashmap_put(ns, MAL_SYMBOL, "empty?", new_function(empty_p));
    hashmap_put(ns, MAL_SYMBOL, "count", new_function(count));
    hashmap_put(ns, MAL_SYMBOL, ">", new_function(greater_than));
    hashmap_put(ns, MAL_SYMBOL, ">=", new_function(greater_than_or_equal_to));
    hashmap_put(ns, MAL_SYMBOL, "<", new_function(less_than));
    hashmap_put(ns, MAL_SYMBOL, "<=", new_function(less_than_or_equal_to));
    hashmap_put(ns, MAL_SYMBOL, "=", new_function(equals));
    hashmap_put(ns, MAL_SYMBOL, "read-string", new_function(read_string));
    hashmap_put(ns, MAL_SYMBOL, "slurp", new_function(slurp));
    hashmap_put(ns, MAL_SYMBOL, "eval", new_function(eval));

    hashmap_put(ns, MAL_SYMBOL, "atom", new_function(atom));
    hashmap_put(ns, MAL_SYMBOL, "atom?", new_function(atom_p));
    hashmap_put(ns, MAL_SYMBOL, "deref", new_function(deref));
    hashmap_put(ns, MAL_SYMBOL, "reset!", new_function(reset_exclamation_mark));
    hashmap_put(ns, MAL_SYMBOL, "swap!", new_function(swap_exclamation_mark));
    hashmap_put(ns, MAL_SYMBOL, "cons", new_function(cons));
    hashmap_put(ns, MAL_SYMBOL, "concat", new_function(concat));
    hashmap_put(ns, MAL_SYMBOL, "vec", new_function(vec));
    hashmap_put(ns, MAL_SYMBOL, "vector", new_function(vector));
    hashmap_put(ns, MAL_SYMBOL, "vector?", new_function(vector_p));

    hashmap_put(ns, MAL_SYMBOL, "macro?", new_function(macro_p));

    hashmap_put(ns, MAL_SYMBOL, "nth", new_function(nth));
    hashmap_put(ns, MAL_SYMBOL, "first", new_function(first));
    hashmap_put(ns, MAL_SYMBOL, "rest", new_function(rest));

    hashmap_put(ns, MAL_SYMBOL, "throw", new_function(throw));

    hashmap_put(ns, MAL_SYMBOL, "hash-map", new_function(hashmap));
    hashmap_put(ns, MAL_SYMBOL, "assoc", new_function(assoc));
    hashmap_put(ns, MAL_SYMBOL, "dissoc", new_function(dissoc));
    hashmap_put(ns, MAL_SYMBOL, "get", new_function(get_from_hashmap));
    hashmap_put(ns, MAL_SYMBOL, "contains?", new_function(contains_p));
    hashmap_put(ns, MAL_SYMBOL, "map?", new_function(map_p));
    hashmap_put(ns, MAL_SYMBOL, "keys", new_function(keys));
    hashmap_put(ns, MAL_SYMBOL, "vals", new_function(vals));

    hashmap_put(ns, MAL_SYMBOL, "symbol?", new_function(symbol_p));
    hashmap_put(ns, MAL_SYMBOL, "symbol", new_function(symbol));
    hashmap_put(ns, MAL_SYMBOL, "keyword?", new_function(keyword_p));
    hashmap_put(ns, MAL_SYMBOL, "keyword", new_function(keyword));

    hashmap_put(ns, MAL_SYMBOL, "nil?", new_function(nil_p));
    hashmap_put(ns, MAL_SYMBOL, "true?", new_function(true_p));
    hashmap_put(ns, MAL_SYMBOL, "false?", new_function(false_p));
    hashmap_put(ns, MAL_SYMBOL, "sequential?", new_function(sequential));
    hashmap_put(ns, MAL_SYMBOL, "apply", new_function(apply));

    hashmap_put(ns, MAL_SYMBOL, "map", new_function(map));

    hashmap_put(ns, MAL_SYMBOL, "readline", new_function(mal_readline));
    hashmap_put(ns, MAL_SYMBOL, "number?", new_function(number_p));
    hashmap_put(ns, MAL_SYMBOL, "time-ms", new_function(time_ms));
    hashmap_put(ns, MAL_SYMBOL, "string?", new_function(string_p));
    hashmap_put(ns, MAL_SYMBOL, "fn?", new_function(func_p));
    hashmap_put(ns, MAL_SYMBOL, "seq", new_function(seq));
    hashmap_put(ns, MAL_SYMBOL, "meta", new_function(meta));
    hashmap_put(ns, MAL_SYMBOL, "with-meta", new_function(with_meta));
    hashmap_put(ns, MAL_SYMBOL, "conj", new_function(_conj));

    return ns;
}

MalEnvironment *make_initial_environment()
{
    MalEnvironment *environment = make_environment(NULL, NULL, NULL, NULL);
    HashMap *ns = core_namespace();

    HashMapIterator it = hashmap_iterator(ns);

    while (hashmap_next(&it))
    {
        set_in_environment(environment, make_symbol(it.key), it.value);
    }

    set_in_environment(environment, &MAL_NIL, &MAL_NIL);
    set_in_environment(environment, &MAL_TRUE, &MAL_TRUE);
    set_in_environment(environment, &MAL_FALSE, &MAL_FALSE);

    return environment;
}
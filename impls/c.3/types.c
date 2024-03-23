#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "gc.h"
#include "env.h"
#include "types.h"
#include "printer.h"

MalValue MAL_NIL = {
    .valueType = MAL_TYPE_NIL,
    .value = "nil"};

MalValue MAL_FALSE = {
    .valueType = MAL_TYPE_FALSE,
    .value = "false"};

MalValue MAL_TRUE = {
    .valueType = MAL_TYPE_TRUE,
    .value = "true"};


static const MalSymbol EOF_SYMBOL = {
    .name = "EOF",
    .package = &MAL_NIL
};

MalValue MAL_EOF = {
    .valueType = MAL_SYMBOL,
    .symbol = &EOF_SYMBOL
};

bool is_nil(const MalValue *value)
{
    return value == &MAL_NIL;
}

bool is_true(const MalValue *value)
{
    return value == &MAL_TRUE;
}

bool is_false(const MalValue *value)
{
    return value == &MAL_FALSE;
}

bool is_atom(MalValue *value)
{
    return value->valueType == MAL_ATOM;
}

bool is_list(MalValue *value)
{
    return value->valueType == MAL_LIST;
}

bool is_vector(MalValue *value)
{
    return value->valueType == MAL_VECTOR;
}

bool is_sequence(MalValue *value)
{
    return value->valueType == MAL_LIST || value->valueType == MAL_VECTOR;
}

bool is_symbol(const MalValue *value)
{
    return value && value->valueType == MAL_SYMBOL;
}

bool is_keyword(MalValue *value)
{
    return value && value->valueType == MAL_KEYWORD;
}

bool is_named_symbol(MalValue *value, const char *symbol_name)
{
    return is_symbol(value) && strcmp(get_symbol_name(value), symbol_name) == 0;
}

bool is_error(MalValue *value)
{
    return value->valueType == MAL_ERROR;
}

bool is_function(MalValue *value)
{
    return value->valueType == MAL_FUNCTION;
}

bool is_closure(MalValue *value)
{
    return value->valueType == MAL_CLOSURE;
}

bool is_macro(MalValue *value)
{
    return is_closure(value) && value->closure->is_macro;
}

bool is_executable(MalValue *value)
{
    return is_function(value) || is_closure(value);
}

bool is_number_type(MalValue *value)
{
    switch (value->valueType)
    {
    case MAL_FIXNUM:
        return true;

    default:
        break;
    }

    return false;
}

bool is_self_evaluating(MalValue *value)
{
    bool result = true;

    switch (value->valueType)
    {
    case MAL_LIST:
    case MAL_VECTOR:
    case MAL_HASHMAP:
    case MAL_SYMBOL:
        result = false;
        break;
    default:
        break;
    }

    return result;
}

/**
 * Validate that given value is a MAL_FIXNUM.
 *
 * @return {code}true{code} when the given value a MAL_FIXNUM, {code}false{code} otherwise.
 */
bool is_fixnum(MalValue *value)
{
    return value->valueType == MAL_FIXNUM;
}

/**
 * Validate that given value is number type.
 *
 * @return {code}true{code} when the given value is one of
 * <ul>
 * <li>MAL_FIXNUM or</li>
 * </ul>
 */
bool is_number(MalValue *value)
{
    return value->valueType == MAL_FIXNUM;
}

bool is_string_type(MalValue *value)
{
    switch (value->valueType)
    {
    case MAL_STRING:
    case MAL_SYMBOL:
    case MAL_KEYWORD:
        return true;

    default:
        break;
    }

    return false;
}

bool is_string(MalValue *value)
{
    return value->valueType == MAL_STRING;
}

bool is_hashmap(MalValue *value)
{
    return value->valueType == MAL_HASHMAP;
}

bool is_package(MalValue *value)
{
    return value->valueType == MAL_PACKAGE;
}

MalValue *new_value(enum MalValueType valueType)
{
    MalValue *value = mal_calloc(1, sizeof(MalValue));
    value->valueType = valueType;

    return value;
}

// FIXME: determine function signature for registering functions as MalValue-objects
MalValue *new_function(MalValue *(*function)(MalCell *))
{
    MalValue *fn = new_value(MAL_FUNCTION);
    fn->function = function;

    return fn;
}

MalValue *make_value(enum MalValueType valueType, const char *value)
{
    MalValue *mal_value = new_value(valueType);
    mal_value->value = value;

    return mal_value;
}

MalValue *make_symbol(const char *symbol_name)
{
    MalValue *value = new_value(MAL_SYMBOL);
    value->symbol = mal_calloc(1, sizeof(MalSymbol));
    uint64_t len = strlen(symbol_name) + 1;
    char *name = mal_malloc(len);

    for (uint64_t i = 0; i < len; i++)
    {
        name[i] = tolower(symbol_name[i]);
    }

    name[len] = '\0';
    value->symbol->name = name;
    value->symbol->package = &MAL_NIL;

    return value;
}

char *get_symbol_name(const MalValue *symbol)
{
    // FIXME: should use symbolp(MalValue*)
    assert(is_symbol(symbol) || is_nil(symbol) || is_true(symbol) || is_false(symbol));

    return is_symbol(symbol) ? symbol->symbol->name : symbol->value;
}

MalValue *make_error(char *fmt, ...)
{
    va_list arg_ptr;
    int result = -1;
    size_t buf_len = 2 * strlen(fmt);
    char *message = "";

    while (result < 0)
    {
        char buffer[buf_len];
        va_start(arg_ptr, fmt);
        result = vsnprintf(buffer, buf_len, fmt, arg_ptr);

        if (result <= buf_len)
        {
            message = (char *)mal_malloc(result + 1);
            strcpy(message, buffer);
        }
        else
        {
            buf_len *= 2;
            result = -1;
        }

        va_end(arg_ptr);
    }

    MalValue *error = new_value(MAL_ERROR);
    error->malValue = make_string(message, false);

    return error;
}

MalValue *wrap_error(MalValue *value)
{
    MalValue *error = new_value(MAL_ERROR);
    error->malValue = value;

    return error;
}

MalValue *make_fixnum(int64_t number)
{
    MalValue *value = new_value(MAL_FIXNUM);
    value->fixnum = number;

    return value;
}

MalValue *make_closure(MalEnvironment *outer, MalCell *context)
{
    if (!context->cdr)
    {
        return make_error("missing closure body: '%s'", print_values_readably(context)->value);
    }

    MalValue *value = new_value(MAL_CLOSURE);
    MalClosure *closure = mal_calloc(1, sizeof(MalClosure));
    closure->environment = make_environment(outer, NULL, NULL, NULL);

    closure->ast = context->cdr->value;
    closure->bindings = context->value;
    closure->is_macro = false;

    MalCell *args = context->value->list;
    MalCell *marker = NULL;

    while (args)
    {
        if (strcmp("&", args->value->value) == 0)
        {
            marker = args;

            if (args->cdr)
            {
                if (args->cdr->cdr)
                {
                    return make_error("only one symbol to receive the rest of the argument list is allowed: '(%s)'", print_values_readably(args)->value);
                }

                closure->rest_symbol = args->cdr->value;
                break;
            }

            return make_error("expected a symbol to receive the rest of the argument list: '(%s)'", print_values_readably(args)->value);
        }

        args = args->cdr;
    }

    args = context->value->list;

    if (marker)
    {
        closure->bindings = make_list(NULL);

        while (args != marker)
        {
            push(closure->bindings, args->value);
            args = args->cdr;
        }
    }

    value->closure = closure;

    return value;
}

MalValue *make_string(const char *value, bool unescape)
{
    MalValue *_value = new_value(MAL_STRING);

    if (!unescape)
    {
        _value->value = value;
        return _value;
    }

    size_t len = strlen(value);
    char tmp[len * 2];
    int j = 0;

    for (int i = 0; i < len; i++, j++)
    {
        if (value[i] == '\\')
        {
            switch (value[i + 1])
            {
            case '\\':
                tmp[j] = '\\';
                i++;
                break;

            case 'n':
                tmp[j] = '\n';
                i++;
                break;

            case '"':
                tmp[j] = '"';
                i++;
                break;

            default:
                tmp[j] = value[i + 1];
                break;
            }
        }
        else
        {
            tmp[j] = value[i];
        }
    }

    char *result = mal_calloc(j + 1, sizeof(char));
    strncpy(result, tmp, j);

    _value->value = result;
    return _value;
}

MalValue *make_list(MalCell *values)
{
    MalValue *result = new_value(MAL_LIST);

    if (values && values->value)
    {
        result->list = values;
    }

    return result;
}

MalValue *make_vector(MalCell *values)
{
    MalValue *result = new_value(MAL_VECTOR);
    push_all(result, values);

    return result;
}

void push(MalValue *list, MalValue *value)
{
    assert(list->valueType == MAL_LIST || list->valueType == MAL_VECTOR);

    if (list->list == NULL)
    {
        list->list = mal_calloc(1, sizeof(MalCell));
        list->list->value = value;
        return;
    }

    MalCell *cell = list->list;

    while (cell->cdr != NULL)
    {
        cell = cell->cdr;
    }

    cell->cdr = mal_malloc(sizeof(MalCell));
    cell->cdr->cdr = NULL;
    cell->cdr->value = value;
}

/**
 * Append MalCells to end of given list.
 *
 * @param list the list to append cells to
 * @param values cells to append at end of the list
 */
void push_all(MalValue *list, MalCell *values)
{
    assert(list->valueType == MAL_LIST || list->valueType == MAL_VECTOR);

    if (list->list == NULL)
    {
        list->list = values;
        return;
    }

    MalCell *cell = list->list;

    while (cell->cdr != NULL)
    {
        cell = cell->cdr;
    }

    cell->cdr = values;
}

void prepend(MalValue *list, MalValue *value)
{
    MalCell *head = mal_malloc(sizeof(MalCell));

    head->value = value;
    head->cdr = list->list;
    list->list = head;
}

MalValue *reverse(MalValue *list)
{
    assert(is_sequence(list));

    MalValue *reversed = make_list(NULL);
    MalCell *current = list->list;

    while (current)
    {
        prepend(reversed, current->value);
        current = current->cdr;
    }

    return reversed;
}

MalValue *make_hashmap()
{
    MalValue *map = new_value(MAL_HASHMAP);
    map->hashMap = new_hashmap();

    return map;
}

const char *put(MalValue *map, MalValue *key, MalValue *value)
{
    assert(map->valueType == MAL_HASHMAP);

    return hashmap_put(map->hashMap, key->valueType, key->value, value);
}

MalValue *mal_clone(MalValue *value)
{
    MalValue *result = new_value(value->valueType);

    switch (value->valueType)
    {
    case MAL_FUNCTION:
        result->function = value->function;
        break;

    case MAL_CLOSURE:
    {
        result->closure = mal_calloc(1, sizeof(MalClosure));
        result->closure->ast = value->closure->ast;
        result->closure->bindings = value->closure->bindings;
        result->closure->environment = value->closure->environment;
        result->closure->is_macro = value->closure->is_macro;
        result->closure->rest_symbol = value->closure->rest_symbol;
    }
    break;

    case MAL_HASHMAP:
        result->hashMap = value->hashMap;
        break;

    case MAL_LIST:
    case MAL_VECTOR:
        result->list = value->list;
        break;

    case MAL_STRING:
    case MAL_SYMBOL:
    case MAL_KEYWORD:
        result->value = value->value;
        break;

    case MAL_FIXNUM:
        result->fixnum = value->fixnum;
        break;

    case MAL_ATOM:
        result->malValue = value->malValue;
        break;

    default:
        assert(false);
        break;
    }

    result->metadata = value->metadata;

    return result;
}

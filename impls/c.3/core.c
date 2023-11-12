#include <assert.h>
#include <string.h>
#include "core.h"
#include "env.h"
#include "printer.h"
#include "reader.h"
#include "types.h"
#include "gc.h"

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
        if (value->value->list && value->value->list->value)
        {
            return &MAL_FALSE;
        }

        return &MAL_TRUE;
    }

    return make_error("Illegal argument type!");
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

    if (!current)
    {
        return count;
    }

    if (is_sequence(value->value))
    {
        current = value->value->list;
    }
    else if (current->value == &MAL_NIL)
    {
        current = NULL;
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

bool is_equal_by_type(MalValue *left, MalValue *right)
{
    assert(left->valueType == right->valueType);

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

    default:
        break;
    }

    return result;
}

bool is_equal(MalValue *left, MalValue *right)
{
    if (left->valueType == right->valueType)
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
    if (!values || values->value->valueType != MAL_STRING)
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

        buffer = mal_malloc(len);
        bytes_read = fread(buffer, 1, len, stream);
        fclose(stream);

        if (bytes_read != len)
        {
            free(buffer);
            buffer = NULL;
        }
        else
        {
            buffer[len] = '\0';
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

bool is_atom(MalValue *value)
{
    return value->valueType == MAL_ATOM;
}

MalValue *atom_p(MalCell *values)
{
    if (!values || !values->value)
    {
        return make_error("Illegal number of arguments!");
    }

    return is_atom(values->value) ? &MAL_TRUE : &MAL_FALSE;
}

MalValue *deref(MalCell *values)
{
    MalValue *result = atom_p(values);

    if (result->valueType == MAL_ERROR)
    {
        return result;
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

MalValue *vector(MalCell *values) {
    return make_vector(values);
}

HashMap *core_namespace()
{
    HashMap *ns = make_hashmap();

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
    hashmap_put(ns, MAL_SYMBOL, "vec", new_function(vector));

    return ns;
}

MalEnvironment *make_initial_environment()
{
    MalEnvironment *environment = make_environment(NULL, NULL, NULL, NULL);
    HashMap *ns = core_namespace();

    HashMapIterator it = hashmap_iterator(ns);

    while (hashmap_next(&it))
    {
        set_in_environment(environment, make_value(MAL_SYMBOL, it.key), it.value);
    }

    set_in_environment(environment, &MAL_NIL, &MAL_NIL);
    set_in_environment(environment, &MAL_TRUE, &MAL_TRUE);
    set_in_environment(environment, &MAL_FALSE, &MAL_FALSE);

    return environment;
}
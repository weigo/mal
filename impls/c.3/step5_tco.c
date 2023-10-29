#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "env.h"
#include "libs/readline/readline.h"
#include "printer.h"
#include "reader.h"
#include "core.h"
#include "types.h"

static const char *HISTORY_FILENAME = ".mal_history";
FILE *output_stream;
void PRINT(MalValue *value);
MalValue *READ(Reader *reader)
{
    return read_str(reader);
}

MalValue *EVAL(MalValue *value, MalEnvironment *environment);

MalValue *eval_ast(MalValue *value, MalEnvironment *environment)
{
    MalValue *result = NULL;

    switch (value->valueType)
    {
    case MAL_SYMBOL:
    {
        MalValue *symbol = lookup_in_environment(environment, value);

        if (symbol)
        {
            result = symbol;
        }
        else
        {
            result = make_error("'%s' not found", value->value);
        }
    }
    break;

    case MAL_LIST:
    case MAL_VECTOR:
    {
        MalCell *cdr = value->list->cdr;
        MalValue *tmp = EVAL(value->list->value, environment);

        assert(tmp);

        MalValue *list = new_value(value->valueType);
        push(list, tmp);

        while (cdr != NULL)
        {
            tmp = EVAL(cdr->value, environment);
            assert(tmp);

            if (tmp->valueType == MAL_ERROR)
            {
                return tmp;
            }

            push(list, tmp);
            cdr = cdr->cdr;
        }

        result = list;
    }
    break;

    case MAL_HASHMAP:
    {
        MalValue *h = new_value(MAL_HASHMAP);
        h->hashMap = make_hashmap();
        MalValue *r = NULL;
        HashMapIterator it = hashmap_iterator(value->hashMap);

        while (hashmap_next(&it))
        {
            r = EVAL(it.value, environment);

            if (r)
            {
                put(h, it.key, r);
            }
            else
            {
                free_hashmap(h->hashMap);
                return NULL;
            }
        }

        result = h;
    }
    break;

    default:
        result = value;
        break;
    }

    return result;
}

MalValue *def_exclamation_mark(MalCell *head, MalEnvironment *environment)
{
    MalValue *t = EVAL(head->cdr->cdr->value, environment);

    // !t means symbol not found and should already be recorded in struct error
    if (t && set_in_environment(environment, head->cdr->value, t))
    {
        // FIXME: Report to repl that a value has been redefined.
        //        register_error(VALUE_REDEFINED, head->cdr->value->value);
    }

    return t;
}

/**
 *  create a new environment using the current environment as the outer value
 *  and then use the first parameter as a list of new bindings in the "let*" environment.
 *  Take the second element of the binding list, call EVAL using the new "let*" environment
 *  as the evaluation environment, then call set on the "let*" environment using the first
 *  binding list element as the key and the evaluated second element as the value. This is
 *  repeated for each odd/even pair in the binding list. Note in particular, the bindings
 * earlier in the list can be referred to by later bindings. Finally, the second parameter
 * (third element) of the original let* form is evaluated using the new "let*" environment
 *  and the result is returned as the result of the let* (the new let environment is discarded
 *  upon completion).
 */
void let_star(MalValue **value, MalEnvironment **environment)
{
    MalCell *head = (*value)->list;

    if (!head->cdr || !head->cdr->value || (head->cdr->value->valueType != MAL_LIST && head->cdr->value->valueType != MAL_VECTOR))
    {
        *value = make_error("expected a list of bindings for '(%s)', got: '%s'", head->value->value, print_values_readably(head->cdr)->value);
        return;
    }

    MalEnvironment *nested_environment = make_environment(*environment, NULL, NULL, NULL);

    MalCell *bindings = head->cdr->value->list;
    MalValue *binding = NULL;
    MalValue *symbol = NULL;

    if (_count(bindings) % 2 == 1)
    {
        *value = make_error("expected an even number of arguments as bindings: got '%s'", print_values_readably(bindings)->value);
        return;
    }

    while (bindings)
    {
        symbol = bindings->value;
        binding = EVAL(bindings->cdr->value, nested_environment);

        if (binding->valueType == MAL_ERROR)
        {
            free_environment(nested_environment);
            *value = binding;
            return;
        }

        set_in_environment(nested_environment, symbol, binding);
        bindings = bindings->cdr->cdr;
    }

    *value = head->cdr->cdr->value;
    *environment = nested_environment;
}

/**
 * do: change the eval_ast call to evaluate all the parameters except for the last
 * (2nd list element up to but not including last). Set ast to the last element of ast.
 *  Continue at the beginning of the loop (env stays unchanged).
 */
void do_(MalValue **value, MalEnvironment **environment)
{
    MalCell *params = (*value)->list->cdr;
    MalValue *result = NULL;

    while (params && params->cdr)
    {
        result = EVAL(params->value, *environment);

        if (result->valueType == MAL_ERROR)
        {
            *value = result;
            return;
        }

        params = params->cdr;
    }

    *value = params->value;
}

void if_(MalValue **value, MalEnvironment **environment)
{
    MalCell *head = (*value)->list;
    MalValue *condition = EVAL(head->cdr->value, *environment);

    if (condition != &MAL_NIL && condition != &MAL_FALSE)
    {
        *value = head->cdr->cdr->value;
    }
    else if (head->cdr->cdr->cdr && head->cdr->cdr->cdr->value)
    {
        *value = head->cdr->cdr->cdr->value;
    }
    else
    {
        *value = &MAL_NIL;
    }
}

MalValue *fn_star(MalCell *context, MalEnvironment *environment)
{
    return make_closure(environment, context);
}

MalValue *EVAL(MalValue *value, MalEnvironment *environment)
{
    MalValue *current = value;

    while (true)
    {
        if (!current)
        {
            return &MAL_NIL;
        }

        if (current->valueType != MAL_LIST && current->valueType != MAL_VECTOR)
        {
            // ast is not a list: then return the result of calling eval_ast on it.
            return eval_ast(current, environment);
        }

        MalCell *head = current->list;

        // ast is an empty list: return ast unchanged.
        if (head == NULL)
        {
            return current;
        }

        if (current->valueType == MAL_LIST && head->value->valueType == MAL_SYMBOL)
        {
            if (strcmp("def!", head->value->value) == 0)
            {
                return def_exclamation_mark(head, environment);
            }
            else if (strcmp("let*", head->value->value) == 0)
            {
                let_star(&current, &environment);
                continue;
            }
            else if (strcmp("do", head->value->value) == 0)
            {
                do_(&current, &environment);
                continue;
            }
            else if (strcmp("if", head->value->value) == 0)
            {
                if_(&current, &environment);
                continue;
            }
            else if (strcmp("fn*", head->value->value) == 0)
            {
                return fn_star(head->cdr, environment);
            }
        }

        // ast is a list: call eval_ast to get a new evaluated list.
        // Take the first item of the evaluated list and call it as
        // function using the rest of the evaluated list as its arguments.
        MalValue *tmp = eval_ast(current, environment);

        assert(tmp);

        if (tmp->valueType != MAL_LIST)
        {
            return make_error("Not callable: %s.", print_values(head)->value);
        }

        head = tmp->list;

        if (head->value->valueType == MAL_FUNCTION)
        {
            return head->value->function(head->cdr);
        }
        if (head->value->valueType == MAL_CLOSURE)
        {
            MalClosure *closure = head->value->closure;
            int64_t bindings_count = _count(closure->bindings->list);
            int64_t arg_count = _count(head->cdr);

            if (bindings_count > arg_count)
            {
                return make_error("Expected '%d' arguments, but got '%d'", bindings_count, arg_count);
            }
            else if (arg_count > bindings_count && !closure->rest_symbol)
            {
                return make_error("Too many arguments! Expected '%d', but got '%d'. Perhaps you didn't supply an argument consuming the rest of the arugment list?", bindings_count, arg_count);
            }

            environment = make_environment(closure->environment, closure->bindings->list, head->cdr, closure->rest_symbol);

            if (!environment)
            {
                return make_error("Could not allocate environment!");
            }

            current = closure->ast;
            continue;
        }

        return make_error("Not callable: %s.", print_values(head)->value);
    }
}

void PRINT(MalValue *value)
{
    print(output_stream, value, true);
    fprintf(output_stream, "\n");
}

void rep(char *input, MalEnvironment *environment)
{
    Reader reader = {.input = input};
    Token token = {};
    reader.token = &token;

    MalValue *value = READ(&reader);

    if (value->valueType != MAL_ERROR)
    {
        MalValue *result = EVAL(value, environment);

        PRINT(result);
    }
    else
    {
        PRINT(value);
    }
}

char *get_history_filename()
{
    char *home_folder = getenv("HOME");
    char *history_file = malloc(strlen(home_folder) + strlen(HISTORY_FILENAME) + 1 + 1);
    sprintf(history_file, "%s/%s", home_folder, HISTORY_FILENAME);

    return history_file;
}

int main(int argc, char **argv)
{
    char *input = NULL;
    output_stream = stdout;
    char *history_file = get_history_filename();
    _read_history(history_file);

    MalEnvironment *environment = make_initial_environment();
    rep("(def! not (fn* (a) (if a false true)))", environment);

    while (1)
    {
        input = _readline("user> ");

        if (input && *input && *input != 0x0)
        {
            _add_history(input);
            rep(input, environment);
        }
        else
        {
            break;
        }
    }

    _save_history(history_file);
    free(history_file);
}
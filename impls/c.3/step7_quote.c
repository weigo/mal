#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "env.h"
#include "libs/readline/readline.h"
#include "printer.h"
#include "reader.h"
#include "core.h"
#include "symbol.h"
#include "types.h"

#define MULTILINE_STRING(...) #__VA_ARGS__

const char *LISP_LIBRARY =
    "(do \n"
    "(def! not (fn* (a) (if a false true)))\n"
    "(def! load-file (fn* (f)\n"
    "                     (eval (read-string (str \"(do \" (slurp f) \"\nnil)\"))))))";

static const char *HISTORY_FILENAME = ".mal_history";
FILE *output_stream;
void PRINT(MalValue *value);

extern MalEnvironment *global_environment;

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

        if (tmp->valueType == MAL_ERROR)
        {
            return tmp;
        }

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
                hashmap_put(h->hashMap, it.keyType, it.key, r);
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

MalValue *quote(MalValue *value, MalEnvironment *environment)
{
    MalCell *tmp = value->list->cdr;

    if (!tmp)
    {
        return &MAL_NIL;
    }

    if (tmp->cdr)
    {
        return make_error("Too many arguments to 'quote': '%s'!", print_values_readably(tmp->cdr)->value);
    }

    return tmp->value;
}

MalValue *quasiquote(MalValue *value);
MalValue *quasiquote_list(MalValue *value);

MalValue *quasiquote_vector(MalValue *value)
{
    MalValue *result = make_list(NULL);
    MalCell *vector = value->list;

    if (vector)
    {
        MalValue *first = vector->value;

        if (is_named_symbol(first, SYMBOL_UNQUOTE))
        {
            push(result, make_value(MAL_SYMBOL, SYMBOL_QUOTE));
            push(result, value);

            return result;
        }
    }

    push(result, make_value(MAL_SYMBOL, SYMBOL_VEC));
    MalValue *quasiquoted_list = quasiquote_list(value);

    if (is_error(quasiquoted_list))
    {
        return quasiquoted_list;
    }

    push(result, quasiquoted_list);
    return result;
}

MalValue *quasiquote_list(MalValue *value)
{
    MalCell *list = value->list;

    if (!list)
    {
        return make_list(NULL);
    }

    MalValue *first = list->value;

    if (is_named_symbol(first, SYMBOL_UNQUOTE) && list->cdr)
    {
        if (list->cdr->cdr)
        {
            return make_error("'unquote' expects one argument!");
        }

        return list->cdr->value;
    }

    if (is_list(first) && first->list &&
        is_named_symbol(first->list->value, SYMBOL_SPLICE_UNQUOTE))
    {
        if (!first->list->cdr)
        {
            return make_error("'splice-unquote' expects one argument!");
        }

        MalValue *spliced = make_list(NULL);
        push(spliced, make_value(MAL_SYMBOL, SYMBOL_CONCAT));
        push(spliced, first->list->cdr->value);
        MalValue *rest = quasiquote(make_list(list->cdr));

        if (is_error(rest))
        {
            return rest;
        }

        push(spliced, rest);

        return spliced;
    }

    first = quasiquote(first);

    if (is_error(first))
    {
        return first;
    }

    MalValue *rest = quasiquote(make_list(list->cdr));

    if (is_error(rest))
    {
        return rest;
    }

    MalValue *cons = make_list(NULL);

    push(cons, make_value(MAL_SYMBOL, SYMBOL_CONS));
    push(cons, first);
    push(cons, rest);

    return cons;
}

/**
 * Add the quasiquote function. The quasiquote function takes a parameter ast and has the following conditional.
 *
 * If ast is a list starting with the "unquote" symbol, return its second element.
 *
 * If ast is a list failing previous test, the result will be a list populated by the following process.
 *
 * The result is initially an empty list. Iterate over each element elt of ast in reverse order:
 *
 * If elt is a list starting with the "splice-unquote" symbol, replace the current result with a list containing: the "concat" symbol, the second element of elt, then the previous result.
 * Else replace the current result with a list containing: the "cons" symbol, the result of calling quasiquote with elt as argument, then the previous result.
 * This process can also be described recursively:
 *
 * If ast is empty return it unchanged. else let elt be its first element.
 *
 * If elt is a list starting with the "splice-unquote" symbol, return a list containing: the "concat" symbol, the second element of elt, then the result of processing the rest of ast.
 * Else return a list containing: the "cons" symbol, the result of calling quasiquote with elt as argument, then the result of processing the rest of ast.
 * If ast is a map or a symbol, return a list containing: the "quote" symbol, then ast.
 *
 * Else return ast unchanged. Such forms are not affected by evaluation, so you may quote them as in the previous case if implementation is easier.
 */
MalValue *quasiquote(MalValue *value)
{
    MalValue *result = NULL;

    switch (value->valueType)
    {
    case MAL_LIST:
        result = quasiquote_list(value);
        break;
    case MAL_VECTOR:
    {
        result = quasiquote_vector(value);
        break;
    }
    case MAL_HASHMAP:
    case MAL_SYMBOL:
    {
        if (is_named_symbol(value, SYMBOL_NIL))
        {
            result = value;
            break;
        }

        result = make_list(NULL);
        push(result, make_value(MAL_SYMBOL, SYMBOL_QUOTE));
        push(result, value);
        break;
    }

    default:
        result = value;
        break;
    }

    return result;
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

        if (!is_sequence(current))
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

        if (is_sequence(current))
        {
            if (is_named_symbol(head->value, SYMBOL_DEF_BANG))
            {
                return def_exclamation_mark(head, environment);
            }
            else if (is_named_symbol(head->value, SYMBOL_LET_STAR))
            {
                let_star(&current, &environment);
                continue;
            }
            else if (is_named_symbol(head->value, "do"))
            {
                do_(&current, &environment);
                continue;
            }
            else if (is_named_symbol(head->value, SYMBOL_IF))
            {
                if_(&current, &environment);
                continue;
            }
            else if (is_named_symbol(head->value, SYMBOL_FN_STAR))
            {
                return fn_star(head->cdr, environment);
            }
            else if (is_named_symbol(head->value, SYMBOL_QUOTE))
            {
                return quote(current, environment);
            }
            else if (is_named_symbol(head->value, SYMBOL_QUASI_QUOTE_EXPAND))
            {
                return quasiquote(head->cdr->value);
            }
            else if (is_named_symbol(head->value, SYMBOL_QUASI_QUOTE))
            {
                current = quasiquote(head->cdr->value);
                continue;
            }
        }

        // ast is a list: call eval_ast to get a new evaluated list.
        // Take the first item of the evaluated list and call it as
        // function using the rest of the evaluated list as its arguments.
        MalValue *tmp = eval_ast(current, environment);

        assert(tmp);

        if (is_error(tmp))
        {
            return tmp;
        }

        if (!is_sequence(tmp))
        {
            return make_error("Not callable: %s.", pr_str(tmp, true));
        }

        head = tmp->list;

        if (is_function(head->value))
        {
            return head->value->function(head->cdr);
        }

        if (is_closure(head->value))
        {
            MalClosure *closure = head->value->closure;
            int64_t bindings_count = _count(closure->bindings->list);
            int64_t arg_count = _count(head->cdr);
            fprintf(stderr, print_values(head->cdr)->value);

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

        //        return make_error("Not callable: %s.", pr_str(tmp, true));
        return tmp;
    }
}

void PRINT(MalValue *value)
{
    print(output_stream, value, true);
    fprintf(output_stream, "\n");
}

void rep(const char *input, MalEnvironment *environment, bool print_result)
{
    Reader reader = {.input = input};
    Token token = {};
    reader.token = &token;

    MalValue *value = READ(&reader);

    if (value->valueType != MAL_ERROR)
    {
        MalValue *result = EVAL(value, environment);

        if (print_result)
        {
            PRINT(result);
        }
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
    output_stream = stdout;
    global_environment = make_initial_environment();
    //    rep("(def! not (fn* (a) (if a false true)))", global_environment, false);
    //    rep("(def! load-file (fn* (f) (eval (read-string (str \"(do \" (slurp f) \"\nnil)\"))))))", global_environment, false);
    rep(LISP_LIBRARY, global_environment, false);

    MalValue *args = make_list(NULL);
    set_in_environment(global_environment, make_value(MAL_SYMBOL, "*ARGV*"), args);

    if (argc > 1)
    {
        for (int i = 2; i < argc; i++)
        {
            push(args, make_string(argv[i], false));
        }

        char buffer[strlen(argv[1]) + 1 + 13];
        sprintf(buffer, "(load-file \"%s\")\n", argv[1]);
        rep(buffer, global_environment, false);

        return 0;
    }

    char *input = NULL;
    char *history_file = get_history_filename();
    _read_history(history_file);

    while (1)
    {
        input = _readline("user> ");

        if (input && *input && *input != 0x0)
        {
            _add_history(input);
            rep(input, global_environment, true);
        }
        else
        {
            break;
        }
    }

    _save_history(history_file);
    free(history_file);
    return 0;
}
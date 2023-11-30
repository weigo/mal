#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "reader.h"
#include "symbol.h"
#include "token.h"
#include "types.h"
#include "gc.h"

MalValue *read_form(Reader *reader, bool readNextToken);

/**
 * Create MAL_ERROR using the given error message and input.
 */
MalValue *make_input_error(char *fmt, char *input, size_t len)
{
    char tmp[len + 1];
    strncpy(tmp, input, len);
    tmp[len + 1] = '\0';

    return make_error(fmt, tmp);
}

char peek(char *input)
{
    return *(input + 1);
}

int64_t parse_fixnum(char *value, bool negative)
{
    int64_t result = 0;
    unsigned int len = strlen(value);

    for (int i = 0; i < len; i++)
    {
        result = 10 * result + (value[i] - '0');
    }

    return negative ? -result : result;
}

void fill_token(Token *token, enum TokenType tokenType, char *value, unsigned int len)
{
    token->tokenType = tokenType;

    if (value != NULL)
    {
        token->value = mal_malloc(len + 1);
        token->value[len] = '\0';
        strncpy(token->value, value, len);
    }
    else
    {
        token->value = NULL;
    }
}

enum TokenType next_token(Reader *reader)
{
    while (isspace(*reader->input) || ',' == *reader->input)
    {
        reader->input++;
    }

    char *start = reader->input;
    char ch;

    switch (*start)
    {
    case '\0':
        fill_token(reader->token, TOKEN_EOF, start, 1);
        break;
    case '~':
        if ('@' == peek(start))
        {
            reader->input++;
            fill_token(reader->token, TOKEN_TILDE_AT, start, 2);
        }
        else
        {
            fill_token(reader->token, TOKEN_TILDE, NULL, 1);
        }
        reader->input++;
        break;
    case '[':
        fill_token(reader->token, TOKEN_LEFT_BRACKET, NULL, 1);
        reader->input++;
        break;
    case ']':
        fill_token(reader->token, TOKEN_RIGHT_BRACKET, NULL, 1);
        reader->input++;
        break;
    case '(':
        fill_token(reader->token, TOKEN_LEFT_PAREN, NULL, 1);
        reader->input++;
        break;
    case ')':
        fill_token(reader->token, TOKEN_RIGHT_PAREN, NULL, 1);
        reader->input++;
        break;
    case '{':
        fill_token(reader->token, TOKEN_LEFT_BRACE, NULL, 1);
        reader->input++;
        break;
    case '}':
        fill_token(reader->token, TOKEN_RIGHT_BRACE, NULL, 1);
        reader->input++;
        break;
    case '\'':
        fill_token(reader->token, TOKEN_SINGLE_QUOTE, NULL, 1);
        reader->input++;
        break;
    case '`':
        fill_token(reader->token, TOKEN_BACK_TICK, NULL, 1);
        reader->input++;
        break;
    case '^':
        fill_token(reader->token, TOKEN_CARET, start, 1);
        reader->input++;
        break;
    case '@':
        fill_token(reader->token, TOKEN_AT, NULL, 1);
        reader->input++;
        break;
    case '"':
        reader->input++;

        while ((ch = *reader->input) && '\0' != ch)
        {
            if ('\\' == ch)
            {
                char next = peek(reader->input);

                if ('"' == next || '\\' == next)
                {
                    reader->input++;
                }
            }
            else if ('"' == ch)
            {
                break;
            }

            reader->input++;
        }

        if ('"' == ch)
        {
            // do not include leading and trailing '"'
            fill_token(reader->token, TOKEN_STRING, start + 1, (reader->input - 1) - start);
            reader->input++;
        }
        else
        {
            fill_token(reader->token, TOKEN_UNBALANCED_STRING, start, (reader->input) - start);
        }
        break;
    case '-':
    case '+':
    {
        if (isdigit(peek(reader->input)))
        {
            bool negative = *start == '-';
            reader->input++;

            while (isdigit(*reader->input))
            {
                reader->input++;
            }

            fill_token(reader->token, TOKEN_NUMBER, start + 1, reader->input - (start + 1));
            reader->token->fixnum = parse_fixnum(reader->token->value, negative);
        }
        else
        {
            while ((ch = peek(reader->input)) && ch != '\0' && !isspace(ch))
            {
                reader->input++;
            }
            reader->input++;
            fill_token(reader->token, TOKEN_SYMBOL, start, reader->input - start);
        }
        break;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        while (isdigit(*reader->input))
        {
            reader->input++;
        }

        fill_token(reader->token, TOKEN_NUMBER, start, reader->input - start);
        reader->token->fixnum = parse_fixnum(reader->token->value, false);

        break;
    case ';':
        while ((ch = *++reader->input) && ch != '\0' && ch != '\n')
            ;
        fill_token(reader->token, TOKEN_COMMENT, start, reader->input - start);
        break;
    default:
        bool finish = false;

        while ((ch = peek(reader->input)) && ch != '\0' && !isspace(ch) && !finish)
        {
            switch (ch)
            {
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '\'':
            case '`':
            case '"':
            case ',':
            case ';':
                finish = true;
                break;
            default:
                reader->input++;
                break;
            }
        }

        reader->input++;
        fill_token(reader->token, TOKEN_SYMBOL, start, reader->input - start);
        break;
    }

    return reader->token->tokenType;
}

MalValue *read_list_like(Reader *reader, MalValue *list_like, enum TokenType endToken)
{
    MalValue *value = NULL;
    enum TokenType tokenType;
    char *start = reader->input;

    while ((tokenType = next_token(reader)) != endToken)
    {
        if (tokenType == TOKEN_COMMENT)
        {
            continue;
        }

        if (tokenType == TOKEN_EOF)
        {
            switch (endToken)
            {
            case TOKEN_RIGHT_BRACKET:
                return make_input_error("unbalanced ']' in '[%s'", start, reader->input - start);
                break;
            case TOKEN_RIGHT_PAREN:
                return make_input_error("unbalanced ')' in '(%s'", start, reader->input - start);
                break;

            default:
                return make_input_error("unexpected EOF in '%s'", start, reader->input - start);
                break;
            }
        }

        value = read_form(reader, false);

        assert(value);

        if (is_error(value))
        {
            return value;
        }

        push(list_like, value);
    }

    return list_like;
}

MalValue *read_list(Reader *reader)
{
    return read_list_like(reader, new_value(MAL_LIST), TOKEN_RIGHT_PAREN);
}

MalValue *read_vector(Reader *reader)
{
    return read_list_like(reader, new_value(MAL_VECTOR), TOKEN_RIGHT_BRACKET);
}

MalValue *read_atom(Token *token)
{
    MalValue *value = NULL;

    switch (token->tokenType)
    {
    case TOKEN_TILDE:
        value = make_symbol(token->value);
        break;
    case TOKEN_STRING:
        value = make_string(token->value, true);
        break;
    case TOKEN_COMMENT:
        value = make_value(MAL_COMMENT, token->value);
        break;
    case TOKEN_NUMBER:
        value = make_fixnum(token->fixnum);
        break;
    case TOKEN_UNBALANCED_STRING:
        value = make_error("missing closing quote: '%s'", token->value);
        break;
    case TOKEN_EOF:
        break;
    case TOKEN_SYMBOL:
    default:
        if (':' == *token->value)
        {
            value = make_value(MAL_KEYWORD, token->value);
        }
        else
        {
            value = make_symbol(token->value);
        }
        break;
    }

    return value;
}

MalValue *read_reader_macro(Reader *reader, char *symbol)
{
    MalValue *quote = new_value(MAL_LIST);

    push(quote, make_symbol(symbol));
    push(quote, read_form(reader, true));

    return quote;
}

MalValue *read_hash_map(Reader *reader)
{
    MalValue *map = new_value(MAL_HASHMAP);
    map->hashMap = make_hashmap();
    MalValue *key = NULL;
    MalValue *value = NULL;
    enum TokenType tokenType;
    char *start = reader->input;

    while ((tokenType = next_token(reader)) != TOKEN_RIGHT_BRACE)
    {
        if (tokenType == TOKEN_COMMENT)
        {
            continue;
        }

        if (tokenType == TOKEN_EOF)
        {
            free_hashmap(map->hashMap);
            free(map);

            return make_input_error("unbalanced '}' in '{%s'", start, reader->input - start);
        }

        key = read_form(reader, false);

        // FIXME: return a meaningful error message, don't die.
        assert(key->valueType == MAL_STRING || key->valueType == MAL_SYMBOL || key->valueType == MAL_KEYWORD);

        value = read_form(reader, true);

        put(map, key, value);
    }

    return map;
}

MalValue *read_with_metadata(Reader *reader)
{
    MalValue *metadata = read_form(reader, true);
    assert(metadata->valueType == MAL_HASHMAP);
    MalValue *value = read_form(reader, true);
    setMetadata(value, metadata->hashMap);
    free(metadata);
    MalValue *list = new_value(MAL_LIST);
    push(list, make_symbol(SYMBOL_WITH_META));

    return value;
}

MalValue *read_form(Reader *reader, bool readNextToken)
{
    if (readNextToken)
    {
        next_token(reader);
    }

    MalValue *value = NULL;

    switch (reader->token->tokenType)
    {
    case TOKEN_LEFT_PAREN:
        value = read_list(reader);
        break;
    case TOKEN_LEFT_BRACKET:
        value = read_vector(reader);
        break;
    case TOKEN_BACK_TICK:
        value = read_reader_macro(reader, SYMBOL_QUASI_QUOTE);
        break;
    case TOKEN_SINGLE_QUOTE:
        value = read_reader_macro(reader, SYMBOL_QUOTE);
        break;
    case TOKEN_TILDE:
        value = read_reader_macro(reader, SYMBOL_UNQUOTE);
        break;
    case TOKEN_TILDE_AT:
        value = read_reader_macro(reader, SYMBOL_SPLICE_UNQUOTE);
        break;
    case TOKEN_AT:
        value = read_reader_macro(reader, SYMBOL_DEREF);
        break;
    case TOKEN_LEFT_BRACE:
        value = read_hash_map(reader);
        break;
    case TOKEN_RIGHT_PAREN:
    case TOKEN_RIGHT_BRACKET:
    case TOKEN_RIGHT_BRACE:
    case TOKEN_KOMMA:
    case TOKEN_COMMENT:
        break;
    case TOKEN_CARET:
        value = read_with_metadata(reader);
        break;
    default:
        value = read_atom(reader->token);
    }

    return value;
}

MalValue *read_str(Reader *reader)
{
    return read_form(reader, true);
}

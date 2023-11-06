#ifndef _MAL_TOKEN_H
#define _MAL_TOKEN_H

#include <stdint.h>

enum TokenType
{
    TOKEN_AT,
    TOKEN_BACK_TICK,
    TOKEN_CARET,
    TOKEN_COMMENT,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_KOMMA,
    TOKEN_LEFT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_LEFT_PAREN,
    TOKEN_NUMBER,
    TOKEN_RIGHT_BRACE,
    TOKEN_RIGHT_BRACKET,
    TOKEN_RIGHT_PAREN,
    TOKEN_SINGLE_QUOTE,
    TOKEN_STRING,
    TOKEN_SYMBOL,
    TOKEN_TILDE_AT,
    TOKEN_TILDE,
    TOKEN_UNBALANCED_STRING
};

typedef struct Token
{
    enum TokenType tokenType;
    char *value;
    int64_t fixnum;
} Token;

#endif
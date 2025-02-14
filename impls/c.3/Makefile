CC = gcc

CFLAGS = -std=c99  -Wall
DFLAGS = -g3 #-fsanitize=address

LIBS = -lreadline
FFI_LIBS = -ldl -lffi

SRC = reader.c types.c printer.c env.c core.c gc.c
HEADERS = reader.h types.h printer.h token.h env.h core.h gc.h symbol.h

LIB_DIR = ./libs

LIB_READLINE_H = $(LIB_DIR)/readline/readline.h
LIB_READLINE_SRC = $(LIB_DIR)/readline/readline.c

LIB_MAP_SRC = hashmap.c

LIBS_H = $(LIB_READLINE_H)
LIBS_SRC = $(LIB_MAP_SRC) $(LIB_READLINE_SRC)

S0_SRC = step0_repl.c readline.c
S1_SRC = step1_read_print.c reader.c types.c printer.c token.c gc.c $(LIBS_SRC)
S2_SRC = step2_eval.c $(SRC) $(LIBS_SRC)
S3_SRC = step3_env.c $(SRC) $(LIBS_SRC)
S4_SRC = step4_if_fn_do.c $(SRC) $(LIBS_SRC)
S5_SRC = step5_tco.c $(SRC) $(LIBS_SRC)
S6_SRC = step6_file.c $(SRC) $(LIBS_SRC)
S7_SRC = step7_quote.c $(SRC) $(LIBS_SRC)
S8_SRC = step8_macros.c $(SRC) $(LIBS_SRC)
S9_SRC = step9_try.c $(SRC) $(LIBS_SRC)
SA_SRC = stepA_mal.c $(SRC) $(LIBS_SRC)

S0_HEADERS =
S1_HEADERS = reader.h types.h printer.h token.h gc.h $(LIB_LIST_H)
S2_HEADERS = $(HEADERS) $(LIBS_H)
S3_HEADERS = $(HEADERS) $(LIBS_H)
S4_HEADERS = $(HEADERS) $(LIBS_H)
S5_HEADERS = $(HEADERS) $(LIBS_H)
S6_HEADERS = $(HEADERS) $(LIBS_H)
S7_HEADERS = $(HEADERS) $(LIBS_H)
S8_HEADERS = $(HEADERS) $(LIBS_H)
S9_HEADERS = $(HEADERS) $(LIBS_H)
SA_HEADERS = $(HEADERS) $(LIBS_H)

S0 = step0_repl
S1 = step1_read_print
S2 = step2_eval
S3 = step3_env
S4 = step4_if_fn_do
S5 = step5_tco
S6 = step6_file
S7 = step7_quote
S8 = step8_macros
S9 = step9_try
SA = stepA_mal

all: $(S0) $(S1) $(S2) $(S3) $(S4) $(S5) $(S6) $(S7) $(S8) $(S9) $(SA)

$(S0): $(S0_SRC) $(S0_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S0_SRC) $(LIBS) -o $(S0)

$(S1): $(S1_SRC) $(S1_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S1_SRC) $(LIBS) -o $(S1)

$(S2): $(S2_SRC) $(S2_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S2_SRC) $(LIBS) -o $(S2)

$(S3): $(S3_SRC) $(S3_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S3_SRC) $(LIBS) -o $(S3)

$(S4): $(S4_SRC) $(S4_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S4_SRC) $(LIBS) -o $(S4)

$(S5): $(S5_SRC) $(S5_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S5_SRC) $(LIBS) -o $(S5)

$(S6): $(S6_SRC) $(S6_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S6_SRC) $(LIBS) -o $(S6)

$(S7): $(S7_SRC) $(S7_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S7_SRC) $(LIBS) -o $(S7)

$(S8): $(S8_SRC) $(S8_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S8_SRC) $(LIBS) -o $(S8)

$(S9): $(S9_SRC) $(S9_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(S9_SRC) $(LIBS) -o $(S9)

$(SA): $(SA_SRC) $(SA_HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(SA_SRC) $(LIBS) $(FFI_LIBS) -DWITH_FFI -o $(SA)

.PHONY clean:
	rm -f $(S0) $(S1) $(S2) $(S3) $(S4) $(S5) $(S6) $(S7) $(S8) $(S9) $(SA)

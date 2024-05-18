#include "types.h"
#include "env.h"

/**
 * Get the current package.
 */
MalValue *get_current_package();

/**
 * Initialize the mal package system using the given environment.
 */
// void rep(const char *input, MalEnvironment *environment, bool print_result)
void init_packages(void (*rep)(const char *input, MalEnvironment *environment, bool print_result));

/**
 * Find the MalPackage associated with the given symbol.
 *
 * @return {code}MalValue{code} when a package with the given name has been registered, {code}NULL{code} otherwise.
 */
MalValue *find_package(const MalValue *symbol);

/**
 * Look the given symbol up in the provided package.
 *
 * @return the value found in the given package or {code}NULL{code}.
 */
MalValue *lookup_in_package(MalValue *package, MalValue *symbol);

/**
 * Return the home package of that symbol (a package object or NIL).
 */
MalValue *symbol_package(MalValue *symbol);

/**
 * export makes one or more symbols that are accessible in package (whether directly or by inheritance) be external symbols of that package.
 * If any of the symbols is already accessible as an external symbol of package, export has no effect on that symbol. If the symbol is present in package as an internal symbol, it is simply changed to external status. If it is accessible as an internal symbol via use-package, it is first imported into package, then exported. (The symbol is then present in the package whether or not package continues to use the package through which the symbol was originally inherited.)
 * export makes each symbol accessible to all the packages that use package. All of these packages are checked for name conflicts: (export s p) does (find-symbol (symbol-name s) q) for each package q in (package-used-by-list p). Note that in the usual case of an export during the initial definition of a package, the result of package-used-by-list is nil and the name-conflict checking takes negligible time. When multiple changes are to be made, for example when export is given a list of symbols, it is permissible for the implementation to process each change separately, so that aborting from a name conflict caused by any but the first symbol in the list does not unexport the first symbol in the list. However, aborting from a name-conflict error caused by export of one of symbols does not leave that symbol accessible to some packages and inaccessible to others; with respect to each of symbols processed, export behaves as if it were as an atomic operation.
 *
 * A name conflict in export between one of symbols being exported and a symbol already present in a package that would inherit the newly-exported symbol may be resolved in favor of the exported symbol by uninterning the other one, or in favor of the already-present symbol by making it a shadowing symbol.
 *
 * Examples:
 *
 * (make-package 'temp :use nil) =>  #<PACKAGE "TEMP">
 * (use-package 'temp) =>  T
 * (intern "TEMP-SYM" 'temp) =>  TEMP::TEMP-SYM, NIL
 * (find-symbol "TEMP-SYM") =>  NIL, NIL
 * (export (find-symbol "TEMP-SYM" 'temp) 'temp) =>  T
 * (find-symbol "TEMP-SYM") =>  TEMP-SYM, :INHERITED
 */
void export_symbol(MalValue *package, MalValue *symbol);

/**
 * @brief intern enters a symbol named string into package.
 *
 * <p>intern enters a symbol named string into package. If a symbol whose name is the same as string is already accessible in package, it is returned. If no such symbol is accessible in package, a new symbol with the given name is created and entered into package as an internal symbol, or as an external symbol if the package is the KEYWORD package; package becomes the home package of the created symbol.</p>
 *
 * <p>The first value returned by intern, symbol, is the symbol that was found or created. The meaning of the secondary value, status, is as follows:</p>
 *
 * :internal
 * The symbol was found and is present in package as an internal symbol.
 * :external
 * The symbol was found and is present as an external symbol.
 * :inherited
 * The symbol was found and is inherited via use-package (which implies that the symbol is internal).
 * nil
 * No pre-existing symbol was found, so one was created.
 * It is implementation-dependent whether the string that becomes the new symbol's name is the given string or a copy of it. Once a string has been given as the string argument to intern in this situation where a new symbol is created, the consequences are undefined if a subsequent attempt is made to alter that string.
 *
 * Examples:
 *
 * (in-package "COMMON-LISP-USER") =>  #<PACKAGE "COMMON-LISP-USER">
 * (intern "Never-Before") =>  |Never-Before|, NIL
 * (intern "Never-Before") =>  |Never-Before|, :INTERNAL
 * (intern "NEVER-BEFORE" "KEYWORD") =>  :NEVER-BEFORE, NIL
 * (intern "NEVER-BEFORE" "KEYWORD") =>  :NEVER-BEFORE, :EXTERNAL
 *
 * @param package
 * @param symbol
 * @return MalValue*
 */
MalValue *intern_symbol(MalValue *package, MalValue *symbol);

/**
 * @brief  find-symbol locates a symbol whose name is string in a package.
 * 
 * find-symbol locates a symbol whose name is string in a package. If a symbol named string is found in package, directly or by inheritance, the symbol found is returned as the first value; the second value is as follows:
 *
 * :internal
 * If the symbol is present in package as an internal symbol.
 * :external
 * If the symbol is present in package as an external symbol.
 * :inherited
 * If the symbol is inherited by package through use-package, but is not present in package.
 * If no such symbol is accessible in package, both values are nil.
 *
 * Examples:
 * {code}
 * (find-symbol "NEVER-BEFORE-USED") =>  NIL, NIL
 * (find-symbol "NEVER-BEFORE-USED") =>  NIL, NIL
 * (intern "NEVER-BEFORE-USED") =>  NEVER-BEFORE-USED, NIL
 * (intern "NEVER-BEFORE-USED") =>  NEVER-BEFORE-USED, :INTERNAL
 * (find-symbol "NEVER-BEFORE-USED") =>  NEVER-BEFORE-USED, :INTERNAL
 * (find-symbol "never-before-used") =>  NIL, NIL
 * (find-symbol "CAR" 'common-lisp-user) =>  CAR, :INHERITED
 * (find-symbol "CAR" 'common-lisp) =>  CAR, :EXTERNAL
 * (find-symbol "NIL" 'common-lisp-user) =>  NIL, :INHERITED
 * (find-symbol "NIL" 'common-lisp) =>  NIL, :EXTERNAL
 * (find-symbol "NIL" (prog1 (make-package "JUST-TESTING" :use '())
 *                         (intern "NIL" "JUST-TESTING")))
 * =>  JUST-TESTING::NIL, :INTERNAL
 * (export 'just-testing::nil 'just-testing)
 * (find-symbol "NIL" 'just-testing) =>  JUST-TESTING:NIL, :EXTERNAL
 * (find-symbol "NIL" "KEYWORD")
 * =>  NIL, NIL
 * OR=>  :NIL, :EXTERNAL
 * (find-symbol (symbol-name :nil) "KEYWORD") =>  :NIL, :EXTERNAL
 * {code}
 * 
 * @param symbol_name name of symbol
 * @param package packge to find symbol in
 * @return MalValue* 
 */
MalValue *find_symbol(const char *symbol_name, MalValue *package);

/**
 * @brief 
 * 
 * @param keyword_name 
 * @return MalValue* 
 */
MalValue *make_keyword(const char *keyword_name);
# MAL package system

Packages organize symbol names along with their values (variables, functions).

Together with the package's name this provides a namespace.

### Printer

The printer should serialize a package like this: `#<PACKAGE "<name of package>">`.

### Current Package

The global variable `*package*` holds the *current package*.
This is a symbol in the `system` package.

### Internal Symbols

Internal symbols exist in this package only.

### External Symbols

External symbols reference symbols defined in another package (together with the originating package name).

See for instance the `lisp` package in the following section for the use of external symbols.

## Standard Packages

The package system defines the following standard packages:

| package name | description |
| :-- | :-- |
| `system` | this package provides symbols like `nil`, `true` and all of the functionality implemented throughout steps 1 to A (registered in the core name space) |
| `lisp` | <ul><li>provides all symbols from the `system` package as *external* symbols</li><li>what else</li></ul> |
| `keyword` | all keywords are registered into this package (by the reader or the `keyword` function). |
| `mal-user` | `mal` prompts the user initially in this package as a scratch pad for interaction with the system |

## Interacting with `EVAL`

Until now `EVAL` takes the current environment and passes it to functions for looking up and creating new symbols.

This needs to change to take the current package into account since the symbol lookup not only has to look into the current environment in order to resolve symbols but also into other packages that are used by the current package (external symbols).


## API

Define a new type for mal packages. It should include

* the package name (a mal symbol),
* map of internal symbols (symbol names to symbols)
* map of names of used packages to symbols imported from these packages
* a list of exported mal symbols.

A variable, in the top level environment, `*package*` holds the current package in use.

### Functions
#### `symbol-name`

The `symbol-name` function looks the given symbol up and returns it according to the setting of the global variable `*print-case*`.

#### `defpackage`

The `defpackage` function defines a new package, listing it's dependencies and exported symbols.

The `defpackage` function takes the following arguments:

| Argument | type | description |
|:--|:--| :-- |
| #1 | key word | the package name |
| Optional | list of used packages | first element must be `:use`, the rest of the list contains key words as package names |
| optional | list of symbols to export | first element must be `:export`, the rest of the list contains the symbols to export |

The `defpackage` registers a new instance of type package in the global package registry/environment. A package has its own environment where functions, closures, macros, symbols, keywords, etc. are registered.

The newly created package instance is returned.

#### `in-package`

The `in-package` function changes the environment to the one indicated with the first argument.

| Argument | type | description |
|:--|:--| :-- |
| #1 | key word | the package name to use as current environment |

#### `make-package`

The `make-package` function create a new package.

| Argument | type | description |
|:--|:--| :-- |
| #1 | string | the package name to use for the new package |
| #2 | keyword `:use` | list of packages to import |

#### `find-package`

The `find-package` function looks up the given package name.

| Argument | type | description |
|:-- |:--| :-- |
| #1 | string or quoted symbol | the package name lookup |

Returns the package instance or `nil`.


# MAL package system

A package has its own environment where functions, closures, macros, symbols, keywords, etc. are registered.

When mal is executed it starts with the core ns environment (introduced in step2, expanded in step3 and step4). This environment contains the functions defined thus far in step1 to stepA of the `make a lisp` process.

When all intialisation is done, mal switches to the `mal-user` environment (package) which has (implicit) access to the symbols from the core namespace. It ':use's the core environment/package.

## API

Define a new type for mal packages. It should include

* the package name (a mal symbol),
* a list of used packages (mal symbols),
* a list of exported mal symbols.

A variable in the top level environment `*package*` holds the current package in use.

### Printer

The printer should serialize a package aslike this: `#<PACKAGE "<name of package>">`.

### Functions
#### `defpackage`

The `defpackage` function defines a new package, listing it's dependencies and exported symbols.

The `defpackage` function takes the following arguments:

| Argument | type | description |
|:--:|:--:| :--: |
| #1 | key word | the package name |
| Optional | list of used packages | first element must be `:use`, the rest of the list contains key words as package names |
| optional | list of symbols to export | first element must be `:export`, the rest of the list contains the symbols to export |

The `defpackage` registers a new instance of type package in the global package registry/environment. A package has its own environment where functions, closures, macros, symbols, keywords, etc. are registered.

The newly created package instance is returned.

#### `in-package`

The `in-package` function changes the environment to the one indicated with the first argument.

| Argument | type | description |
|:--:|:--:| :--: |
| #1 | key word | the package name to use as current environment |

#### `make-package`

The `make-package` function create a new package.

| Argument | type | description |
|:--:|:--:| :--: |
| #1 | string | the package name to use for the new package |
| #2 | keyword `:use` | list of packages to import |

#### `find-package`

The `find-package` function looks up the given package name.

| Argument | type | description |
|:--:|:--:| :--: |
| #1 | string or quoted symbol | the package name lookup |

Returns the package instance or `nil`.
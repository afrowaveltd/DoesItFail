# Lesson 01 — From C source to a reusable static library

**Project:** DoesItFail (C17), consumed by CBox  
**Audience:** C learners with a working Debian/WSL GCC + GNU Make toolchain  
**Status:** Educational explanation of the current source tree, not a new API or an ABI promise.

## Learning objectives

After this lesson you should be able to explain:

1. Why a public header (`.h`) and an implementation (`.c`) are separate.
2. What a relocatable object file (`.o`) contains.
3. What the archive `libdoesitfail.a` contains, and how `ar` creates it.
4. Why a compiler can accept a function call but the linker can still report an undefined reference.
5. How CBox uses the library without copying DoesItFail's source into its own source files.

Keep the educational sequence: **mechanism → inspection → learner explanation → next step**. No need to change production code today.

## The actual files in this repository

| File | Responsibility |
| --- | --- |
| `include/does_it_fail/test.h` | Public declarations: `dif_run_suite`, `dif_expect_int_eq`, callback types, events, summary |
| `src/test.c` | Implementation of runner and integer check |
| `include/does_it_fail/reporter_console.h` | Public console reporter declarations |
| `src/reporter_console.c` | Console reporter implementation |
| `build/test.o` | Compiler output from `src/test.c` |
| `build/reporter_console.o` | Compiler output from `src/reporter_console.c` |
| `build/libdoesitfail.a` | Static archive combining the two object files |
| `tests/test_core.c` | An executable consumer (after compiling and linking) |
| CBox: `tests/does_it_fail_smoke.c` | Independent project consuming the public API and archive |

The `build/` directory is generated. It is not source code and is excluded from Git.

## Step A — `.h` is a declaration, not an implementation

The following public declaration from `test.h` lets the compiler check arguments and the return type:

```c
dif_summary dif_run_suite(const char *suite_name,
                          const dif_test_case *tests,
                          size_t count,
                          dif_reporter reporter,
                          void *user_data);
```

The corresponding body is defined in `src/test.c`. A source file includes the header with:

```c
#include "does_it_fail/test.h"
```

The compiler's **preprocessor** handles `#include` first; the header's text is made part of the translation unit. The header is not normally compiled into a standalone object file.

The command-line option `-Iinclude` adds this repository's `include/` directory to the header search path, making the nested path `does_it_fail/test.h` resolvable.

## Step B — `.c` becomes `.o`

From the DoesItFail repository:

```sh
make -B all
```

The `-B` option asks GNU Make to rebuild even if the targets appear current; this is useful to observe the exact commands. The relevant compilation commands from the Makefile are equivalent to:

```sh
cc -Iinclude -std=c17 -O0 -g -Wall -Wextra -Wpedantic \
   -c src/test.c -o build/test.o

cc -Iinclude -std=c17 -O0 -g -Wall -Wextra -Wpedantic \
   -c src/reporter_console.c -o build/reporter_console.o
```

**`-c` is the critical switch:** compile to an object file, but do not link an executable. A relocatable `.o` can contain machine instructions, data, symbol definitions, symbol references, and relocation information. It is **not directly executable**.

**`-o` names the output file.** For this command it produces `build/test.o`.

## Step C — `.a` is an archive, not a new translation

The Makefile executes:

```sh
ar rcs build/libdoesitfail.a build/test.o build/reporter_console.o
```

The archive collects already-compiled object files; it does not translate the C source again.

Inspect its contents:

```sh
ar t build/libdoesitfail.a
```

Expected member names:

```text
test.o
reporter_console.o
```

The archive allows the linker to extract needed object members when building an executable. Archive member selection, library ordering, and platform details matter; linking is **not** the same as a runtime module loader.

## Step D — The linker resolves references

Our bootstrap test program compiles and links using the explicit archive path:

```sh
cc -Iinclude -std=c17 -O0 -g -Wall -Wextra -Wpedantic \
   tests/test_core.c build/libdoesitfail.a -o build/test_core
```

The `cc` *driver* performs compilation of the test's `.c` and invokes the linker. The test uses declarations from `test.h`, while the linker connects referenced functions such as `dif_run_suite` to definitions in the archive.

When a header declares a function but the linked inputs contain no matching definition, compilation may succeed while linking fails with **undefined reference**. Conversely, a wrong declaration can cause a compiler diagnostic before linking.

Run the completed test executable:

```sh
make test
```

This bootstrapping suite deliberately uses the C standard `assert` so the testing framework's own tests do not depend entirely on the framework being tested.

## Step E — Inspect symbols, not just filenames

From the repository root:

```sh
nm -g build/test.o
nm -g build/libdoesitfail.a
```

In common GNU `nm` output:

- `T` denotes a global symbol defined in the text/code section, e.g. `dif_run_suite`.
- `U` denotes a symbol **undefined in that object**, to be resolved by linking against another object/library. `U` does not mean the build has failed.
- Actual output may vary across platforms and toolchains.

In `src/test.c`, the helper `emit` is declared `static`, so it has internal linkage and is not exposed as a public global symbol in `nm -g`.

## Step F — Verify the independent consumer

Keep sibling repositories:

```text
Afrowave Projects/
    CBox/
    DoesItFail/
```

In the CBox checkout:

```sh
make test-framework
```

This target asks DoesItFail to build `libdoesitfail.a`, then compiles CBox's own `tests/does_it_fail_smoke.c` and links it against the archive. It does **not** test CBox's learner-owned `result.c`; that remains a separate exercise.

## Why not directly include `test.c` in CBox?

`#include` of `.c` files can accidentally duplicate definitions, blur ownership, and complicate builds. A public header plus a separately built library establishes a clean, testable boundary.

A static `.a` differs from a shared `.so`: the former is used to resolve/copy object code at link time; the latter typically remains an external shared object loaded when the executable starts or later. The exact behavior is platform-specific.

## A glimpse of the next lesson: function pointers

The actual header declares:

```c
typedef void (*dif_test_function)(dif_test_context *context);
```

And the runner calls the registered function from its test list:

```c
tests[i].function(&context);
```

This is **a function pointer**, not a pointer to ordinary data. The test functions are compiled into CBox (or another consumer), while DoesItFail knows only the agreed function signature. This is how one generic runner executes tests it has never seen before.

We will explore the memory model, type matching, and callback/reporting path in Lesson 02.

## Practical checkpoint — answer before reading ahead

1. What does `-c` prevent the compiler driver from doing?
2. Why can `tests/test_core.c` compile when the body of `dif_run_suite` is not in the same source file?
3. What files should `ar t build/libdoesitfail.a` display?
4. What is the difference between `T` and `U` in GNU `nm`?
5. If you edit `src/test.c`, which target(s) need rebuilding before the test program reflects the change?

**Experiment:** run `make -B all`, `ar t build/libdoesitfail.a`, `nm -g build/test.o`, and `make test` in order, then explain the output in your own words. No source edits are necessary.

## References within the project

- [Public API](API.md)
- [Development environment](DEVELOPMENT.md)
- [Architecture](ARCHITECTURE.md)
- [CBox integration guide](https://github.com/afrowaveltd/CBox/blob/main/Docs/DOES_IT_FAIL_INTEGRATION.md)

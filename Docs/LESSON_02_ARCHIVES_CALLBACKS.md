# Lesson 02 — Static archives and function pointers in DoesItFail

**Level:** C17 learner, after [Lesson 01](LESSON_01_COMPILATION_LINKING.md)  
**Scope:** Existing source only: no implementation changes or new APIs.  
**Method:** Explain the mechanism, inspect the real source, perform an experiment, then answer questions.

## What we already know

Imagine two source files:

```c
/* calculator.h */
int add(int a, int b);

/* main.c */
#include "calculator.h"

int main(void)
{
    return add(2, 3);
}
```

Compiling `main.c` with `-c` can succeed because the declaration supplies the function signature. But if the final link step receives only `main.o` and no other input defining `add`, it fails with an undefined reference. The compiler driver does not automatically look through every `.o` file in the current directory.

Correct the link by explicitly supplying an object file or a library defining `add`:

```sh
cc main.o add.o -o calculator
```

The same idea drives the CBox / DoesItFail integration.

## 1. Why archive files end in `.a`

Our actual DoesItFail Makefile defines:

```makefile
LIBRARY = $(BUILD_DIR)/libdoesitfail.a
OBJS = $(BUILD_DIR)/test.o $(BUILD_DIR)/reporter_console.o

$(LIBRARY): $(OBJS)
	$(AR) rcs $@ $^
```

On Debian, this results in an `ar` invocation equivalent to:

```sh
ar rcs build/libdoesitfail.a build/test.o build/reporter_console.o
```

`ar` is an **archiver**, not a C compiler. It stores separately compiled object files as members of a single archive. Inspect the members:

```sh
make all
ar t build/libdoesitfail.a
```

Expected:

```text
test.o
reporter_console.o
```

The linker searches the archive to resolve external symbols referenced by already-seen input files; it commonly selects needed **object members**, not individual C functions. On traditional GNU/Unix one-pass archive processing, place the consumer's object files **before** the archive:

```sh
cc -Iinclude -std=c17 tests/test_core.c build/libdoesitfail.a -o build/test_core
```

This is **link-time** integration. The `.a` archive is not a separate service or interpreter running beside the executable.

## 2. Function pointers have types

The exact public header `include/does_it_fail/test.h` contains:

```c
typedef struct dif_test_context dif_test_context;
typedef void (*dif_test_function)(dif_test_context *context);
```

Read the second declaration **from the inside out**:

- `dif_test_function` is the name of the type alias.
- `*dif_test_function` describes a **pointer**.
- `(dif_test_context *context)` is the function's argument list.
- `void` means that calling the function returns no value.

The first declaration is a forward declaration of an **incomplete structure type**. Consumers can work with `dif_test_context *` without seeing the structure's fields. The full structure is private to `src/test.c`. This avoids coupling test implementations to the framework's internal state.

A function matching the type already exists in `examples/console_demo.c`:

```c
static void example_pass(dif_test_context *context)
{
    DIF_EXPECT_INT_EQ(context, 5, 5);
}
```

The test function returns `void`, takes one `dif_test_context *`, and has **internal linkage** because of `static`. That does **not** prevent passing its function pointer into a library. The library invokes the pointer, not a separately linked global name `example_pass`.

### Passing a pointer is not calling a function

```c
/* Pass the function pointer: does NOT execute the test here. */
{"example_pass", example_pass}

/* Conceptually equivalent expression for the function pointer: */
{"example_pass", &example_pass}

/* Calling would use parentheses and a valid context: */
example_pass(context);
```

For the actual existing test registry the first form is used; the function expression converts to a pointer in this context.

Do **not** cast unrelated function pointer types to fit the signature. Calling a function through an incompatible function pointer type can cause undefined behavior.

## 3. Our registry stores names and function pointers

The actual header defines:

```c
typedef struct
{
    const char *name;
    dif_test_function function;
} dif_test_case;
```

The demo registers tests:

```c
const dif_test_case successful[] =
{
    {"example_pass", example_pass}
};
```

There are two separate pieces of information:

1. `name` is a borrowed C string for reporting and identification.
2. `function` is a pointer to executable test code.

Changing the display name does not change which function executes. The same function can appear under different names (the demo's failing suite registers `example_pass` more than once).

## 4. The runner calls a function it did not know when compiled

In `src/test.c`, `dif_run_suite` validates the test entries, builds a temporary test context, and performs:

```c
tests[i].function(&context);
```

Here:

- `tests[i]` selects the registered test case.
- `.function` retrieves its function pointer.
- `(&context)` calls the pointed-to function and passes the **address** of the current context.

This is an **indirect call**. The runner is generic and does not hard-code the test functions. The consumer compiles its own functions; the compiled program passes their pointers to DoesItFail at runtime.

The context is a local variable within the runner and is borrowed for the duration of the call. A test must not retain that pointer after it returns.

**Why this matters:** our `libdoesitfail.a` can remain unchanged when CBox adds a new test. Only the consuming test executable must be rebuilt with the new registry/code.

## 5. A second function pointer is the reporter callback

The header also defines:

```c
typedef void (*dif_reporter)(const dif_event *event, void *user_data);
```

The runner invokes a reporter via an internal helper:

```c
if (reporter != NULL)
{
    reporter(event, user_data);
}
```

This is the same function-pointer mechanism, but for reporting rather than executing tests.

- `const dif_event *event`: a temporary, read-only event.
- `void *user_data`: an opaque caller-supplied context; a concrete reporter may cast it back to its own expected type.

The current API documents these pointers as borrowed. A reporter must not store the event pointer for use after the callback returns. The runner is synchronous and does not create threads.

With separate reporters we can keep test logic independent of console output. The existing console reporter uses `stdio`; ncurses is **not yet implemented**.

## 6. Manual experiment — inspect without editing source

In the DoesItFail repository:

```sh
make -B all
ar t build/libdoesitfail.a
nm -g build/test.o | grep dif_run_suite
make test
make demo
```

On typical GNU `nm`, `T dif_run_suite` means that `build/test.o` **defines** this global function. A `U` in a consuming object indicates an unresolved *reference* that the linker can resolve later. No fixed address is expected in the output.

To explicitly see the unresolved symbol in a consumer **before linking**, try:

```sh
cc -std=c17 -Iinclude -c tests/test_core.c -o build/test_core_unlinked.o
nm -g build/test_core_unlinked.o | grep dif_run_suite
```

Expected symbol class: `U dif_run_suite`. The compilation succeeds because `test.h` provides the prototype; linking will succeed after supplying the archive.

The intentionally failing demo can be run with `make demo-fail`; it is expected to return a **nonzero** status and therefore should not be used as a green CI target.

## 7. Questions to answer before we advance

1. Why does the linker usually need `build/libdoesitfail.a` **after** `test_core.o` on the command line?
2. In `{"example_pass", example_pass}`, does `example_pass` execute immediately? Why?
3. What is the purpose of `typedef void (*dif_test_function)(dif_test_context *context);`?
4. What exactly happens when `tests[i].function(&context);` runs?
5. Why can the callback function be declared `static` and still be invoked by a library?
6. Why must a test not keep the `dif_test_context *` after it returns?

## Next lesson

Explore the stack lifetime of `dif_test_context`, callbacks and `void *user_data`; then build a small new test **written by the learner** with DoesItFail rather than adding a large new implementation.

## See also

- [Lesson 01](LESSON_01_COMPILATION_LINKING.md)
- [Public API](API.md)
- [Architecture](ARCHITECTURE.md)
- [CBox integration](https://github.com/afrowaveltd/CBox/blob/main/Docs/DOES_IT_FAIL_INTEGRATION.md)

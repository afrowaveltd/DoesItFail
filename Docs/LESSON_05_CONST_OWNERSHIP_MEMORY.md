# Lesson 05 — `const`, ownership, `malloc` and `free` in C17

**Series:** [Lesson 04 — Struct values and pointers](LESSON_04_STRUCT_VALUES_POINTERS.md) → Lesson 05.
**Purpose:** Explain how to control mutation, ownership and lifetime in library APIs. Documentation only: no DoesItFail implementation or API change.

## Review: the pointer modifies the original

```c
printer printer_a = {10};
printer *ptr = &printer_a;
ptr->printed_count = 20;
```

The correct answer is **B**: `printer_a.printed_count` is now `20`. The value copied into `ptr` designates the original `printer_a` object; `ptr->printed_count` accesses that object rather than a copy of the structure.

## Three questions for every pointer

1. **Lifetime:** Is the pointed-to object still alive?
2. **Mutation:** Can the function modify the object through this pointer?
3. **Ownership:** Who is responsible for eventually freeing an allocation, if any?

In C, `const` helps with the second question; neither `const` nor a bare pointer automatically solves the other two. Ownership is an API contract, not a built-in C language feature.

## `const` and pointer declarations

| Declaration | What is restricted? |
| --- | --- |
| `const printer *p` | Cannot change `printer` members through `p` (unless there are pointer-member subtleties). |
| `printer *const p` | Cannot assign a new pointer value to `p`; may change the pointed-to `printer`. |
| `const printer *const p` | Cannot change the pointer value or the pointed-to structure through it. |

Example:

```c
void printer_show(const printer *self)
{
    if (self != NULL)
    {
        printf("Printed: %u\n", self->printed_count);
    }
}
```

`const printer *` neither establishes ownership nor guarantees that another pointer cannot change the same mutable `printer`. It is not a lock or a thread-safety guarantee. With a pointer member (e.g. `char *name`), const-qualifying the structure does not recursively make the dynamically allocated pointee constant.

## Automatic storage is often enough

```c
int main(void)
{
    printer local_printer = {0};
    printer_show(&local_printer);
    return 0;
}
```

No `malloc` is needed if the object can live in this scope and we can pass its address only while it is alive. Prefer this when it keeps ownership and lifetime simple.

## Allocate an object when its lifetime requires it

`malloc(size)` requests a region of *allocated storage* and returns `void *` (implicitly convertible to an object pointer in C), or `NULL` on failure. Its bytes are uninitialized, so do not read object members before initialization. A successful allocation remains alive until properly freed or reallocated. `free` must be used only on `NULL` or appropriate pointers returned from allocation functions, not on automatic variables.

### Complete learning program (`printer.c`)

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    unsigned printed_count;
} printer;

static void printer_print(printer *self)
{
    if (self != NULL)
    {
        ++self->printed_count;
    }
}

static void printer_show(const printer *self)
{
    if (self != NULL)
    {
        printf("Printed: %u\n", self->printed_count);
    }
}

int main(void)
{
    printer *owned = malloc(sizeof *owned);

    if (owned == NULL)
    {
        fputs("Allocation failed\n", stderr);
        return EXIT_FAILURE;
    }

    *owned = (printer){0};

    printer_print(owned);
    printer_print(owned);
    printer_show(owned);

    free(owned);
    owned = NULL;

    return EXIT_SUCCESS;
}
```

Compile and run in an independent scratch directory:

```sh
cc -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Wshadow printer.c -o printer
./printer
```

Expected:

```text
Printed: 2
```

The program was compiled and executed separately on a C17 compiler using these flags, and also under AddressSanitizer/UndefinedBehaviorSanitizer in this learning session. This does not claim platform-wide verification.

`sizeof *owned` measures one object of the type `owned` points at, so it stays in sync if the pointer's type changes. The cast `(printer){0}` is a C **compound literal**, whose value initializes the allocated structure. Neither a C++-style `new` nor an explicit cast of `malloc`'s result is needed in C.

## Where does ownership live?

In the example, `main` owns the allocation via the variable `owned`. `printer_print` and `printer_show` **borrow** the pointer only for their calls and must not free it or retain it beyond the object lifetime. `printer_show` receives a read-only view through its `const printer *` parameter.

Ownership can be **transferred** to another function/object, but then that contract must explicitly say who later calls `free`. A copy of a pointer is an **alias**, not a second independent allocation.

## After `free`: why `NULL` is not a universal cure

```c
printer *borrowed = owned;
free(owned);
owned = NULL;
/* borrowed must no longer be used to access the freed object. */
```

The assignment only updates `owned`. It does not reset any aliases. Do not dereference or pass freed-memory pointers as live objects; use-after-free is undefined behavior. `free(NULL)` is permitted and performs no action. Calling `free` twice on the same allocation without a new valid allocation in between is undefined behavior.

### Typical failure categories

- **Leak:** an allocation becomes unreachable without a release strategy.
- **Double free:** attempting to free the same allocation more than once.
- **Use after free:** accessing an object whose allocated lifetime ended.
- **Invalid free:** calling `free` on memory that wasn't returned by an appropriate allocation call (for example, an ordinary local variable).

Even code that seems to work can contain undefined behavior; avoid experimenting by intentionally dereferencing freed storage. On supported GCC/Clang setups, `-fsanitize=address,undefined` can help diagnose many memory errors, though it is not a proof of correctness.

## Link back to DoesItFail

In the real public [`include/does_it_fail/test.h`](../include/does_it_fail/test.h):

```c
typedef void (*dif_reporter)(const dif_event *event, void *user_data);
```

The `const dif_event *event` is a **borrowed, read-only pointer**. DoesItFail documents that a reporter must not retain this event pointer: the event is temporary and delivered synchronously. The `void *user_data` pointer is also borrowed; the caller decides what it points to and how long it lives.

Likewise, each `dif_test_context *` passed to test functions is borrowed for the synchronous invocation, not owned by the test. The current runner allocates the context as an automatic object inside the test loop — it does **not** call `malloc` for this context.

**No production change is required just to demonstrate `malloc`**. Keep examples separate from the working test framework.

## Questions to check understanding

1. What does `const printer *self` prevent, and what does it **not** guarantee?
2. Why do we check whether `malloc(sizeof *owned)` returned `NULL`?
3. Why must `*owned` be initialized before reading `owned->printed_count`?
4. Which function owns the memory in the complete example, and which functions only borrow it?
5. Why does `owned = NULL` not make all other pointers to the same freed object safe?

**Next:** Borrowed `void *user_data`, callbacks, safe ownership transfer and simple failure-path cleanup.

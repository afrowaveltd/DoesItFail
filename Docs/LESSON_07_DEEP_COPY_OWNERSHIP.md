# Lesson 07 — Deep copy, constructors, destructors and failure-safe ownership

**Series:** [Lesson 06 — Embedded arrays versus pointers](LESSON_06_ARRAYS_VS_POINTERS.md) → Lesson 07.  
**Language:** ISO C17, educational example. **No change to the DoesItFail runtime or public API.**

## Recap: the answer is B

```c
typedef struct
{
    char name[10];
} person_inline;

person_inline a = {"Bolaji"};
person_inline b = a;
b.name[0] = 'X';
```

The `name[10]` array is embedded in the structure. Assigning the whole structure copies its array members' values, so `a.name` remains `"Bolaji"` and `b.name` becomes `"Xolaji"`.

By contrast, if the structure contains `char *name`, assigning the structure duplicates the **pointer value** only. Both struct variables may now refer to the *same* character allocation. This is a shallow copy; it is not independently owned dynamic storage.

## What does "deep copy" mean here?

For a structure with one owned `char *name`, deep copying means allocating a **new structure and a new character buffer**, then copying the characters. After cloning, modifying the clone's name cannot modify the original's name.

Ownership is an **API contract**, not a feature the C compiler enforces. In this lesson, each successful `person_create` or `person_clone` returns a pointer **owned by its caller**, and `person_destroy` releases both the string allocation and the containing structure allocation.

## Our three functions

| API | On success | On failure | Caller responsibility |
| --- | --- | --- | --- |
| `person_create(id, name)` | Returns a new owned `person *` with a separate copied name | `NULL`, with any partial allocation already released | Call `person_destroy` once |
| `person_clone(source)` | Returns a deep copy with separate storage | `NULL`, leaves source unchanged | Call `person_destroy` once for clone |
| `person_destroy(self)` | Frees `self->name` and `self` | Accepts `NULL` without action | No further access through aliases |

`person_clone` also treats a NULL source or missing source name as invalid input and returns NULL. `name` is expected to be a valid, NUL-terminated C string.

## Complete standalone program: `person.c`

```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int id;
    char *name;
} person;

person *person_create(int id, const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }

    person *created = malloc(sizeof *created);
    if (created == NULL)
    {
        return NULL;
    }

    size_t bytes = strlen(name) + 1;
    created->name = malloc(bytes);
    if (created->name == NULL)
    {
        free(created);
        return NULL;
    }

    memcpy(created->name, name, bytes);
    created->id = id;
    return created;
}

person *person_clone(const person *source)
{
    if (source == NULL || source->name == NULL)
    {
        return NULL;
    }

    return person_create(source->id, source->name);
}

void person_destroy(person *self)
{
    if (self == NULL)
    {
        return;
    }

    free(self->name);
    free(self);
}

int main(void)
{
    person *original = person_create(1, "Bolaji");
    if (original == NULL)
    {
        fputs("Unable to create person\n", stderr);
        return EXIT_FAILURE;
    }

    person *copy = person_clone(original);
    if (copy == NULL)
    {
        person_destroy(original);
        fputs("Unable to clone person\n", stderr);
        return EXIT_FAILURE;
    }

    assert(original != copy);
    assert(original->name != copy->name);

    copy->name[0] = 'X';
    printf("Original: %s\n", original->name);
    printf("Clone:    %s\n", copy->name);

    assert(strcmp(original->name, "Bolaji") == 0);
    assert(strcmp(copy->name, "Xolaji") == 0);

    person_destroy(copy);
    copy = NULL;
    person_destroy(original);
    original = NULL;

    return EXIT_SUCCESS;
}
```

### Build and run

```sh
gcc -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Wshadow person.c -o person
./person
```

Expected:

```text
Original: Bolaji
Clone:    Xolaji
```

This example was compiled and executed with **Debian GCC 14.2.0** and also with the **AddressSanitizer and UndefinedBehaviorSanitizer** flags below, producing the expected output in both runs:

```sh
gcc -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Wshadow \
    -fsanitize=address,undefined -fno-omit-frame-pointer person.c -o person_asan
ASAN_OPTIONS=detect_leaks=1 ./person_asan
```

Sanitizers can detect many errors in exercised paths; passing them is not a proof of correctness in all environments.

## Why `person_create` has two allocations

A `person` with `char *name` contains the pointer value, not the characters. We allocate one object for `person` and one buffer for the NUL-terminated string. `sizeof *created` ensures the first request is the size of one `person`. `strlen(name) + 1` includes the trailing NUL terminator; `memcpy` transfers exactly those bytes.

The `const char *name` parameter is **borrowed** and read-only through that pointer. `person_create` copies the bytes and returns a completely independent owned allocation. The caller is allowed to destroy or reuse its source string after a successful clone, provided the source's own rules allow that.

Do not use `strdup` as if it were an ISO C17 function: it is available in POSIX and newer C contexts, but this example deliberately uses `malloc` and `memcpy` for the C17 baseline.

## Why the failure branch must free the first allocation

Consider:

```c
person *created = malloc(sizeof *created);
/* This succeeds ... */
created->name = malloc(bytes);
/* ... but this second request may fail. */
```

At that point, `created` is already owned by `person_create`, but there is no valid string allocation. The implementation handles the failure:

```c
if (created->name == NULL)
{
    free(created);
    return NULL;
}
```

Returning NULL without `free(created)` would leak the first allocation. The API promises that failed creation leaves **no partially owned object** for the caller to clean up.

Similarly, if cloning fails in `main`, the caller cleans up the already-owned `original` before exiting.

## `person_destroy` is the inverse of creation

```c
void person_destroy(person *self)
{
    if (self == NULL)
    {
        return;
    }

    free(self->name);
    free(self);
}
```

First free the owned nested allocation, then free the enclosing structure. Reversing these operations and then reading `self->name` would access freed storage and be invalid.

`person_destroy(copy)` **does not** set `copy` in the calling function to NULL: C passes the pointer *value* by value. This exercise explicitly assigns `copy = NULL` afterward. That protects only that single pointer variable, not any aliases. Later we can study `person_destroy(person **owner)` for caller-side pointer clearing.

Never call this destroy function with a pointer to an automatic structure (`person p; person_destroy(&p);`) because it attempts `free` on storage not obtained by dynamic allocation. Do not call it twice on the same owned pointer.

## Memory diagram and four allocations

After `person_create` and `person_clone` both succeed:

```text
original ──> [ person {id=1, name=0x...} ] ──> [ 'B','o','l','a','j','i','\0' ]
copy     ──> [ person {id=1, name=0x...} ] ──> [ 'B','o','l','a','j','i','\0' ]
```

There are **four distinct dynamic allocations**: two structures and two strings. The `clone` owns its own separate allocations. This simple approach favors clear ownership over minimizing allocation count.

## Connection to DoesItFail and future AjisFS

The actual DoesItFail runner holds a `dif_test_context` as an automatic local inside its test loop, lending the address to test callbacks. **It does not use `person_create` or allocate that context on the heap.** The `dif_event` pointer in reporter callbacks is also borrowed and short-lived; a reporter that needs to retain information must copy the values (and any relevant strings according to their lifetimes).

For a future AjisFS block with large or variable-length content, independent buffers might be necessary; alternatively we can group allocations or use offsets into a contiguous arena. The appropriate design depends on the workload and should be measured. `malloc` pointers must not be serialized into persistent disk records.

## Mini-exercises

1. Predict the output after replacing `person_clone(original)` with **only** a pointer assignment `person *copy = original;`. Why would the current cleanup then be incorrect? **Do not run the double-free variant.**
2. Explain why `person_clone` takes `const person *source` rather than `person *source`.
3. Find every branch that can return NULL and identify what it owns at that point.
4. Explain the difference between `free(self->name); free(self);` and the unsafe reverse order.
5. Why is `copy = NULL` a helpful but incomplete defense against dangling pointers?

## Next lesson

Build a small test around `person_clone`, then learn failure injection and cleanup patterns. A second useful topic is `void *user_data` in DoesItFail, showing how borrowed context data reaches a callback.

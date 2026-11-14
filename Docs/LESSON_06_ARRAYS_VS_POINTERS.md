# Lesson 06 — Embedded arrays versus pointers in C structures

**Series:** [Lesson 05 — `const`, ownership and dynamic memory](LESSON_05_CONST_OWNERSHIP_MEMORY.md) → Lesson 06.
**C version:** C17. **Status:** Learning-only documentation; no production DoesItFail source or API changes.

## Review question

```c
typedef struct
{
    int number;
    char name[10];
} sample;

sample data;
```

**Correct answer: B.** The ten `char` elements occupy storage **inside** the `sample` object. A fixed-size array member does **not** automatically allocate separate storage on the heap.

Note that `sample data;` inside a function without an initializer leaves its members uninitialized. Construct the object with a suitable initializer before reading its values, for example `sample data = {0};`.

## 1. Direct embedded array

```c
typedef struct
{
    int id;
    char name[10];
} person_inline;

person_inline p = {1, "Bolaji"};
```

`name` is an array of **exactly ten `char` elements**, part of the same structure object. With this initializer, the characters are `B`, `o`, `l`, `a`, `j`, `i`, `\0`, then three additional zero bytes. The array needs no independent `malloc` or `free`.

The struct as a whole is one object with consecutive member storage, potentially with padding for alignment. On a typical 64-bit GCC ABI with four-byte `int`, `sizeof(person_inline)` is 16 bytes (4 for `id`, 10 for `name`, and 2 trailing padding), but exact size/alignment depend on implementation.

If an inline structure is automatic, the member array exists within that automatic object. If the enclosing structure is dynamically allocated, its array exists inside the *same* dynamic allocation.

## 2. Pointer member

```c
typedef struct
{
    int id;
    char *name;
} person_pointer;

person_pointer p = {1, NULL};
```

Here the structure contains only the **pointer member**, not an inline character buffer. The pointer may designate a dynamically allocated array, an existing buffer elsewhere, or a string literal (but a pointer to a literal must not be used to modify or `free` its bytes). The pointer itself is part of the structure.

Example with owned dynamically allocated string storage:

```c
const char *source = "Bolaji";
size_t length = strlen(source) + 1; /* Include the NUL terminator. */

char *name = malloc(length);
if (name == NULL)
{
    /* Handle allocation failure; do not dereference name. */
}
else
{
    memcpy(name, source, length);
    /* Store in a pointer-bearing structure and free once when done. */
    free(name);
}
```

`malloc` returns uninitialized storage. `memcpy` here copies a known valid string including its terminator. The input text is known and its size is measured, avoiding buffer overflow. The `strlen(source) + 1` pattern assumes a valid NUL-terminated source string and sufficient representable size.

## 3. Struct copies: inline arrays versus aliases

An entire C structure is assignable, even if it includes an array member. The embedded array **participates in the structure value copy**. But a pointer member copies only the pointer value and leaves its pointed-to data shared.

### Complete, safe exercise program (`person.c`)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int id;
    char name[10];
} person_inline;

typedef struct
{
    int id;
    char *name;
} person_pointer;

int main(void)
{
    person_inline a = {1, "Bolaji"};
    person_inline b = a;
    b.name[0] = 'X';
    printf("Embedded: a=%s, b=%s\n", a.name, b.name);

    const char *source = "Bolaji";
    size_t length = strlen(source) + 1;
    char *allocated_name = malloc(length);

    if (allocated_name == NULL)
    {
        fputs("Allocation failed\n", stderr);
        return EXIT_FAILURE;
    }

    memcpy(allocated_name, source, length);

    person_pointer c = {1, allocated_name};
    person_pointer d = c;
    d.name[0] = 'X';
    printf("Pointer: c=%s, d=%s\n", c.name, d.name);

    free(c.name);
    c.name = NULL;
    d.name = NULL;
    return EXIT_SUCCESS;
}
```

Compile and run on Debian:

```sh
cc -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Wshadow person.c -o person
./person
```

Expected output:

```text
Embedded: a=Bolaji, b=Xolaji
Pointer: c=Xolaji, d=Xolaji
```

This code was compiled and executed on Debian GCC 14.2.0 with C17 warnings and AddressSanitizer/UndefinedBehaviorSanitizer enabled in the learning session, producing the expected output. This does not prove correctness for every environment.

### Ownership contract for this example

- `c.name` and `d.name` have the **same pointer value** after `d = c`; they are aliases to one allocation.
- For this exercise, **`c` is designated owner** and `d` is a temporary borrower. Ownership is a **convention/API contract**, not a property C enforces.
- `free(c.name)` is called **exactly once** after both aliases have finished using the data.
- Assigning `c.name = NULL` does not null `d.name`; therefore the example explicitly nulls both and never accesses either after freeing.
- A second `free(d.name)` would be invalid: it would attempt a double free of the same storage.

Reassigning or copying one of these structures does not automatically duplicate, free or transfer any pointed-to allocation. Be explicit about ownership, especially across function boundaries.

## 4. The `sizeof` difference

```c
printf("inline member: %zu\n", sizeof(((person_inline *)0)->name));
printf("pointer member: %zu\n", sizeof(((person_pointer *)0)->name));
```

These `sizeof` operands are **unevaluated** (for the fixed types shown), so the illustrative null pointer is not dereferenced at runtime; nevertheless, a simpler alternative is to instantiate the two structures and write `sizeof a.name` and `sizeof c.name`.

The inline member has a size of exactly ten **bytes**, because `sizeof(char)` is 1. The pointer member is as large as a `char *` on that implementation (commonly 8 bytes on 64-bit Linux, but not guaranteed by C). Pointer size is **not** the length of the pointed-to text.

Beware: arrays passed to most function parameters undergo **array-to-pointer conversion**. Inside a function declared `void inspect(char name[10])`, the parameter is adjusted to `char *name`; `sizeof name` then yields the pointer size, not ten. By contrast, accessing an array directly as a struct member in `sizeof` preserves its array type.

## 5. Connection to CBox and AjisFS

Inline arrays provide fixed capacity, predictable embedded storage and a straightforward structure copy. They are appropriate when limits are known and small. A dynamically allocated buffer supports variable sizes but requires an explicit lifetime, owner, and cleanup policy. For large queues, dynamic buffers can reduce the size of each descriptor but increase allocation complexity.

Never serialize these structures to a stable disk format merely by writing `sizeof(struct)` raw bytes: padding, integer representation and pointer values are not portable. A future AjisFS record needs explicitly specified field widths, byte order and stored offsets or identifiers instead of RAM pointers.

## Review questions

1. Does `person_inline a = {1, "Bolaji"};` require `malloc` for `name`?
2. Does `person_inline b = a;` duplicate the contents of the embedded array?
3. After `person_pointer d = c`, does `d.name` own a new independent allocation?
4. What happens if both `c.name` and `d.name` are passed to `free` without new allocation?
5. Does `sizeof(c.name)` reveal the length of a NUL-terminated string?

**Next:** Designing a deep-copy function and a destructor (`person_clone` / `person_destroy`) with safe failure cleanup, then using these ownership rules in a test framework callback.

References: [Lesson 05](LESSON_05_CONST_OWNERSHIP_MEMORY.md), [API](API.md), [CBox integration](https://github.com/afrowaveltd/CBox/blob/main/Docs/DOES_IT_FAIL_INTEGRATION.md).

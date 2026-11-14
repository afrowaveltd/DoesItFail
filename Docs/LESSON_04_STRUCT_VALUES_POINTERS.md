# Lesson 04 — Struct values, pointers, `.` and `->`

**Series:** [Lesson 03 — Scope and lifetime](LESSON_03_SCOPE_LIFETIME_STORAGE.md) → Lesson 04.
**C version:** C17. **Status:** Learning-only documentation. No DoesItFail production API or source changes.

## The principle: C passes arguments by value

Every parameter receives a value, including pointer parameters. If the argument is a structure, its structure value is copied. If the argument is a pointer, the *pointer value* (an address) is copied, but both pointers can designate the same underlying object. This is not C++ reference passing and not a hidden `this` parameter.

## Two independent instances of a structure

```c
typedef struct
{
    int printed_count;
} printer;

printer printer_a = {0};
printer printer_b = {0};
```

`printer_a` and `printer_b` are separate structure objects with their own `printed_count` members. The analogy with two instances of a C# class is helpful for *independent state*, but the C structure variables themselves have value semantics.

## Case A — Copy the structure

```c
void print_by_value(printer copy)
{
    ++copy.printed_count;
}

/* Call: */
print_by_value(printer_a);
```

Inside the function, `copy` is a distinct structure object. Its `printed_count` changes, while `printer_a.printed_count` does not. No pointer needs to be dereferenced.

## Case B — Copy the pointer, access the original object

```c
void print_by_pointer(printer *self)
{
    if (self != NULL)
    {
        ++self->printed_count;
    }
}

/* Call: */
print_by_pointer(&printer_a);
```

`&printer_a` obtains an address. The copied pointer parameter `self` designates `printer_a`. Changing `self->printed_count` therefore changes the original object. Reassigning the local pointer *itself* does not change the caller's pointer variable.

## Member access operators

| Expression | Meaning |
| --- | --- |
| `printer_a.printed_count` | Access a member on a structure expression. |
| `self->printed_count` | Access a member through a pointer to the structure. |
| `(*self).printed_count` | Equivalent to `self->printed_count`. |

`.` binds more tightly than unary `*`, so the parentheses in `(*self).printed_count` are needed. The parameter name `self` is a code convention, analogous to C# `this`, not a C keyword.

## Hands-on: complete scratch example

Write this file yourself as `printer.c` in a disposable practice directory, outside the library repository:

```c
#include <stdio.h>

typedef struct
{
    int printed_count;
} printer;

static void print_by_value(printer copy)
{
    ++copy.printed_count;
}

static void print_by_pointer(printer *self)
{
    if (self != NULL)
    {
        ++self->printed_count;
    }
}

int main(void)
{
    printer printer_a = {0};
    printer printer_b = {0};

    print_by_value(printer_a);
    printf("After value: A=%d, B=%d\n",
           printer_a.printed_count, printer_b.printed_count);

    print_by_pointer(&printer_a);
    printf("After pointer: A=%d, B=%d\n",
           printer_a.printed_count, printer_b.printed_count);

    return 0;
}
```

Compile and run on Debian/WSL:

```sh
cc -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Wshadow printer.c -o printer
./printer
```

Expected:

```text
After value: A=0, B=0
After pointer: A=1, B=0
```

The example was separately compiled and run with the C17 flags above. The repository itself is unchanged.

## Important caveat: a structure copy is not always a deep copy

```c
typedef struct
{
    int *data;
} buffer_ref;
```

Copying `buffer_ref` duplicates the *pointer value*, not the object to which it points. Both copies may refer to the same allocation. If either copy modifies `*data`, the other can observe that modification. Ownership, allocation and `free()` remain separate responsibilities.

## Access without mutation

```c
void inspect_printer(const printer *self)
{
    if (self != NULL)
    {
        printf("Printed: %d\n", self->printed_count);
    }
}
```

The `const printer *` parameter lets the function inspect the object while preventing modifications through this pointer. This does *not* necessarily make the object immutable to all other code.

## Actual DoesItFail source: an opaque context pointer

In [`src/test.c`](../src/test.c), the test runner creates one automatic `dif_test_context context` for each test and calls:

```c
tests[i].function(&context);
```

The matching public typedef from [`test.h`](../include/does_it_fail/test.h) is:

```c
typedef struct dif_test_context dif_test_context;
typedef void (*dif_test_function)(dif_test_context *context);
```

Consumers have only a forward declaration of `dif_test_context`. They can receive/pass its pointer but cannot directly access its members, because the complete structure definition is private to `src/test.c`.

Inside the library, however, `dif_expect_int_eq` can write:

```c
++context->passed_checks;
```

This changes the original context, not a copy. The borrowed context pointer remains valid only during the synchronous invocation; saving it for later would violate the lifetime contract.

## Comprehension check

1. What changes after `print_by_value(printer_a)`?
2. What changes after `print_by_pointer(&printer_a)`?
3. Why is `(*self).printed_count` valid but `*self.printed_count` not equivalent?
4. If `printer` contained `char *name`, would copying `printer` also duplicate the text pointed to by `name`?
5. Why does DoesItFail use a pointer to an incomplete `dif_test_context` type?

**Next:** Explore ownership and borrowing for pointer members, `const`, `malloc`/`free`, and `void *user_data` callbacks.

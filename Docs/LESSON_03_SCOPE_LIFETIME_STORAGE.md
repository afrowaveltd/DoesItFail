# Lesson 03 — Scope, lifetime, storage duration, and explicit state

**Project:** DoesItFail / C17. **Status:** Learning-only documentation, not a new API.

## Quiz: block-scope `static`

In the example below, answer **B** is correct, not C. The pointer still refers to the same persistent object even after another call to `example()`.

```c
int *global_pointer = NULL;

void example(void)
{
    static int number = 42;
    global_pointer = &number;
}
```

Here `number` has **block scope**, **static storage duration**, and **lifetime throughout the program**. The name is not visible from `main`, but code with a valid pointer can access the object. `static` initialization is not repeated with each function call.

## The three terms (and a fourth: linkage)

| Term | Main question | Example |
| --- | --- | --- |
| Scope | Where can the identifier be written in source? | `number` can be named only inside its block. |
| Lifetime | When does the object exist? | A static object persists for the whole execution. |
| Storage duration | Which duration/allocation rule governs it? | Automatic, static, thread, or allocated storage in C17. |
| Linkage | Whether identically named identifiers in different scopes/translation units refer to one entity? | File-scope `static` gives internal linkage. |

Scope and lifetime are not the same. Nor are they descriptions of exact physical memory regions: C17 does not require all automatic objects to occupy a hardware stack or all static objects to occupy one specific segment.

## Four common patterns

### Automatic local

```c
void work(void)
{
    int number = 42;
}
```

`number` has automatic storage duration; the object's lifetime ends on exiting the associated block. Keeping a pointer to that object in a global does not extend its lifetime.

### Function-local static

```c
int next_id(void)
{
    static int last_id = 0;
    return ++last_id;
}
```

Only `next_id` can refer to the identifier `last_id` by name, and calls share the **same** persistent object. This is a small form of encapsulation, but has serious tradeoffs: hidden mutable state, lack of reset, multiple callers sharing one counter, and data races if called concurrently without synchronization.

Two different functions may each declare a local `static int counter`: those are independent objects. Repeating the same definition in the same scope is an error. Shadowing a different-scope identifier is often permitted but confusing; use `-Wshadow` on GCC/Clang to catch many such cases.

### File-scope state

```c
static int private_counter = 0;
int public_counter = 0;
```

Both objects have static storage duration. At file scope `static` also gives `private_counter` **internal linkage** (private to that translation unit). `public_counter` normally has external linkage; declaring it with `extern` in a header can make it accessible from other translation units. Avoid placing a non-`static` variable **definition** in a widely included header: that can cause multiple definitions during linking.

Putting all globals in `main.c` can be practical for tiny standalone programs, but does not make a reusable library independent. Other modules would need external declarations and the global variables would remain shared mutable state.

### Explicit context (preferred for our libraries)

```c
typedef struct
{
    int last_id;
    int processed_items;
} app_context;

int get_next_id(app_context *context)
{
    return ++context->last_id;
}

int main(void)
{
    app_context context = {0};
    int first = get_next_id(&context);
    int second = get_next_id(&context);
    (void)first;
    (void)second;
    return 0;
}
```

The state is explicit and can have multiple independent instances. The caller must keep the context alive while borrowed by a function. Mutable state is not automatically thread-safe just because it lives in a structure: concurrent callers need a suitable synchronization or isolation policy.

## How DoesItFail handles this

In `src/test.c`, `dif_run_suite()` executes the following inside its test loop:

```c
dif_test_context context = {0};
/* initialize fields */
tests[i].function(&context);
```

`context` is an **automatic** local object, newly created for each loop iteration. Its address is borrowed by the test **only during its synchronous invocation**. The object's lifetime ends at the end of the loop body for that iteration, not only when the entire runner returns. Holding `&context` in a global or keeping it for asynchronous work would be unsafe.

The public `typedef struct dif_test_context dif_test_context;` exposes a **pointer-compatible incomplete type**, not its private fields. This limits coupling to the runner's internal design.

## When should we use which design?

- Constants with suitable static storage duration are common and usually not the hidden-mutable-state problem.
- Function-local `static`: small private state where persistence and a single shared instance are intentional and documented.
- File-scope `static`: private state belonging to one module, if single-instance behavior is intentional.
- `extern` globals: rare, deliberate shared state with explicit ownership and synchronization.
- Context structures and parameter passing: preferred for reusable CBox modules, DoesItFail instances and future AJIS/AjisFS components.

## Safe experiment

Create a temporary `counter.c` outside the repository and type:

```c
#include <stdio.h>

void counter(void)
{
    static int value = 0;
    printf("Value: %d\n", ++value);
}

int main(void)
{
    counter();
    counter();
    counter();
    return 0;
}
```

Compile and run:

```sh
cc -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Wshadow counter.c -o counter
./counter
```

Expected output: 1, 2, 3 on separate lines. Remove only `static` and recompile: output is 1, 1, 1.

Do **not** experiment by dereferencing pointers to expired automatic objects; that is undefined behavior and cannot be used as a reliable test.

## Questions

1. Does declaring a variable `static` inside a function make its **name** global?
2. Does a second call to the function allocate a new static local?
3. What is the difference between local `static` and file-scope `static`?
4. Why is a long-lived global pointer not proof that its target is still alive?
5. Why is an explicit state structure often better than hidden function-local `static` for a library supporting multiple independent instances?

## Next

Study pointer ownership, copying versus borrowing, and `void *user_data` in the DoesItFail reporter callback. This builds toward `malloc`/`free` and safe event queues.

References: [Lesson 02](LESSON_02_ARCHIVES_CALLBACKS.md), [DoesItFail API](API.md).

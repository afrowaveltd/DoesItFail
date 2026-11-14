# Initial public API

The first API is intentionally minimal and **provisional**. All identifiers are declared in `<does_it_fail/test.h>` unless noted.

## Register and run a suite

```c
typedef void (*dif_test_function)(dif_test_context *context);
typedef struct { const char *name; dif_test_function function; } dif_test_case;

dif_summary dif_run_suite(const char *suite_name,
                          const dif_test_case *tests,
                          size_t count,
                          dif_reporter reporter,
                          void *user_data);
```

`tests` and its names must remain valid for the complete call. With `count > 0`, `tests` must be non-NULL, and each test needs a non-NULL name and function. Invalid registration returns `summary.setup_error = true`; it does not pretend the suite passed. Valid empty suites are allowed. Tests run synchronously in registration order. Each test is executed even when an earlier test has ordinary recorded expectation failures.

`dif_summary` reports test totals and check totals. A test with no failed checks passes. Callers should return nonzero if `setup_error` or `failed_tests != 0`.

## Typed expectation

```c
DIF_EXPECT_INT_EQ(context, expected, actual);
```

The macro forwards to `dif_expect_int_eq` with `__FILE__`/`__LINE__`. The function compares integers, records success/failure on the active context, and returns `bool`. On failure it emits `DIF_CHECK_FAILED` with the two integer values and source location. **It does not abort**. Do not use the macro around expressions whose side effects depend on argument-evaluation order. There is no type-generic assertion yet.

## Reporter callback

```c
typedef void (*dif_reporter)(const dif_event *event, void *user_data);
```

The event is borrowed and short-lived; do not store its pointer. The event stream currently supports suite start, test start, integer check failure, test finish, and suite finish. Only the relevant fields for a given event type are meaningful.

The optional console callback is declared in `<does_it_fail/reporter_console.h>`:

```c
dif_console_options output = {0};
output.stream = stdout;
output.color = false;

dif_run_suite("Suite", tests, count, dif_report_console, &output);
```

`output` must outlive `dif_run_suite`. Setting `color = true` adds ANSI SGR sequences for PASSED/FAILED; the caller is responsible for knowing whether the output stream/terminal supports them. This reporter uses `stdio`, whereas the core does not. An ncurses callback is planned, **not supplied**.

## Ownership / lifecycle

No public allocation or cleanup routine exists in this seed. Test names and suite name are borrowed NUL-terminated strings. Test context is valid only for the duration of a test case. Event pointers are valid only during a reporter callback. No API is safe for shared concurrent calls without additional external synchronization; a thread-aware design is a future phase.

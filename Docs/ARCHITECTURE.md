# Architecture and design decisions

**Status: educational seed, subject to change.** This document separates what works today from proposed design choices.

## Agreed responsibility boundaries

1. DoesItFail is an **independent Git repository** and C library, not `CBox/testing` and not `CBox/when_it_fails`.
2. CBox, MemView, and other software **consume DoesItFail for development tests only**. Production CBox Essentials remains dependency-free beyond its standard C runtime requirements.
3. Core is independent of text output, `stdio`, ncurses, IDEs, and CI systems. A reporter receives **structured synchronous callbacks**. This decouples test declarations from presentation.
4. Each test owns its test code. The library owns the runner, statistics, failure events, and reporter contracts.
5. C17 is the primary baseline; older C99 support is assessed only for modules where it is useful. No C23 features are required.
6. `EXPECT` records the failure and continues. `REQUIRE` should end only the current test when introduced. A test failure is **not** a caught process crash: `assert`, `abort`, segmentation faults, and undefined behavior can terminate the runner. Process isolation is a separate future feature.
7. Automatic runs return a nonzero process status if a test fails. The console reporter never enters alternate-screen mode. ncurses is **optional** and never a build dependency of the core.
8. Test results can include expected and actual values, source file and line, case name, case number, and total summary.
9. Documentation is updated alongside API, build, VS Code configurations, and consumer integration changes.

## Current implementation

```text
Consumer test functions + case array
               |
               v
    DoesItFail core: dif_run_suite
       |                  |
       v                  v
 dif_expect_int_eq    dif_event stream
                          |
          +---------------+----------------+
          |                                |
  Console reporter                 Future ncurses reporter
  stdio + optional ANSI            not implemented
```

The test runner contains **no `printf`**; the console reporter is in `src/reporter_console.c`. No heap allocation is used by the core. Test case arrays, labels, and reporter context are caller-owned/borrowed. Callbacks happen synchronously; do not retain event pointers.

## Proposed extension points (not committed APIs)

- `EXPECT_TRUE`, `EXPECT_NULL`, `EXPECT_STRING_EQ`, `EXPECT_MEMORY_EQ`, `EXPECT_SIZE_EQ`, `EXPECT_FLOAT_NEAR`.
- `REQUIRE` checks that record failure but abort only the current case.
- More expressive failure payloads (typed/formatters, borrowed vs owned message policy) before any stable ABI.
- `DIF_EVENT_*` subscribers for console, JSON/JUnit XML, IDE integration, and optional ncurses.
- Color policy: off for noninteractive/CI by default; opt-in for ANSI, no escape sequences in machine-readable reports.
- Optional subprocess isolation for fatal crashes and timeouts.
- Benchmark tools separated from correctness checks; resource usage counters only when measured reliably.

## Deliberate non-goals for phase 1

No test discovery using reflection (not a C feature); no dependency on CBox, .NET, third-party GUI libraries, or ncurses; no implicit promises of ABI stability or automatic platform compatibility.

## Cross-project references

- `CBox/testing` is the **consumer-side collection of tests**, not a test framework.
- `CBox/when_it_fails` addresses **application/runtime failures**, not tests.
- `DoesItFail` addresses **testing code and reporting assertion results**.
- Shared test scenarios can later compare CBox and .NET Toolbox behavior without requiring identical source code, runtime, or memory layouts.

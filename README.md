# DoesItFail

**DoesItFail** is a small, independent, reusable **C17 testing library** in the Afrowave family. It is not part of CBox. Projects such as CBox, MemView, or third-party C programs may use it **as a development/test dependency**, not a runtime dependency of production libraries.

**Current status: educational seed / prototype, not a released stable API.** The first usable functionality is a portable test runner, a typed `int` equality check, structured events, and an optional `stdio` console reporter. String/memory checks, `REQUIRE`, process isolation, and ncurses are planned, **not implemented**.

## Responsibility split

- **Core**: executes registered tests, counts checks and failures, sends structured events. Does **not** print, allocate test result strings, or depend on ncurses.
- **Assertions/helpers**: typed checks such as `DIF_EXPECT_INT_EQ` now; other types later.
- **Console reporter**: turns events into human-readable text; optional ANSI color.
- **ncurses reporter**: future optional frontend, not required by the core.
- **Consumers**: own test functions and test registries; for CBox these stay in CBox's `testing` / module test directories.

## Quick start (Debian / GCC)

```sh
sudo apt install build-essential gdb
make help
make all
make test
make demo
```

The command `make demo-fail` deliberately returns a nonzero exit code and demonstrates how the runner continues to the following test after an ordinary assertion failure. `make ncurses-test` currently exits with code 2, explicitly saying that ncurses is **not implemented**. Automatic test commands do not consume/control the terminal.

## Example for another project

```c
#include <stdio.h>
#include "does_it_fail/test.h"
#include "does_it_fail/reporter_console.h"

static void test_calculation(dif_test_context *context)
{
    DIF_EXPECT_INT_EQ(context, 5, 2 + 3);
}

int main(void)
{
    const dif_test_case tests[] =
    {
        {"test_calculation", test_calculation}
    };
    dif_console_options options = {0};
    options.stream = stdout;
    options.color = false;
    dif_summary result = dif_run_suite("My project", tests,
        sizeof(tests) / sizeof(tests[0]), dif_report_console, &options);
    return (result.setup_error || result.failed_tests != 0) ? 1 : 0;
}
```

Compile after `make all`:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Iinclude examples/console_demo.c \
    build/libdoesitfail.a -o build/manual_example
```

The **include directory and archive path** are changed when used from another project. This is a regular C static archive built with `ar`; a shared `.so`, install rules, `pkg-config`, CMake package, Conan recipe, and vcpkg port are possible future packaging work, not already provided.

## Repository layout

```text
DoesItFail/
├── include/does_it_fail/test.h
├── include/does_it_fail/reporter_console.h
├── src/test.c
├── src/reporter_console.c
├── tests/test_core.c
├── examples/console_demo.c
├── Docs/
│   ├── ARCHITECTURE.md
│   ├── API.md
│   ├── ROADMAP.md
│   └── DEVELOPMENT.md
├── .vscode/                 # shareable editor settings/tasks
└── Makefile
```

## Learning policy

Explain the mechanism first, develop one feature at a time, and test before advancing. Keep the initial public API minimal. Source code and docs are in English; C style is C17, Allman braces, four-space indentation, `snake_case` and `dif_` for exported identifiers. No external dependency is permitted in the core.

See [Architecture](Docs/ARCHITECTURE.md), [API](Docs/API.md), [Development](Docs/DEVELOPMENT.md), and [Roadmap](Docs/ROADMAP.md).

## Using DoesItFail from CBox

[CBox](https://github.com/afrowaveltd/CBox) is an initial external consumer. Clone both repositories as siblings, then invoke the optional CBox test integration:

```bash
# Example parent directory containing both repositories:
#   CBox/
#   DoesItFail/
cd CBox
make test-framework
```

This verifies that CBox can compile and link the **independent static library**. It is not a test of the learner-owned Result implementation; `make test-result` remains a separate CBox exercise. See [CBox integration guide](https://github.com/afrowaveltd/CBox/blob/main/Docs/DOES_IT_FAIL_INTEGRATION.md).

## License

MIT, see [LICENSE](LICENSE).

# DoesItFail roadmap

This is an incremental **learning plan**, not a release commitment.

## Phase 0 — independent reusable C17 seed

- [x] Standalone source tree with MIT license, GNU Make, `.gitignore`, formatting, and VS Code examples
- [x] `libdoesitfail.a` static archive and consumer link example
- [x] Explicit test-case array and callback-based event contract
- [x] Integer EXPECT equality with expected/actual values and file/line location
- [x] Continue to the next test after ordinary failed checks
- [x] Plain console reporter and opt-in ANSI colors; no ncurses dependency
- [x] Independent bootstrap tests using standard `assert`
- [ ] Publish this independent tree to its own GitHub repository (not created by the seed package)
- [ ] Link CBox's production Result tests to DoesItFail, replacing `assert` *after* the learner has completed and committed `essentials/src/result.c`

## Phase 1 — user-driven growth

- [ ] Add `EXPECT_TRUE`, `EXPECT_NULL`, and typed `EXPECT_STRING_EQ` (decide NULL contract)
- [ ] Add `REQUIRE` that ends only the current test
- [ ] Introduce consistent failure records with noninteger formatting
- [ ] Choose a structured error/event representation before publishing stable ABI
- [ ] Test color disabled for CI and redirected output

## Phase 2 — alternative reporters

- [ ] Add optional ncurses reporter with `make ncurses-test`; isolate UI dependency and clean terminal restoration
- [ ] Add JSON and/or JUnit XML reporting if actual consumers need them
- [ ] Add CI build matrix for GCC, Clang, and Windows toolchains

## Phase 3 — packaging

- [ ] Public `pkg-config` metadata and install/uninstall rules
- [ ] Shared library with intentional ABI/export policy
- [ ] Versioned GitHub releases, CMake package, ConanCenter/vcpkg only once useful and stable
- [ ] Explore crash/timeout isolation using subprocesses, separate from the core

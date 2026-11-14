# Development environment and VS Code

## Guided C learning

- [Lesson 01 — From C source to a static library](LESSON_01_COMPILATION_LINKING.md) — `.h`, `.c`, `.o`, `.a`, compilation and linking using this real repository.
- [Lesson 02 — Static archives and function pointers](LESSON_02_ARCHIVES_CALLBACKS.md) — `ar`, indirect calls, test registration, borrowed contexts, and reporter callbacks.
- [Lesson 03 — Scope, lifetime, storage duration and explicit state](LESSON_03_SCOPE_LIFETIME_STORAGE.md) — static locals, globals, shadowing, context pointers and state ownership.

## Debian setup

```sh
sudo apt install build-essential gdb
make help
make all
make test
make demo
```

Use VS Code **Microsoft C/C++ extension** (`ms-vscode.cpptools`) for IntelliSense and GDB. The `.vscode/` configuration is provided for Linux/WSL; Windows native compilers should override the compiler path and debugger path locally. Do not install two competing C/C++ language servers for the same workspace.

## Editor tasks

- `DoesItFail: build library` => `make all`
- `DoesItFail: test` => `make test`
- `DoesItFail: console demo` => `make demo`
- `DoesItFail: clean` => `make clean`

The GDB launch configuration starts the console demo; it does not require ncurses. Run `make demo-fail` to see an intentionally failed test and a nonzero exit status; it is not a green CI target.

## Consume from a sibling CBox clone

Recommended local paths:

```text
Afrowave Projects/
    CBox/
    DoesItFail/
```

Or set `DOES_IT_FAIL_DIR` in the CBox `make test-framework` command to the actual location:

```sh
make test-framework DOES_IT_FAIL_DIR="/path/to/DoesItFail"
```

The CBox Makefile **builds a test-only integration binary** using this static library. It does not require DoesItFail for normal `make test` or for building CBox production modules. On Windows/MinGW the environment may require a different compiler/ar path; CBox uses the compiler specified by its Makefile.

## Rules for future additions

1. Public API -> header, implementation, bootstrap tests, example, API docs.
2. Reporter change -> keep core independent; test callback flow without console.
3. Build change -> update Makefile, VS Code tasks, README/Docs in the same commit.
4. Support claim -> verify actual compiler/platform rather than guessing.
5. Check errors, output/exit code, memory ownership, and ANSI color policy.
6. Use small commits and discuss one concept before introducing larger implementations; the learner writes features where practical.

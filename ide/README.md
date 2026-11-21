# PlusMinus Studio

A local, monochrome dark desktop IDE for PlusMinus. The C++ runner uses the repository's existing compiler and VM, isolated in a separate process for each run. No interpreter changes, browser, web server or Python packages are required.

## Build and launch

From the repository root, install Python 3 with Tk, CMake and a C++20 compiler. On Fedora:

```sh
sudo dnf install python3-tkinter cmake gcc-c++
cmake -S ide -B ide/build -DCMAKE_BUILD_TYPE=Release
cmake --build ide/build --config Release
python3 ide/app.py
```

On Windows, install Python with Tcl/Tk, CMake, and Visual Studio Build Tools with C++ desktop support. Run the same two CMake commands, followed by `py -3 ide/app.py`. The IDE discovers the executable in `ide/build`, `ide/build/Release`, or `ide/build/Debug`; use **Runner…** to select another location. For a direct Linux build:

```sh
mkdir -p ide/build
g++ -std=c++20 -O2 -pthread -I. ide/runner.cpp Parser.cpp -o ide/build/plusminus_runner
```

## Features

- Source editor with grayscale syntax highlighting, search, undo, file open/save, indentation and cursor position.
- Run (F5) and compile-only (F6) using the actual PlusMinus backend.
- Output, return value, global variable/array inspection, VM bytecode and diagnostics.
- Independent process for each execution, wall-clock timeout and Stop; the editor remains responsive. This is process isolation, not a security sandbox. Run only trusted source and use OS-level limits for untrusted programs.
- Built-in examples, recent execution history (double-click to restore), exact output or VM-result tests, batch execution and JSON import/export.
- The current interpreter has no runtime input API; the runner's stdin carries the source code. Output from the VM is returned after completion, not streamed. Timing includes process startup and serialization. The bytecode view is a listing, not a step debugger.

## Smoke test against the repository's interpreter

After building:

```sh
python3 ide/smoke_test.py
```

This checks arithmetic output, variables, compilation, errors, timeout and test-runner comparison. It does not replace the existing Visual Studio unit tests.

To test the GUI manually, launch it, run each built-in example, inspect Variables and Bytecode, run all five cases, try a malformed program and stop a nonterminating loop. Save and reopen a source file, and export/import a test suite.

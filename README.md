# PlusMinus Studio

PlusMinus is a C++20 interpreter with a local dark Tkinter desktop IDE.

## Double-click startup

For a **source checkout**, use the launcher in the repository root:

- **Windows:** double-click `Start-PlusMinus.cmd`.
- **Linux (desktop environment):** double-click `Start-PlusMinus.desktop`; some file managers require a one-time **Allow Launching / Mark as Trusted** action. Alternatively, run `sh Start-PlusMinus.sh`.
- **macOS:** double-click `Start-PlusMinus.command` in Finder. macOS may require confirming an unsigned script.

These launchers delegate to `launch.py` in the same directory, so you can start them from a folder whose name contains spaces. They compile the C++ runner on first use and launch the IDE; subsequent launches reuse the build. **Starting from source requires an existing Python 3.9+ installation**. The launcher asks before installing missing system tools where supported, and the first launch may take some time.

For **true no-dependency double-click startup**, use the prebuilt application from GitHub Releases: `PlusMinusStudio.exe` (Windows), `PlusMinusStudio` or the accompanying `.desktop` file (Linux), and `PlusMinusStudio.app` (macOS). Linux desktops may still require the file to be marked executable/trusted.

## Install-free desktop releases

Get the archive for your platform from [GitHub Releases](https://github.com/Maksim23060/PlusMinus/releases) after a version tag is published:

- **Windows x86-64:** extract `PlusMinusStudio-windows-x86_64.zip` and run `PlusMinusStudio.exe`.
- **Linux x86-64:** extract `PlusMinusStudio-linux-x86_64.tar.gz` and run `PlusMinusStudio/PlusMinusStudio` on a compatible glibc-based desktop Linux distribution.
- **macOS Intel:** extract `PlusMinusStudio-macos-x86_64.zip` and open `PlusMinusStudio.app`. These unsigned apps may require an exception in macOS Privacy & Security.

Standalone releases contain the Python runtime, Tkinter and C++ runner. No compiler, CMake or separately installed Python is required. Extract the archive before running. A single executable cannot work across all operating systems and architectures: ARM64, musl Linux and older glibc distributions are not tested.

## Launch from source

Python **3.9+** must already be available.

```sh
python launch.py
```

Use `python3 launch.py` on Linux where necessary. The launcher checks Tkinter, CMake and a C++20 compiler, asks before installing missing system packages when supported (apt, dnf, pacman, zypper and winget), builds the native runner and launches the GUI. Internet access and elevated permissions may be needed for system package installation. Apple Command Line Tools are required for source builds on macOS.

```sh
python launch.py --check         # only report prerequisites
python launch.py --no-install    # do not install system packages
python launch.py --yes           # consent to installing missing packages
python launch.py --build-only    # compile without launching GUI
python launch.py --test          # run C++ runner smoke tests
python launch.py --rebuild       # force a clean build
```

The runner is reused without rebuilding when its sources have not changed.

## Development and releases

```sh
python launch.py --no-install --build-only
python ide/smoke_test.py
python -m unittest discover -s tools -p "test_*.py"
python -m pip install pyinstaller==6.16.0
python tools/package.py
```

`tools/package.py` writes an archive to `release/`; bundles support `--self-test` to verify the embedded runner. GitHub Actions builds and tests Windows, Linux and Intel macOS on PRs, and publishes binaries for version tags (for example `v1.0.0`).

See [ide/README.md](ide/README.md) for interface features. Running source programs in a separate process is **not** a security sandbox; do not run untrusted programs.

# NEXUS Installation & Quickstart (v1.0.0)

This guide documents requirements, compilation instructions, and command usage for NEXUS.

## Prerequisites

- **GCC/g++**: A compiler supporting C++17 or later.
- **PowerShell**: Required to execute the build script on Windows.

## Installation

To compile the NEXUS compiler and test verification suite, run the build script:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

This compiles:
- `nxs.exe`: The main compiler CLI driver.
- `test_suite.exe`: The verification suite.

---

## Command Usage

### 1. Build a C++ File to NEXUS IR (`.nxs`)
- Command: `nxs build <filename.cpp>`
- Example: `./nxs build examples/hello.cpp`
- Output: Creates `examples/hello.nxs`.

### 2. Run a Program in the VM
You can execute either the high-level C++ file (compiled in JIT mode) or a pre-compiled `.nxs` file directly:
- Run C++: `./nxs run examples/hello.cpp`
- Run IR: `./nxs run examples/hello.nxs`

### 3. List Supported Target Architectures
- Command: `nxs target list`

### 4. Display Version
- Command: `nxs version` or `nxs --version`

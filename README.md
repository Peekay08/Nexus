# NEXUS Compiler & VM (v1.0.0)

NEXUS is a deterministic compiler toolchain and register-based virtual execution environment designed to compile, serialize, optimize, and execute a stable, safe subset of C++.

---

## 🧱 Project Architecture

The NEXUS compiler and interpreter pipeline is split into distinct, highly modular components:

```
Source Code (.cpp)
       │
       ▼
 [ Lexical Analyzer ]  ---> (Line & Column Tracking)
       │
       ▼
 [ Syntax Parser ]     ---> (Recursive Descent AST construction)
       │
       ▼
 [ IR Generator ]      ---> (Translates AST to register-based IR)
       │
       ▼
 [ IR Optimizer ]      ---> (Constant Folding & Dead Code/Alloca Elimination)
       │
       ▼
  [ Serializer ]       ---> (Outputs .nxs IR assembly)
       │
       ▼
  [ IR Loader ]        ---> (Syntactic & Semantic Label verification)
       │
       ▼
 [ Virtual Machine ]   ---> (Stack Frame interpreter + STL built-ins)
```

- **Frontend**: A deterministic lexer and parser that validates code structure, operator precedence, and nested syntax. Unsupported features like classes, templates, exceptions, and pointers are rejected cleanly.
- **Diagnostics**: A centralized diagnostic pipeline (`[NX-1XX]` to `[NX-4XX]`) providing unified, column-precise error logs.
- **Optimizer**: Evaluates constants at compile-time and removes unused variables/allocations using Store-to-Load Forwarding and Dead Store Elimination.
- **Modular Runtime VM**: A sandboxed interpreter with stack-frame tracing, safety boundaries (1000 stack frame limit), math built-ins (`sqrt`, `pow`, `abs`, `sin`, `cos`), and file built-ins (`file_open`, `file_write`, `file_read`, `file_close`).

---

## 📄 Formal Specifications

Formal specification contracts are frozen and documented under [spec/](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/spec/):
* **[spec/language.md](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/spec/language.md)**: Documents supported types, operators, and statements.
* **[spec/ir.md](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/spec/ir.md)**: Defines the instruction set format, syntax, and loader checks.
* **[spec/vm.md](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/spec/vm.md)**: Establishes calling conventions, stack architecture, and safety bounds.

---

## ⚙️ Installation

To build the NEXUS compiler toolchain and the test verification suite, run the PowerShell build script:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

This compiles:
- `nxs.exe`: The main compiler CLI driver.
- `test_suite.exe`: The full verification test suite.

---

## 🛠️ CLI Developer Tools & Commands

NEXUS includes advanced CLI developer tools to compile, optimize, trace, and debug programs:

### 1. Build a C++ File to IR
Compiles and serializes the source file to optimized IR:
```bash
./nxs build examples/fibonacci.cpp
# Generates examples/fibonacci.nxs
```

### 2. Run a Program
Runs a high-level C++ file (JIT mode) or a pre-compiled `.nxs` file on the VM:
```bash
./nxs run examples/fibonacci.cpp
./nxs run examples/fibonacci.nxs
```

### 3. Dump Intermediate Representation
Compiles and outputs the optimized IR assembly directly to stdout:
```bash
./nxs dump-ir examples/hello.cpp
```

### 4. Trace Execution
Logs each executed instruction along with register states before and after execution:
```bash
./nxs trace examples/hello.cpp
```

### 5. Benchmark Performance Metrics
Measures compiler build times, VM execution speed, instruction count, and peak memory:
```bash
./nxs stats examples/fibonacci.cpp
```

### 6. Environment Doctor Check
Verifies path environments, compiler integrity (`g++`), and folder directories:
```bash
./nxs doctor
```

### 7. Interactive Step Debugger
Launches an interactive, instruction-level step debugger:
```bash
./nxs debug examples/fibonacci.cpp
```
**Debugger Commands**:
* `step` (or `s` / Enter): Execute the current instruction.
* `regs` (or `r`): Print active registers.
* `vars` (or `v`): Print local variables.
* `stack` (or `st`): Print call stack trace.
* `continue` (or `c`): Run to completion.
* `quit` (or `q`): Exit the debugger.

---

## 🧪 Verification Tests

Run the verification test suite to validate lexer tokenization, syntax nesting, IR optimizations, VM exceptions, and integration programs:

```bash
./test_suite.exe
```

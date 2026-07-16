# NEXUS Compiler & VM (v1.0.0)

NEXUS is a deterministic compiler toolchain and register-based virtual machine designed to compile, serialize, optimize, and execute a stable, clean subset of C++.

## Core Architecture
- **Compiler Frontend**: A deterministic Lexer and Parser with strict operator precedence, nesting validation, and filename-based diagnostics. Rejects classes, templates, exceptions, namespaces, and structs cleanly.
- **Intermediate Representation (IR)**: A low-level, register-based intermediate representation designed for program serialization and analysis.
- **Optimizer**: A compiler pass performing Constant Folding (evaluation of arithmetic, comparisons, and string concats at compile time) and Dead Code Elimination (removal of unused pure instructions).
- **Virtual Machine**: A stack-frame based virtual execution engine supporting standard library file IO built-ins, math built-ins (`sqrt`, `pow`, `abs`, `sin`, `cos`), and strict safety limits (e.g. 1000 call frames recursion depth limit).
- **Diagnostics**: Standardized diagnostics formatting using error codes `[NX-1XX]` through `[NX-4XX]`.

## Documentation
- Detailed technical specs are located in [docs/](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/docs/):
  - [Architecture Specification](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/docs/architecture.md)
  - [IR Instruction Set Spec](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/docs/ir_spec.md)
  - [VM Execution Spec](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/docs/vm.md)
  - [Diagnostics Reference](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/docs/diagnostics.md)

## Examples
- Quick-start programs are located in [examples/](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/examples/):
  - [hello.cpp](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/examples/hello.cpp) - Basic output demo
  - [fibonacci.cpp](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/examples/fibonacci.cpp) - Iterative loops demo
  - [calculator.cpp](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/examples/calculator.cpp) - Function calling and parameters demo
  - [file_demo.cpp](file:///c:/Users/Promise%20Kolade/OneDrive/Desktop/Peekay/Programing/Projects/NEXUS/examples/file_demo.cpp) - Built-in file system interaction demo

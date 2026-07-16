# Changelog

All notable changes to the NEXUS project will be documented in this file.

## [1.0.0] - 2026-07-16

This is the stable core release of the NEXUS compilation toolchain and virtual machine.

### Added
- **Standardized Diagnostics System**: Implemented unique code ranges `[NX-1XX]` through `[NX-4XX]` with standardized source pointer trace formatting.
- **IR Optimizer**: Added a pipeline optimization pass with Constant Folding and Dead Code Elimination.
- **Strict IR Validation**: Added `.nxs` header verification (`; NEXUS IR v1.0`), duplicate labels check, syntax check on registers/arguments, and semantic checking of jump target labels.
- **VM Safety Enhancements**: Added division-by-zero checks, invalid register references check, recursion stack limit of 1000 frames, and validated file built-ins.
- **CLI Utilities**: Added `nxs version` command.
- **Examples**: Built quickstart guides including `hello.cpp`, `fibonacci.cpp`, `calculator.cpp`, and `file_demo.cpp`.

### Changed
- **Consistent AST Naming**: Renamed `IfNode` -> `IfStmtNode`, `WhileNode` -> `WhileStmtNode`, and `ReturnNode` -> `ReturnStmtNode`.
- **Diagnostics Format**: Transformed all compile-time and runtime exceptions to yield standardized messages.
- **IR Serialization**: Version header frozen as `; NEXUS IR v1.0`.

### Removed
- Unused/dead AST nodes.
- Removed duplicated compiler execution drivers and centralized JIT JOT logic.

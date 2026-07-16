# Contributing to NEXUS

Thank you for your interest in contributing to the NEXUS compiler toolchain and virtual machine!

## Core Guidelines
1. **Deterministic Execution**: The VM and Compiler must remain deterministic. No experimental language extensions.
2. **Diagnostics Compliance**: All errors must yield standardized messages conforming to the `[NX-XXX]` spec.
3. **No External STL Expansion**: Maintain the frozen scope of language features (C++ subset).
4. **Safety Verification**: Ensure the verification suite passes in its entirety before creating a pull request.

## Development Workflow
1. **Writing Code**:
   - Write compiler changes in the `compiler/` folder.
   - Write VM interpreter updates in `runtime/interpreter/`.
2. **Adding Tests**:
   - Every bug fix or feature optimization should have a corresponding test case in `tests/test_suite.cpp`.
3. **Building**:
   - Run `./build.ps1` to compile the executables.
4. **Verifying**:
   - Run `./test_suite.exe` to run verification checks.

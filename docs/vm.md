# NEXUS Virtual Machine Execution Specification (v1.0.0)

This document describes the execution model, stack framing, register allocation, standard library built-ins, and safety limits of the NEXUS Runtime Virtual Machine (VM).

## Execution Model

The NEXUS Virtual Machine executes programs represented as `IRProgram` AST/structures. Execution starts at a named entry point function (usually `main`) and progresses instruction-by-instruction.

## Call Stack & Stack Frames

Every function call creates a new `StackFrame` pushed onto the call stack. A stack frame contains:
- `funcName`: Name of the executing function.
- `registers`: Mapped virtual register bindings (local temporary registers e.g. `%0`, `%1`).
- `variables`: Mapped local variables allocated via `alloca` (stack variables e.g. `%addr_x`).
- `labels`: Maps defined label names to indices in the function's instruction list for quick jumps.
- `ip`: Instruction pointer pointing to the current executing instruction.

### Recursion Limit (Stack Overflow Prevention)
The call stack depth is strictly limited to **1000 frames**. If a program exceeds this depth (such as during infinite recursion), the VM halts execution and throws stack overflow error `[NX-407]`.

## Standard Library Built-ins

The VM intercepts call operations to specific predefined names:

### Math Functions
- **`sqrt(float/int)`**: Returns square root as float.
- **`pow(base, exp)`**: Returns power base^exp as float.
- **`abs(float/int)`**: Returns absolute value.
- **`sin(float/int)`**: Returns sine.
- **`cos(float/int)`**: Returns cosine.

### File Operations
- **`file_open(filename, mode)`**: Opens file and returns a unique integer file handle, or -1 on error. Modes: `"r"`, `"w"`, `"a"`.
- **`file_write(handle, content)`**: Writes string content to the open file handle. Returns number of characters written. Throws `[NX-405]` if handle is invalid.
- **`file_read(handle)`**: Reads a line of text from the open file handle. Returns string. Throws `[NX-405]` if handle is invalid.
- **`file_close(handle)`**: Closes the file handle. Throws `[NX-405]` if handle is invalid.

### System Utilities
- **`sleep(ms)`**: Halts the thread execution for `ms` milliseconds.
- **`system(command)`**: Invokes the host environment command runner.

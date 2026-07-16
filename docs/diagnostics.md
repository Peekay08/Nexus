# NEXUS Diagnostics Specification (v1.0.0)

This document describes the standardized diagnostic error format and specifies the error codes returned by the NEXUS compiler toolchain and virtual machine.

## Standard Diagnostic Format

All compilation and runtime errors emitted by the toolchain must conform to the following standard multi-line diagnostic format:

```text
[NX-CODE] Error Type

Description of the error.

--> filename:line:column
```

### Components:
- **`[NX-CODE]`**: The standard unique diagnostic code (where CODE is a 3-digit number).
- **`Error Type`**: A short classification of the error (e.g. `Parser Error`).
- **`Description`**: A user-friendly message explaining the exact cause of the failure.
- **`--> filename:line:column`**: Source code location pointer. For VM runtime errors, this resolves to `filename:functionName:instructionIndex`.

---

## Error Categories & Codes

### 1. Lexer Errors (`[NX-1XX]`)
- **`[NX-101] Lexical Error`**: Unexpected character encountered that is not part of the NEXUS language specification.
- **`[NX-102] Lexical Error`**: Unterminated string literal.

### 2. Parser Errors (`[NX-2XX]`)
- **`[NX-201] Parser Error`**: Expected token missing (e.g., missing semicolons `;`, braces `{}`, or parentheses `()`).
- **`[NX-202] Parser Error`**: Expected expression missing.
- **`[NX-203] Parser Error (Unsupported Feature)`**: Use of unsupported C++ syntax (e.g., classes, templates, exceptions, namespaces, structs, or new/delete operators).

### 3. IR Loader Errors (`[NX-3XX]`)
- **`[NX-301] IR Loader Error`**: Missing or invalid version header. Expected `; NEXUS IR v1.0` as the first statement in the `.nxs` file.
- **`[NX-302] IR Loader Error`**: Malformed instruction structure or function declaration syntax.
- **`[NX-303] IR Loader Error`**: Invalid register/address reference format or unknown opcode.
- **`[NX-304] IR Loader Error`**: Undefined jump target label referenced in a branch instruction.
- **`[NX-305] IR Loader Error`**: Duplicate label definition in the same function scope.
- **`[NX-306] IR Loader Error`**: File system read error (cannot open `.nxs` file).

### 4. VM Runtime Errors (`[NX-4XX]`)
- **`[NX-401] Runtime Error`**: Division or modulo division by zero.
- **`[NX-402] Runtime Error`**: Access to an invalid, uninitialized, or out-of-bounds register.
- **`[NX-403] Runtime Error`**: Invalid or missing target label pointer in jump instruction execution.
- **`[NX-404] Runtime Error`**: Function execution failure (called function name not found).
- **`[NX-405] Runtime Error`**: File system Built-in failure (invalid file handle for read/write/close operations).
- **`[NX-406] Runtime Error`**: Stack underflow.
- **`[NX-407] Runtime Error`**: Stack overflow (call stack recursion depth exceeded 1000 frames).

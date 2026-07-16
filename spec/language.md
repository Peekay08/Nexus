# NEXUS C++ Subset Language Specification (v1.0.0)

This document specifies the exact subset of C++ supported by the NEXUS 1.0 toolchain.

## Supported Types

| Type | Description | Keyword |
| :--- | :--- | :--- |
| Integer | 32-bit signed integer | `int` |
| Floating point | 32-bit single-precision float | `float` |
| String | UTF-8 dynamic character string | `std::string` |
| Void | Null return type for functions | `void` |

## Supported Statements

- **Variable Declarations**: Local variable definitions with optional initializers (e.g. `int x = 42;`).
- **If / Else**: Conditional execution checks (e.g. `if (x < 10) { ... } else { ... }`).
- **While Loops**: Loop execution while condition holds true (e.g. `while (x > 0) { ... }`).
- **Return Statements**: Returns a value or leaves control of the function (e.g. `return x;`).
- **Expressions**: Standalone expressions, assignments, or function calls (e.g. `x = x + 1;` or `foo();`).

## Supported Operators

- **Arithmetic**: `+` (addition), `-` (subtraction), `*` (multiplication), `/` (division), `%` (modulo).
- **Comparison**: `==` (equality), `!=` (inequality), `<` (less than), `<=` (less than or equal), `>` (greater than), `>=` (greater than or equal).
- **Assignment**: `=` (simple value assignment).

## Transitive Built-ins (Standard Library)

- Output: `std::cout << val << std::endl;` / `std::cout << val;` (translated to VM runtime PRINT).
- System Call: `system(command)`
- Timing: `sleep(milliseconds)`
- Mathematics: `sqrt(x)`, `pow(base, exp)`, `abs(x)`, `sin(x)`, `cos(x)`
- File IO: `file_open(path, mode)`, `file_write(handle, text)`, `file_read(handle)`, `file_close(handle)`

---

## Unsupported Features (Explicitly Rejected)

The compiler frontend actively rejects these C++ features to prevent compile-time/runtime feature creep:

- ✗ **Object Orientation**: `class`, `struct`, `union`, `public`, `private`, `protected`, `virtual`, `inheritance`.
- ✗ **Generics & Templates**: `template`, `<typename T>`, `<class T>`.
- ✗ **Modularization**: `namespace` declarations.
- ✗ **Exception Handling**: `try`, `catch`, `throw`.
- ✗ **Memory Allocation & Pointers**: Pointers (`*`), references (`&`), `new`, `delete`.
- ✗ **STL Containers**: `std::vector`, `std::map`, `std::list`, etc.

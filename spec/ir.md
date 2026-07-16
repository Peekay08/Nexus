# NEXUS IR Instruction Set Specification (v1.0.0)

This document serves as the formal specification contract for NEXUS Intermediate Representation (IR).

## File Validation Header
Every serialized NEXUS IR file must begin with:
```text
; NEXUS IR v1.0
```

---

## 1. Memory Operations

### ALLOCA
* **Syntax**: `%dest = alloca <type>`
* **Description**: Allocates local stack memory for a variable.
* **Operands**: `dest` (virtual address register), `type` (variable type).
* **Errors**: `NX-302` if malformed; `NX-303` if dest is not a register starting with `%`.

### LOAD
* **Syntax**: `%dest = load %addr`
* **Description**: Loads value from the variable at `%addr` into a register.
* **Operands**: `dest` (target register), `addr` (address register).
* **Errors**: `NX-402` at runtime if `%addr` is uninitialized.

### STORE
* **Syntax**: `store %src, %addr`
* **Description**: Stores value from `%src` (or direct literal) into `%addr`.
* **Operands**: `src` (source value), `addr` (target address).
* **Errors**: `NX-302` if missing parameters.

### CONST
* **Syntax**: `%dest = const <value>`
* **Description**: Loads constant numeric or string literal into a register.
* **Operands**: `dest` (target register), `value` (literal value).

---

## 2. Arithmetic Instructions

* **ADD**: `%dest = add %lhs, %rhs` (Performs `%lhs + %rhs`)
* **SUB**: `%dest = sub %lhs, %rhs` (Performs `%lhs - %rhs`)
* **MUL**: `%dest = mul %lhs, %rhs` (Performs `%lhs * %rhs`)
* **DIV**: `%dest = div %lhs, %rhs` (Performs `%lhs / %rhs`)
* **MOD**: `%dest = mod %lhs, %rhs` (Performs `%lhs % %rhs`)

* **Errors**:
  - `NX-401` at runtime if `%rhs` evaluates to `0` in `DIV` or `MOD`.
  - `NX-402` at runtime if operands are not integers in `MOD`.

---

## 3. Comparison Instructions

All operations return `1` (true) or `0` (false) into the target register:
* **EQ**: `%dest = eq %lhs, %rhs`
* **NE**: `%dest = ne %lhs, %rhs`
* **LT**: `%dest = lt %lhs, %rhs`
* **LE**: `%dest = le %lhs, %rhs`
* **GT**: `%dest = gt %lhs, %rhs`
* **GE**: `%dest = ge %lhs, %rhs`

---

## 4. Control Flow Instructions

### LABEL
* **Syntax**: `<name>:`
* **Description**: Defines jump point targets.
* **Errors**: `NX-305` if duplicate label is defined in the same function scope.

### BR
* **Syntax**: `br label %<target>`
* **Description**: Unconditional branch.
* **Errors**: `NX-304` if `%<target>` label is undefined.

### CBR
* **Syntax**: `cbr %cond, label %<then_target>, label %<else_target>`
* **Description**: Conditional branch based on `%cond` (zero is false, non-zero is true).
* **Errors**: `NX-304` if jump target label is undefined.

---

## 5. Function & Execution Instructions

### CALL
* **Syntax**: `%dest = call @<func_name>(%arg1, %arg2, ...)`
* **Description**: Call function and store return value.
* **Errors**: `NX-404` at runtime if `<func_name>` is undefined.

### RET
* **Syntax**: `ret %val` or `ret`
* **Description**: Return control to caller frame.
* **Errors**: `NX-406` at runtime if stack underflow occurs.

### PRINT
* **Syntax**: `print %val`
* **Description**: Print register value or literal directly to stdout.

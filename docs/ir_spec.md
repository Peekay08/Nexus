# NEXUS IR Specification (v1.0.0)

This document specifies the frozen instruction set and serialization format for NEXUS Intermediate Representation (IR).

## File Header

Every valid NEXUS IR file (`.nxs`) must start with the exact version header on the first non-empty line:
```text
; NEXUS IR v1.0
```

## Functions

Functions are defined using the `define` keyword:
```text
define <return_type> @<name>(%param1, %param2, ...) {
  <instructions>
}
```

## Instruction Set

### 1. Memory Operations

#### `alloca`
Allocates stack memory for a variable.
- Syntax: `%addr_x = alloca <type>`
- Example: `%addr_x = alloca int`

#### `load`
Loads a value from stack memory into a virtual register.
- Syntax: `%dest = load %addr_x`
- Example: `%0 = load %addr_x`

#### `store`
Stores a value (or register) into stack memory.
- Syntax: `store %val, %addr_x`
- Example: `store 42, %addr_x`

#### `const`
Assigns a constant literal (int, float, or string) to a virtual register.
- Syntax: `%dest = const <value>`
- Example: `%0 = const 5`

---

### 2. Arithmetic Operations

Arithmetic instructions operate on registers or constants and assign the result to a destination register:
- **`add`**: Adds two operands. E.g. `%2 = add %0, %1`
- **`sub`**: Subtracts second operand from first. E.g. `%2 = sub %0, %1`
- **`mul`**: Multiplies two operands. E.g. `%2 = mul %0, %1`
- **`div`**: Divides first operand by second. E.g. `%2 = div %0, %1`
- **`mod`**: Calculates remainder of division. E.g. `%2 = mod %0, %1`

---

### 3. Comparison Operations

Evaluate conditional relations. Result is `1` (true) or `0` (false) stored in the destination register:
- **`eq`**: Equal to.
- **`ne`**: Not equal to.
- **`lt`**: Less than.
- **`le`**: Less than or equal to.
- **`gt`**: Greater than.
- **`ge`**: Greater than or equal to.

---

### 4. Control Flow Operations

#### `label`
Defines a branch destination point.
- Syntax: `<label_name>:`
- Example: `cond_0:`

#### `br`
Unconditional jump to a label.
- Syntax: `br label %<label_name>`
- Example: `br label %cond_0`

#### `cbr`
Conditional branch. Jumps to first label if register evaluates to true (non-zero), otherwise jumps to second label.
- Syntax: `cbr %cond, label %<then_label>, label %<else_label>`
- Example: `cbr %0, label %body_0, label %end_0`

---

### 5. Function Operations

#### `call`
Calls a function with arguments and stores return value.
- Syntax: `%dest = call @<func_name>(%arg1, %arg2, ...)`
- Example: `%2 = call @add(%0, %1)`

#### `ret`
Returns from the current function.
- Syntax: `ret %val` or `ret` (for void functions)
- Example: `ret %0`

---

### 6. Runtime Built-ins

#### `print`
Outputs a register or literal directly to stdout.
- Syntax: `print %val`
- Example: `print %0`

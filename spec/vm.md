# NEXUS VM Execution Model Specification (v1.0.0)

This document specifies the execution architecture, memory model, and runtime constraints of the NEXUS Virtual Machine (VM).

---

## 1. Execution Model & State

The Virtual Machine executes register-based intermediate representation instructions. It processes instructions sequentially until either a return instruction is reached in the entry frame or an error is encountered.

The global state of the VM consists of:
* **Function Table**: Map of function signatures (e.g. `@main`, `@fact`) to instructions.
* **Call Stack**: A dynamic list of executing function frames.
* **File Handles Map**: List of open file descriptors mapped to unique integer identifiers.

---

## 2. Stack Frame Architecture

Every active function execution context is encapsulated in a `StackFrame`:

* **Instruction Pointer (IP)**: Index of the current instruction executing in the function.
* **Virtual Registers**: A map of temporary registers (SSA form: e.g. `%0`, `%1`) storing `int`, `float`, or `std::string`.
* **Local Variables**: A map of stack allocations initialized via `alloca`.
* **Local Labels**: Offset dictionary of label names to index numbers in the function instructions.

---

## 3. Function Calling & Argument Passing

When executing `call @func(args...)`:
1. The VM verifies that the target `@func` exists in the global Function Table. If missing, execution halts with `NX-404`.
2. A new `StackFrame` is allocated.
3. The arguments specified in the call command are evaluated in the caller's frame and mapped to the callee's parameter registers (`%param_name`) in the new frame.
4. The callee's labels are indexed into the frame dictionary.
5. The stack frame is pushed onto the global Call Stack.
6. Execution proceeds at instruction pointer `0` of the callee function.

### Return Execution (`ret`)
1. The return value is evaluated in the callee frame.
2. The VM pops the current stack frame.
3. If the stack is empty, the return value becomes the final program result.
4. If a parent frame exists, the return value is loaded into the caller's target destination register (if any) at the instruction pointer where the call was initiated.

---

## 4. Safety Constraints & Error Contracts

* **Stack Depth Limit**: The call stack cannot exceed **1000 frames**. Crossing this limit halts execution and triggers a stack overflow exception `NX-407`.
* **Division by Zero**: Dividing/modulo dividing any integer or float by `0` triggers `NX-401`.
* **Invalid Register Ref**: Accessing any register or variable that has not been defined/initialized in the current frame triggers `NX-402`.
* **Invalid File IO**: Using an integer handle in file operations that does not exist or has been closed triggers `NX-405`.

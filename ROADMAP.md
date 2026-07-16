# NEXUS Development Roadmap

This document outlines the development directions for the NEXUS platform.

## NEXUS 1.x: Stability & Tooling
- **Optimization passes**: Introduce copy propagation and simple loop invariant code motion (LICM).
- **Interactive Debugger**: Build a command-line debugger (`nxs dbg`) to step through `.nxs` IR instructions, print stack frames, and watch register state changes.
- **Diagnostics refinement**: Enhance parsing error recovery to report multiple errors rather than aborting on the first failure.
- **Detailed VM performance counters**: Profiling tools to measure CPU/Memory usage per VM instruction.

## NEXUS 2.x: Platform Evolution
- **Extended VM Data Types**: Support array types, pointers, and structures.
- **C++ Standard Library Extensions**: Implement basic string utilities and formatting functions inside `nexus_builtins.h`.
- **AOT compilation**: Introduce native binary code generator targets (`x86_64-windows`, `x86_64-linux`, and `aarch64-android`) using LLVM bindings or custom backends.

## NEXUS 3.x: Ecosystem Integration
- **Game Engine Runtimes**: Integrate NEXUS as a scripting layer inside game engine runtimes.
- **Sandboxed Execution**: Implement micro-containerization and IAM policies for secure, remote script execution.
- **Robotics Control VM**: Adapt the virtual machine for real-time safety critical robotics control loops.

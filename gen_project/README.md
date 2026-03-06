# AQ Native Generator

This directory contains the tools to "bake" a Lisp-defined audio graph from `demo/main.fe` into native C code. This allows you to bypass the Lisp runtime for production or performance-critical environments.

## Files
- `generator.fe`: The Lisp script that shadows the DSP API and emits C code for the entire graph structure.
- `wrapper.c`: A minimal native host that initializes SDL/DSP and calls the generated `setup_graph()`.
- `Makefile`: Compiles the generated C code and `wrapper.c` against the core AQ DSP library.
- `compile_test.sh`: Orchestrates the generation and build process.

## Workflow
1. **Design** your instruments and routing in the high-level `demo/main.fe` script using the standard `aq` environment.
2. **Export** the graph to C by running:
   ```bash
   cd gen_project
   ./compile_test.sh
   ```
3. **Verify** the results by running the compiled `./test_app`.
4. **Iterate** by making changes in `demo/` and re-running the export script.

## Structure vs. Behavior
- **Structure (Automated)**: All `dsp:new` and `dsp:link` calls (including those inside factory functions like `make-bass`) are automatically recorded and emitted as C initialization code.
- **Behavior (Manual)**: High-level logic such as sequencers (`on-tick`) or UI sliders that call `dsp:set` are not automatically transpiled to C. For a full production build, you should manually reimplement your sequencer logic in `wrapper.c`'s `tick_callback` to maintain high performance.

## Git Management
- **Commit**: `generator.fe`, `wrapper.c`, `Makefile`, and `compile_test.sh`.
- **Ignore**: Generated artifacts like `graph.c`, `full_graph.c`, object files (`*.o`), and the compiled binary (`test_app`).

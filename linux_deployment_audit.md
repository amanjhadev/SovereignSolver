# Sovereign Solver — Linux Deployment Audit

**Date**: August 30, 2026  
**Target Architecture**: Linux Container (Node.js Express + C++ Sovereign Solver CLI)  
**Repository**: `https://github.com/amanjhadev/SovereignSolver`

---

## 1. Executive Summary

This audit evaluates the codebase located at `C:\Solver` for Linux containerization readiness. The Sovereign Solver project consists of a high-performance C++20 optimization engine and a thin Node.js Express REST API backend (`workbench/backend`). 

The audit identified specific Windows platform dependencies, path resolution assumptions, and build configuration issues that require portability refactoring.

---

## 2. Component Audit Findings

### 2.1 C++ Build System (`CMakeLists.txt`)

- **Primary Target Name**: `workbench_cli` (defined at line 262: `add_executable(workbench_cli src/cli/sovereign_cli.cpp)`).
- **Secondary Target Name**: `sovereign_cli` (defined at line 117: `add_executable(sovereign_cli apps/sovereign_cli.cpp)`).
- **Target Source**: `src/cli/sovereign_cli.cpp` parses `--mps`, `--timeout`, `--threads`, `--validate-only`, `--stream-telemetry`, and emits structured JSON output to stdout.
- **Portability Barrier**: Lines 126–304 in `CMakeLists.txt` include 35+ hardcoded absolute Windows paths to developer scratch files (`C:/Users/amanj/.gemini/antigravity/brain/...`). When compiling inside a Linux container, CMake configuration fails if these target files do not exist.
- **Compiler Compatibility**: Uses `cxx_std_20`. MSVC flags (`/W4 /permissive-`) and GCC/Clang flags (`-Wall -Wextra -Wpedantic`) are properly conditioned.

### 2.2 Backend CLI Path Resolution (`workbench/backend/server.js`)

- **Current Implementation**:
  ```javascript
  const CLI_PATH = path.resolve(__dirname, '../../build/Release/workbench_cli.exe');
  ```
- **Portability Issues**:
  1. **Hardcoded `.exe` extension**: Linux binaries do not use `.exe`.
  2. **Hardcoded MSVC multi-config path (`Release/`)**: On Linux Makefile/Ninja generators, single-config builds place the binary at `build/workbench_cli`.
  3. **No Environment Variable Support**: `SOVEREIGN_SOLVER_PATH` environment variable is not queried.

### 2.3 Network Binding & Port Handling (`workbench/backend/server.js`)

- **Current Implementation**:
  ```javascript
  const PORT = process.env.PORT || 3001;
  app.listen(PORT, () => { ... });
  ```
- **Portability Issue**: Omitting the host address in `app.listen(PORT, ...)` defaults to local binding behaviour depending on OS/Node version. For Docker containers, Express must explicitly bind to `0.0.0.0` so container port forwarding operates correctly.

### 2.4 Health Check API (`/api/health`)

- **Current Implementation**:
  ```javascript
  app.get('/api/health', (req, res) => {
    const cliExists = fs.existsSync(CLI_PATH);
    return res.json({
      status: 'ok',
      version: '0.2.0',
      solver_executable: CLI_PATH,
      solver_ready: cliExists
    });
  });
  ```
- **Audit Assessment**: Correctly checks `fs.existsSync(CLI_PATH)`. However, `CLI_PATH` resolution must be updated to inspect `process.env.SOVEREIGN_SOLVER_PATH` and fallback paths before returning state.

### 2.5 Temp Files & Directory Handling

- **Current Implementation**:
  - `temp_models` created via `fs.mkdirSync(TEMP_DIR, { recursive: true })`.
  - `multer` uses `temp_uploads` destination.
- **Audit Assessment**: Directory creation uses `path.join(__dirname, ...)` which is cross-platform portable. Ensure `temp_uploads` is also guaranteed to exist prior to uploads.

---

## 3. Required Portability Changes Summary

| File | Current State | Required Modification |
| :--- | :--- | :--- |
| `workbench/backend/server.js` | Hardcoded `../../build/Release/workbench_cli.exe` | Implement dynamic `SOVEREIGN_SOLVER_PATH` env lookup + cross-platform relative path fallbacks |
| `workbench/backend/server.js` | `app.listen(PORT)` without host parameter | Explicitly bind to `HOST = process.env.HOST \|\| '0.0.0.0'` |
| `CMakeLists.txt` | Hardcoded Windows scratch paths causing CMake failure | Clean up / wrap scratch targets with `if(EXISTS ...)` |
| `Dockerfile` | Missing | Add 2-stage Linux build (`ubuntu:22.04` builder + `node:20-slim` runtime) |
| `.dockerignore` | Missing | Create `.dockerignore` excluding `build/`, `node_modules/`, `.git/`, `.env` |
| `.env.example` | Missing | Create `.env.example` specifying `PORT=3001`, `HOST=0.0.0.0`, `SOVEREIGN_SOLVER_PATH=/app/bin/workbench_cli` |

---

## 4. Benchmark Model Scope

Per deployment instructions, model execution during container verification is strictly restricted to:
- `test_assignment_4x4.mps` (Linear / MILP Assignment model, expected objective = `13.000000`, status = `Optimal`).

Longer benchmarks (`10teams.mps`, `control30-3-2-3.mps`) will be excluded from deployment validation.

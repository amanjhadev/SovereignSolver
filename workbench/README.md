# Sovereign Workbench — Autonomous Optimization Web GUI

The **Sovereign Workbench** provides an industrial-grade mathematical optimization application interface for creating, editing, validating, and solving Linear Programming (LP) and Mixed-Integer Linear Programming (MILP) models directly using the C++ Sovereign Solver engine.

---

## 1. System Architecture

```
┌────────────────────────────────────────────────────────┐
│               React + TypeScript + Vite                │
│             (Sovereign Workbench UI App)               │
└───────────────────────────┬────────────────────────────┘
                            │  HTTP REST API
                            ▼
┌────────────────────────────────────────────────────────┐
│                   Node.js Express                      │
│                (Thin Backend Gateway)                  │
└───────────────────────────┬────────────────────────────┘
                            │  Child Process Execution
                            ▼
┌────────────────────────────────────────────────────────┐
│              Sovereign C++ CLI Engine                  │
│     (DualSimplexSolver + ParallelMILPSolver + LU)     │
└───────────────────────────┴────────────────────────────┘
```

---

## 2. Primary Optimization Workflow

```
┌─────────────────┐     ┌────────────────────────┐     ┌────────── font-mono ──────────┐
│   NEW PROBLEM   │ ──> │  ENTER / UPLOAD MODEL  │ ──> │          VALIDATE             │
└─────────────────┘     └────────────────────────┘     └────────────── font-mono ──────┘
                                                                       │
                                                                       ▼
┌─────────────────┐     ┌────────────────────────┐     ┌───────────────────────────────┐
│ RESULTS & DATA  │ <── │        SOLVING         │ <── │          SOLVE MODEL          │
└─────────────────┘     └────────────────────────┘     └───────────────────────────────┘
```

1. **New Problem Creation**: Click `+ NEW PROBLEM` to name a problem and select between **Structured Model** text syntax or **MPS Upload** format.
2. **Model Authoring / Upload**: Edit structured models with monospaced line-numbered code editor or drag & drop standard `.mps` / `.mps.gz` benchmark files.
3. **Syntax Validation**: Click **VALIDATE** to parse model dimensions, binary/continuous variable counts, constraint counts, and nonzeros before solving.
4. **Solve Execution**: Click **SOLVE MODEL** to run the C++ solver backend with real-time elapsed duration and cancellation (`STOP SOLVE`).
5. **Results & Telemetry**: Inspect status badges (`OPTIMAL`, `TIME LIMIT`, `FEASIBLE`, `INFEASIBLE`, etc.), objective value, best bound, MIP gap %, solution table (searchable, filterable by non-zero, sortable), telemetry diagnostics, MIP progress convergence, B&B search tree, and execution logs.

---

## 3. Prerequisites

- **C++ Compiler**: MSVC 2022 / Visual Studio (Windows C++20) or GCC / Clang (Linux/macOS)
- **CMake**: 3.20 or newer
- **Node.js**: v18.0.0 or newer

---

## 4. Build & Setup Instructions

### Step 1: Build C++ Sovereign Solver CLI Engine
From the repository root (`c:\Solver`):

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target workbench_cli
```

*This compiles `build/Release/workbench_cli.exe`.*

---

### Step 2: Start Backend Server
From `workbench/backend`:

```bash
cd workbench/backend
npm install
node server.js
```

*Backend REST API runs on `http://localhost:3001`.*

Endpoints:
- `GET  /api/health` - Backend connectivity & solver binary readiness check
- `POST /api/validate` - Model syntax & structure validation
- `POST /api/solve` - Model optimization solve execution
- `POST /api/stop-solve` - Terminate active solve process
- `POST /api/upload-mps` - Upload and parse `.mps` and `.mps.gz` files

---

### Step 3: Start Frontend Workbench UI
From `workbench/frontend`:

```bash
cd workbench/frontend
npm install
npm run dev
```

*Frontend web application opens at `http://localhost:3000`.*

---

## 5. Input Formats & Presets

### A. Structured Model Text Format
```text
VARIABLES
x1 binary
x2 binary
x3 continuous

MINIMIZE
9*x1 + 4*x2 + 3*x3

SUBJECT TO
job1: x1 + x2 >= 1
job2: x2 + x3 <= 10
```

### B. MPS & GZIP MPS Upload
Upload raw standard `.mps` or compressed `.mps.gz` files directly via drag and drop.

### C. Preset Benchmark Examples
- **Assignment 4×4** (Optimal = 13.0)
- **Capacitated Facility Location 5×8** (MILP 45 vars, 13 cons)
- **10teams** (MIPLIB benchmark)
- **control30-3-2-3** (MIPLIB benchmark)

---

## 6. Solver Status Semantics

The Workbench UI faithfully reports solver statuses directly from backend CLI execution:
- `OPTIMAL` - Proven optimal solution found
- `FEASIBLE` - Valid integer incumbent found within limit
- `TIME LIMIT` - Search terminated at configured timeout limit
- `NODE LIMIT` - Search terminated at node limit
- `INFEASIBLE` - Problem proven infeasible
- `UNBOUNDED` - Problem proven unbounded
- `NUMERICAL FAILURE` - Solver encountered numerical difficulty
- `USER INTERRUPTED` - Solve process stopped by user request
- `UNRESOLVED` / `NOT SOLVED` - Search incomplete or not run

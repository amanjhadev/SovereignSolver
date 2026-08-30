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
┌─────────────────┐     ┌────────────────────────┐     ┌───────────────────────────────┐
│   NEW PROBLEM   │ ──> │  ENTER / UPLOAD MODEL  │ ──> │          VALIDATE             │
└─────────────────┘     └────────────────────────┘     └───────────────────────────────┘
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

- **C++ Compiler**: MSVC 2022 / Visual Studio (Windows C++20) or GCC / Clang (Linux/macOS C++20)
- **CMake**: 3.20 or newer
- **Node.js**: v18.0.0 or newer
- **Docker** *(Optional for Container Deployment)*: Docker Desktop / Engine v20.10+

---

## 4. Build & Setup Instructions

### Option A: Local Windows / Host Development

#### Step 1: Build C++ Sovereign Solver CLI Engine
From the repository root:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target workbench_cli
```

*This compiles `build/Release/workbench_cli.exe` (Windows) or `build/workbench_cli` (Linux).*

#### Step 2: Start Backend Server
From `workbench/backend`:

```bash
cd workbench/backend
npm install
node server.js
```

*Backend REST API runs on `http://localhost:3001` (or `http://0.0.0.0:3001`).*

---

### Option B: Linux Docker Container Deployment

#### Step 1: Build Docker Image
From the repository root:

```bash
docker build -t sovereign-backend .
```

*This triggers a 2-stage build: compiling the C++ `workbench_cli` engine using GCC inside Ubuntu 22.04 and packaging it with the Node.js 20 Express backend.*

#### Step 2: Run Container
```bash
docker run --rm -p 3001:3001 sovereign-backend
```

#### Step 3: Health & Solver Verification

1. **Health Check**:
   ```bash
   curl http://localhost:3001/api/health
   ```
   *Expected Response:*
   ```json
   {
     "status": "ok",
     "version": "0.2.0",
     "solver_executable": "/app/bin/workbench_cli",
     "solver_path": "/app/bin/workbench_cli",
     "solver_exists": true,
     "solver_ready": true,
     "solver_version": "0.2.0"
   }
   ```

2. **Test Solver Request (`test_assignment_4x4.mps`)**:
   ```bash
   curl -X POST http://localhost:3001/api/solve \
     -H "Content-Type: application/json" \
     -d "{\"type\":\"mps\",\"content\":\"$(cat test_assignment_4x4.mps | sed 's/"/\\"/g' | s/$/\\n/ | tr -d '\r\n')\",\"modelName\":\"test_assignment_4x4\"}"
   ```
   *Expected Result:* `"status_name": "Optimal"`, `"objective_value": 13`.

---

## 5. API Endpoints

- `GET  /api/health` - Backend status & C++ solver binary readiness check
- `POST /api/validate` - Model syntax & structure validation
- `POST /api/solve` - Model optimization solve execution (supports streaming SSE)
- `POST /api/stop-solve` - Terminate active solve process
- `POST /api/upload-mps` - Upload and parse `.mps` and `.mps.gz` files

---

## 6. Frontend Environment Configuration

To configure the React frontend to communicate with the deployed backend API, set `VITE_API_BASE_URL` in `workbench/frontend/.env`:

```env
VITE_API_BASE_URL=http://localhost:3001
```

*(Or point to your deployed backend container URL).*

---

## 7. Input Formats & Presets

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

---

## 8. Solver Status Semantics

- `OPTIMAL` - Proven optimal solution found
- `FEASIBLE` - Valid integer incumbent found within limit
- `TIME LIMIT` - Search terminated at configured timeout limit
- `NODE LIMIT` - Search terminated at node limit
- `INFEASIBLE` - Problem proven infeasible
- `UNBOUNDED` - Problem proven unbounded
- `NUMERICAL FAILURE` - Solver encountered numerical difficulty
- `USER INTERRUPTED` - Solve process stopped by user request
- `UNRESOLVED` / `NOT SOLVED` - Search incomplete or not run

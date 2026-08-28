#ifndef SOVEREIGN_PUBLIC_SOLVER_API_HPP
#define SOVEREIGN_PUBLIC_SOLVER_API_HPP

// Sovereign Solver Core & API Layers
#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/core/thread_pool.hpp"

// LP & Presolve Engine
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/presolve_solver.hpp"
#include "sovereign/lp/lp_writer.hpp"

// Barrier LP & Barrier QP
#include "sovereign/barrier/barrier_solver.hpp"
#include "sovereign/qp/qp_model.hpp"
#include "sovereign/qp/qp_solver.hpp"

// MILP Branch-and-Bound Engine
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/milp/parallel_milp_solver.hpp"

// Industrial Model Builders & Diagnostics Layer
#include "sovereign/industrial/blending_model.hpp"
#include "sovereign/industrial/power_dispatch.hpp"
#include "sovereign/industrial/production_planning.hpp"
#include "sovereign/industrial/supply_chain.hpp"
#include "sovereign/industrial/industrial_diagnostics.hpp"
#include "sovereign/industrial/industrial_scaling.hpp"
#include "sovereign/industrial/solver_selector.hpp"
#include "sovereign/industrial/run_report.hpp"
#include "sovereign/industrial/stress_test_generator.hpp"

#endif // SOVEREIGN_PUBLIC_SOLVER_API_HPP

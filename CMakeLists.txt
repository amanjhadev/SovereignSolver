cmake_minimum_required(VERSION 3.20)
project(SovereignSolver VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core, LinAlg, LP, Presolve, Barrier, QP, and MILP Subsystem Source files
set(SOVEREIGN_SOURCES
    src/core/status.cpp
    src/core/config.cpp
    src/core/logger.cpp
    src/core/arena.cpp
    src/core/thread_pool.cpp
    src/linalg/coo_matrix.cpp
    src/linalg/csc_matrix.cpp
    src/linalg/csr_matrix.cpp
    src/linalg/lu_factor.cpp
    src/linalg/residual.cpp
    src/lp/lp_model.cpp
    src/lp/basis.cpp
    src/lp/basis_matrix.cpp
    src/lp/initial_basis.cpp
    src/lp/mps_parser.cpp
    src/lp/lp_validator.cpp
    src/lp/simplex_state.cpp
    src/lp/ratio_test.cpp
    src/lp/devex_pricing.cpp
    src/lp/primal_simplex.cpp
    src/lp/dual_simplex.cpp
    src/lp/presolve_solver.cpp
    src/presolve/presolve_stack.cpp
    src/presolve/presolve_engine.cpp
    src/presolve/postsolve_engine.cpp
    src/barrier/sparse_cholesky.cpp
    src/barrier/barrier_solver.cpp
    src/barrier/presolve_barrier_solver.cpp
    src/qp/qp_model.cpp
    src/qp/qp_solver.cpp
    src/qp/presolve_qp_solver.cpp
    src/milp/milp_model.cpp
    src/milp/bb_node.cpp
    src/milp/branching.cpp
    src/milp/cut_pool.cpp
    src/milp/gomory_cut.cpp
    src/milp/mir_cut.cpp
    src/milp/cover_cut.cpp
    src/milp/feasibility_pump.cpp
    src/milp/diving_heuristic.cpp
    src/milp/rins_heuristic.cpp
    src/milp/milp_solver.cpp
    src/milp/presolve_milp_solver.cpp
    src/milp/atomic_incumbent.cpp
    src/milp/concurrent_tree_manager.cpp
    src/milp/parallel_milp_solver.cpp
    src/lp/lp_writer.cpp
    src/industrial/industrial_diagnostics.cpp
    src/industrial/industrial_scaling.cpp
    src/industrial/solver_selector.cpp
    src/industrial/blending_model.cpp
    src/industrial/power_dispatch.cpp
    src/industrial/production_planning.cpp
    src/industrial/supply_chain.cpp
    src/industrial/stress_test_generator.cpp
    src/industrial/run_report.cpp
)

# Sovereign Library Target
add_library(sovereign_lib STATIC ${SOVEREIGN_SOURCES})
target_include_directories(sovereign_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(sovereign_lib PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(sovereign_lib PRIVATE /W4 /permissive-)
else()
    target_compile_options(sovereign_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test Suite Executable Target
set(TEST_SOURCES
    tests/main_test.cpp
    tests/test_core.cpp
    tests/test_sparse_matrix.cpp
    tests/test_lu_factor.cpp
    tests/test_lp_model.cpp
    tests/test_basis.cpp
    tests/test_mps_parser.cpp
    tests/test_lp_infrastructure.cpp
    tests/test_hardening.cpp
    tests/test_simplex.cpp
    tests/test_validation_extension.cpp
    tests/test_presolve.cpp
    tests/test_barrier.cpp
    tests/test_qp.cpp
    tests/test_milp.cpp
    tests/test_parallel_milp.cpp
    tests/test_industrial.cpp
    tests/test_basis_update.cpp
    tests/test_phase1_transition.cpp
    tests/test_feasibility_pump.cpp
    tests/test_bb_status.cpp
)

add_executable(sovereign_tests ${TEST_SOURCES})
target_link_libraries(sovereign_tests PRIVATE sovereign_lib)
target_compile_features(sovereign_tests PRIVATE cxx_std_20)

# Benchmark Harness Executable Target
add_executable(sovereign_benchmark apps/solver_benchmark.cpp)
target_link_libraries(sovereign_benchmark PRIVATE sovereign_lib)
target_compile_features(sovereign_benchmark PRIVATE cxx_std_20)

# Industrial CLI Executable Target
add_executable(sovereign_cli apps/sovereign_cli.cpp)
target_link_libraries(sovereign_cli PRIVATE sovereign_lib)
target_compile_features(sovereign_cli PRIVATE cxx_std_20)

# Workbench CLI Target (used by Node.js Express backend)
add_executable(workbench_cli src/cli/sovereign_cli.cpp)
target_link_libraries(workbench_cli PRIVATE sovereign_lib)
target_compile_features(workbench_cli PRIVATE cxx_std_20)

# Scalability Validation Benchmark Target
add_executable(sovereign_scalability apps/scalability_benchmark.cpp)
target_link_libraries(sovereign_scalability PRIVATE sovereign_lib)
target_compile_features(sovereign_scalability PRIVATE cxx_std_20)

# Helper function to conditionally register scratch executables if files exist on host
function(add_scratch_target TARGET_NAME SOURCE_PATH)
    if(EXISTS ${SOURCE_PATH})
        add_executable(${TARGET_NAME} ${SOURCE_PATH})
        target_link_libraries(${TARGET_NAME} PRIVATE sovereign_lib)
        target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    endif()
endfunction()

add_scratch_target(checkpoint_10teams C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/checkpoint_10teams.cpp)
add_scratch_target(diagnostic_10teams C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/diagnostic_10teams.cpp)
add_scratch_target(diagnostic_phase2_convergence C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/diagnostic_phase2_convergence.cpp)
add_scratch_target(run_10teams_milp_checkpoint C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_milp_checkpoint.cpp)
add_scratch_target(run_10teams_bb_profile C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_bb_profile.cpp)
add_scratch_target(audit_equivalence C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_equivalence.cpp)
add_scratch_target(profile_fpump_10teams C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/profile_fpump_10teams.cpp)
add_scratch_target(run_10teams_fp_checkpoint C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_fp_checkpoint.cpp)
add_scratch_target(run_10teams_post_fp_checkpoint C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_post_fp_checkpoint.cpp)
add_scratch_target(audit_bb_correctness C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_bb_correctness.cpp)
add_scratch_target(run_10teams_bb_status_regression C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_bb_status_regression.cpp)
add_scratch_target(benchmark_lu_factor C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/benchmark_lu_factor.cpp)
add_scratch_target(validate_sparse_lu_post C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/validate_sparse_lu_post.cpp)
add_scratch_target(validate_simplex_pfi_child C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/validate_simplex_pfi_child.cpp)
add_scratch_target(run_10teams_full_milp C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_full_milp.cpp)
add_scratch_target(profile_10teams_child_lps C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/profile_10teams_child_lps.cpp)
add_scratch_target(checkpoint_5node_warmstart C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/checkpoint_5node_warmstart.cpp)
add_scratch_target(trace_warmstart_node1 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/trace_warmstart_node1.cpp)
add_scratch_target(diagnose_warmstart_iterations C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/diagnose_warmstart_iterations.cpp)
add_scratch_target(compare_warmstart_trajectory C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/compare_warmstart_trajectory.cpp)
add_scratch_target(study_child_initializations C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/study_child_initializations.cpp)
add_scratch_target(test_targeted_basis_repair C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_targeted_basis_repair.cpp)
add_scratch_target(simulate_warm_phase1 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/simulate_warm_phase1.cpp)
add_scratch_target(checkpoint_warm_phase1 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/checkpoint_warm_phase1.cpp)
add_scratch_target(profile_warm_phase1_breakdown C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/profile_warm_phase1_breakdown.cpp)
add_scratch_target(debug_parent_basis_singularity C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/debug_parent_basis_singularity.cpp)
add_scratch_target(inspect_control30 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/inspect_control30.cpp)
add_scratch_target(solve_control30_serial C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/solve_control30_serial.cpp)
add_scratch_target(validate_control30_incumbent C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/validate_control30_incumbent.cpp)
add_scratch_target(inspect_root_cuts C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/inspect_root_cuts.cpp)
add_scratch_target(inspect_obj_coeffs C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/inspect_obj_coeffs.cpp)
add_scratch_target(test_heuristics_control30 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_heuristics_control30.cpp)
add_scratch_target(run_assignment_benchmark C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_assignment_benchmark.cpp)
add_scratch_target(run_facility_benchmark C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_facility_benchmark.cpp)
add_scratch_target(audit_knapsack_18bin C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_knapsack_18bin.cpp)
add_scratch_target(test_guided_diving C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_guided_diving.cpp)
add_scratch_target(audit_contradiction C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_contradiction_exact_mps.cpp)
add_scratch_target(run_experiment C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_guided_diving_experiment.cpp)
add_scratch_target(find_weights C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/find_exact_weights.cpp)
add_scratch_target(test_370 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_exact_370_benchmark.cpp)
add_scratch_target(audit_bound C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_best_bound_correctness.cpp)
add_scratch_target(trace_path C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/trace_optimal_path.cpp)
add_scratch_target(find_370_vec C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/find_370_binary_vector.cpp)
add_scratch_target(test_c30_parser C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_control30_parser.cpp)

enable_testing()
add_test(NAME SovereignUnitTests COMMAND sovereign_tests)
set_tests_properties(SovereignUnitTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

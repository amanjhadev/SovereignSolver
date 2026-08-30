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

# Scalability Validation Benchmark Target
add_executable(sovereign_scalability apps/scalability_benchmark.cpp)
target_link_libraries(sovereign_scalability PRIVATE sovereign_lib)
target_compile_features(sovereign_scalability PRIVATE cxx_std_20)

add_executable(checkpoint_10teams C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/checkpoint_10teams.cpp)
target_link_libraries(checkpoint_10teams PRIVATE sovereign_lib)
target_compile_features(checkpoint_10teams PRIVATE cxx_std_20)

add_executable(diagnostic_10teams C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/diagnostic_10teams.cpp)
target_link_libraries(diagnostic_10teams PRIVATE sovereign_lib)
target_compile_features(diagnostic_10teams PRIVATE cxx_std_20)

add_executable(diagnostic_phase2_convergence C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/diagnostic_phase2_convergence.cpp)
target_link_libraries(diagnostic_phase2_convergence PRIVATE sovereign_lib)
target_compile_features(diagnostic_phase2_convergence PRIVATE cxx_std_20)

add_executable(run_10teams_milp_checkpoint C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_milp_checkpoint.cpp)
target_link_libraries(run_10teams_milp_checkpoint PRIVATE sovereign_lib)
target_compile_features(run_10teams_milp_checkpoint PRIVATE cxx_std_20)

add_executable(run_10teams_bb_profile C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_bb_profile.cpp)
target_link_libraries(run_10teams_bb_profile PRIVATE sovereign_lib)
target_compile_features(run_10teams_bb_profile PRIVATE cxx_std_20)

add_executable(audit_equivalence C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_equivalence.cpp)
target_link_libraries(audit_equivalence PRIVATE sovereign_lib)
target_compile_features(audit_equivalence PRIVATE cxx_std_20)

add_executable(profile_fpump_10teams C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/profile_fpump_10teams.cpp)
target_link_libraries(profile_fpump_10teams PRIVATE sovereign_lib)
target_compile_features(profile_fpump_10teams PRIVATE cxx_std_20)

add_executable(run_10teams_fp_checkpoint C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_fp_checkpoint.cpp)
target_link_libraries(run_10teams_fp_checkpoint PRIVATE sovereign_lib)
target_compile_features(run_10teams_fp_checkpoint PRIVATE cxx_std_20)

add_executable(run_10teams_post_fp_checkpoint C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_post_fp_checkpoint.cpp)
target_link_libraries(run_10teams_post_fp_checkpoint PRIVATE sovereign_lib)
target_compile_features(run_10teams_post_fp_checkpoint PRIVATE cxx_std_20)

add_executable(audit_bb_correctness C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_bb_correctness.cpp)
target_link_libraries(audit_bb_correctness PRIVATE sovereign_lib)
target_compile_features(audit_bb_correctness PRIVATE cxx_std_20)

add_executable(run_10teams_bb_status_regression C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_bb_status_regression.cpp)
target_link_libraries(run_10teams_bb_status_regression PRIVATE sovereign_lib)
target_compile_features(run_10teams_bb_status_regression PRIVATE cxx_std_20)

add_executable(benchmark_lu_factor C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/benchmark_lu_factor.cpp)
target_link_libraries(benchmark_lu_factor PRIVATE sovereign_lib)
target_compile_features(benchmark_lu_factor PRIVATE cxx_std_20)

add_executable(validate_sparse_lu_post C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/validate_sparse_lu_post.cpp)
target_link_libraries(validate_sparse_lu_post PRIVATE sovereign_lib)
target_compile_features(validate_sparse_lu_post PRIVATE cxx_std_20)

add_executable(validate_simplex_pfi_child C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/validate_simplex_pfi_child.cpp)
target_link_libraries(validate_simplex_pfi_child PRIVATE sovereign_lib)
target_compile_features(validate_simplex_pfi_child PRIVATE cxx_std_20)

add_executable(run_10teams_full_milp C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_10teams_full_milp.cpp)
target_link_libraries(run_10teams_full_milp PRIVATE sovereign_lib)
target_compile_features(run_10teams_full_milp PRIVATE cxx_std_20)

add_executable(profile_10teams_child_lps C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/profile_10teams_child_lps.cpp)
target_link_libraries(profile_10teams_child_lps PRIVATE sovereign_lib)
target_compile_features(profile_10teams_child_lps PRIVATE cxx_std_20)

add_executable(checkpoint_5node_warmstart C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/checkpoint_5node_warmstart.cpp)
target_link_libraries(checkpoint_5node_warmstart PRIVATE sovereign_lib)
target_compile_features(checkpoint_5node_warmstart PRIVATE cxx_std_20)

add_executable(trace_warmstart_node1 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/trace_warmstart_node1.cpp)
target_link_libraries(trace_warmstart_node1 PRIVATE sovereign_lib)
target_compile_features(trace_warmstart_node1 PRIVATE cxx_std_20)

add_executable(diagnose_warmstart_iterations C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/diagnose_warmstart_iterations.cpp)
target_link_libraries(diagnose_warmstart_iterations PRIVATE sovereign_lib)
target_compile_features(diagnose_warmstart_iterations PRIVATE cxx_std_20)

add_executable(compare_warmstart_trajectory C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/compare_warmstart_trajectory.cpp)
target_link_libraries(compare_warmstart_trajectory PRIVATE sovereign_lib)
target_compile_features(compare_warmstart_trajectory PRIVATE cxx_std_20)

add_executable(study_child_initializations C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/study_child_initializations.cpp)
target_link_libraries(study_child_initializations PRIVATE sovereign_lib)
target_compile_features(study_child_initializations PRIVATE cxx_std_20)

add_executable(test_targeted_basis_repair C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_targeted_basis_repair.cpp)
target_link_libraries(test_targeted_basis_repair PRIVATE sovereign_lib)
target_compile_features(test_targeted_basis_repair PRIVATE cxx_std_20)

add_executable(simulate_warm_phase1 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/simulate_warm_phase1.cpp)
target_link_libraries(simulate_warm_phase1 PRIVATE sovereign_lib)
target_compile_features(simulate_warm_phase1 PRIVATE cxx_std_20)

add_executable(checkpoint_warm_phase1 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/checkpoint_warm_phase1.cpp)
target_link_libraries(checkpoint_warm_phase1 PRIVATE sovereign_lib)
target_compile_features(checkpoint_warm_phase1 PRIVATE cxx_std_20)

add_executable(profile_warm_phase1_breakdown C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/profile_warm_phase1_breakdown.cpp)
target_link_libraries(profile_warm_phase1_breakdown PRIVATE sovereign_lib)
target_compile_features(profile_warm_phase1_breakdown PRIVATE cxx_std_20)

add_executable(debug_parent_basis_singularity C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/debug_parent_basis_singularity.cpp)
target_link_libraries(debug_parent_basis_singularity PRIVATE sovereign_lib)
target_compile_features(debug_parent_basis_singularity PRIVATE cxx_std_20)

add_executable(inspect_control30 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/inspect_control30.cpp)
target_link_libraries(inspect_control30 PRIVATE sovereign_lib)
target_compile_features(inspect_control30 PRIVATE cxx_std_20)

add_executable(solve_control30_serial C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/solve_control30_serial.cpp)
target_link_libraries(solve_control30_serial PRIVATE sovereign_lib)
target_compile_features(solve_control30_serial PRIVATE cxx_std_20)

add_executable(validate_control30_incumbent C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/validate_control30_incumbent.cpp)
target_link_libraries(validate_control30_incumbent PRIVATE sovereign_lib)
target_compile_features(validate_control30_incumbent PRIVATE cxx_std_20)

add_executable(inspect_root_cuts C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/inspect_root_cuts.cpp)
target_link_libraries(inspect_root_cuts PRIVATE sovereign_lib)
target_compile_features(inspect_root_cuts PRIVATE cxx_std_20)

add_executable(inspect_obj_coeffs C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/inspect_obj_coeffs.cpp)
target_link_libraries(inspect_obj_coeffs PRIVATE sovereign_lib)
target_compile_features(inspect_obj_coeffs PRIVATE cxx_std_20)

add_executable(test_heuristics_control30 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_heuristics_control30.cpp)
target_link_libraries(test_heuristics_control30 PRIVATE sovereign_lib)
target_compile_features(test_heuristics_control30 PRIVATE cxx_std_20)

add_executable(run_assignment_benchmark C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_assignment_benchmark.cpp)
target_link_libraries(run_assignment_benchmark PRIVATE sovereign_lib)
target_compile_features(run_assignment_benchmark PRIVATE cxx_std_20)

add_executable(run_facility_benchmark C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_facility_benchmark.cpp)
target_link_libraries(run_facility_benchmark PRIVATE sovereign_lib)
target_compile_features(run_facility_benchmark PRIVATE cxx_std_20)

add_executable(workbench_cli src/cli/sovereign_cli.cpp)
target_link_libraries(workbench_cli PRIVATE sovereign_lib)
target_compile_features(workbench_cli PRIVATE cxx_std_20)

add_executable(audit_knapsack_18bin C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_knapsack_18bin.cpp)
target_link_libraries(audit_knapsack_18bin PRIVATE sovereign_lib)
target_compile_features(audit_knapsack_18bin PRIVATE cxx_std_20)

add_executable(test_guided_diving C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_guided_diving.cpp)
target_link_libraries(test_guided_diving PRIVATE sovereign_lib)
target_compile_features(test_guided_diving PRIVATE cxx_std_20)

add_executable(audit_contradiction C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_contradiction_exact_mps.cpp)
target_link_libraries(audit_contradiction PRIVATE sovereign_lib)
target_compile_features(audit_contradiction PRIVATE cxx_std_20)

add_executable(run_experiment C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/run_guided_diving_experiment.cpp)
target_link_libraries(run_experiment PRIVATE sovereign_lib)
target_compile_features(run_experiment PRIVATE cxx_std_20)

add_executable(find_weights C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/find_exact_weights.cpp)
target_link_libraries(find_weights PRIVATE sovereign_lib)
target_compile_features(find_weights PRIVATE cxx_std_20)

add_executable(test_370 C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_exact_370_benchmark.cpp)
target_link_libraries(test_370 PRIVATE sovereign_lib)
target_compile_features(test_370 PRIVATE cxx_std_20)

add_executable(audit_bound C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/audit_best_bound_correctness.cpp)
target_link_libraries(audit_bound PRIVATE sovereign_lib)
target_compile_features(audit_bound PRIVATE cxx_std_20)

add_executable(trace_path C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/trace_optimal_path.cpp)
target_link_libraries(trace_path PRIVATE sovereign_lib)
target_compile_features(trace_path PRIVATE cxx_std_20)

add_executable(find_370_vec C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/find_370_binary_vector.cpp)
target_link_libraries(find_370_vec PRIVATE sovereign_lib)
target_compile_features(find_370_vec PRIVATE cxx_std_20)

add_executable(test_c30_parser C:/Users/amanj/.gemini/antigravity/brain/e9e92b58-3889-458c-a706-6cff9402d28c/scratch/test_control30_parser.cpp)
target_link_libraries(test_c30_parser PRIVATE sovereign_lib)
target_compile_features(test_c30_parser PRIVATE cxx_std_20)

enable_testing()
add_test(NAME SovereignUnitTests COMMAND sovereign_tests)

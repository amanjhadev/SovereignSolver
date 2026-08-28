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


enable_testing()
add_test(NAME SovereignUnitTests COMMAND sovereign_tests)

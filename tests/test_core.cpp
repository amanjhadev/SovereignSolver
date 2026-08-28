#include "test_framework.hpp"
#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/core/logger.hpp"
#include "sovereign/core/arena.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <vector>

TEST_CASE(CoreStatusAndTypes) {
    EXPECT_TRUE(sovereign::is_ok(sovereign::Status::Ok));
    EXPECT_FALSE(sovereign::is_ok(sovereign::Status::SingularMatrix));
    EXPECT_EQ(sovereign::status_to_string(sovereign::Status::Ok), "Ok");
    EXPECT_EQ(sovereign::status_to_string(sovereign::Status::SingularMatrix), "SingularMatrix");
}

TEST_CASE(CoreSolverConfig) {
    auto config = sovereign::SolverConfig::default_config();
    EXPECT_NEAR(config.pivot_threshold, 0.1, 1e-6);
    EXPECT_NEAR(config.zero_tolerance, 1e-13, 1e-15);
    EXPECT_EQ(config.verbosity_level, 1);
}

TEST_CASE(CoreLogger) {
    auto& logger = sovereign::Logger::instance();
    logger.set_level(sovereign::LogLevel::Off);
    EXPECT_EQ(static_cast<int>(logger.level()), static_cast<int>(sovereign::LogLevel::Off));
    logger.set_level(sovereign::LogLevel::Info);
}

TEST_CASE(CoreMemoryArena) {
    sovereign::MemoryArena arena(1024);
    EXPECT_EQ(arena.bytes_allocated(), 0);

    double* arr1 = arena.allocate_array<double>(10);
    EXPECT_TRUE(arr1 != nullptr);
    EXPECT_EQ(arena.bytes_allocated(), 80);

    arr1[0] = 42.0;
    arr1[9] = 99.0;
    EXPECT_NEAR(arr1[0], 42.0, 1e-12);
    EXPECT_NEAR(arr1[9], 99.0, 1e-12);

    int32_t* arr2 = arena.allocate_array<int32_t>(20);
    EXPECT_TRUE(arr2 != nullptr);
    EXPECT_TRUE(arena.bytes_allocated() >= 160);

    arena.reset();
    EXPECT_EQ(arena.bytes_allocated(), 0);
}

TEST_CASE(CoreVectorOps) {
    std::vector<double> x = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> y = {5.0, 6.0, 7.0, 8.0};

    double dot = sovereign::dot_product(x, y);
    EXPECT_NEAR(dot, 5.0 + 12.0 + 21.0 + 32.0, 1e-12); // 70.0

    sovereign::axpy(2.0, x, y); // y = y + 2*x = {7, 10, 13, 16}
    EXPECT_NEAR(y[0], 7.0, 1e-12);
    EXPECT_NEAR(y[1], 10.0, 1e-12);
    EXPECT_NEAR(y[2], 13.0, 1e-12);
    EXPECT_NEAR(y[3], 16.0, 1e-12);

    double inf_norm = sovereign::infinity_norm(y);
    EXPECT_NEAR(inf_norm, 16.0, 1e-12);

    double l1 = sovereign::l1_norm(x);
    EXPECT_NEAR(l1, 10.0, 1e-12);

    double l2 = sovereign::l2_norm(x); // sqrt(1 + 4 + 9 + 16) = sqrt(30)
    EXPECT_NEAR(l2, std::sqrt(30.0), 1e-12);
}

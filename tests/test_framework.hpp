#ifndef SOVEREIGN_TESTS_TEST_FRAMEWORK_HPP
#define SOVEREIGN_TESTS_TEST_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <chrono>

namespace sovereign::test {

struct TestResult {
    std::string test_name;
    bool passed{true};
    int assertions_passed{0};
    int assertions_failed{0};
    std::vector<std::string> failure_messages;
    double duration_ms{0.0};
};

class TestRegistry {
private:
    struct TestCaseEntry {
        std::string name;
        std::function<void(TestResult&)> fn;
    };
    std::vector<TestCaseEntry> tests_;

    TestRegistry() = default;

public:
    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void register_test(const std::string& name, std::function<void(TestResult&)> fn) {
        tests_.push_back({name, fn});
    }

    int run_all() {
        int total_passed = 0;
        int total_failed = 0;
        double total_duration = 0.0;

        std::cout << "========================================================\n";
        std::cout << "  SOVEREIGN SOLVER BATCH 1 QUALITY GATE TEST SUITE\n";
        std::cout << "========================================================\n\n";

        for (const auto& test : tests_) {
            TestResult res;
            res.test_name = test.name;

            std::cout << " [RUN ] " << test.name << "..." << std::flush;

            auto start = std::chrono::high_resolution_clock::now();
            try {
                test.fn(res);
            } catch (const std::exception& e) {
                res.passed = false;
                res.assertions_failed++;
                res.failure_messages.push_back(std::string("Unhandled Exception: ") + e.what());
            } catch (...) {
                res.passed = false;
                res.assertions_failed++;
                res.failure_messages.push_back("Unhandled Unknown Exception");
            }
            auto end = std::chrono::high_resolution_clock::now();
            res.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            total_duration += res.duration_ms;

            if (res.passed) {
                total_passed++;
                std::cout << "\r [PASS] " << test.name << " (" << res.duration_ms << " ms, "
                          << res.assertions_passed << " checks)\n" << std::flush;
            } else {
                total_failed++;
                std::cout << "\r [FAIL] " << test.name << " (" << res.duration_ms << " ms)\n" << std::flush;
                for (const auto& msg : res.failure_messages) {
                    std::cout << "        -> " << msg << "\n" << std::flush;
                }
            }
        }

        std::cout << "\n========================================================\n";
        std::cout << " TEST SUMMARY: " << total_passed << " Passed, " << total_failed << " Failed\n";
        std::cout << " TOTAL TIME:   " << total_duration << " ms\n";
        std::cout << "========================================================\n";

        return (total_failed == 0) ? 0 : 1;
    }
};

inline void expect_true_impl(bool cond, const char* expr, const char* file, int line, TestResult& res) {
    if (cond) {
        res.assertions_passed++;
    } else {
        res.passed = false;
        res.assertions_failed++;
        res.failure_messages.push_back(std::string("EXPECT_TRUE failed: (") + expr + ") at " + file + ":" + std::to_string(line));
    }
}

inline void expect_near_impl(double val1, double val2, double tol, const char* expr1, const char* expr2, const char* file, int line, TestResult& res) {
    double diff = std::abs(val1 - val2);
    if (diff <= tol) {
        res.assertions_passed++;
    } else {
        res.passed = false;
        res.assertions_failed++;
        res.failure_messages.push_back(std::string("EXPECT_NEAR failed: |") + expr1 + " - " + expr2 + "| = |" +
                                       std::to_string(val1) + " - " + std::to_string(val2) + "| = " +
                                       std::to_string(diff) + " > " + std::to_string(tol) + " at " + file + ":" + std::to_string(line));
    }
}

} // namespace sovereign::test

#define TEST_CASE(name) \
    void name(::sovereign::test::TestResult& _test_res); \
    static struct Register_##name { \
        Register_##name() { \
            ::sovereign::test::TestRegistry::instance().register_test(#name, name); \
        } \
    } global_register_##name; \
    void name(::sovereign::test::TestResult& _test_res)

#define EXPECT_TRUE(cond) ::sovereign::test::expect_true_impl((cond), #cond, __FILE__, __LINE__, _test_res)
#define EXPECT_FALSE(cond) ::sovereign::test::expect_true_impl(!(cond), "!(" #cond ")", __FILE__, __LINE__, _test_res)
#define EXPECT_EQ(a, b) ::sovereign::test::expect_true_impl((a) == (b), #a " == " #b, __FILE__, __LINE__, _test_res)
#define EXPECT_NEAR(a, b, tol) ::sovereign::test::expect_near_impl((a), (b), (tol), #a, #b, __FILE__, __LINE__, _test_res)
#define EXPECT_STATUS_OK(status) ::sovereign::test::expect_true_impl(::sovereign::is_ok(status), "is_ok(" #status ")", __FILE__, __LINE__, _test_res)

#endif // SOVEREIGN_TESTS_TEST_FRAMEWORK_HPP

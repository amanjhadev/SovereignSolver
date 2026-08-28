#include "test_framework.hpp"
#include "sovereign/core/logger.hpp"

int main() {
    sovereign::Logger::instance().set_level(sovereign::LogLevel::Off);
    int ret = sovereign::test::TestRegistry::instance().run_all();
    std::cout << std::flush;
    return ret;
}

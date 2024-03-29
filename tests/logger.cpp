#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "logger.hpp"

using namespace std;
namespace fs = std::filesystem;

TEST_CASE("logger") {
    SECTION("no log file") {
        logger.info("");
        logger.warn("");
        CHECK_THROWS_WITH(logger.error(""), "");
        CHECK_THROWS_WITH(logger.error("", ENOENT),
                          ": No such file or directory");
    }

    SECTION("normal log file") {
        logger.enable_file_logging("normal.log");
        logger.info("");
        logger.warn("");
        CHECK_THROWS_WITH(logger.error(""), "");
        CHECK_THROWS_WITH(logger.error("", 0), ": Success");
        logger.disable_file_logging();
        REQUIRE(fs::remove("normal.log"));
    }
}

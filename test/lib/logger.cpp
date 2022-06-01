#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/fs.hpp"
#include "lib/logger.hpp"

TEST_CASE("logger") {
    SECTION("no log file") {
        Logger::info("");
        Logger::warn("");
        CHECK_THROWS_WITH(Logger::error(""), "");
        CHECK_THROWS_WITH(Logger::error("", ENOENT),
                          ": No such file or directory");
    }

    SECTION("normal log file") {
        Logger::enable_file_logging("normal.log");
        Logger::info("");
        Logger::warn("");
        CHECK_THROWS_WITH(Logger::error(""), "");
        CHECK_THROWS_WITH(Logger::error("", 0), ": Success");
        Logger::disable_file_logging();
        REQUIRE_NOTHROW(fs::remove("normal.log"));
    }
}

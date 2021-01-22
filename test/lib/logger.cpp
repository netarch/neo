#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <catch2/catch.hpp>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

TEST_CASE("logger")
{
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

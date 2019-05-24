#include <catch2/catch.hpp>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

TEST_CASE("logger")
{
    Logger& logger = Logger::get_instance();

    SECTION("no log file") {
        logger.info("");
        logger.warn("");
        CHECK_THROWS_WITH(logger.err(""), "");
        CHECK_THROWS_WITH(logger.err("", ENOENT),
                          ": No such file or directory");
    }

    SECTION("non-existent log file") {
        logger.set_file("non/existent/log/file");
        CHECK_THROWS_WITH(logger.info(""),
                          "Failed to open log file: non/existent/log/file");
    }

    SECTION("normal log file") {
        logger.set_file("normal.log");
        logger.info("");
        logger.warn("");
        CHECK_THROWS_WITH(logger.err(""), "");
        CHECK_THROWS_WITH(logger.err("", 0), ": Success");
        logger.set_file(std::string());
        REQUIRE_NOTHROW(fs::remove("normal.log"));
    }
}

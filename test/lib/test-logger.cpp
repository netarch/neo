#include <catch.hpp>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

TEST_CASE("logger")
{
    Logger& logger = Logger::get();

    SECTION("verbosity") {
        int outfd, nullfd;
        REQUIRE((nullfd = open("/dev/null", O_WRONLY)) > 2);
        REQUIRE((outfd = dup(1)) > nullfd);
        REQUIRE(dup2(nullfd, 1) == 1);
        REQUIRE(close(nullfd) == 0);

        logger.set_verbose(true);
        logger.out("");
        logger.info("");
        logger.warn("");
        logger.set_verbose(false);

        REQUIRE(dup2(outfd, 1) == 1);
        REQUIRE(close(outfd) == 0);
    }

    SECTION("no log file") {
        logger.info("");
        logger.warn("");
        CHECK_THROWS_WITH(logger.err(""), "");
        CHECK_THROWS_WITH(logger.err("", ENOENT),
                          ": No such file or directory");
    }

    SECTION("non-existent log file") {
        CHECK_THROWS_WITH(logger.set_file("non/existent/log/file"),
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

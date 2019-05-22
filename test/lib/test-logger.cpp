#include <catch2/catch.hpp>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

TEST_CASE("logger")
{
    Logger& logger = Logger::get_instance();
    logger.set_verbose(true);

    SECTION("standard output") {
        logger.out("Hello, world!");
    }

    SECTION("no log file") {
        logger.out("no log file");
        logger.info("no log file");
        logger.warn("no log file");
        try {
            logger.err("no log file");
        } catch (std::exception& e) {
            logger.out(e.what());
        }
        try {
            logger.err("no log file", ENOENT);
        } catch (std::exception& e) {
            logger.out(e.what());
        }
    }

    SECTION("non-existent log file") {
        logger.set_file("non/existent/log/file");
        try {
            logger.info("log message");
        } catch (std::exception& e) {
            logger.out(e.what());
        }
    }

    SECTION("normal log file") {
        logger.set_file("normal.log");
        logger.out("normal log file");
        logger.info("normal log file");
        logger.warn("normal log file");
        try {
            logger.err("normal log file");
        } catch (std::exception& e) {
            logger.out(e.what());
        }
        try {
            logger.err("normal log file", 0);
        } catch (std::exception& e) {
            logger.out(e.what());
        }
        fs::remove("normal.log");
    }
}

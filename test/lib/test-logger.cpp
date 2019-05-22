#include <catch2/catch.hpp>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

TEST_CASE("logger")
{
    Logger& logger = Logger::get_instance();

    SECTION("no log file") {
        logger.info("");
        logger.warn("");
        try {
            logger.err("");
        } catch (std::exception& e) {}
        try {
            logger.err("", ENOENT);
        } catch (std::exception& e) {}
    }

    SECTION("non-existent log file") {
        logger.set_file("non/existent/log/file");
        try {
            logger.info("");
        } catch (std::exception& e) {}
    }

    SECTION("normal log file") {
        logger.set_file("normal.log");
        logger.info("");
        logger.warn("");
        try {
            logger.err("");
        } catch (std::exception& e) {}
        try {
            logger.err("", 0);
        } catch (std::exception& e) {}
        fs::remove("normal.log");
    }
}

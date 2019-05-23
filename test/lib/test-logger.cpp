#include <catch2/catch.hpp>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

TEST_CASE("logger")
{
    Logger& logger = Logger::get_instance();

    SECTION("no log file") {
        int err = 0;
        logger.info("");
        logger.warn("");
        try {
            logger.err("");
        } catch (std::exception& e) {
            err++;
        }
        CHECK(err == 1);
        try {
            logger.err("", ENOENT);
        } catch (std::exception& e) {
            err++;
        }
        CHECK(err == 2);
    }

    SECTION("non-existent log file") {
        int err = 0;
        logger.set_file("non/existent/log/file");
        try {
            logger.info("");
        } catch (std::exception& e) {
            err++;
        }
        CHECK(err == 1);
    }

    SECTION("normal log file") {
        int err = 0;
        logger.set_file("normal.log");
        logger.info("");
        logger.warn("");
        try {
            logger.err("");
        } catch (std::exception& e) {
            err++;
        }
        CHECK(err == 1);
        try {
            logger.err("", 0);
        } catch (std::exception& e) {
            err++;
        }
        CHECK(err == 2);
        logger.set_file(std::string());
        fs::remove("normal.log");
    }
}

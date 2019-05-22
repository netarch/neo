#include <catch2/catch.hpp>
#include <climits>
#include <unistd.h>

#include "lib/fs.hpp"
#include "lib/logger.hpp"

TEST_CASE("fs")
{
    SECTION("mkdir") {
        fs::mkdir("new normal dir");
        fs::remove("new normal dir");
        try {
            fs::mkdir("a/somewhat/abnormal/deep/dir");
        } catch (std::exception& e) {}
    }

    SECTION("exists") {
        CHECK(fs::exists("test-fs.o"));
        CHECK_FALSE(fs::exists("non/existent/file"));
    }

    SECTION("remove") {
        fs::mkdir("hey");
        fs::remove("hey");
        try {
            fs::remove("a/somewhat/abnormal/deep/dir");
        } catch (std::exception& e) {}
    }

    SECTION("realpath") {
        char cwd[PATH_MAX];
        if (getcwd(cwd, PATH_MAX) == NULL) {
            Logger::get_instance().err("getcwd() failed", errno);
        }
        CHECK(fs::realpath("test-fs.o") == std::string(cwd) + "/test-fs.o");
        try {
            fs::realpath("non/existent/file");
        } catch (std::exception& e) {}
    }

    SECTION("append") {
        CHECK(std::string("a/b/c/d") == fs::append("a/b", "c/d"));
        CHECK(std::string("a/b/c/d") == fs::append("", "a/b/c/d"));
        CHECK(std::string("a/b/c/d") == fs::append("a/b/c/d", ""));
        CHECK(std::string("a/b/c/d") == fs::append("a/b/", "c/d"));
        CHECK(std::string("a/b/c/d") == fs::append("a/b", "/c/d"));
    }
}

#include <catch2/catch.hpp>
#include <climits>
#include <unistd.h>

#include "lib/fs.hpp"
#include "lib/logger.hpp"

TEST_CASE("fs")
{
    SECTION("mkdir") {
        REQUIRE_NOTHROW(fs::mkdir("new normal dir"));
        REQUIRE_NOTHROW(fs::remove("new normal dir"));
        CHECK_THROWS_WITH(fs::mkdir("a/b/c/deep/dir"),
                          "a/b/c/deep/dir: No such file or directory");
    }

    SECTION("exists") {
        CHECK(fs::exists("test-fs.o"));
        CHECK_FALSE(fs::exists("non/existent/file"));
        CHECK_THROWS_WITH(fs::exists("/proc/1/fd/0"),
                          "/proc/1/fd/0: Permission denied");
    }

    SECTION("remove") {
        REQUIRE_NOTHROW(fs::mkdir("hey"));
        REQUIRE_NOTHROW(fs::remove("hey"));
        CHECK_THROWS_WITH(fs::remove("a/b/c/deep/dir"),
                          "Failed to remove a/b/c/deep/dir: "
                          "No such file or directory");
        CHECK_THROWS_WITH(fs::remove("/proc/1/fd"),
                          "/proc/1/fd: Operation not permitted");
        CHECK_THROWS_WITH(fs::remove("/proc/1/stat"),
                          "/proc/1/stat: Operation not permitted");
    }

    SECTION("realpath") {
        char cwd[PATH_MAX];
        REQUIRE(getcwd(cwd, PATH_MAX) != NULL);
        CHECK(fs::realpath("test-fs.o") == std::string(cwd) + "/test-fs.o");
        CHECK_THROWS_WITH(fs::realpath("non/existent/file"),
                          "non/existent/file: No such file or directory");
    }

    SECTION("append") {
        CHECK("a/b/c/d" == fs::append("a/b", "c/d"));
        CHECK("a/b/c/d" == fs::append("", "a/b/c/d"));
        CHECK("a/b/c/d" == fs::append("a/b/c/d", ""));
        CHECK("a/b/c/d" == fs::append("a/b/", "c/d"));
        CHECK("a/b/c/d" == fs::append("a/b", "/c/d"));
    }
}

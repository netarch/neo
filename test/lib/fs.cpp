#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <climits>
#include <unistd.h>

#include "lib/fs.hpp"

TEST_CASE("fs") {
    SECTION("mkdir") {
        REQUIRE_NOTHROW(fs::mkdir("new normal dir"));
        REQUIRE_NOTHROW(fs::remove("new normal dir"));
        CHECK_THROWS_WITH(fs::mkdir("a/b/c/deep/dir"),
                          "a/b/c/deep/dir: No such file or directory");
    }

    SECTION("exists") {
        CHECK(fs::exists("lib/fs.cpp"));
        CHECK_FALSE(fs::exists("non/existent/file"));
        CHECK_THROWS_WITH(fs::exists("/proc/1/fd/0"),
                          "/proc/1/fd/0: Permission denied");
    }

    SECTION("remove") {
        REQUIRE_NOTHROW(fs::mkdir("hey"));
        REQUIRE_NOTHROW(fs::remove("hey"));
        CHECK_THROWS_WITH(fs::remove("a/b/c/deep/dir"),
                          "a/b/c/deep/dir: No such file or directory");
        CHECK_THROWS(fs::remove("/proc/1/fd"));
        CHECK_THROWS(fs::remove("/proc/1/stat"));
    }

    SECTION("realpath") {
        char cwd[PATH_MAX];
        REQUIRE(getcwd(cwd, PATH_MAX) != NULL);
        CHECK(fs::realpath("lib/fs.cpp") == std::string(cwd) + "/lib/fs.cpp");
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

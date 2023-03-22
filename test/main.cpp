#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <catch2/catch_session.hpp>

#include "logger.hpp"

using namespace std;
namespace fs = boost::filesystem;

string test_data_dir;

int main(int argc, char **argv) {
    Catch::Session session;

    // Build a new parser on top of Catch2's
    auto cli = session.cli() |
               Catch::Clara::Opt(test_data_dir,
                                 "test_data_dir")["-t"]["--test-data-dir"](
                   "Path to the test data files");

    // Now pass the new composite back to Catch2 so it uses that
    session.cli(cli);

    // Let Catch2 (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        return returnCode;
    }

    if (test_data_dir.empty()) {
        cerr << "Error: please specify --test-data-dir <path>" << endl;
        return 1;
    }

    if (!fs::is_directory(test_data_dir)) {
        cerr << test_data_dir << " is not a directory" << endl;
        return 1;
    }

    // Remove trailing slash
    if (test_data_dir.back() == '/') {
        test_data_dir.pop_back();
    }

    test_data_dir = fs::canonical(test_data_dir).string();
    logger.enable_console_logging();
    return session.run();
}

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include "plankton.hpp"

using namespace std;
namespace fs = std::filesystem;
namespace po = boost::program_options;

int main(int argc, char **argv) {
    po::options_description desc("Neo options");
    desc.add_options()("help,h", "Show help message");
    desc.add_options()("all,a", "Verify all ECs after violation");
    desc.add_options()("force,f", "Remove output dir if exists");
    desc.add_options()(
        "parallel-invs,p",
        "Allow verifying invariants in parallel (default: disabled). This is "
        "mostly used for narrow invariants who only have one EC to check.");
    desc.add_options()("jobs,j", po::value<size_t>()->default_value(1),
                       "Max number of parallel tasks [default: 1]");
    desc.add_options()("emulations,e", po::value<size_t>()->default_value(0),
                       "Max number of emulations");
    desc.add_options()("drop,d", po::value<string>()->default_value("timeout"),
                       "Drop detection method: ['timeout', 'dropmon', 'ebpf']");
    desc.add_options()("input,i", po::value<string>()->default_value(""),
                       "Input configuration file");
    desc.add_options()("output,o", po::value<string>()->default_value(""),
                       "Output directory");
    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    if (vm.count("help")) {
        cout << desc << endl;
        return 0;
    }

    bool all_ecs = vm.count("all");
    bool rm_out_dir = vm.count("force");
    bool parallel_invs = vm.count("parallel-invs");
    size_t max_jobs = vm.at("jobs").as<size_t>();
    size_t max_emu = vm.at("emulations").as<size_t>();
    string drop = vm.at("drop").as<string>();
    string input_file = vm.at("input").as<string>();
    string output_dir = vm.at("output").as<string>();

    if (max_jobs < 1) {
        cerr << "Invalid number of parallel tasks" << endl;
        return 1;
    }

    if (drop != "timeout" && drop != "dropmon" && drop != "ebpf") {
        cerr << "Invalid drop detection method" << endl;
        return 1;
    }

    if (input_file.empty() || !fs::exists(input_file)) {
        cerr << "Missing input file " << input_file << endl;
        return 1;
    }

    if (output_dir.empty()) {
        cerr << "Missing output directory" << endl;
        return 1;
    }

    if (rm_out_dir && fs::exists(output_dir)) {
        fs::remove_all(output_dir);
    }

    Plankton &plankton = Plankton::get();
    plankton.init(all_ecs, parallel_invs, max_jobs, max_emu, drop, input_file,
                  output_dir);
    return plankton.run();
}

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>

#include "plankton.hpp"

static void usage(const std::string &progname) {
    std::cout
        << "Usage: " + progname +
               " [OPTIONS] -i <file> -o <dir>\n"
               "Options:\n"
               "    -h, --help           print this help message\n"
               "    -a, --all            verify all ECs after violation\n"
               "    -f, --force          remove pre-existent output directory\n"
               "    -d, --dropmon        use drop monitor instead of timer\n"
               "    -j, --jobs <N>       number of parallel tasks\n"
               "    -e, --emulations <N> maximum number of emulation "
               "instances\n"
               "    -i, --input <file>   network configuration file\n"
               "    -o, --output <dir>   output directory\n";
}

static void parse_args(int argc,
                       char **argv,
                       bool &all_ECs,
                       bool &rm_out_dir,
                       bool &dropmon,
                       size_t &max_jobs,
                       int &emulations,
                       std::string &input_file,
                       std::string &output_dir) {
    int opt;
    const char *optstring = "hafdj:e:i:o:";

    const struct option longopts[] = {
        {      "help",       no_argument, 0, 'h'},
        {       "all",       no_argument, 0, 'a'},
        {     "force",       no_argument, 0, 'f'},
        {   "dropmon",       no_argument, 0, 'd'},
        {      "jobs", required_argument, 0, 'j'},
        {"emulations", required_argument, 0, 'e'},
        {     "input", required_argument, 0, 'i'},
        {    "output", required_argument, 0, 'o'},
        {           0,                 0, 0,   0}
    };

    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            exit(0);
        case 'a':
            all_ECs = true;
            break;
        case 'f':
            rm_out_dir = true;
            break;
        case 'd':
            dropmon = true;
            break;
        case 'j':
            max_jobs = atoi(optarg);
            break;
        case 'e':
            emulations = atoi(optarg);
            break;
        case 'i':
            input_file = optarg;
            break;
        case 'o':
            output_dir = optarg;
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if (max_jobs < 1) {
        std::cerr << "Error: invalid number of parallel tasks" << std::endl;
        exit(1);
    }
    if (input_file.empty()) {
        std::cerr << "Error: missing input file" << std::endl;
        exit(1);
    }
    if (output_dir.empty()) {
        std::cerr << "Error: missing output directory" << std::endl;
        exit(1);
    }
}

int main(int argc, char **argv) {
    bool all_ECs = false, rm_out_dir = false, dropmon = false;
    size_t max_jobs = 1;
    int emulations = -1;
    std::string input_file, output_dir;
    parse_args(argc, argv, all_ECs, rm_out_dir, dropmon, max_jobs, emulations,
               input_file, output_dir);

    Plankton &plankton = Plankton::get();
    plankton.init(all_ECs, rm_out_dir, dropmon, max_jobs, emulations,
                  input_file, output_dir);
    return plankton.run();
}

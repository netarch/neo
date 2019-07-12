#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <string>

#include "plankton.hpp"


static void usage(const std::string& progname)
{
    std::cout <<
              "Usage: " + progname + " [OPTIONS] -i <file> -o <dir>\n"
              "Options:\n"
              "    -h, --help             print this help message\n"
              "    -v, --verbose          print more debugging information\n"
              "    -f, --force            remove the output directory\n"
              "    -j, --jobs <nprocs>    number of parallel tasks\n"
              "\n"
              "    -i, --input <file>     network configuration file\n"
              "    -o, --output <dir>     output directory\n";
}

static void parse_args(int argc, char **argv, bool& verbose, bool& rm_out_dir,
                       size_t& max_jobs, std::string& input_file,
                       std::string& output_dir)
{
    int opt;
    const char *optstring = "hvfj:i:o:";

    const struct option longopts[] = {
        {"help",       no_argument,       0, 'h'},
        {"verbose",    no_argument,       0, 'v'},
        {"force",      no_argument,       0, 'f'},
        {"jobs",       required_argument, 0, 'j'},
        {"input",      required_argument, 0, 'i'},
        {"output",     required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                exit(0);
            case 'v':
                verbose = true;
                break;
            case 'f':
                rm_out_dir = true;
                break;
            case 'j':
                if ((max_jobs = atoi(optarg)) < 1) {
                    max_jobs = 1;
                }
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

    if (input_file.empty()) {
        std::cerr << "Error: missing input file" << std::endl
                  << "Try '" << argv[0] << " --help' for more information"
                  << std::endl;
        exit(1);
    }
    if (output_dir.empty()) {
        std::cerr << "Error: missing output directory" << std::endl
                  << "Try '" << argv[0] << " --help' for more information"
                  << std::endl;
        exit(1);
    }
}

int main(int argc, char **argv)
{
    bool verbose = false, rm_out_dir = false;
    size_t max_jobs = 1;
    std::string input_file, output_dir;
    parse_args(argc, argv, verbose, rm_out_dir, max_jobs, input_file,
               output_dir);

    Plankton& plankton = Plankton::get_instance();
    plankton.init(verbose, rm_out_dir, max_jobs, input_file, output_dir);
    plankton.run();

    return 0;
}

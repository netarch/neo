#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <string>

#include "plankton.hpp"


static void usage(const std::string& progname)
{
    std::cout <<
              "Usage: " + progname + " [-h] [-v] [-j <nprocs>] -i <file> -o <dir>\n"
              "    -h, --help             print this help message\n"
              "    -v, --verbose          print more debugging information\n"
              "    -j, --jobs <nprocs>    number of parallel tasks\n"
              "    -i, --input <file>     network configuration file\n"
              "    -o, --output <dir>     output directory\n";
}

static void parse_args(int argc, char **argv, bool& verbose, int& max_jobs,
                       std::string& input_file, std::string& output_dir)
{
    int opt;
    const char *optstring = "hvj:i:o:";

    const struct option longopts[] = {
        {"help",       no_argument,       0, 'h'},
        {"verbose",    no_argument,       0, 'v'},
        {"jobs",       required_argument, 0, 'j'},
        {"input",      required_argument, 0, 'i'},
        {"output",     required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                verbose = true;
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
                exit(EXIT_FAILURE);
        }
    }

    if (input_file.empty()) {
        std::cerr << "Error: missing input file" << std::endl
                  << "Try '" << argv[0] << " --help' for more information"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    if (output_dir.empty()) {
        std::cerr << "Error: missing output directory" << std::endl
                  << "Try '" << argv[0] << " --help' for more information"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    bool verbose = false;
    int max_jobs = 1;
    std::string input_file, output_dir;
    parse_args(argc, argv, verbose, max_jobs, input_file, output_dir);

    Plankton plankton(verbose, max_jobs, input_file, output_dir);
    plankton.run();

    return EXIT_SUCCESS;
}

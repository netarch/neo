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
              "    -a, --all              verify all ECs after violation\n"
              "    -f, --force            remove the output directory\n"
              "    -j, --jobs <nprocs>    number of parallel tasks\n"
              "    -v, --verbose          print more debugging information\n"
              "\n"
              "    -i, --input <file>     network configuration file\n"
              "    -o, --output <dir>     output directory\n";
}

static void parse_args(int argc, char **argv, bool& all_ECs, bool& rm_out_dir,
                       size_t& max_jobs, bool& verbose, std::string& input_file,
                       std::string& output_dir)
{
    int opt;
    const char *optstring = "hafj:vi:o:";

    const struct option longopts[] = {
        {"help",    no_argument,       0, 'h'},
        {"all",     no_argument,       0, 'a'},
        {"force",   no_argument,       0, 'f'},
        {"jobs",    required_argument, 0, 'j'},
        {"verbose", no_argument,       0, 'v'},
        {"input",   required_argument, 0, 'i'},
        {"output",  required_argument, 0, 'o'},
        {0, 0, 0, 0}
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
            case 'j':
                if ((max_jobs = atoi(optarg)) < 1) {
                    max_jobs = 1;
                }
                break;
            case 'v':
                verbose = true;
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
    bool all_ECs = false, rm_out_dir = false, verbose = false;
    size_t max_jobs = 1;
    std::string input_file, output_dir;
    parse_args(argc, argv, all_ECs, rm_out_dir, max_jobs, verbose, input_file,
               output_dir);

    Plankton& plankton = Plankton::get_instance();
    plankton.init(all_ECs, rm_out_dir, max_jobs, verbose, input_file,
                  output_dir);
    return plankton.run();
}

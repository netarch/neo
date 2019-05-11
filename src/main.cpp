#include <getopt.h>
#include <cstdlib>
#include <string>

#include "lib/logger.hpp"


static void usage(const std::string& progname)
{
    Logger& logger = Logger::get_instance();
    logger.out(
        "Usage: " + progname + " [-h] -i dir -o dir\n"
        "    -h, --help             print this help message\n"
        "    -i, --input dir        input directory\n"
        "    -o, --output dir       output directory\n");
}

static void parse_args(int argc, char **argv, std::string& input_dir,
                       std::string& output_dir)
{
    int opt;
    bool noinput = true, nooutput = true;
    const char *optstring = "hi:o:";

    const struct option longopts[] = {
        {"help",       no_argument,       0, 'h'},
        {"input",      required_argument, 0, 'i'},
        {"output",     required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'i':
                input_dir = optarg;
                noinput = false;
                break;
            case 'o':
                output_dir = optarg;
                nooutput = false;
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (noinput) {
        Logger& logger = Logger::get_instance();
        logger.err("missing input directory\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (nooutput) {
        Logger& logger = Logger::get_instance();
        logger.err("missing output directory\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    std::string input_dir, output_dir;

    parse_args(argc, argv, input_dir, output_dir);



    //static char param0[] = "neo";  // argv[0]
    //static char param1[] = "-m100000";
    //static char *spin_args[] = {param0, param1};

    //spin_main(sizeof(spin_args) / sizeof(char *), spin_args);

    return EXIT_SUCCESS;
}

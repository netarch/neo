#include "plankton.hpp"
#include "lib/fs.hpp"


Plankton::Plankton(int max_jobs, const std::string& input_dir,
                   const std::string& output_dir)
    : max_jobs(max_jobs), logger(Logger::get_instance())
{
    fs::mkdir(output_dir);
    in_dir = fs::realpath(input_dir);
    out_dir = fs::realpath(output_dir);
    logger.set_file(fs::append(out_dir, "verify.log"));
}

void Plankton::run()
{
    //static char param0[] = "neo";  // argv[0]
    //static char param1[] = "-m100000";
    //static char *spin_args[] = {param0, param1};
    //spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
}

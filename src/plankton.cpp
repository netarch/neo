#include "plankton.hpp"
#include "lib/fs.hpp"
#include "lib/cpptoml.hpp"


Plankton::Plankton(bool verbose, int max_jobs, const std::string& input_file,
                   const std::string& output_dir)
    : logger(Logger::get_instance()), max_jobs(max_jobs)
{
    fs::mkdir(output_dir);
    in_file = fs::realpath(input_file);
    out_dir = fs::realpath(output_dir);
    logger.set_file(fs::append(out_dir, "verify.log"));
    logger.set_verbose(verbose);
}

void Plankton::load_config()
{
    logger.info("Loading network configurations...");

    auto config = cpptoml::parse_file(in_file);
    auto nodes_config = config->get_table_array("nodes");
    auto links_config = config->get_table_array("links");
    auto policies_config = config->get_table_array("policies");

    topology.load_config(nodes_config, links_config);
    //policies.load_config(policies_config);
}

void Plankton::run()
{
    load_config();

    //static char param0[] = "neo";  // argv[0]
    //static char param1[] = "-m100000";
    //static char *spin_args[] = {param0, param1};
    //spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
}

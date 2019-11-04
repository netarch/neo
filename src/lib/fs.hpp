#pragma once

#include <string>
#include <cpptoml/cpptoml.hpp>

namespace fs
{

void chdir(const std::string&);
void mkdir(const std::string&);
bool exists(const std::string&);
void remove(const std::string&);
std::string realpath(const std::string&);
std::string append(const std::string&, const std::string&);
std::shared_ptr<cpptoml::table> get_toml_config(const std::string&);
std::string get_cwd(void);
bool copy(const std::string src_path, const std::string dest_path);
bool is_regular_file(const std::string file_path);
} // namespace fs

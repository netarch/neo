#ifndef FS_HPP
#define FS_HPP

#include <string>
#include <cpptoml/cpptoml.hpp>

namespace fs
{

void mkdir(const std::string&);
bool exists(const std::string&);
void remove(const std::string&);
std::string realpath(const std::string&);
std::string append(const std::string&, const std::string&);
std::shared_ptr<cpptoml::table> get_toml_config(const std::string&);

} // namespace fs

#endif

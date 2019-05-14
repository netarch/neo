#ifndef FS_HPP
#define FS_HPP

#include <string>

namespace fs
{

void mkdir(const std::string&);
bool exists(const std::string&);
void remove(const std::string&);
std::string realpath(const std::string&);
std::string append(const std::string&, const std::string&);

} // namespace fs

#endif

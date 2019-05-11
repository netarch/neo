#ifndef FS_HPP
#define FS_HPP

#include <string>

namespace fs
{

std::string realpath(const std::string&);
void mkdir(const std::string&);

} // namespace fs

#endif

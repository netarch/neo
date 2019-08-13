#pragma once

namespace hash
{

size_t hash(const void *data, size_t len);
size_t& hash_combine(size_t&, size_t);

} // namespace hash

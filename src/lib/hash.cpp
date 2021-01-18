#include "lib/hash.hpp"

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

namespace hash
{

size_t hash(const void *data, size_t len)
{
    return XXH3_64bits(data, len);
}

size_t& hash_combine(size_t& seed, size_t value)
{
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

} // namespace hash

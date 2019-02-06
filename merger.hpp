/*
 * Memory request coalescing logic
 * Author: Nilanjan Goswami
 * Date: 4th February, 2019
 */

#pragma once

#include "merger_template.hpp"
#include <list>
#include <random>
#include <iomanip>

typedef uint64_t Address;

/*
 * Returns: base0, base1 and cache_line_cross
 */
std::tuple<Address, Address, bool> CalcBaseAddr(Address& addr,
                                                uint32_t req_size,
                                                const uint32_t cache_line_size = kCacheLineSize);
Address InitAddr();
void MakeCacheReq(Address creq, uint32_t merge_count);

class Merger : public MergerTemplate<Address, InitAddr, CalcBaseAddr, MakeCacheReq> {
private:
    uint32_t               seed_;
    uint32_t               test_size_;
    uint32_t               cache_line_size_;

    void SelfTest();

public:
    Merger(uint32_t cache_line_size, uint32_t seed, uint32_t test_size = 8096) :
      MergerTemplate<Address, InitAddr, CalcBaseAddr, MakeCacheReq> (cache_line_size),
      seed_(seed),
      test_size_(test_size),
      cache_line_size_(cache_line_size)
    {
        SelfTest();
    }
};

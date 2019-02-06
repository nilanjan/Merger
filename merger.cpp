#include "merger.hpp"

/*
 * Returns: base0, base1 and cache_line_cross
 */
std::tuple<Address, Address, bool> CalcBaseAddr(Address& addr,
                                                uint32_t req_size,
                                                const uint32_t cache_line_size) {
   Address base0 = addr - (addr % cache_line_size);
   bool b_cline_cross = (addr + req_size) <= (base0 + cache_line_size) ? false : true;
   Address base1 = b_cline_cross ? base0 + cache_line_size : 0xdeadbeefdeadbeef;

   return std::make_tuple(base0, base1, b_cline_cross);
}

Address InitAddr() {
    return 0xdeadbeefdeadbeef;
}

void MakeCacheReq(Address creq, uint32_t merge_count) {
    std::cout<<"Req2Cache <> Merge Base: "<<std::hex<<creq<<" ";
    std::cout<<"MergeCount: "<<std::dec<<merge_count<<std::endl;
}

void Merger::SelfTest()
{
    std::list<Address> list_of_addr;

    srand(seed_);

    for (uint32_t idx=0; idx<test_size_; ++idx) {
        Address addr = static_cast<Address>(0x8000000000 + static_cast<Address>(rand())%test_size_);
        uint32_t req_size = static_cast<uint32_t>(rand())%cache_line_size_;

        MemAddress<Address, CalcBaseAddr> mem_addr(addr, req_size, cache_line_size_);

        Merge(mem_addr);
    }

    std::tuple<Address, bool, uint32_t, uint32_t, uint32_t> stat = GetCurrState();
    uint32_t merged_addr_count = std::get<4>(stat);
    float_t percent_merged = static_cast<float>((static_cast<float>(merged_addr_count)/test_size_) * 100);

    std::cout<<"Unmerged Addr Count: "<<test_size_<<
               " Merged Addr Count: "<<merged_addr_count<<
               " %Merged: "<<std::fixed << std::setprecision(2) <<percent_merged<<std::endl;
}

/*
 * Cache line request coalescing logic
 * Only models the request path;
 * can easily be augmented to include
 * the response path.
 * Author: Nilanjan Goswami
 * Date: 4th February, 2019
 */

#pragma once

#include <iostream>
#include <cassert>
#include <tuple>

const uint32_t kCacheLineSize = 64; // Override this in the instantiation

template <typename AddressType,
          std::tuple<AddressType, AddressType, bool> (*CalcBaseAddr) (AddressType& addr,
                                                                      uint32_t req_size,
                                                                      const uint32_t cache_line_size)>
class MemAddress {
private:
    AddressType               base0_;
    AddressType               base1_;
    bool                      b_cline_cross_;
    uint32_t                  req_size_;

public:
    /* Cache requests cannot be bigger than the cache line size.
     * Note that, there is no support for the byte mask sent with
     * the cache line request to the cache.
     */
    MemAddress(AddressType addr, uint32_t req_size, uint32_t cache_line_size = kCacheLineSize) :
        req_size_(req_size)
    {
        assert(req_size <= cache_line_size);
        std::tuple<AddressType, AddressType, bool> ret = CalcBaseAddr(addr, req_size, cache_line_size);
        base0_ = std::get<0>(ret);
        base1_ = std::get<1>(ret);
        b_cline_cross_ = std::get<2>(ret);
    }

    AddressType GetBase0() { return base0_; }
    AddressType GetBase1() { return base1_; }
    bool        IsClineCross() { return b_cline_cross_; }
    uint32_t    GetReqSize() { return req_size_; }
};

template <typename AddressType,
          AddressType (*InitAddr) (),
          std::tuple<AddressType, AddressType, bool> (*CalcBaseAddr) (AddressType& addr,
                                                                      uint32_t req_size,
                                                                      const uint32_t cache_line_size),
          void (*MakeCacheReq)(AddressType creq, uint32_t merge_count)>
class MergerTemplate {
private:
    bool                         b_first_addr_;
    AddressType                  curr_merge_base_;
    uint32_t                     cache_line_size_;

    /* Merge count of 0 means only one request in the
     * cache line; no merging
     */
    uint32_t                     curr_merge_count_;   // Merge count for the ongoing merge (biased by 1)
    uint32_t                     total_merge_count_;  // Merge count for all the merges till last Reset() (biased by 1)
    uint32_t                     merged_req_count_;

    uint32_t                     last_creq_merge_count_;
    AddressType                  last_creq_base_;

    void StartNewMerge(AddressType addr, bool b_incr_total_merge_count=true) {
        last_creq_base_ = curr_merge_base_;
        last_creq_merge_count_ = curr_merge_count_;

        curr_merge_base_ = addr;
        curr_merge_count_ = 0;
        if(b_incr_total_merge_count)
            ++total_merge_count_;

        ++merged_req_count_;
    }

public:
    MergerTemplate(uint32_t cache_line_size) :
        b_first_addr_(true),
        curr_merge_base_(InitAddr()),
        cache_line_size_(cache_line_size),
        curr_merge_count_(0),
        total_merge_count_(0),
        merged_req_count_(0),
        last_creq_merge_count_(0),
        last_creq_base_(InitAddr())
    {}

    void Reset() {
        b_first_addr_ = true;
        curr_merge_base_ = InitAddr();
        curr_merge_count_ = 0;
        total_merge_count_ = 0;
        last_creq_merge_count_ = 0;
        last_creq_base_ = InitAddr();
    }

    std::tuple<AddressType, bool, uint32_t, uint32_t, uint32_t> GetCurrState() {
        return std::make_tuple(curr_merge_base_, b_first_addr_, curr_merge_count_, total_merge_count_, merged_req_count_);
    }

    std::tuple<AddressType, uint32_t> GetLastCreqInfo() {
        return std::make_tuple(last_creq_base_, last_creq_merge_count_);
    }

    void Merge(MemAddress<AddressType, CalcBaseAddr> addr) {
        AddressType base0 = addr.GetBase0();
        AddressType base1 = addr.GetBase1();
        bool        b_cline_cross = addr.IsClineCross();

        if(b_first_addr_) {
            /* Ensure the merge base is at InitAddr() */
            assert(curr_merge_base_ == InitAddr());

            b_first_addr_ = false;
            if(b_cline_cross) {
               ++merged_req_count_;
               curr_merge_base_ = base0;
               MakeCacheReq(curr_merge_base_, curr_merge_count_);
               ++merged_req_count_;
               curr_merge_base_ = base1;
            } else {
               ++merged_req_count_;
               curr_merge_base_ = base0;
            }

            ++curr_merge_count_;
            ++total_merge_count_;
        } else {
            if(b_cline_cross) {
                if(curr_merge_base_ == base0) {
                    // Part 1 of the multi cline cache access
                    ++curr_merge_count_;
                    ++total_merge_count_; // only counted for the part 1
                    MakeCacheReq(curr_merge_base_, curr_merge_count_);

                    // Part 2 of the multi cline cache access
                    // total_merge_count increment is not
                    // considered for Part 2
                    StartNewMerge(base1, false);
                } else {
                    // No merging with the onging merge base
                    // Make a memory request
                    MakeCacheReq(curr_merge_base_, curr_merge_count_);

                    // Part 1 of the multi cline cache access
                    StartNewMerge(base0);
                    MakeCacheReq(curr_merge_base_, curr_merge_count_);

                    // Part 2 of the multi cline cache access
                    // total_merge_count increment is not
                    // considered for Part 2
                    StartNewMerge(base1, false);
                }
            } else {
                if(curr_merge_base_ == base0) {
                    // Request got merged with the exisitng
                    // ongoing merge
                    ++curr_merge_count_;
                    ++total_merge_count_;
                } else {
                    // No merging with the onging merge base
                    // Make a memory request
                    MakeCacheReq(curr_merge_base_, curr_merge_count_);

                    // Start a new merge
                    // total_merge_count is only considered for Part 1
                    StartNewMerge(base0);
                }
            }// if(b_cline_cross)
        } // if(b_first_addr_)

        // Make a memory request
        MakeCacheReq(curr_merge_base_, curr_merge_count_);

        return;
    }

};


#ifndef RANGE_CACHE_CPP_
#define RANGE_CACHE_CPP_

#include "range_cache.h"
#include <iostream>

using namespace std;

template<class ADDRESS, class FLAGS>
RangeCache<ADDRESS, FLAGS>::RangeCache(Oracle<ADDRESS, FLAGS> *wp_ref) {
   oracle_wp = wp_ref;
   kickout_dirty=0;
   kickout=0;
   complex_updates=0;
}

template<class ADDRESS, class FLAGS>
RangeCache<ADDRESS, FLAGS>::RangeCache() {
   kickout_dirty=0;
   kickout=0;
   complex_updates=0;
}

template<class ADDRESS, class FLAGS>
RangeCache<ADDRESS, FLAGS>::~RangeCache() {
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::watch_fault(ADDRESS start_addr, ADDRESS end_addr) {
   return general_fault(start_addr, end_addr);
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::read_fault(ADDRESS start_addr, ADDRESS end_addr) {
   return general_fault(start_addr, end_addr);
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::write_fault(ADDRESS start_addr, ADDRESS end_addr) {
   return general_fault(start_addr, end_addr);
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::add_watchpoint(ADDRESS start_addr, ADDRESS end_addr) {
   return wp_operation(start_addr, end_addr);
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::rm_watchpoint(ADDRESS start_addr, ADDRESS end_addr) {
   return wp_operation(start_addr, end_addr);
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::general_fault(ADDRESS start_addr, ADDRESS end_addr, bool dirty) {
   typename std::deque< watchpoint_t<ADDRESS, FLAGS> >::iterator rc_read_iter;
   watchpoint_t<ADDRESS, FLAGS> temp;
   int rc_miss = 0;
   bool searching = true;     // searching = true until all ranges are covered
   while (searching) {
      rc_read_iter = search_address(start_addr);
      if (rc_read_iter == rc_data.end()) {
         // if cache miss
         rc_miss++;
         // get new range from backing store
         rc_read_iter = oracle_wp->search_address(start_addr);
         temp = *rc_read_iter;
         if (dirty)
            temp.flags |= DIRTY;
         rc_data.push_back(*rc_read_iter);
         rc_read_iter = search_address(start_addr);
      }
      if (rc_read_iter->end_addr >= end_addr)
         searching = false;
      temp = *rc_read_iter;
      start_addr = temp.end_addr+1;
      if (dirty)
         temp.flags |= DIRTY;
      rc_data.erase(rc_read_iter);
      rc_data.push_back(temp);
   }
   while (cache_overflow())
      cache_kickout();
   return rc_miss;
}

template<class ADDRESS, class FLAGS>
int RangeCache<ADDRESS, FLAGS>::wp_operation(ADDRESS start_addr, ADDRESS end_addr) {
   bool complex_update = false;
   int rc_miss = 0;
   typename std::deque< watchpoint_t<ADDRESS, FLAGS> >::iterator rc_write_iter;
   // extend start_addr if necessary
   rc_write_iter = search_address(start_addr);
   if (rc_write_iter != rc_data.end()) {        // in case of split
      if (start_addr > rc_write_iter->start_addr)
         general_fault(rc_write_iter->start_addr, start_addr-1);
   }
   else 
      complex_update = true;
   rc_write_iter = oracle_wp->search_address(start_addr);
   if (rc_write_iter->start_addr < start_addr)  // in case of merge
      start_addr = rc_write_iter->start_addr;
   // check if complex_update
   rc_write_iter++;
   if (rc_write_iter->end_addr<end_addr)
      complex_update = true;
   // extend end_addr if necessary
   rc_write_iter = search_address(end_addr);
   if (rc_write_iter != rc_data.end()) {        // in case of split
      if (end_addr < rc_write_iter->end_addr)
         general_fault(end_addr+1, rc_write_iter->end_addr);
   }
   else
      complex_update = true;
   rc_write_iter = oracle_wp->search_address(end_addr);
   if (rc_write_iter->end_addr > end_addr)      // in case of merge
      end_addr = rc_write_iter->end_addr;
   // rm all entries within the new range
   rc_miss = general_fault(start_addr, end_addr);
   rm_range(start_addr, end_addr);
   // update these entries
   general_fault(start_addr, end_addr, true);
   if (complex_update)
      complex_updates++;
   return rc_miss;
}

template<class ADDRESS, class FLAGS>
bool RangeCache<ADDRESS, FLAGS>::cache_overflow() {
   if (rc_data.size() > CACHE_SIZE)
      return true;
   return false;
}

template<class ADDRESS, class FLAGS>
void RangeCache<ADDRESS, FLAGS>::cache_kickout() {
   kickout++;
   if (rc_data.front().flags & DIRTY)
      kickout_dirty++;
   rc_data.pop_front();
}

template<class ADDRESS, class FLAGS>
typename std::deque< watchpoint_t<ADDRESS, FLAGS> >::iterator 
RangeCache<ADDRESS, FLAGS>::search_address(ADDRESS target_addr) {
   typename std::deque< watchpoint_t<ADDRESS, FLAGS> >::iterator i;
   for (i=rc_data.begin();i!=rc_data.end();i++) {
      if (target_addr >= i->start_addr && target_addr <= i->end_addr)
         return i;
   }
   return rc_data.end();
}

template<class ADDRESS, class FLAGS>
void RangeCache<ADDRESS, FLAGS>::rm_range(ADDRESS start_addr, ADDRESS end_addr) {
   typename std::deque< watchpoint_t<ADDRESS, FLAGS> >::iterator rc_rm_iter;
   bool searching = true;  // searching = true until all ranges are removed
   while (searching) {
      rc_rm_iter = search_address(start_addr);
      if (rc_rm_iter != rc_data.end()) {
         if (rc_rm_iter->end_addr >= end_addr)
            searching = false;
         start_addr = rc_rm_iter->end_addr+1;
         rc_data.erase(rc_rm_iter);
      }
      else if (start_addr != end_addr)
         start_addr++;
      else
         searching = false;
   }
}

template<class ADDRESS, class FLAGS>
void  RangeCache<ADDRESS, FLAGS>::
get_stats(long long &kickout_out, long long &kickout_dirty_out, long long &complex_updates_out) {
   kickout_out = kickout;
   kickout_dirty_out = kickout_dirty;
   complex_updates_out = complex_updates;
}

#endif
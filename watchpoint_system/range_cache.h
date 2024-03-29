#ifndef RANGE_CACHE_H_
#define RANGE_CACHE_H_
#include <deque>
#include <algorithm>
#include <cassert>
#include <iostream>
#include "oracle_wp.h"
#include "wp_data_struct.h"
#include "off_cbm.h"
using namespace std;

#ifndef PAGE_OFFSET_LENGTH
#define  PAGE_OFFSET_LENGTH      12UL
#endif

#define     CACHE_SIZE     128
#define     OCBM_LEN       10

#define     DIRTY          16

template<class ADDRESS, class FLAGS>
class RangeCache {
public:
   RangeCache(Oracle<ADDRESS, FLAGS> *wp_ref, int thread_id, bool ocbm_in = false, bool offcbm_in = false);
   RangeCache(ostream &output_stream, Oracle<ADDRESS, FLAGS> *wp_ref, int tid, bool ocbm_in = false, bool offcbm_in = false);
   RangeCache();
   ~RangeCache();
   // for all ranges referred to, update lru if found in rc; load from backing store if not found.
   unsigned int   general_fault  (ADDRESS start_addr, ADDRESS end_addr);
   unsigned int   watch_fault (ADDRESS start_addr, ADDRESS end_addr);
   unsigned int   read_fault  (ADDRESS start_addr, ADDRESS end_addr);
   unsigned int   write_fault (ADDRESS start_addr, ADDRESS end_addr);
   // actually updating all the entries referred by this range
   unsigned int   wp_operation   (ADDRESS start_addr, ADDRESS end_addr, bool is_update);
   unsigned int   add_watchpoint (ADDRESS start_addr, ADDRESS end_addr, bool is_update);
   unsigned int   rm_watchpoint  (ADDRESS start_addr, ADDRESS end_addr, bool is_update);
   // print for debugging
   void watch_print     (ostream &output = cout);
   // only for range_cache with ocbm function
   void check_ocbm      (ADDRESS start_addr, ADDRESS end_addr);
   // statistics:
   long long                              offcbm_switch, 
                                          range_switch, 
                                          kickout_dirty, 
                                          kickout, 
                                          complex_updates, 
                                          wlb_miss, 
                                          wlb_miss_size;
private:
   bool ocbm, off_cbm;
   ostream &trace_output;
   int thread_id;
   // data structure used in range cache
   deque< watchpoint_t<ADDRESS, FLAGS> >  rc_data;       // cache data
   Oracle<ADDRESS, FLAGS>*                oracle_wp;     // storing a pointer to its corresponding oracle
   Offcbm<ADDRESS, FLAGS>*                offcbm_wp;     // storing a pointer to its corresponding off-chip bitmap
   // private functions used in range cache
   typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator
      search_address    (ADDRESS target_addr);           // search for target addr in rc, return rc_data.end() if miss
   unsigned int rm_range(ADDRESS start_addr, ADDRESS end_addr);  // remove all entries within the range
   void rm_for_offcbm_switch(ADDRESS start_addr, ADDRESS end_addr);  // count dirty ranges as dirty kickouts
   bool cache_overflow();
   void print_bitmap();
   void cache_kickout();   // kickout one lru entry at a time
   void print_trace(int command, int thread_id, unsigned int starter, unsigned int ender);
};

#include "range_cache.cpp"
#endif /*RANGE_CACHE_H_*/

#ifndef CACHE_ALGO_
#define CACHE_ALGO_

#include <deque>
#include "wp_data_struct.h"

#define     CACHE_SIZE     64

template<class ADDRESS>
class CacheAlgo {
public:
   CacheAlgo();
   ~CacheAlgo();

   int modify          (ADDRESS start_addr, ADDRESS end_addr, ADDRESS new_start, ADDRESS new_end, bool update = true);
   int insert          (ADDRESS start_addr, ADDRESS end_addr);
   int remove          (ADDRESS start_addr, ADDRESS end_addr);
   bool search         (ADDRESS start_addr, ADDRESS end_addr, bool update = true);
   void clear          ();
   
   Range<ADDRESS> kickout  ();
   
   void print              ();
   
//private:
   typename std::deque< Range<ADDRESS> >::iterator search_iter         (ADDRESS start_addr, ADDRESS end_addr, bool update = true);
   bool check_overlap                                                  (ADDRESS start_addr, ADDRESS end_addr);
   std::deque< Range<ADDRESS> > ranges;
};

#include "cache_algo.cpp"

#endif /*CACHE_ALGO_*/

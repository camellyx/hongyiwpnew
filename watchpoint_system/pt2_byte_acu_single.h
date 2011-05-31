/*
 * pt2_byte_acu_single.h
 *
 *  Created on: May 27, 2011
 *      Author: luoyixin
 */
#ifdef PT2_BYTE_ACU_SINGLE_H_
#define PT2_BYTE_ACU_SINGLE_H_

#include "oracle_wp.h"

template<class ADDRESS, class FLAGS>
class PT2_byte_acu_single : public Virtual_wp<ADDRESS, FLAGS> {
public:
   PT2_byte_acu_single();
   ~PT2_byte_acu_single();
	/*
	 * this function tells all pages covered by this range is watched or not
	 */
	int     general_fault  (ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags = 0);
	int     watch_fault    (ADDRESS start_addr, ADDRESS end_addr);
	int     read_fault     (ADDRESS start_addr, ADDRESS end_addr);
	int     write_fault    (ADDRESS start_addr, ADDRESS end_addr);
	
	/*
	 * returns the number of changes it does on bit_map
	 * for counting the number of changes: changes = add + rm;
	 */
	int      add_watchpoint (ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags = 0);
	int      rm_watchpoint  (ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags = 0);
	
private:
	/*
	 * initialized when constructing
	 * used for checking each page's state when rm_watchpoint is called
	 */
   Oracle<ADDRESS, FLAGS> *wp;
   /*
	 * two bits for all pages (in segmentation register)
	 * watched(10), unwatched(01) or missed(00)
	 */S
   bool seg_reg_watched;
   bool seg_reg_unwatched;
   /*
	 * two bits for each 2nd_level_page
	 * watched(10), unwatched(01) or missed(00)
	 */
	unsigned char superpage_watched[SUPER_PAGE_BIT_MAP_NUMBER];
	unsigned char superpage_unwatched[SUPER_PAGE_BIT_MAP_NUMBER];
	/*
	 * two bit for each page
	 * keeping track of whether this page is watched(10) or 
	 * unwatched(01) or trie(00)
	 */
	unsigned char pt_watched[BIT_MAP_NUMBER];
	unsigned char pt_unwatched[BIT_MAP_NUMBER];
};

#include "pt2_byte_acu_single.cpp"
#endif /* PT2_BYTE_ACU_SINGLE_H_ */

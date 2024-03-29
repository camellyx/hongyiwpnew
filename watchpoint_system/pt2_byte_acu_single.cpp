#ifndef PT2_BYTE_ACU_SINGLE_CPP_
#define PT2_BYTE_ACU_SINGLE_CPP_

#include "pt2_byte_acu_single.h"

template<class ADDRESS, class FLAGS>
PT2_byte_acu_single<ADDRESS, FLAGS>::PT2_byte_acu_single(Virtual_wp<ADDRESS, FLAGS> *wp_ref) {
   wp = wp_ref;
   plb_misses = 0;
   seg_reg_read_watched = false;
   seg_reg_write_watched = false;
   seg_reg_unknown = false;
   superpage_read_watched.reset();
   superpage_write_watched.reset();
   superpage_unknown.reset();
   pt_read_watched.reset();
   pt_write_watched.reset();
   pt_unknown.reset();
}

template<class ADDRESS, class FLAGS>
PT2_byte_acu_single<ADDRESS, FLAGS>::PT2_byte_acu_single() {
   wp = NULL;
}

template<class ADDRESS, class FLAGS>
PT2_byte_acu_single<ADDRESS, FLAGS>::~PT2_byte_acu_single() {
}

template<class ADDRESS, class FLAGS>
void PT2_byte_acu_single<ADDRESS, FLAGS>::plb_shootdown(ADDRESS start_addr, ADDRESS end_addr)
{
   plb.plb_shootdown(start_addr, end_addr);
}

template<class ADDRESS, class FLAGS>
int PT2_byte_acu_single<ADDRESS, FLAGS>::general_fault(ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags) {
   bool really_watched = false;
   bool unwatched = false;
   bool found_them_all = true;
   int any_missed_plb = 0;
   ADDRESS last_superpage = -1;
   ADDRESS last_page = -1;
   // update plb
   PLB_entry<ADDRESS> temp;
   temp.tag = 0;
   temp.level = 'a';
   // checking the highest level hits
   if (((target_flags & WA_READ) && seg_reg_read_watched) ||
         ((target_flags & WA_WRITE) && seg_reg_write_watched)) {
      if (!plb.check_and_update(temp))
         plb_misses++;
      return ALL_WATCHED;
   }
   if (!seg_reg_unknown) {
      if (!plb.check_and_update(temp))
         plb_misses++;
      return ALL_UNWATCHED;
   }
   // checking superpage hits
   ADDRESS superpage_number_start = (start_addr>>SUPERPAGE_OFFSET_LENGTH);
   ADDRESS superpage_number_end   = (end_addr  >>SUPERPAGE_OFFSET_LENGTH);
   for (ADDRESS i=superpage_number_start;i<=superpage_number_end;i++) {
      // update plb
      PLB_entry<ADDRESS> temp;
      temp.tag = i;
      temp.level = 's';
      // check watched or not
      if (((target_flags & WA_READ) && superpage_read_watched[i])  ||
            ((target_flags & WA_WRITE) && superpage_write_watched[i]) ) {
         if (!plb.check_and_update(temp)) {
            any_missed_plb = 1;
            plb_misses++;
         }
         really_watched = true;
      }
      if (superpage_unknown[i])
         found_them_all = false;
      else {
         if (!plb.check_and_update(temp)) {
            any_missed_plb = 1;
            plb_misses++;
         }
         unwatched = true;
      }
   }
   if (any_missed_plb)
      plb_misses+=any_missed_plb; // We're adding "one more memory read" because we had to load in the base address for this table.
   if (really_watched)
      return SUPERPAGE_WATCHED;
   if (unwatched && found_them_all)
      return SUPERPAGE_UNWATCHED;
   found_them_all = true;
   really_watched = false;
   unwatched = false;
   any_missed_plb = 0;
   // calculating the starting V.P.N. and the ending V.P.N.
   ADDRESS page_number_start = (start_addr>>PAGE_OFFSET_LENGTH);
   ADDRESS page_number_end   = (end_addr  >>PAGE_OFFSET_LENGTH);
   for (ADDRESS i=page_number_start;i<=page_number_end;i++) {           // for each page, 
      if (superpage_unknown[i>>SECOND_LEVEL_PAGE_NUM_LENGTH]) {
         // update plb
         PLB_entry<ADDRESS> temp;
         temp.tag = i;
         temp.level = 'p';
         // check watched or not
         if (((target_flags & WA_READ) && pt_read_watched[i]) ||
               ((target_flags & WA_WRITE) && pt_write_watched[i])) {
            if (!plb.check_and_update(temp)) {
               if (last_superpage != i >> SECOND_LEVEL_PAGE_NUM_LENGTH) {
                  last_superpage = i >> SECOND_LEVEL_PAGE_NUM_LENGTH;
                  any_missed_plb++;
               }
               any_missed_plb++;
            }
            really_watched = true;
         }
         if (pt_unknown[i])
            found_them_all = false;
         else {
            if (!plb.check_and_update(temp)) {
               if (last_superpage != i >> SECOND_LEVEL_PAGE_NUM_LENGTH) {
                  last_superpage = i >> SECOND_LEVEL_PAGE_NUM_LENGTH;
                  any_missed_plb++;
               }
               any_missed_plb++;
            }
            unwatched = true;
         }
      }
   }
   if (any_missed_plb) {
      plb_misses+=any_missed_plb; // We're adding a memory read for every level of the table we had to read.
      plb_misses++; // Plus one for the top level.
   }
   if (really_watched)
      return PAGETABLE_WATCHED;
   if (unwatched && found_them_all)
      return PAGETABLE_UNWATCHED;
   found_them_all = true;
   really_watched = false;
   unwatched = false;
   any_missed_plb = 0;
   last_superpage = (ADDRESS)-1;
   
   ADDRESS plb_line_start = (start_addr>>PLB_LINE_OFFSET_LENGTH);
   ADDRESS plb_line_end   = (end_addr  >>PLB_LINE_OFFSET_LENGTH);
   for (ADDRESS i=plb_line_start;i<=plb_line_end;i++) {
      if (pt_unknown[i>>(PAGE_OFFSET_LENGTH-PLB_LINE_OFFSET_LENGTH)]) {
         // update plb
         PLB_entry<ADDRESS> temp;
         temp.tag = i;
         temp.level = 'b';
         if (!plb.check_and_update(temp)) {
            if (last_superpage != i >> SECOND_LEVEL_PAGE_NUM_LENGTH) {
               last_superpage = i >> SECOND_LEVEL_PAGE_NUM_LENGTH;
               any_missed_plb++;
            }
            if (last_page != i >> (PAGE_OFFSET_LENGTH-PLB_LINE_OFFSET_LENGTH)) {
               last_page = i >> (PAGE_OFFSET_LENGTH-PLB_LINE_OFFSET_LENGTH);
               any_missed_plb++;
            }
            any_missed_plb++;
         }
      }
   }
   if (any_missed_plb) {
      plb_misses+=any_missed_plb;
      plb_misses++;
   }
   if (wp->general_fault(start_addr, end_addr, target_flags))
      return BITMAP_WATCHED;
   return BITMAP_UNWATCHED;
}

template<class ADDRESS, class FLAGS>
int PT2_byte_acu_single<ADDRESS, FLAGS>::watch_fault(ADDRESS start_addr, ADDRESS end_addr) {
   return general_fault(start_addr, end_addr, WA_READ | WA_WRITE);
}

template<class ADDRESS, class FLAGS>
int PT2_byte_acu_single<ADDRESS, FLAGS>::read_fault(ADDRESS start_addr, ADDRESS end_addr) {
   return general_fault(start_addr, end_addr, WA_READ);
}

template<class ADDRESS, class FLAGS>
int PT2_byte_acu_single<ADDRESS, FLAGS>::write_fault(ADDRESS start_addr, ADDRESS end_addr) {
   return general_fault(start_addr, end_addr, WA_WRITE);
}

template<class ADDRESS, class FLAGS>
int PT2_byte_acu_single<ADDRESS, FLAGS>::add_watchpoint(ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags) {
   // calculating the starting V.P.N. and the ending V.P.N.
   ADDRESS page_number_start = (start_addr>>PAGE_OFFSET_LENGTH);
   ADDRESS page_number_end = (end_addr>>PAGE_OFFSET_LENGTH);
   ADDRESS superpage_number_start = (start_addr>>SUPERPAGE_OFFSET_LENGTH);
   ADDRESS superpage_number_end = (end_addr>>SUPERPAGE_OFFSET_LENGTH);
   bool changed_page_from_bitmap = false;
   bool changed_page_to_bitmap = false;
   bool changed_superpage_from_bitmap = false;
   bool changed_superpage_to_bitmap = false;
   int total_number_of_sets = 0;
   /*unsigned int firstpage_bitmap_change = 0, 
                lastpage_bitmap_change = 0, 
                first_superpage_bitmap_change = 0, 
                last_superpage_bitmap_change = 0;*/
   if (page_number_start == page_number_end) {  // if in the same page
      // check if read watched
      if (target_flags & WA_READ) {
         if (check_page_level_consistency(page_number_start, WA_READ, true)) {
            if (!pt_read_watched[page_number_start]) {
               changed_page_from_bitmap = true;
               pt_read_watched[page_number_start] = true;
               if (check_superpage_level_consistency(superpage_number_start, WA_READ, true)) {
                  if (!superpage_read_watched[superpage_number_start]) {
                     changed_superpage_from_bitmap = true;
                     superpage_read_watched[superpage_number_start] = true;
                     if (check_seg_reg_level_consistency(WA_READ, true))
                        seg_reg_read_watched = true;
                     else {
                        seg_reg_read_watched = false;
                        seg_reg_unknown = true;
                     }
                  }
               }
               else {
                  if(superpage_read_watched[superpage_number_start])
                     changed_superpage_to_bitmap = true;
                  superpage_read_watched[superpage_number_start] = false;
                  superpage_unknown[superpage_number_start] = true;
               }
            }
         }
         else {
            if(pt_read_watched[page_number_start])
               changed_page_to_bitmap = true;
            pt_read_watched[page_number_start] = false;
            pt_unknown[page_number_start] = true;
         }
      }
      // check if write watched  (same code as above)
      if (target_flags & WA_WRITE) {
         if (check_page_level_consistency(page_number_start, WA_WRITE, true)) {
            if (!pt_write_watched[page_number_start]) {
               changed_page_from_bitmap = true;
               pt_write_watched[page_number_start] = true;
               if (check_superpage_level_consistency(superpage_number_start, WA_WRITE, true)) {
                  if (!superpage_write_watched[superpage_number_start] ) {
                     changed_superpage_from_bitmap = true;
                     superpage_write_watched[superpage_number_start] = true;
                     if (check_seg_reg_level_consistency(WA_WRITE, true))
                        seg_reg_write_watched = true;
                     else {
                        seg_reg_write_watched = false;
                        seg_reg_unknown = true;
                     }
                  }
                  else {
                     if(superpage_write_watched[superpage_number_start])
                        changed_superpage_to_bitmap = true;
                     superpage_write_watched[superpage_number_start] = false;
                     superpage_unknown[superpage_number_start] = true;
                  }
               }
            }
         }
         else {
            if(pt_read_watched[page_number_start])
               changed_page_to_bitmap = true;
            pt_write_watched[page_number_start] = false;
            pt_unknown[page_number_start] = true;
         }
      }
      if (changed_page_from_bitmap) {
         if (changed_superpage_from_bitmap)
            return end_addr - start_addr + 3;
         else
            return end_addr - start_addr + 2;
      }
      else if (changed_page_to_bitmap) {
         if (changed_superpage_to_bitmap)
            return end_addr - start_addr + 1024 + 1024;
         else
             return end_addr - start_addr + 1024;
      }
      else
         return end_addr - start_addr + 1;
   }
   else {   // if not in the same page
      // setting start pagetables
      if (target_flags & WA_READ) {
         if (check_page_level_consistency(page_number_start, WA_READ, true)) {
            if (!pt_read_watched[page_number_start]) {
               changed_page_from_bitmap = true;
               pt_read_watched[page_number_start] = true;
            }
         }
         else {
            if (pt_read_watched[page_number_start]) {
               changed_page_to_bitmap = true;
            }
            pt_read_watched[page_number_start] = false;
            pt_unknown[page_number_start] = true;
         }
      }
      if (target_flags & WA_WRITE) {
         if (check_page_level_consistency(page_number_start, WA_WRITE, true)) {
            if (!pt_write_watched[page_number_start]) {
               changed_page_from_bitmap = true;
               pt_write_watched[page_number_start] = true;
            }
         }
         else {
            if (pt_write_watched[page_number_start]) {
               changed_page_to_bitmap = true;
            }
            pt_write_watched[page_number_start] = false;
            pt_unknown[page_number_start] = true;
         }
      }
      /*if (pt_unknown[page_number_start])
         firstpage_bitmap_change = ((page_number_start+1)<<PAGE_OFFSET_LENGTH) - start_addr;
      else
         firstpage_bitmap_change = 1;*/
      
      if(changed_page_to_bitmap)
         total_number_of_sets += 1025;
      else if (changed_page_from_bitmap)
         total_number_of_sets += (((page_number_start+1)<<PAGE_OFFSET_LENGTH) - start_addr + 1);
      else
         total_number_of_sets += (((page_number_start)<<PAGE_OFFSET_LENGTH) - start_addr);
      changed_page_from_bitmap = false;
      changed_page_to_bitmap = false;

      // setting end pagetables
      if (target_flags & WA_READ) {
         if (check_page_level_consistency(page_number_end, WA_READ, true)) {
            if (!pt_read_watched[page_number_end]) {
               changed_page_from_bitmap = true;
               pt_read_watched[page_number_end] = true;
            }
         }
         else {
            if (pt_read_watched[page_number_end])
               changed_page_to_bitmap = true;
            pt_read_watched[page_number_end] = false;
            pt_unknown[page_number_end] = true;
         }
      }
      if (target_flags & WA_WRITE) {
         if (check_page_level_consistency(page_number_end, WA_WRITE, true)) {
            if (!pt_write_watched[page_number_end]) {
               changed_page_from_bitmap = true;
               pt_write_watched[page_number_end] = true;
            }
         }
         else {
            if (pt_write_watched[page_number_end])
               changed_page_to_bitmap = true;
            pt_write_watched[page_number_end] = false;
            pt_unknown[page_number_end] = true;
         }
      }
      /*if (pt_unknown[page_number_end])
         lastpage_bitmap_change = end_addr - (page_number_end<<PAGE_OFFSET_LENGTH) + 1;
      else
         lastpage_bitmap_change = 1;*/

      if(changed_page_to_bitmap)
         total_number_of_sets += 1025;
      else if (changed_page_from_bitmap)
         total_number_of_sets += end_addr - (page_number_end<<PAGE_OFFSET_LENGTH) + 1;
      else
         total_number_of_sets += end_addr - (page_number_end<<PAGE_OFFSET_LENGTH);

      // setting all pagetables in the middle
      if (target_flags & WA_READ) {
         for (ADDRESS i=page_number_start+1;i!=page_number_end;i++)
            pt_read_watched[i] = true;
      }
      if (target_flags & WA_WRITE) {
         for (ADDRESS i=page_number_start+1;i!=page_number_end;i++)
            pt_write_watched[i] = true;
      }

      if((page_number_start+1 < page_number_end)) {
         if (superpage_number_start == superpage_number_end) {
            total_number_of_sets += (page_number_end - page_number_start - 1);
         }
         else {
            int number_of_pages_not_in_super = 0;
            int temp_page_number_start = page_number_start >> SECOND_LEVEL_PAGE_NUM_LENGTH;
            int temp_page_number_end = page_number_end >> SECOND_LEVEL_PAGE_NUM_LENGTH;
            temp_page_number_start++;
            temp_page_number_start = temp_page_number_start << SECOND_LEVEL_PAGE_NUM_LENGTH;
            temp_page_number_end = temp_page_number_end << SECOND_LEVEL_PAGE_NUM_LENGTH;
            temp_page_number_end++;
            number_of_pages_not_in_super = temp_page_number_start - page_number_start;
            number_of_pages_not_in_super += page_number_end - temp_page_number_end;
            total_number_of_sets  += number_of_pages_not_in_super;
         }
      }

      if (superpage_number_start == superpage_number_end) { // if in the same superpage
         // check if read watched
         if (target_flags & WA_READ) {
            if (check_superpage_level_consistency(superpage_number_start, WA_READ, true)) {
               if(!superpage_read_watched[superpage_number_start]) {
                  changed_superpage_from_bitmap = true;
                  superpage_read_watched[superpage_number_start] = true;
                  if (check_seg_reg_level_consistency(WA_READ, true))
                     seg_reg_read_watched = true;
                  else {
                     seg_reg_read_watched = false;
                     seg_reg_unknown = true;
                  }
               }
            }
            else {
               if(superpage_read_watched[superpage_number_start])
                  changed_superpage_to_bitmap = true;
               superpage_read_watched[superpage_number_start] = false;
               superpage_unknown[superpage_number_start] = true;
            }
         }
         // check if write watched (the same code as above)
         if (target_flags & WA_WRITE) {
            if (check_superpage_level_consistency(superpage_number_start, WA_WRITE, true)) {
               if(!superpage_write_watched[superpage_number_start]) {
                  changed_superpage_to_bitmap = true;
                  superpage_write_watched[superpage_number_start] = true;
                  if (check_seg_reg_level_consistency(WA_WRITE, true))
                     seg_reg_write_watched = true;
                  else {
                     seg_reg_write_watched = false;
                     seg_reg_unknown = true;
                  }
               }
            }
            else {
               if(superpage_read_watched[superpage_number_start])
                  changed_superpage_to_bitmap = true;
               superpage_write_watched[superpage_number_start] = false;
               superpage_unknown[superpage_number_start] = true;
            }
         }
         if(changed_superpage_from_bitmap)
            return total_number_of_sets + (page_number_end - page_number_start - 1) + 2;
         else if(changed_superpage_to_bitmap)
            return total_number_of_sets + 1024;
         else
            return total_number_of_sets + (page_number_end - page_number_start - 1);
      }
      else {   // if not in the same superpage
         // setting start superpage
         changed_superpage_from_bitmap = false;
         changed_superpage_to_bitmap = false;
         if (target_flags & WA_READ) {
            if (check_superpage_level_consistency(superpage_number_start, WA_READ, true)) {
               if (!superpage_read_watched[superpage_number_start]) {
                  changed_superpage_from_bitmap = true;
                  superpage_read_watched[superpage_number_start] = true;
               }
            }
            else {
               if (superpage_read_watched[superpage_number_start]) {
                  changed_superpage_to_bitmap = true;
               }
               superpage_read_watched[superpage_number_start] = false;
               superpage_unknown[superpage_number_start] = true;
            }
         }
         if (target_flags & WA_WRITE) {
            if (check_superpage_level_consistency(superpage_number_start, WA_WRITE, true)) {
               if (!superpage_write_watched[superpage_number_start]) {
                  changed_superpage_from_bitmap = true;
                  superpage_write_watched[superpage_number_start] = true;
               }
            }
            else {
               if (superpage_write_watched[superpage_number_start]) {
                  changed_superpage_to_bitmap = true;
               }
               superpage_write_watched[superpage_number_start] = false;
               superpage_unknown[superpage_number_start] = true;
            }
         }
         /*if (superpage_unknown[superpage_number_start])
            first_superpage_bitmap_change = ((superpage_number_start+1)<<SECOND_LEVEL_PAGE_NUM_LENGTH) - page_number_start;
         else
            first_superpage_bitmap_change = 1;*/

         if(changed_superpage_to_bitmap)
            total_number_of_sets += 1025;
         else if (changed_superpage_from_bitmap)
            total_number_of_sets += (((superpage_number_start+1)<<SECOND_LEVEL_PAGE_NUM_LENGTH) - page_number_start + 1);
         else
            total_number_of_sets += (((superpage_number_start+1)<<SECOND_LEVEL_PAGE_NUM_LENGTH) - page_number_start);
         changed_superpage_to_bitmap = false;
         changed_superpage_from_bitmap = false;

         // setting end superpage (the same code as above)
         if (target_flags & WA_READ) {
            if (check_superpage_level_consistency(superpage_number_end, WA_READ, true)) {
               if (!superpage_read_watched[superpage_number_end]) {
                  changed_superpage_from_bitmap = true;
                  superpage_read_watched[superpage_number_end] = true;
               }
            }
            else {
               if (superpage_read_watched[superpage_number_end])
                  changed_superpage_to_bitmap = true;
               superpage_read_watched[superpage_number_end] = false;
               superpage_unknown[superpage_number_end] = true;
            }
         }
         if (target_flags & WA_WRITE) {
            if (check_superpage_level_consistency(superpage_number_end, WA_WRITE, true)) {
               if (!superpage_write_watched[superpage_number_end]) {
                  changed_superpage_from_bitmap = true;
                  superpage_write_watched[superpage_number_end] = true;
               }
            }
            else {
               if (superpage_read_watched[superpage_number_end])
                  changed_superpage_to_bitmap = true;
               superpage_write_watched[superpage_number_end] = false;
               superpage_unknown[superpage_number_end] = true;
            }
         }
         /*if (superpage_unknown[superpage_number_end])
            last_superpage_bitmap_change = page_number_end - (superpage_number_end<<SECOND_LEVEL_PAGE_NUM_LENGTH) + 1;
         else
            last_superpage_bitmap_change = 1;*/
         if(changed_superpage_to_bitmap)
            total_number_of_sets += 1025;
         else if (changed_superpage_from_bitmap)
            total_number_of_sets += page_number_end - (superpage_number_end<<SECOND_LEVEL_PAGE_NUM_LENGTH) + 1;
         else
            total_number_of_sets += page_number_end - (superpage_number_end<<SECOND_LEVEL_PAGE_NUM_LENGTH);

         // set all superpages in the middle
         if (target_flags & WA_READ) {
            for (ADDRESS i=superpage_number_start+1;i!=superpage_number_end;i++)
               superpage_read_watched[i] = true;
         }
         if (target_flags & WA_WRITE) {
            for (ADDRESS i=superpage_number_start+1;i!=superpage_number_end;i++)
               superpage_write_watched[i] = true;
         }

         if(superpage_number_start+1 < superpage_number_end)
            total_number_of_sets += (superpage_number_end - superpage_number_start -1);

         if (target_flags & WA_READ) {
            if (check_seg_reg_level_consistency(WA_READ, true))
               seg_reg_read_watched = true;
            else {
               seg_reg_read_watched = false;
               seg_reg_unknown = true;
            }
         }
         if (target_flags & WA_WRITE) {
            if (check_seg_reg_level_consistency(WA_WRITE, true))
               seg_reg_write_watched = true;
            else {
               seg_reg_write_watched = false;
               seg_reg_unknown = true;
            }
         }
         /*if (seg_reg_unknown)
            return firstpage_bitmap_change + lastpage_bitmap_change 
              + first_superpage_bitmap_change + last_superpage_bitmap_change 
              + superpage_number_end - superpage_number_start - 1;
         else
            return 1;*/
         // We're skipping figure out the change-status of the seg register. Might miscount one store.. oh no!
         return total_number_of_sets;
      }
   }
}

template<class ADDRESS, class FLAGS>
int PT2_byte_acu_single<ADDRESS, FLAGS>::rm_watchpoint(ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags) {
   int page_change = 0, superpage_change = 0, total_change = 0;
   bool consistent;
   ADDRESS page_number = (start_addr>>PAGE_OFFSET_LENGTH);
   ADDRESS superpage_number = (start_addr>>SUPERPAGE_OFFSET_LENGTH);
   ADDRESS i=start_addr;
   ADDRESS low_page_mark, high_page_mark;
   ADDRESS low_superpage_mark, high_superpage_mark;

   low_page_mark = ((start_addr>>PAGE_OFFSET_LENGTH)+1)<<PAGE_OFFSET_LENGTH;
   high_page_mark = ((end_addr>>PAGE_OFFSET_LENGTH)<<PAGE_OFFSET_LENGTH)-1;
   low_superpage_mark = ((start_addr>>SUPERPAGE_OFFSET_LENGTH)+1)<<SUPERPAGE_OFFSET_LENGTH;
   high_superpage_mark = ((end_addr>>SUPERPAGE_OFFSET_LENGTH)<<SUPERPAGE_OFFSET_LENGTH)-1;
   bool full_remove = (target_flags == (WA_READ|WA_WRITE));
   do {
      if ( ((target_flags & WA_READ ) && !wp->general_fault(i, i, WA_READ )) 
        || ((target_flags & WA_WRITE) && !wp->general_fault(i, i, WA_WRITE)) )
         page_change++;                               // if unwatched, overwrite in bitmap
      if ( (i & (PAGE_SIZE-1)) == (PAGE_SIZE-1) ) {   // if page_end
         if (page_change) {
            consistent = true;
            if (target_flags & WA_READ) {
               if (check_page_level_consistency(page_number, WA_READ, false)) {
                  pt_read_watched[page_number] = false;
                  if (!check_page_level_consistency(page_number))
                     pt_unknown[page_number] = false;
               }
               else {
                  pt_read_watched[page_number] = false;
                  pt_unknown[page_number] = true;
                  consistent = false;
               }
            }
            if (target_flags & WA_WRITE) {
               if (check_page_level_consistency(page_number, WA_WRITE, false)) {
                  pt_write_watched[page_number] = false;
                  if (!check_page_level_consistency(page_number))
                     pt_unknown[page_number] = false;
               }
               else {
                  pt_write_watched[page_number] = false;
                  pt_unknown[page_number] = true;
                  consistent = false;
               }
            }
            if (full_remove && consistent && i > low_page_mark && i < high_page_mark) {
               superpage_change++;
            }
            else
               superpage_change += page_change;
         }
         page_change = 0;
         page_number++;
      }
      if ( (i & ((1<<SUPERPAGE_OFFSET_LENGTH)-1)) == ((1<<SUPERPAGE_OFFSET_LENGTH)-1) ) {    // if superpage_end
         if (superpage_change) {
            consistent = true;
            if (target_flags & WA_READ) {
               if (check_superpage_level_consistency(superpage_number, WA_READ, false)) {
                  superpage_read_watched[superpage_number] = false;
                  if (check_superpage_level_consistency(superpage_number))
                     superpage_unknown[superpage_number] = false;
               }
               else {
                  superpage_read_watched[superpage_number] = false;
                  superpage_unknown[superpage_number] = true;
                  consistent = false;
               }
            }
            if (target_flags & WA_WRITE) {
               if (check_superpage_level_consistency(superpage_number, WA_WRITE, false)) {
                  superpage_write_watched[superpage_number] = false;
                  if (check_superpage_level_consistency(superpage_number))
                     superpage_unknown[superpage_number] = false;
               }
               else {
                  superpage_write_watched[superpage_number] = false;
                  superpage_unknown[superpage_number] = true;
                  consistent = false;
               }
            }
            if (full_remove && consistent && i > low_superpage_mark && i < high_superpage_mark)
               total_change++;
            else
               total_change += superpage_change;
         }
         superpage_change = 0;
         superpage_number++;
      }
      i++;
   } while (i!=end_addr+1);
   if (page_change) {
      consistent = true;
      if (target_flags & WA_READ) {
         if (check_page_level_consistency(page_number, WA_READ, false)) {
            pt_read_watched[page_number] = false;
            if (check_page_level_consistency(page_number))
               pt_unknown[page_number] = false;
         }
         else {
            pt_read_watched[page_number] = false;
            pt_unknown[page_number] = true;
            consistent = false;
         }
      }
      if (target_flags & WA_WRITE) {
         if (check_page_level_consistency(page_number, WA_WRITE, false)) {
            pt_write_watched[page_number] = false;
            if (check_page_level_consistency(page_number))
               pt_unknown[page_number] = false;
         }
         else {
            pt_write_watched[page_number] = false;
            pt_unknown[page_number] = true;
            consistent = false;
         }
      }
      if (consistent)
         superpage_change++;
      else
         superpage_change += page_change;
   }
   if (superpage_change) {
      consistent = true;
      if (target_flags & WA_READ) {
         if (check_superpage_level_consistency(superpage_number, WA_READ, false)) {
            superpage_read_watched[superpage_number] = false;
            if (check_superpage_level_consistency(superpage_number))
               superpage_unknown[superpage_number] = false;
         }
         else {
            superpage_read_watched[superpage_number] = false;
            superpage_unknown[superpage_number] = true;
            consistent = false;
         }
      }
      if (target_flags & WA_WRITE) {
         if (check_superpage_level_consistency(superpage_number, WA_WRITE, false)) {
            superpage_write_watched[superpage_number] = false;
            if (check_superpage_level_consistency(superpage_number))
               superpage_unknown[superpage_number] = false;
         }
         else {
            superpage_write_watched[superpage_number] = false;
            superpage_unknown[superpage_number] = true;
            consistent = false;
         }
      }
      if (consistent)
         total_change++;
      else
         total_change += superpage_change;
   }
   if (total_change) {
      consistent = true;
      if (target_flags & WA_READ) {
         if (check_seg_reg_level_consistency(WA_READ, false)) {
            seg_reg_read_watched = false;
            if (check_seg_reg_level_consistency())
               seg_reg_unknown = false;
         }
         else {
            seg_reg_read_watched = false;
            seg_reg_unknown = true;
            consistent = false;
         }
      }
      if (target_flags & WA_WRITE) {
         if (check_seg_reg_level_consistency(WA_WRITE, false)) {
            seg_reg_write_watched = false;
            if (check_seg_reg_level_consistency())
               seg_reg_unknown = false;
         }
         else {
            seg_reg_write_watched = false;
            seg_reg_unknown = true;
            consistent = false;
         }
      }
      if (consistent)
         total_change = 1;
      else
         total_change += 1;
   }
   //fprintf(stderr, "%d\n", total_change);
   return total_change;
}
// page level: check if the whole page is consistent or unknown
template<class ADDRESS, class FLAGS>
bool PT2_byte_acu_single<ADDRESS, FLAGS>::check_page_level_consistency(ADDRESS page_number) {
   ADDRESS start = (page_number<<PAGE_OFFSET_LENGTH);
   ADDRESS end   = ((page_number+1)<<PAGE_OFFSET_LENGTH)-1;
   typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator 
      search_iter = wp->search_address(start);
   if (search_iter->end_addr >= end)
      return true;
   else
      return false;
}
// page level: check if a certain flag is consistent
template<class ADDRESS, class FLAGS>
bool PT2_byte_acu_single<ADDRESS, FLAGS>::check_page_level_consistency(ADDRESS page_number, FLAGS target_flag, bool watched) {
   ADDRESS start = (page_number<<PAGE_OFFSET_LENGTH);
   ADDRESS end   = ((page_number+1)<<PAGE_OFFSET_LENGTH)-1;
   bool search = true;
   typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator search_iter;
   while (search) {
      search_iter = wp->search_address(start);
      if ((search_iter->flags & target_flag) != watched)
         return false;
      if (search_iter->end_addr >= end)
         search = false;
      else
         start = search_iter->end_addr+1;
   }
   return true;
}
// superpage level: check if the whole superpage is consistent or unknown
template<class ADDRESS, class FLAGS>
bool PT2_byte_acu_single<ADDRESS, FLAGS>::check_superpage_level_consistency(ADDRESS superpage_number) {
   ADDRESS start = (superpage_number<<SUPERPAGE_OFFSET_LENGTH);
   ADDRESS end   = ((superpage_number+1)<<SUPERPAGE_OFFSET_LENGTH)-1;
   typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator 
      search_iter = wp->search_address(start);
   if (search_iter->end_addr >= end)
      return true;
   else
      return false;
}
// superpage level: check if a certain flag is consistent
template<class ADDRESS, class FLAGS>
bool PT2_byte_acu_single<ADDRESS, FLAGS>::check_superpage_level_consistency(ADDRESS superpage_number, FLAGS target_flag, bool watched) {
   ADDRESS page_number_start = (superpage_number<<SECOND_LEVEL_PAGE_NUM_LENGTH);
   ADDRESS page_number_end = ((superpage_number+1)<<SECOND_LEVEL_PAGE_NUM_LENGTH)-1;
   if (target_flag & WA_READ) {
      if (watched) {
         for (ADDRESS i=page_number_start;i!=page_number_end;i++) {
            if (!pt_read_watched[i])
               return false;
         }
         return true;
      }
      else {
         for (ADDRESS i=page_number_start;i!=page_number_end;i++) {
            if (pt_read_watched[i] || pt_unknown[i])
               return false;
         }
         return true;
      }
   }
   if (target_flag & WA_WRITE) {
      if (watched) {
         for (ADDRESS i=page_number_start;i!=page_number_end;i++) {
            if (!pt_write_watched[i])
               return false;
         }
         return true;
      }
      else {
         for (ADDRESS i=page_number_start;i!=page_number_end;i++) {
            if (pt_write_watched[i] || pt_unknown[i])
               return false;
         }
         return true;
      }
   }
   return false;
}
// segment register level: check if all memory addresses are consistent
template<class ADDRESS, class FLAGS>
bool PT2_byte_acu_single<ADDRESS, FLAGS>::check_seg_reg_level_consistency() {
   typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator 
      search_iter = wp->search_address(0);
   if (search_iter->end_addr == (ADDRESS)-1)
      return true;
   else
      return false;
}
// segment register level: check if a certain flag is consistent
template<class ADDRESS, class FLAGS>
bool PT2_byte_acu_single<ADDRESS, FLAGS>::check_seg_reg_level_consistency(FLAGS target_flag, bool watched) {
   if (target_flag & WA_READ) {
      if (watched) {
         if (superpage_read_watched.count() == SUPERPAGE_NUM)
            return true;
         else
            return false;
      }
      else {
         if (superpage_read_watched.count() == 0 && superpage_unknown.count() == 0)
            return true;
         else
            return false;
      }
   }
   if (target_flag & WA_WRITE) {
      if (watched) {
         if (superpage_write_watched.count() == SUPERPAGE_NUM)
            return true;
         else
            return false;
      }
      else {
         if (superpage_write_watched.count() == 0 && superpage_unknown.count() == 0)
            return true;
         else
            return false;
      }
   }
   return false;
}

#endif

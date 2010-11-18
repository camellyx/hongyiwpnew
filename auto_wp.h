/*
 * auto_wp.h
 *
 *  Created on: Nov 18, 2010
 *      Author: xhongyi
 */

#ifndef AUTO_WP_H_
#define AUTO_WP_H_

#include <deque>
#include <iostream>

template<class ADDRESS, class FLAGS>
struct watchpoint_t {
	ADDRESS		start_addr;
	ADDRESS		end_addr;
	FLAGS		flags;
};

template<class ADDRESS, class FLAGS>
class WatchPoint {
public:
	WatchPoint();
	~WatchPoint();

	void	add_watch	(ADDRESS start_addr, ADDRESS end_addr);
	void	add_read	(ADDRESS start_addr, ADDRESS end_addr);
	void	add_write	(ADDRESS start_addr, ADDRESS end_addr);

	void	rm_watch	(ADDRESS start_addr, ADDRESS end_addr);
	void	rm_read		(ADDRESS start_addr, ADDRESS end_addr);
	void	rm_write	(ADDRESS start_addr, ADDRESS end_addr);

	bool	watch_fault	(ADDRESS start_addr, ADDRESS end_addr);
	bool	read_fault		(ADDRESS start_addr, ADDRESS end_addr);
	bool	write_fault	(ADDRESS start_addr, ADDRESS end_addr);

	void	watch_print();
private:
	void	add_watchpoint	(ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags);
	void	rm_watchpoint	(ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags);
	void	general_fault	(ADDRESS start_addr, ADDRESS end_addr, FLAGS target_flags);

	deque< watchpoint_t<ADDRESS, FLAGS> > wp;
};

#endif /* AUTO_WP_H_ */
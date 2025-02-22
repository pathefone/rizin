// SPDX-FileCopyrightText: 2008-2020 pancake <pancake@nopcode.org>
// SPDX-License-Identifier: LGPL-3.0-only

#include <rz_util.h>

// TODO: use rz_list instead of list.h
// TODO: redesign this api.. why? :)
// TODO: add tags to ranges

// void (*ranges_new_callback)(struct range_t *r) = NULL;

RZ_API RRange *rz_range_new(void) {
	RRange *r = RZ_NEW0(RRange);
	if (r) {
		r->count = r->changed = 0;
		r->ranges = rz_list_new();
		if (!r->ranges) {
			rz_range_free(r);
			return NULL;
		}
		r->ranges->free = free;
	}
	return r;
}

RZ_API RRange *rz_range_free(RRange *r) {
	rz_list_purge(r->ranges);
	free(r);
	return NULL;
}

// TODO: optimize by just returning the pointers to the internal foo?
RZ_API int rz_range_get_data(RRange *rgs, ut64 addr, ut8 *buf, int len) {
	RRangeItem *r = rz_range_item_get(rgs, addr);
	if (!r) {
		return 0;
	}
	if (r->datalen < len) {
		len = r->datalen;
	}
	memcpy(buf, r->data, len);
	return len;
}

RZ_API int rz_range_set_data(RRange *rgs, ut64 addr, const ut8 *buf, int len) {
	RRangeItem *r = rz_range_item_get(rgs, addr);
	if (!r) {
		return 0;
	}
	r->data = (ut8 *)malloc(len);
	if (!r->data) {
		return 0;
	}
	r->datalen = len;
	memcpy(r->data, buf, len);
	return 1;
}

RZ_API RRangeItem *rz_range_item_get(RRange *rgs, ut64 addr) {
	RRangeItem *r;
	RzListIter *iter;
	rz_list_foreach (rgs->ranges, iter, r) {
		if (addr >= r->fr && addr < r->to) {
			return r;
		}
	}
	return NULL;
}

/* returns the sum of all the ranges contained */
// XXX: can be caught while adding/removing elements
RZ_API ut64 rz_range_size(RRange *rgs) {
	ut64 sum = 0;
	RzListIter *iter;
	RRangeItem *r;
	rz_list_foreach (rgs->ranges, iter, r) {
		sum += r->to - r->fr;
	}
	return sum;
}

RZ_API RRange *rz_range_new_from_string(const char *string) {
	RRange *rgs = rz_range_new();
	rz_range_add_from_string(rgs, string);
	return rgs;
}

RZ_API int rz_range_add_from_string(RRange *rgs, const char *string) {
	ut64 addr, addr2;
	int i, len = strlen(string) + 1;
	char *str, *ostr = malloc(len);
	if (!ostr) {
		return 0;
	}
	char *p = str = ostr;
	char *p2 = NULL;

	memcpy(str, string, len);
	for (i = 0; i < len; i++) {
		switch (str[i]) {
		case '-':
			str[i] = '\0';
			p2 = p;
			p = str + i + 1;
			break;
		case ',':
			str[i] = '\0';
			if (p2) {
				addr = rz_num_get(NULL, p);
				addr2 = rz_num_get(NULL, p2);
				rz_range_add(rgs, addr, addr2, 1);
				p2 = NULL;
			} else {
				addr = rz_num_get(NULL, p);
				rz_range_add(rgs, addr, addr + 1, 1);
			}
			p = str + i + 1;
			str[i] = ',';
			break;
		}
	}
	if (p2) {
		addr = rz_num_get(NULL, p);
		addr2 = rz_num_get(NULL, p2);
		rz_range_add(rgs, addr, addr2, 1);
	} else if (p) {
		addr = rz_num_get(NULL, p);
		rz_range_add(rgs, addr, addr + 1, 1);
	}
	free(ostr);
	return rgs ? rgs->changed : 0;
}

#if 0
    update to      new one     update fr   update fr/to  ignore

   |______|        |___|           |_____|      |____|      |_______|  range_t
+     |______|   +      |__|   + |___|      + |_________|  +  |__|     fr/to
  ------------   -----------   -----------  -------------  -----------
=  |_________|   = |___||__|   = |_______|  = |_________|   |_______|  result
#endif

RZ_API RRangeItem *rz_range_add(RRange *rgs, ut64 fr, ut64 to, int rw) {
	RzListIter *iter;
	RRangeItem *r, *ret = NULL;
	int add = 1;

	rz_num_minmax_swap(&fr, &to);
	rz_list_foreach (rgs->ranges, iter, r) {
		if (r->fr == fr && r->to == to) {
			add = 0;
		} else if (r->fr <= fr && r->fr <= to && r->to >= fr && r->to <= to) {
			r->to = to;
			ret = r;
			add = 0;
		} else if (r->fr >= fr && r->fr <= to && r->to >= fr && r->to >= to) {
			r->fr = fr;
			ret = r;
			add = 0;
		} else if (r->fr <= fr && r->fr <= to && r->to >= fr && r->to >= to) {
			/* ignore */
			add = 0;
		} else if (r->fr >= fr && r->fr <= to && r->to >= fr && r->to <= to) {
			r->fr = fr;
			r->to = to;
			ret = r;
			add = 0;
		}
	}

	if (rw && add) {
		ret = RZ_NEW(RRangeItem);
		ret->fr = fr;
		ret->to = to;
		ret->datalen = 0;
		ret->data = NULL;
		rz_list_append(rgs->ranges, ret);
		rgs->changed = 1;
	}

	return ret;
}

#if 0
    update to      ignore      update fr      delete        split

   |______|        |___|           |_____|      |____|       |________|  range_t
-     |______|   -      |__|   - |___|      - |_________|  -    |__|     fr/to
  ------------   -----------   -----------  -------------  ------------
=  |__|          =             =     |___|  =                |__|  |__|   result
#endif

RZ_API int rz_range_sub(RRange *rgs, ut64 fr, ut64 to) {
	RRangeItem *r;
	RzListIter *iter;

	rz_num_minmax_swap(&fr, &to);

__reloop:
	rz_list_foreach (rgs->ranges, iter, r) {
		/* update to */
		if (r->fr < fr && r->fr < to && r->to > fr && r->to < to) {
			r->to = fr;
		} else if (r->fr > fr && r->fr < to && r->to > fr && r->to > to) {
			/* update fr */
			r->fr = to;
		}
		/* delete */
		if (r->fr > fr && r->fr < to && r->to > fr && r->to < to) {
			/* delete */
			rz_list_delete(rgs->ranges, iter);
			rgs->changed = 1;
			goto __reloop;
		}
		/* split */
		if (r->fr < fr && r->fr < to && r->to > fr && r->to > to) {
			r->to = fr;
			rz_range_add(rgs, to, r->to, 1);
			// ranges_add(rang, to, r->to, 1);
			goto __reloop;
		}
	}
	return 0;
}

#if 0
/* TODO: should remove some of them right? */
RZ_API void rz_range_merge(RRange *rgs, RRange *r) {
	RzListIter *iter;
	RRangeItem *r;
	rz_list_foreach (rgs->ranges, iter, r)
		rz_range_add (rgs, r->fr, r->to, 0);
}
#endif

// int ranges_is_used(ut64 addr)
RZ_API int rz_range_contains(RRange *rgs, ut64 addr) {
	RRangeItem *r;
	RzListIter *iter;
	rz_list_foreach (rgs->ranges, iter, r) {
		if (addr >= r->fr && addr <= r->to) {
			return true;
		}
	}
	return false;
}

static int cmp_ranges(void *a, void *b) {
	RRangeItem *first = (RRangeItem *)a;
	RRangeItem *second = (RRangeItem *)b;
	return (first->fr > second->fr) - (first->fr < second->fr);
}

RZ_API int rz_range_sort(RRange *rgs) {
	bool ch = rgs->ranges->sorted;
	if (!rgs->changed) {
		return false;
	}
	rgs->changed = false;
	rz_list_sort(rgs->ranges, (RzListComparator)cmp_ranges);
	if (ch != rgs->ranges->sorted) {
		rgs->changed = true;
	}
	return rgs->changed;
}

RZ_API void rz_range_percent(RRange *rgs) {
	RzListIter *iter;
	RRangeItem *r;
	int w, i;
	ut64 seek, step;
	ut64 dif, fr = -1, to = -1;

	rz_list_foreach (rgs->ranges, iter, r) {
		if (fr == -1) {
			/* init */
			fr = r->fr;
			to = r->to;
		} else {
			if (fr > r->fr) {
				fr = r->fr;
			}
			if (to < r->to) {
				to = r->to;
			}
		}
	}
	w = 65; // columns
	if (fr != -1) {
		dif = to - fr;
		if (dif < w) {
			step = 1; // XXX
		} else {
			step = dif / w;
		}
	} else {
		step = fr = to = 0;
	}
	seek = 0;
	// XXX do not use printf here!
	printf("0x%08" PFMT64x " [", fr);
	for (i = 0; i < w; i++) {
		if (rz_range_contains(rgs, seek)) {
			printf("#");
		} else {
			printf(".");
		}
		seek += step;
	}
	printf("] 0x%08" PFMT64x "\n", to);
}

// TODO: total can be cached in rgs!!
RZ_API int rz_range_list(RRange *rgs, int rad) {
	ut64 total = 0;
	RRangeItem *r;
	RzListIter *iter;
	rz_range_sort(rgs);
	rz_list_foreach (rgs->ranges, iter, r) {
		if (rad) {
			printf("ar+ 0x%08" PFMT64x " 0x%08" PFMT64x "\n", r->fr, r->to);
		} else {
			printf("0x%08" PFMT64x " 0x%08" PFMT64x " ; %" PFMT64d "\n", r->fr, r->to, r->to - r->fr);
		}
		total += (r->to - r->fr);
	}
	eprintf("Total bytes: %" PFMT64d "\n", total);
	return 0;
}

RZ_API int rz_range_get_n(RRange *rgs, int n, ut64 *fr, ut64 *to) {
	int count = 0;
	RRangeItem *r;
	RzListIter *iter;
	rz_range_sort(rgs);
	rz_list_foreach (rgs->ranges, iter, r) {
		if (count == n) {
			*fr = r->fr;
			*to = r->to;
			return 1;
		}
		count++;
	}
	return 0;
}

#if 0
     .....|______________________|...
      |_____|  |____|  |_______|
    ---------------------------------
            |__|    |__|       |_|
#endif
RZ_API RRange *rz_range_inverse(RRange *rgs, ut64 fr, ut64 to, int flags) {
	RzListIter *iter;
	RRangeItem *r = NULL;
	RRange *newrgs = rz_range_new();

	rz_range_sort(rgs);

	rz_list_foreach (rgs->ranges, iter, r) {
		if (r->fr > fr && r->fr < to) {
			rz_range_add(newrgs, fr, r->fr, 1);
			fr = r->to;
		}
	}
	if (fr < to) {
		rz_range_add(newrgs, fr, to, 1);
	}
	return newrgs;
}

/*
	return true if overlap
	in *d
*/
// TODO: make it a macro
// TODO: move to num.c ?
RZ_API int rz_range_overlap(ut64 a0, ut64 a1, ut64 b0, ut64 b1, int *d) {
	// TODO: ensure ranges minmax .. innecesary at runtime?
	// rz_num_minmax_swap (&a0, &a1);
	// rz_num_minmax_swap (&b0, &b1);
	return *d = (b0 - a0), !(a1 < b0 || a0 > b1);
#if 0
	// does not overlap
	// a  |__|           |__|
	// b      |__|   |__|
	if (a1<b0 || a0>b1)
		return 0;

	// a     |____|   |_____|  |____|     |_____|
	// b  |____|        |_|       |____| |_______|
	//      b needs    a needs   a needs   b needs
	// delta required
	return (b0-a0);
#endif
}

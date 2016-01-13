/*
 * kern/htable.c - chained hash table
 * 
 * Copyright (c) 2015 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include "htable.h"
#include "config.h"

static 
int __ht_alloc(struct htable_s *hd, uint32_t nb_buckets)
{
	int i;

	hd->nb_buckets = nb_buckets; 
	hd->buckets = malloc(sizeof(struct list_entry)*nb_buckets);

	for(i=0; i < hd->nb_buckets; i++)
	{
		list_root_init(&hd->buckets[i]);
	}

	return 0;
}

int ht_init(struct htable_s *hd, hhash_ft *hhash, 
		hcompare_t *hcompare, uint32_t nb_buckets)
{

	hd->hhash = hhash;
	hd->hcompare = hcompare;
	__ht_alloc(hd, nb_buckets);

	return 0;
}

int ht_destroy(struct htable_s *hd)
{
	free(hd->buckets);
	return 0;
}

int hn_init(struct hnode_s *hn)
{
	hn->hash = (uint32_t) -1;
	list_entry_init(&hn->list);

	return 0;
}

int hn_destroy(struct hnode_s *hn)
{
	return 0;
}

static 
struct hnode_s* __hfind(struct htable_s *hd, void *key)
{
	int res;
	hash_t hval;
	uint32_t bkt;
	struct hnode_s *hn;
	struct list_entry *lh, *iter;

	hd->hhash(key, &hval, &bkt);
	lh = &hd->buckets[bkt];

	list_foreach(lh, iter)
	{
		hn = list_element(iter, struct hnode_s, list);
		if(hn->hash == hval)
		{
			res = hd->hcompare(hn, key);
			if(!res)
				return hn;
			if(res > 0)
				return NULL;
		}
	}

	return NULL;
}

struct hnode_s* ht_find(struct htable_s *hd, void *key)
{
	struct hnode_s *hn;

	if(!(hn =__hfind(hd, key)))
		return NULL;

	return hn;
}

int ht_insert(struct htable_s *hd, struct hnode_s *hn, void *key)
{
	hash_t hval;
	uint32_t bkt;
	int comp_res;
	struct hnode_s *thn;
	struct list_entry *lh, *iter;

	hd->hhash(key, &hval, &bkt);
	hn->hash = hval;
	lh = &hd->buckets[bkt];

	list_foreach(lh, iter)
	{
		thn = list_element(iter, struct hnode_s, list);
		if(thn->hash >= hval)
		{
			comp_res = hd->hcompare(thn, key);
			if(!comp_res)
				return -1;//already exist
			if(comp_res > 0)
			{
				list_add_pred(iter, &hn->list);
				return 0;
			}
		}
	}

	list_add_pred(lh, &hn->list);

	return 0;
}

struct hnode_s* ht_get_bfirst(struct htable_s *hd, uint32_t bucket)
{
	struct list_entry *lh;

	assert(bucket < hd->nb_buckets);
	lh = &hd->buckets[bucket];

	if(list_empty(lh)) 
		return NULL;

	return list_first(lh, struct hnode_s, list);
}

struct hnode_s* ht_get_next(struct htable_s *hd, uint32_t bucket, struct hnode_s* hn)
{
	struct list_entry *lh;
	struct list_entry *ln;

	assert(bucket < hd->nb_buckets);
	lh = &hd->buckets[bucket];
	assert(!list_empty(lh));

	ln = list_next(lh, &hn->list);

	if(!ln)
		return NULL;

	return list_element(ln, struct hnode_s, list);
}

/*
 * kern/htable.h - chained hash table
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

#ifndef _HTABLE_H_
#define _HTABLE_H_

#include <stdint.h>
#include "list.h"

struct hnode_s;
typedef uint32_t hash_t;

/* User defined functions */
typedef int hhash_ft(void *key, hash_t * hash, uint32_t * bkt);
typedef int hcompare_t(struct hnode_s *hn, void *key);

/* The hash table */
struct htable_s {
    hhash_ft *hhash;
    hcompare_t *hcompare;
    uint32_t nb_buckets;
    struct list_entry *buckets;
};

/* A node of the table */
struct hnode_s {
    hash_t hash;
    struct list_entry list;
};

/* Initialises a node */
int hn_init(struct hnode_s *hn);

/* destroy ressource called by hn_init */
int hn_destroy(struct hnode_s *hn);

/* Initialises a hash table */
int ht_init(struct htable_s *hd, hhash_ft * hhash, hcompare_t * hcompare, uint32_t nb_buckets);

/* destroys ressource called by ht_init */
int ht_destroy(struct htable_s *hd);

/* Find an element with the key 'key' in the htable. 	*
 * Return the element (hn) if success; else return NULL */
struct hnode_s *ht_find(struct htable_s *hd, void *key);

/* Insert a new element (hn) with the key 'key' in the htable. 	*
 * Return 0 if success; return -1 if failed or element exist.	*/
int ht_insert(struct htable_s *hd, struct hnode_s *hn, void *key);

/* Helper function to walk the table */
struct hnode_s *ht_get_bfirst(struct htable_s *hd, uint32_t bucket);
struct hnode_s *ht_get_next(struct htable_s *hd, uint32_t bucket, struct hnode_s *hn);

#endif                          /* _HTABLE_H_ */

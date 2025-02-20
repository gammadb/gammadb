/*
 * Copyright (c) 2024 Gamma Data, Inc. <jackey@gammadb.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "postgres.h"

#include "port/atomics.h"
#include "storage/lwlock.h"
#include "storage/spin.h"
#include "storage/s_lock.h"

#include "storage/gamma_toc.h"

#define TOC_ENTRY_IS_PINNED(entry) \
	(pg_atomic_read_u32(&(entry)->refcount) > 0)

#define TOC_ENTRY_IS_INVALID(entry) \
	((entry)->flags & TOC_ENTRY_INVALID)

struct gamma_toc
{
	uint64		toc_magic;				/* Magic number identifying this TOC */
	LWLock		toc_lwlock;
	Size		toc_total_bytes;		/* Bytes managed by this TOC */
	Size		toc_allocated_bytes;	/* Bytes allocated of those managed */
	slock_t		toc_fifo_lock;
	uint32		toc_fifo_head;			/* begin with 1, 0 means invalid */
	uint32		toc_fifo_tail;			/* 0 means invalid */
	uint32		toc_nentry;				/* Number of entries in TOC */
	gamma_toc_entry toc_entry[FLEXIBLE_ARRAY_MEMBER];
};

/*
 * Initialize a region of shared memory with a table of contents.
 */
gamma_toc *
gamma_toc_create(uint64 magic, void *address, Size nbytes)
{
	gamma_toc    *toc = (gamma_toc *) address;

	Assert(nbytes > offsetof(gamma_toc, toc_entry));
	toc->toc_magic = magic;
	LWLockInitialize(&toc->toc_lwlock, LWLockNewTrancheId());
	LWLockRegisterTranche(toc->toc_lwlock.tranche, "gammadb_dsm_toc");

	SpinLockInit(&toc->toc_fifo_lock);
	toc->toc_fifo_head = 0;
	toc->toc_fifo_tail = 0;

	/*
	 * The alignment code in gamma_toc_allocate() assumes that the starting
	 * value is buffer-aligned.
	 */
	toc->toc_total_bytes = BUFFERALIGN_DOWN(nbytes);
	toc->toc_allocated_bytes = 0;
	toc->toc_nentry = 0;

	return toc;
}

gamma_toc *
gamma_toc_attach(uint64 magic, void *address)
{
	gamma_toc    *toc = (gamma_toc *) address;

	if (toc->toc_magic != magic)
		return NULL;

	Assert(toc->toc_total_bytes >= toc->toc_allocated_bytes);
	Assert(toc->toc_total_bytes > offsetof(gamma_toc, toc_entry));

	return toc;
}

static bool
gamma_toc_enough(gamma_toc *toc, Size nbytes)
{
	volatile gamma_toc *vtoc = toc;
	Size total_bytes;
	Size allocated_bytes;
	Size nentry;
	Size remain_bytes;

	/* compute the free space */
	total_bytes = vtoc->toc_total_bytes;
	allocated_bytes = vtoc->toc_allocated_bytes;
	nentry = vtoc->toc_nentry;
	remain_bytes = offsetof(gamma_toc, toc_entry) +
		(nentry * sizeof(gamma_toc_entry)) +
		allocated_bytes;

	/* Check for memory exhaustion and overflow. */
	if (remain_bytes + nbytes > total_bytes ||
			remain_bytes + nbytes < remain_bytes)
	{
		return false;
	}

	return true;
}

/* 
 * Check if it is possible to take an entry that is already
 * invalid (all tuples have been removed)
 */
static uint32
gamma_toc_invalid(gamma_toc *toc, Size nbytes)
{
	uint32 i = toc->toc_fifo_tail;

	while (i > 0)
	{
		gamma_toc_entry *target_entry = &(toc->toc_entry[i - 1]);

		if (TOC_ENTRY_IS_PINNED(target_entry))
		{
			i = target_entry->fifo_prev;
			continue;
		}

		if (TOC_ENTRY_IS_INVALID(target_entry) &&
			toc->toc_entry[i - 1].nbytes >= nbytes)
		{
			return i;
		}

		i = target_entry->fifo_prev;
	}

	return 0;
}

static bool
gamma_toc_merge(gamma_toc *toc, Size nbytes)
{
	uint32 nentry;
	uint32 i,j;

	nentry = toc->toc_nentry;

	for (i = nentry; i > 0; i--)
	{
		gamma_toc_entry *tail_entry = &(toc->toc_entry[i - 1]);
		bool move = false;

		if (TOC_ENTRY_IS_PINNED(tail_entry))
			return false;

		/* if tail entry is invalid, remove it directly */
		if (TOC_ENTRY_IS_INVALID(tail_entry))
		{
			toc->toc_allocated_bytes -= tail_entry->nbytes;
			toc->toc_nentry--;

			Assert(!TOC_ENTRY_IS_PINNED(tail_entry));

			SpinLockAcquire(&toc->toc_fifo_lock);

			if (toc->toc_fifo_head == i)
				toc->toc_fifo_head = tail_entry->fifo_next;

			if (toc->toc_fifo_tail == i)
				toc->toc_fifo_tail = tail_entry->fifo_prev;

			if (tail_entry->fifo_prev != 0)
				toc->toc_entry[tail_entry->fifo_prev - 1].fifo_next = tail_entry->fifo_next;

			if (tail_entry->fifo_next != 0)
				toc->toc_entry[tail_entry->fifo_next - 1].fifo_prev = tail_entry->fifo_prev;

			tail_entry->fifo_prev = 0; /* invalid */
			tail_entry->fifo_next = 0;

			//TODO:gamma_lru_ht_delete();

			SpinLockRelease(&toc->toc_fifo_lock);

			/* check if memory is enough */
			if (gamma_toc_enough(toc, nbytes))
				return true;

			continue;
		}

		/*
		 * Try to move the trailing entry forward
		 * GAMMA NOTE: use toc->toc_nentry, don't use nentry directly
		*/
		for (j = 0; j < i - 1; j++)
		{
			char *target_addr;
			char *tail_addr;
			gamma_toc_entry *target_entry = &(toc->toc_entry[j]);

			if (!(TOC_ENTRY_IS_INVALID(target_entry) &&
				target_entry->nbytes >= tail_entry->nbytes))
				continue;

			if (TOC_ENTRY_IS_PINNED(target_entry))
				continue;

			toc->toc_allocated_bytes -= tail_entry->nbytes;
			toc->toc_nentry--;

#if 0
			elog(WARNING, "[move tail to front head] head:%d, tail:%d",
								toc->toc_fifo_head, toc->toc_fifo_tail);
			elog(WARNING, "[move tail to front tail] prev:%d, next:%d",
								tail_entry->fifo_prev, tail_entry->fifo_next);
			elog(WARNING, "[move tail to front target] target: %d, prev:%d, next:%d",
								j+1,  target_entry->fifo_prev, target_entry->fifo_next);
#endif
			/* delete the tail entry in FIFO */
			SpinLockAcquire(&toc->toc_fifo_lock);

			if (toc->toc_fifo_head == i)
				toc->toc_fifo_head = j + 1;

			if (toc->toc_fifo_tail == i)
				toc->toc_fifo_tail = j + 1;

			if (target_entry->fifo_prev != 0)
				toc->toc_entry[target_entry->fifo_prev - 1].fifo_next = target_entry->fifo_next;

			if (target_entry->fifo_next != 0)
				toc->toc_entry[target_entry->fifo_next - 1].fifo_prev = target_entry->fifo_prev;

			target_entry->fifo_prev = tail_entry->fifo_prev;
			target_entry->fifo_next = tail_entry->fifo_next;

			if (tail_entry->fifo_prev != 0)
				toc->toc_entry[tail_entry->fifo_prev - 1].fifo_next = j + 1;

			if (tail_entry->fifo_next != 0)
				toc->toc_entry[tail_entry->fifo_next - 1].fifo_prev = j + 1;


			//TODO:HASH TABLE
			//gamma_lru_ht_delete();
			//gamma_lru_ht_delete();
			//gamma_lru_ht_insert();
			SpinLockRelease(&toc->toc_fifo_lock);

			tail_addr = gamma_toc_addr((gamma_toc *)toc, tail_entry);
			target_addr = gamma_toc_addr((gamma_toc *)toc, target_entry);

			memcpy(target_addr, tail_addr, tail_entry->nbytes);

			target_entry->relid = tail_entry->relid;
			target_entry->rgid = tail_entry->rgid;
			target_entry->attno = tail_entry->attno;
			target_entry->flags = tail_entry->flags;

			move = true;


			/* exchange is over, to check next tail entry */
			break;
		}

		if (!move)
			break;

		/* check if memory is enough */
		if (gamma_toc_enough(toc, nbytes))
			return true;
	}

	return false;
}

static uint32
gamma_toc_force(gamma_toc *toc, Size nbytes)
{
	uint32 i = toc->toc_fifo_tail;

	while (i > 0)
	{
		gamma_toc_entry *target_entry = &(toc->toc_entry[i - 1]);

		if (TOC_ENTRY_IS_PINNED(target_entry))
		{
			i = target_entry->fifo_prev;
			continue;
		}

		/* if tail entry is invalid, remove it directly */
		if (target_entry->nbytes < nbytes)
		{
			i = target_entry->fifo_prev;
			continue;
		}

		return i;
	}

	return 0;
}

static void
gamma_toc_lru(gamma_toc *toc, double percent)
{
	uint32 nentry = toc->toc_nentry;
	uint32 count = 0;
	uint32 ptr = toc->toc_fifo_tail;

	while (((double)count)/((double) nentry) < percent && ptr > 0)
	{
		gamma_toc_entry *cur_entry = &toc->toc_entry[ptr - 1];
		if (TOC_ENTRY_IS_INVALID(cur_entry))
		{
			ptr = cur_entry->fifo_prev;
			count++;
			continue;
		}

		if (TOC_ENTRY_IS_PINNED(cur_entry))
		{
			ptr = cur_entry->fifo_prev;
			count++;
			continue;
		}

		ptr = cur_entry->fifo_prev;
		cur_entry->flags = cur_entry->flags | TOC_ENTRY_INVALID;
		count++;
	}
}

static gamma_toc_entry *
gamma_toc_append(gamma_toc *toc, Size nbytes)
{
	volatile gamma_toc *vtoc = toc;
	gamma_toc_entry *result;
	Size total_bytes;
	Size allocated_bytes;
	Size nentry;
	Size remain_bytes;
	Size offset;

	/* compute the free space */
	total_bytes = vtoc->toc_total_bytes;
	allocated_bytes = vtoc->toc_allocated_bytes;
	nentry = vtoc->toc_nentry;
	remain_bytes = offsetof(gamma_toc, toc_entry) +
		(nentry * sizeof(gamma_toc_entry)) +
		allocated_bytes;

	/* Check for memory exhaustion and overflow. */
	if (remain_bytes + nbytes <= total_bytes &&
			remain_bytes + nbytes >= remain_bytes)
	{
		offset = total_bytes - allocated_bytes - nbytes;

		vtoc->toc_allocated_bytes += nbytes;

		nentry = vtoc->toc_nentry;
		pg_write_barrier();
		result = (gamma_toc_entry *) &(vtoc->toc_entry[nentry]);
		result->values_offset = offset;
		result->nbytes = nbytes;
		
		pg_atomic_init_u32(&result->refcount, 0);

		SpinLockAcquire(&toc->toc_fifo_lock);

		result->fifo_prev = 0; /* invalid */
		result->fifo_next = toc->toc_fifo_head;
		if (toc->toc_fifo_head != 0)
			toc->toc_entry[toc->toc_fifo_head - 1].fifo_prev = nentry + 1;
		toc->toc_fifo_head = nentry + 1;
		if (toc->toc_fifo_tail == 0)
			toc->toc_fifo_tail = nentry + 1;

		SpinLockRelease(&toc->toc_fifo_lock);

		vtoc->toc_nentry = vtoc->toc_nentry + 1;

		return result;
	}

	return NULL;
}

static void
gamma_toc_clear(gamma_toc *toc)
{
	uint32 ptr = toc->toc_fifo_tail;

	while (ptr > 0)
	{
		gamma_toc_entry *cur_entry = &toc->toc_entry[ptr - 1];
		if (TOC_ENTRY_IS_INVALID(cur_entry))
		{
			ptr = cur_entry->fifo_prev;
			continue;
		}

		if (TOC_ENTRY_IS_PINNED(cur_entry))
		{
			ptr = cur_entry->fifo_prev;
			continue;
		}

		ptr = cur_entry->fifo_prev;
		cur_entry->flags = cur_entry->flags | TOC_ENTRY_INVALID;
	}
}

gamma_toc_entry *
gamma_toc_alloc(gamma_toc *toc, Size nbytes)
{
	volatile gamma_toc *vtoc = toc;
	gamma_toc_entry *result;
	Size nentry = vtoc->toc_nentry;
	bool need_lru = true;
	bool need_clear = true;

retry:
	/* Check for memory exhaustion and overflow. */
	result = gamma_toc_append(toc, nbytes);
	if (result != NULL)
		return result;

	if (gamma_toc_merge((gamma_toc *)vtoc, nbytes))
	{
		result = gamma_toc_append(toc, nbytes);
		if (result != NULL)
			return result;
	}

	nentry = gamma_toc_invalid((gamma_toc *)vtoc, nbytes);
	if (nentry == 0 && !need_lru)
		nentry = gamma_toc_force((gamma_toc *)vtoc, nbytes);
	if (nentry != 0)
	{
		result = &(toc->toc_entry[nentry - 1]);
		SpinLockAcquire(&toc->toc_fifo_lock);

		if (toc->toc_fifo_head == nentry)
			toc->toc_fifo_head = result->fifo_next;

		if (toc->toc_fifo_tail == nentry)
			toc->toc_fifo_tail = result->fifo_prev;

		if (result->fifo_prev != 0)
			toc->toc_entry[result->fifo_prev - 1].fifo_next = result->fifo_next;
		if (result->fifo_next != 0)
			toc->toc_entry[result->fifo_next - 1].fifo_prev = result->fifo_prev;

		result->fifo_prev = 0; /* invalid */
		result->fifo_next = toc->toc_fifo_head;
		if (toc->toc_fifo_head != 0)
			toc->toc_entry[toc->toc_fifo_head - 1].fifo_prev = nentry;
		toc->toc_fifo_head = nentry;
		if (toc->toc_fifo_tail == 0)
			toc->toc_fifo_tail = nentry;

		result->flags = 0;

		SpinLockRelease(&toc->toc_fifo_lock);

		return result;
	}

	if (need_lru)
	{
		gamma_toc_lru((gamma_toc *)vtoc, 0.2);
		need_lru = false;
		goto retry;
	}

	if (need_clear)
	{
		gamma_toc_clear((gamma_toc *)vtoc);
		need_clear = false;
		goto retry;
	}

	elog(ERROR, "Gamma Buffer is insufficient!");

	return NULL;
}

char *
gamma_toc_addr(gamma_toc *toc, gamma_toc_entry *entry)
{
	return (((char *)toc) + entry->values_offset);
}

bool
gamma_toc_lookup(gamma_toc *toc, Oid relid, Oid rgid, int16 attno, gamma_buffer_cv *cv)
{
	uint32 i;
	uint32 head = toc->toc_fifo_head;

	pg_read_barrier();

	while (head != 0)
	{
		i = head - 1;

		/* the cv in toc is invalid(eg. it is truncated) */
		if (TOC_ENTRY_IS_INVALID(&toc->toc_entry[i]))
		{
			head = toc->toc_entry[i].fifo_next;
			continue;
		}

		if (toc->toc_entry[i].relid == relid &&
			toc->toc_entry[i].rgid == rgid &&
			toc->toc_entry[i].attno == attno)
		{
			char *cv_begin = NULL;
			gamma_toc_header *cv_header;

			uint32 align_v_nbytes;
			gamma_toc_entry *cur_entry = NULL;

			cur_entry = &toc->toc_entry[i];

			cv_begin = ((char *)toc) + toc->toc_entry[i].values_offset;
			cv_header = (gamma_toc_header *) cv_begin;
			cv_begin += sizeof(gamma_toc_header);

			align_v_nbytes = BUFFERALIGN(cv_header->values_nbytes);
			cv->values = (char *) cv_begin;
			cv->values_nbytes = cv_header->values_nbytes;

			if (cv_header->isnull_nbytes != 0)
			{
				cv->isnull = (bool *)(cv_begin + align_v_nbytes);
				cv->isnull_nbytes = cv_header->isnull_nbytes;
			}
			else
			{
				cv->isnull = NULL; 
				cv->isnull_nbytes = 0;
			}

			cv->dim = cv_header->dim;
			if (toc->toc_entry[i].flags & TOC_ENTRY_HAS_MIN)
				cv->min = (char *) cv_header->min;
			else
				cv->min = NULL;

			if (toc->toc_entry[i].flags & TOC_ENTRY_HAS_MAX)
				cv->max = (char *) cv_header->max;
			else
				cv->max = NULL;
#if 0
			elog(WARNING, "[add reference] entry: %d/%d/%d, refcount:%d",
								toc->toc_entry[i].relid,
								toc->toc_entry[i].rgid,
								toc->toc_entry[i].attno,
								toc->toc_entry[i].refcount);
#endif
			pg_atomic_fetch_add_u32(&toc->toc_entry[i].refcount, 1);

			SpinLockAcquire(&toc->toc_fifo_lock);

			/* delete first */
			if (toc->toc_fifo_head == i + 1)
				toc->toc_fifo_head = cur_entry->fifo_next;

			if (toc->toc_fifo_tail == i + 1)
				toc->toc_fifo_tail = cur_entry->fifo_prev;

			if (cur_entry->fifo_prev != 0)
				toc->toc_entry[cur_entry->fifo_prev - 1].fifo_next = cur_entry->fifo_next;

			if (cur_entry->fifo_next != 0)
				toc->toc_entry[cur_entry->fifo_next - 1].fifo_prev = cur_entry->fifo_prev;

			/* add to head */
			cur_entry->fifo_prev = 0; /* invalid */
			cur_entry->fifo_next = toc->toc_fifo_head;
			if (toc->toc_fifo_head != 0)
				toc->toc_entry[toc->toc_fifo_head - 1].fifo_prev = i + 1;
			toc->toc_fifo_head = i + 1;
			if (toc->toc_fifo_tail == 0)
				toc->toc_fifo_tail = i + 1;

			SpinLockRelease(&toc->toc_fifo_lock);
			
			return true;
		}

		head = toc->toc_entry[i].fifo_next;
	}

	return false;
}

gamma_toc_entry *
gamma_toc_get_entry(gamma_toc *toc, Oid relid, Oid rgid, int16 attno)
{
	uint32 nentry;
	uint32 i;

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
		if (toc->toc_entry[i].relid == relid &&
			toc->toc_entry[i].rgid == rgid &&
			toc->toc_entry[i].attno == attno)
		{
			/* the cv in toc is invalid(eg. it is truncated) */
			if (TOC_ENTRY_IS_INVALID(&toc->toc_entry[i]))
				continue;
			return &toc->toc_entry[i];
		}
	}

	return NULL;
}

void
gamma_toc_invalid_rel(gamma_toc *toc, Oid relid)
{
	uint32		nentry;
	uint32		i;
	uint32		retry = 0;

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
		gamma_toc_entry *cur_entry = &toc->toc_entry[i];

		retry = 0;
		while (TOC_ENTRY_IS_PINNED(cur_entry))
		{
			if (retry++ == 10000)
			{
				elog(WARNING, "Gamma buffers invalid relations: waiting 10s");
			}

			pg_usleep(1000L);
		}

		if (toc->toc_entry[i].relid == relid)
		{
			toc->toc_entry[i].flags = toc->toc_entry[i].flags | TOC_ENTRY_INVALID;
		}
	}
}

void
gamma_toc_invalid_rg(gamma_toc *toc, Oid relid, uint32 rgid)
{
	uint32		nentry;
	uint32		i;
	uint32		retry = 0;

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
		if (toc->toc_entry[i].relid == relid &&
			toc->toc_entry[i].rgid == rgid)
		{
			gamma_toc_entry *cur_entry = &toc->toc_entry[i];

			retry = 0;
			while (TOC_ENTRY_IS_PINNED(cur_entry))
			{
				if (retry++ == 10000)
				{
					elog(WARNING, "Gamma buffers invalid rowgroup: waiting 10s");
				}

				pg_usleep(1000L);
			}
			toc->toc_entry[i].flags = toc->toc_entry[i].flags | TOC_ENTRY_INVALID;
		}
	}
}

void
gamma_toc_invalid_cv(gamma_toc *toc, Oid relid, uint32 rgid, int16 attno)
{
	uint32		nentry;
	uint32		i;
	uint32		retry = 0;

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
		if (toc->toc_entry[i].relid == relid &&
			toc->toc_entry[i].rgid == rgid &&
			toc->toc_entry[i].attno == attno)
		{
			gamma_toc_entry *cur_entry = &toc->toc_entry[i];

			retry = 0;
			while (TOC_ENTRY_IS_PINNED(cur_entry))
			{
				if (retry++ == 10000)
				{
					elog(WARNING, "Gamma buffers invalid rowgroup: waiting 10s");
				}

				pg_usleep(1000L);
			}
			toc->toc_entry[i].flags = toc->toc_entry[i].flags | TOC_ENTRY_INVALID;
		}
	}
}

void
gamma_toc_lock_acquire_x(gamma_toc *toc)
{
	LWLockAcquire(&toc->toc_lwlock, LW_EXCLUSIVE);
}

void
gamma_toc_lock_acquire_s(gamma_toc *toc)
{
	LWLockAcquire(&toc->toc_lwlock, LW_SHARED);
}

void
gamma_toc_lock_release(gamma_toc *toc)
{
	LWLockRelease(&toc->toc_lwlock);
}

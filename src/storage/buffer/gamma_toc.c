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

#include "storage/gamma_toc.h"


struct gamma_toc
{
	uint64		toc_magic;		/* Magic number identifying this TOC */
	LWLock		toc_lwlock;
	Size		toc_total_bytes;	/* Bytes managed by this TOC */
	Size		toc_allocated_bytes;	/* Bytes allocated of those managed */
	slock_t		toc_fifo_lock;
	uint32		toc_fifo_header; /* begin with 1, 0 means invalid */
	uint32		toc_nentry;		/* Number of entries in TOC */
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
	uint32 nentry;
	uint32 i;

	nentry = toc->toc_nentry;

	for (i = 0; i < nentry; ++i)
	{
		if (toc->toc_entry[i].flags & TOC_ENTRY_INVALID &&
			toc->toc_entry[i].nbytes > nbytes)
			return i + 1;
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

		/* if tail entry is invalid, remove it directly */
		if (tail_entry->flags & TOC_ENTRY_INVALID)
		{
			toc->toc_allocated_bytes -= tail_entry->nbytes;
			toc->toc_nentry--;

			/* check if memory is enough */
			if (gamma_toc_enough(toc, nbytes))
				return true;

			continue;
		}

		/*
		 * Try to move the trailing entry forward
		 * GAMMA NOTE: use toc->toc_nentry, don't use nentry directly
		*/
		for (j = 0; j < toc->toc_nentry; j++)
		{
			gamma_toc_entry *target_entry = &(toc->toc_entry[j]);
			if (target_entry->flags & TOC_ENTRY_INVALID &&
				target_entry->nbytes >= tail_entry->nbytes)
			{
				char *target_addr;
				char *tail_addr;

				toc->toc_allocated_bytes -= tail_entry->nbytes;
				toc->toc_nentry--;

				tail_addr = gamma_toc_addr((gamma_toc *)toc, tail_entry);
				target_addr = gamma_toc_addr((gamma_toc *)toc, target_entry);

				memcpy(target_addr, tail_addr, tail_entry->nbytes);

				target_entry->relid = tail_entry->relid;
				target_entry->rgid = tail_entry->rgid;
				target_entry->attno = tail_entry->attno;
				target_entry->flags = tail_entry->flags;

				move = true;

				/* check if memory is enough */
				if (gamma_toc_enough(toc, nbytes))
					return true;
			}
		}

		if (!move)
			break;
	}

	return false;
}

static bool
gamma_toc_lru(gamma_toc *toc, Size nbytes)
{
	//TODO:
	return false;
}

gamma_toc_entry *
gamma_toc_alloc(gamma_toc *toc, Size nbytes)
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

		nentry = vtoc->toc_nentry++;
		pg_write_barrier();
		result = (gamma_toc_entry *) &(vtoc->toc_entry[nentry]);
		result->values_offset = offset;
		result->nbytes = nbytes;

		SpinLockAcquire(&toc->toc_fifo_lock);

		result->fifo_prev = 0; /* invalid */
		result->fifo_next = toc->toc_fifo_header;
		if (toc->toc_fifo_header != 0)
			toc->toc_entry[toc->toc_fifo_header - 1].fifo_prev = nentry + 1;
		toc->toc_fifo_header = nentry + 1;

		SpinLockRelease(&toc->toc_fifo_lock);
	}

	gamma_toc_merge((gamma_toc *)vtoc, nbytes);

	nentry = gamma_toc_invalid((gamma_toc *)vtoc, nbytes);
	if (nentry != 0)
	{
		result = &(toc->toc_entry[nentry - 1]);
		SpinLockAcquire(&toc->toc_fifo_lock);

		toc->toc_entry[result->fifo_prev].fifo_next = result->fifo_next;
		toc->toc_entry[result->fifo_next].fifo_prev = result->fifo_prev;

		result->fifo_prev = 0; /* invalid */
		result->fifo_next = toc->toc_fifo_header;
		toc->toc_entry[toc->toc_fifo_header - 1].fifo_prev = nentry;
		toc->toc_fifo_header = nentry;

		SpinLockRelease(&toc->toc_fifo_lock);

		return result;
	}



			if (gamma_toc_lru((gamma_toc *)vtoc, nbytes))
				continue;




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
			char *cv_begin = NULL;
			gamma_toc_header *cv_header;

			uint32 align_v_nbytes;

			/* the cv in toc is invalid(eg. it is truncated) */
			if (toc->toc_entry[i].flags & TOC_ENTRY_INVALID)
				continue;

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

			return true;
		}
	}

	return false;
}

void
gamma_toc_invalid_rel(gamma_toc *toc, Oid relid)
{
	uint32		nentry;
	uint32		i;

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
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

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
		if (toc->toc_entry[i].relid == relid &&
			toc->toc_entry[i].rgid == rgid)
		{
			toc->toc_entry[i].flags = toc->toc_entry[i].flags | TOC_ENTRY_INVALID;
		}
	}
}

void
gamma_toc_invalid_cv(gamma_toc *toc, Oid relid, uint32 rgid, int16 attno)
{
	uint32		nentry;
	uint32		i;

	nentry = toc->toc_nentry;
	pg_read_barrier();

	for (i = 0; i < nentry; ++i)
	{
		if (toc->toc_entry[i].relid == relid &&
			toc->toc_entry[i].rgid == rgid &&
			toc->toc_entry[i].attno == attno)
		{
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

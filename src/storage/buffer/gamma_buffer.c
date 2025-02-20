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

#include "storage/gamma_buffer.h"
#include "storage/gamma_dsm.h"
#include "storage/gamma_toc.h"

void
gamma_buffer_startup(void)
{
	gamma_buffer_dsm_startup();
}

bool
gamma_buffer_add_cv(Oid relid, Oid rgid, int16 attno, gamma_buffer_cv *cv)
{
	volatile gamma_toc *toc = gamma_buffer_dsm_toc();
	gamma_toc_entry *entry;
	gamma_toc_header header;
	gamma_buffer_cv exist_cv = {0};
	char *entry_addr = NULL;

	uint32 dim = cv->dim;
	char *data = cv->values;
	bool *nulls = cv->isnull;
	Size values_nbytes = cv->values_nbytes;
	Size isnull_nbytes = cv->isnull_nbytes;

	Size nbytes;

	Size align_v_nbytes;
	Size align_n_nbytes;
	Size align_h_nbytes;

	gamma_toc_lock_acquire_x((gamma_toc *)toc);

	align_v_nbytes = BUFFERALIGN(values_nbytes);
	align_n_nbytes = BUFFERALIGN(isnull_nbytes);
	align_h_nbytes = BUFFERALIGN(sizeof(gamma_toc_header));
	nbytes = align_v_nbytes + align_n_nbytes + align_h_nbytes;

	/* check if the other session have been insert the ColumnVector */
	if (gamma_toc_lookup((gamma_toc *)toc, relid, rgid, attno, &exist_cv))
	{
		Assert(values_nbytes == exist_cv.values_nbytes);
		Assert(isnull_nbytes == exist_cv.isnull_nbytes);
		gamma_toc_lock_release((gamma_toc *)toc);
		return true;
	}

	/* initialize the toc entry */
	entry = gamma_toc_alloc((gamma_toc *)toc, nbytes);
	if (entry == NULL)
	{
		gamma_toc_lock_release((gamma_toc *)toc);
		return false;
	}

	entry->relid = relid;
	entry->rgid = rgid;
	entry->attno = attno;
	entry->flags = 0;

	entry_addr = gamma_toc_addr((gamma_toc *)toc, entry);

	header.dim = dim;
	header.values_nbytes = values_nbytes;
	header.isnull_nbytes = isnull_nbytes;
	if (cv->min != NULL)
	{
		entry->flags = entry->flags | TOC_ENTRY_HAS_MIN;
		memcpy(header.min, cv->min, GAMMA_MINMAX_LENGTH);
	}

	if (cv->max != NULL)
	{
		entry->flags = entry->flags | TOC_ENTRY_HAS_MAX;
		memcpy(header.max, cv->max, GAMMA_MINMAX_LENGTH);
	}

	memcpy(entry_addr, &header, sizeof(gamma_toc_header));
	entry_addr += sizeof(gamma_toc_header);
	memcpy(entry_addr, data, values_nbytes);
	if (nulls != NULL)
		memcpy(entry_addr + align_v_nbytes, nulls, isnull_nbytes);

	gamma_toc_lock_release((gamma_toc *)toc);

	return true;
}

bool
gamma_buffer_get_cv(Oid relid, Oid rgid, int16 attno, gamma_buffer_cv *cv)
{
	bool result = false;
	gamma_toc *toc = gamma_buffer_dsm_toc();
	gamma_toc_lock_acquire_s(toc);
	result = gamma_toc_lookup(toc, relid, rgid, attno, cv);
	gamma_toc_lock_release(toc);
	return result;
}

void
gamma_buffer_invalid_rel(Oid relid)
{
	gamma_toc *toc = gamma_buffer_dsm_toc();
	gamma_toc_lock_acquire_x(toc);
	gamma_toc_invalid_rel(toc, relid);
	gamma_toc_lock_release(toc);
}

static void
gamma_buffer_release_entry_callback(ResourceReleasePhase phase,
		bool isCommit,
		bool isTopLevel,
		void *arg)
{
	if (phase == RESOURCE_RELEASE_BEFORE_LOCKS)
	{
		gamma_toc_entry *cur_entry = (gamma_toc_entry *) arg;

#if 0
		elog(WARNING, "[sub ref callback]entry: %d/%d/%d, refcount:%d",
				cur_entry->relid,
				cur_entry->rgid,
				cur_entry->attno,
				cur_entry->refcount);
#endif
		pg_atomic_fetch_sub_u32(&cur_entry->refcount, 1);
	}
}
	
void
gamma_buffer_register_cv(Oid relid, Oid rgid, int16 attno)
{
	gamma_toc_entry *cur_entry;
	gamma_toc *toc = gamma_buffer_dsm_toc();
	gamma_toc_lock_acquire_s(toc);
	cur_entry = gamma_toc_get_entry(toc, relid, rgid, attno);
	RegisterResourceReleaseCallback(gamma_buffer_release_entry_callback, (void *)cur_entry);
	gamma_toc_lock_release(toc);
}

void
gamma_buffer_release_cv(Oid relid, Oid rgid, int16 attno)
{
	gamma_toc_entry *cur_entry;
	gamma_toc *toc = gamma_buffer_dsm_toc();
	gamma_toc_lock_acquire_s(toc);
	cur_entry = gamma_toc_get_entry(toc, relid, rgid, attno);
#if 0
	elog(WARNING, "[sub ref release] entry: %d/%d/%d, refcount:%d",
			cur_entry->relid,
			cur_entry->rgid,
			cur_entry->attno,
			cur_entry->refcount);
#endif
	pg_atomic_fetch_sub_u32(&cur_entry->refcount, 1);
	gamma_toc_lock_release(toc);
	UnregisterResourceReleaseCallback(gamma_buffer_release_entry_callback, (void *)cur_entry);
}

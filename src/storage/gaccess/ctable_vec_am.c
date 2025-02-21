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



#include "storage/gamma_scankeys.h"
#include "storage/ctable_vec_am.h"


bool
vec_ctable_getnextslot(TableScanDesc scan, ScanDirection direction,
		TupleTableSlot * slot)
{
	CTableScanDesc cscan = (CTableScanDesc) scan;
	CVScanDesc cvscan = cscan->cvscan;
	TableScanDesc hscan = (TableScanDesc) cscan->hscan;

	if (cscan->scan_over)
	{
		ExecClearTuple(slot);
		return slot;
	}

	if (!cscan->heap)
	{
		bool fetch = true;
		if (gammadb_storage_skip_keys)
			cvscan->offset = gamma_skip_run_scankeys(cvscan, cvscan->rg, cvscan->offset);

		if (cvscan->offset >= cvscan->rg->dim)
		{
			cvscan->offset = 0;
			fetch = cvtable_loadnext_rg(cvscan, direction);
		}

		if (!fetch)
		{
			cscan->heap = true;
		}
		else
		{
			
			cvscan->offset += tts_vector_slot_from_rg(slot, cvscan->rg,
										cvscan->bms_proj, cvscan->offset);
			return true;
		}
	}

	/* begin scan delta table */
	Assert(cscan->heap);

	if (cscan->heap)
	{
		cscan->scan_over = tts_vector_slot_fill_tuple(hscan, direction, slot);
	}

	return true;
}

TableScanDesc 
vec_ctable_beginscan(Relation rel, Snapshot snapshot, int nkeys,
		struct ScanKeyData * key,
		ParallelTableScanDesc parallel_scan,
		uint32 flags) 
{
	CTableScanDesc scan;
	HeapScanDesc hscan;

	//RelationIncrementReferenceCount(rel);

#if PG_VERSION_NUM > 170000
	/* ANALYZE need to init read stream (in HeapScanDesc)*/
	if (flags & SO_TYPE_ANALYZE)
		flags = flags | SO_TYPE_SEQSCAN;
#endif

	scan = (CTableScanDesc) palloc0(sizeof(CTableScanDescData));
	scan->base.rs_rd = rel;
	scan->base.rs_snapshot = snapshot;
	scan->base.rs_nkeys = nkeys;
	scan->base.rs_key = key;
	scan->base.rs_flags = flags;
	scan->base.rs_parallel = parallel_scan;

	scan->hscan = hscan = (HeapScanDesc)heap_beginscan(rel, snapshot, nkeys,
												key, parallel_scan, flags);
	scan->cvscan = cvtable_beginscan(rel, snapshot, nkeys, key,
												parallel_scan, flags);

	scan->buf_slot = MakeTupleTableSlot(RelationGetDescr(rel),
										&TTSOpsBufferHeapTuple);
	scan->heap = false; /* cv table first */
	scan->scan_over = false;

	return (TableScanDesc) scan;
}

void
vec_ctable_end_scan(TableScanDesc scan)
{
	CTableScanDesc cscan = (CTableScanDesc)scan;

	ExecDropSingleTupleTableSlot(cscan->buf_slot);

	heap_endscan((TableScanDesc) cscan->hscan);
	cvtable_endscan(cscan->cvscan);

	pfree(cscan);
	
	return;
}

void
vec_ctable_rescan(TableScanDesc scan, struct ScanKeyData * key,
		bool set_params, bool allow_strat, bool allow_sync,
		bool allow_pagemode)
{
	CTableScanDesc cscan = (CTableScanDesc) scan;
	if (cscan->heap)
		heap_rescan((TableScanDesc) cscan->hscan, key, set_params,
									allow_strat, allow_sync, allow_pagemode);
	else
		cvtable_rescan(cscan->cvscan, key, set_params, allow_strat,
										allow_sync, allow_pagemode);

	return;
}


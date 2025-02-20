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

#include "access/bufmask.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/heapam_xlog.h"
#include "access/heaptoast.h"
#include "access/hio.h"
#include "access/multixact.h"
#include "access/parallel.h"
#include "access/relscan.h"
#include "access/subtrans.h"
#include "access/syncscan.h"
#include "access/sysattr.h"
#include "access/tableam.h"
#include "access/transam.h"
#include "access/valid.h"
#include "access/visibilitymap.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/xloginsert.h"
#include "access/xlogutils.h"
#include "catalog/catalog.h"
#include "commands/vacuum.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "port/atomics.h"
#include "port/pg_bitutils.h"
#include "storage/bufmgr.h"
#include "storage/freespace.h"
#include "storage/lmgr.h"
#include "storage/predicate.h"
#include "storage/procarray.h"
#include "storage/smgr.h"
#include "storage/spin.h"
#include "storage/standby.h"
#include "utils/datum.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/relcache.h"
#include "utils/snapmgr.h"
#include "utils/spccache.h"

#include "commands/gamma_vacuum.h"
#include "executor/gamma_merge.h"
#include "storage/ctable_dml.h"
#include "storage/gamma_cvtable_am.h"
#include "storage/gamma_meta.h"

double gammadb_delta_table_factor = 0.5;

void
ctable_insert(Relation relation, HeapTuple tup, CommandId cid,
			int options, BulkInsertState bistate)
{
	heap_insert(relation, tup, cid, options, bistate);
}

TM_Result
ctable_delete(Relation relation, ItemPointer tid,
			CommandId cid, Snapshot snapshot, Snapshot crosscheck, bool wait,
			TM_FailureData *tmfd, bool changingPart)
{
	TM_Result result;
	uint32 blkno = ItemPointerGetBlockNumber(tid);
	if (blkno > GAMMA_DELTA_TABLE_NBLOCKS)
	{
		result = cvtable_delete_tuple(relation, tid, cid, snapshot, crosscheck,
											wait, tmfd, changingPart);

	}
	else
	{
		result = heap_delete(relation, tid, cid, crosscheck,
											wait, tmfd, changingPart);
	}
	
	return result;
}

TM_Result
ctable_update(Relation relation, ItemPointer otid, HeapTuple newtup,
			CommandId cid, Snapshot snapshot, Snapshot crosscheck, bool wait,
			TM_FailureData *tmfd, LockTupleMode *lockmode)
{
	TM_Result result;
	result = ctable_delete(relation, otid, cid, snapshot,
						   crosscheck, wait, tmfd, false);
	//TODO:
	ctable_insert(relation, newtup, cid, 0, NULL);

	return result;
}

void
ctable_vacuum_rel(Relation rel, VacuumParams * params,
		BufferAccessStrategy bstrategy)
{
	BlockNumber nblocks;

	heap_vacuum_rel(rel, params, bstrategy);

	/*
	 * Use VACOPT_GAMMA_ANALYZE to distinguish manual-analyze and autoanalyze.
	 * If it is because autoanalyze enters here, we check whether autoanalyze
	 * needs to be done, and block the system's autoanalyze.
	 */
	if (!(params->options & VACOPT_GAMMA_ANALYZE) &&
			(
			 params->options & VACOPT_ANALYZE ||
			 gamma_autoanalyze_needed(RelationGetRelid(rel))
			 )
	   )
	{
		gamma_autoanalyze_rel(RelationGetRelid(rel), params, bstrategy);
		if (params->options & VACOPT_ANALYZE)
			params->options = params->options & (~VACOPT_ANALYZE);
	}

	/* the delta table need to truncate or clean */
	nblocks = RelationGetNumberOfBlocks(rel);
	if (nblocks < (GAMMA_DELTA_TABLE_NBLOCKS * gammadb_delta_table_factor))
	{
		return;
	}

	/* 
	 * Merge the data in the Delta table into the column vector part.
	 * The order of merge is from back to front in the delta table, so that
	 * the pages at the end of the delta table can be cleared and the number
	 * of pages in the tail of delta table can be truncated as early as
	 * possible.
	 */
	if (!ConditionalLockRelation(rel, AccessExclusiveLock))
	{
		return;
	}

	gamma_merge(rel);

	UnlockRelation(rel, AccessExclusiveLock);

	return;
}

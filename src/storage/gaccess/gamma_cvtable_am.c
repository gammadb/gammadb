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

#include "access/genam.h"
#include "access/relscan.h"
#include "access/heapam.h"
#include "access/tableam.h"
#include "catalog/indexing.h"
#include "executor/execdebug.h"
#include "executor/nodeSeqscan.h"
#include "fmgr.h"
#include "storage/bufmgr.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"

#include "storage/gamma_buffer.h"
#include "storage/gamma_cvtable_am.h"
#include "storage/gamma_meta.h"
#include "storage/gamma_rg.h"
#include "storage/gamma_scankeys.h"


static inline void
cvtable_release_ref_cv_list(CVScanDesc cvscan)
{
	if (unlikely(cvscan == NULL))
		return;

	if (cvscan->ref_cv_list != NULL)
	{
		ListCell *lc_cvr;
		foreach (lc_cvr, cvscan->ref_cv_list)
		{
			GammaBufferTag *tag = (GammaBufferTag *) lfirst(lc_cvr);
			gamma_buffer_release_cv(tag->relid, tag->rgid, tag->attno);
		}

		cvscan->ref_cv_list = NULL;
	}
}

CVScanDesc
cvtable_beginscan(Relation rel, Snapshot snapshot, int nkeys,
		struct ScanKeyData * key,
		ParallelTableScanDesc parallel_scan,
		uint32 flags)
{
	CVScanDesc cvscan;
	List *index_oid_list;
	Oid cv_index_oid = InvalidOid;
	Relation cv_index_rel;
	Oid cv_rel_oid = gamma_meta_get_cv_table_rel(rel);
	cvscan = (CVScanDesc) palloc0(sizeof(CVScanDescData));

	cvscan->cv_rel = table_open(cv_rel_oid, AccessShareLock);
	cvscan->base_rel = rel;

	index_oid_list = RelationGetIndexList(cvscan->cv_rel);
	Assert (list_lenght(index_oid_list) == 1);
	cv_index_oid = list_nth_oid(index_oid_list, 0);
	cv_index_rel = index_open(cv_index_oid, AccessShareLock);
	cvscan->cv_index_rel = cv_index_rel;

	cvscan->snapshot = snapshot;

	cvscan->cv_slot = MakeTupleTableSlot(RelationGetDescr(cvscan->cv_rel),
												 &TTSOpsBufferHeapTuple);

	cvscan->rg_context = AllocSetContextCreate(CurrentMemoryContext,
											"row group reset memory context",
											ALLOCSET_DEFAULT_SIZES); 
	cvscan->rg = gamma_rg_build(rel);
	cvscan->offset = 0;
	cvscan->ref_cv_list = NULL;

	if (parallel_scan)
	{
		cvscan->p_b = (ParallelBlockTableScanDesc)parallel_scan;
		cvscan->p_rg = (RowGroupCtableScanDesc)((char *)parallel_scan) +
										sizeof(ParallelBlockTableScanDescData);
	}
	else
	{
		cvscan->p_b = NULL;
		cvscan->p_rg = palloc0(sizeof(RowGroupCtableScanDescData));
		pg_atomic_init_u32(&cvscan->p_rg->cur_rg_id, 0);
		pg_atomic_init_u32(&cvscan->p_rg->max_rg_id, gamma_meta_max_rgid(rel));
	}

	return cvscan;
}

static bool
gamma_cvtable_make_minmax(CVScanDesc cvscan, Relation baserel, int16 attno,
		Datum datum_data, char *cstring_data)
{
	int32 len_data = 0;
	char *val_data = NULL;

	Form_pg_attribute base_attr = &RelationGetDescr(cvscan->base_rel)->attrs[attno - 1];

	val_data = VARDATA_ANY(DatumGetPointer(datum_data));
	len_data = VARSIZE_ANY_EXHDR(DatumGetPointer(datum_data));
	len_data = len_data > (GAMMA_MINMAX_LENGTH - 1) ?
		(GAMMA_MINMAX_LENGTH - 1) : len_data;

	if (base_attr->attlen > 0 && base_attr->attbyval)
	{
		memcpy(cstring_data, val_data, sizeof(Datum));
	}
	else if (base_attr->attlen > 0)
	{
		if (base_attr->attlen > GAMMA_MINMAX_LENGTH)
		{
			memcpy(cstring_data, val_data, GAMMA_MINMAX_LENGTH);
		}
		else
		{
			memcpy(cstring_data, val_data, base_attr->attlen);
		}
	}
	else
	{
		cstring_data[0] = len_data;
		memcpy(&cstring_data[1], val_data, len_data);
	}

	return true;
}

bool
cvtable_load_scankey_cv(CVScanDesc cvscan, uint32 rgid,
								AttrNumber attno, bool sk_check)
{
	static SysScanDesc sscan;
	static ScanKeyData key[2];
	HeapTuple	tuple;
	TupleDesc cv_desc = RelationGetDescr(cvscan->cv_rel);
	bool non_nulls = false;
	bool min_null = false;
	bool max_null = false;
	
	bool isnull = false;
	Datum datum_rows;
	Datum datum_data;
	Datum datum_nulls;
	Datum datum_min;
	Datum datum_max;

	text *text_data;
	text *text_nulls = NULL;

	gamma_buffer_cv read_buffer_cv;
	GammaBufferTag *tag;

	if (!gamma_buffer_get_cv(RelationGetRelid(cvscan->base_rel),
							rgid, attno, &read_buffer_cv))
	{
		char cstring_min[GAMMA_MINMAX_LENGTH];
		char cstring_max[GAMMA_MINMAX_LENGTH];

		ScanKeyInit(&key[0],
				Anum_gamma_rowgroup_rgid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(rgid));

		ScanKeyInit(&key[1],
				Anum_gamma_rowgroup_attno,
				BTGreaterEqualStrategyNumber, F_INT4EQ,
				Int32GetDatum(attno));

		sscan = systable_beginscan(cvscan->cv_rel,
								RelationGetRelid(cvscan->cv_index_rel), true,
								cvscan->snapshot, 2, key);

		tuple = systable_getnext(sscan);
		if (tuple == NULL)
		{
			systable_endscan(sscan);
			return false;
		}

		/* Extract values */
		datum_min = heap_getattr(tuple, Anum_gamma_rowgroup_min, cv_desc, &min_null);
		datum_max = heap_getattr(tuple, Anum_gamma_rowgroup_max, cv_desc, &max_null);
		datum_rows = heap_getattr(tuple, Anum_gamma_rowgroup_count, cv_desc, &isnull);
		datum_data = heap_getattr(tuple, Anum_gamma_rowgroup_values, cv_desc, &isnull);
		datum_nulls = heap_getattr(tuple, Anum_gamma_rowgroup_nulls, cv_desc, &non_nulls);

		read_buffer_cv.dim = DatumGetInt32(datum_rows);

		text_data = DatumGetTextPP(datum_data);  
		read_buffer_cv.values = text_to_cstring(text_data);
		read_buffer_cv.values_nbytes = VARSIZE_ANY_EXHDR(text_data);

		if (!non_nulls)
		{
			text_nulls = DatumGetTextPP(datum_nulls);
			read_buffer_cv.isnull = (bool *)text_to_cstring(text_nulls);
			read_buffer_cv.isnull_nbytes = DatumGetInt32(datum_rows);
		}
		else
		{
			read_buffer_cv.isnull = NULL;
			read_buffer_cv.isnull_nbytes = 0;
		}

		if (!min_null)
		{
			gamma_cvtable_make_minmax(cvscan, cvscan->base_rel, attno,
					datum_min, cstring_min);
			read_buffer_cv.min = cstring_min;
		}
		else
			read_buffer_cv.min = NULL;

		if (!max_null)
		{
			gamma_cvtable_make_minmax(cvscan, cvscan->base_rel, attno,
					datum_max, cstring_max);
			read_buffer_cv.max = cstring_max;
		}
		else
			read_buffer_cv.max = NULL;

		/* 
		 * TODO: now load all the CV into share buffers, it can only load the
		 * header of CV?
		 */
#if 0
		if (scankey != NULL && (!max_null || !min_null))
		{
			if (!gamma_sk_run_scankey(cvscan, scankey,
									read_buffer_cv.min, read_buffer_cv.max))
			{
				systable_endscan(sscan);
				return false;
			}

			sk_checked = true;
		}
#endif
		systable_endscan(sscan);

		if (gamma_buffer_add_cv(RelationGetRelid(cvscan->base_rel),
					rgid, attno, &read_buffer_cv))
		{
			if (read_buffer_cv.values != NULL)
				pfree(read_buffer_cv.values);

			if (!non_nulls && read_buffer_cv.isnull != NULL)
				pfree(read_buffer_cv.isnull);

			if (!gamma_buffer_get_cv(RelationGetRelid(cvscan->base_rel),
					rgid, attno, &read_buffer_cv))
			{
				elog(ERROR, "load CV: gamma shared buffers is not enough.");
			}
			else
			{
				/* pin the cv in buffer */
				gamma_buffer_register_cv(RelationGetRelid(cvscan->base_rel),
											rgid, attno);

				tag = (GammaBufferTag *) palloc(sizeof(GammaBufferTag));
				tag->relid = RelationGetRelid(cvscan->base_rel);
				tag->rgid = rgid;
				tag->attno = attno;
				cvscan->ref_cv_list = lappend(cvscan->ref_cv_list, tag);
			}
		}

		if ((void *)text_data != DatumGetPointer(datum_data))
			pfree(text_data);

		if (!non_nulls && (void *)text_nulls != DatumGetPointer(datum_nulls))
			pfree(text_nulls);
	}
	else
	{
		/* pin the cv in buffer */
		gamma_buffer_register_cv(RelationGetRelid(cvscan->base_rel), rgid, attno);

		tag = (GammaBufferTag *) palloc(sizeof(GammaBufferTag));
		tag->relid = RelationGetRelid(cvscan->base_rel);
		tag->rgid = rgid;
		tag->attno = attno;
		cvscan->ref_cv_list = lappend(cvscan->ref_cv_list, tag);
	}

	if (sk_check && (read_buffer_cv.max != NULL || read_buffer_cv.min != NULL))
	{
		if (!gamma_sk_attr_check(cvscan, attno, read_buffer_cv.min, read_buffer_cv.max))
			return false;
	}

	/* length of read_buffer_cv.isnull is same as read_buffer_cv.dim */
	gamma_cv_fill_data(&cvscan->rg->cvs[attno - 1],
			read_buffer_cv.values, read_buffer_cv.values_nbytes,
			read_buffer_cv.isnull, read_buffer_cv.dim);

	return true;
}

static bool
cvtable_load_cv(CVScanDesc cvscan, uint32 rgid, int16 attno)
{
	return cvtable_load_scankey_cv(cvscan, rgid, attno, false);
}

bool
cvtable_load_rg(CVScanDesc cvscan, uint32 rgid)
{
	int i = 0;
	bool first = true;
	int dim_attno = 0;
	MemoryContext old_context = NULL;
	TupleDesc base_desc = RelationGetDescr(cvscan->base_rel);

	/* release the CV reference in previous loading */
	if (cvscan->ref_cv_list != NULL)
		cvtable_release_ref_cv_list(cvscan);

	/* reset the per-rowgroup memory context */
	if (cvscan->rg_context != NULL)
	{
		MemoryContextResetOnly(cvscan->rg_context);
		old_context = MemoryContextSwitchTo(cvscan->rg_context);
	}

	/* precheck scankeys */
	if (cvscan->sk_count > 0)
	{
		if (!gamma_sk_run_scankeys(cvscan, rgid))
		{
			if (old_context != NULL)
				MemoryContextSwitchTo(old_context);

			return false;
		}
	}

	if (cvscan->bms_proj)
	{
		i = -1;
		while ((i = bms_next_member(cvscan->bms_proj, i)) >= 0)
		{
			int attno = i + FirstLowInvalidHeapAttributeNumber;

			/* be loaded when check scankey */
			if (cvscan->sk_preloaded && cvscan->sk_preloaded[attno])
				continue;

			if (!cvtable_load_cv(cvscan, rgid, attno))
			{
				if (old_context != NULL)
					MemoryContextSwitchTo(old_context);

				return false;
			}

			if (first)
			{
				dim_attno = attno - 1;
				first = false;
			}
		}
	}
	else
	{
		for (i = 0; i < base_desc->natts; i++)
		{
			/* be loaded when check scankey */
			if (cvscan->sk_preloaded && cvscan->sk_preloaded[i + 1])
				continue;

			if (!cvtable_load_cv(cvscan, rgid, i + 1))
			{
				if (old_context != NULL)
					MemoryContextSwitchTo(old_context);

				return false;
			}

			if (first)
			{
				dim_attno = i;
				first = false;
			}
		}
	}

	/* the dim of Row Group */
	cvscan->rg->dim = cvscan->rg->cvs[dim_attno].dim;
	cvscan->rg->rgid = rgid;

	if (old_context != NULL)
		MemoryContextSwitchTo(old_context);

	return true;
}

bool
cvtable_load_rowslot(CVScanDesc cvscan, uint32 rgid,
						int32 rowid, TupleTableSlot *slot)
{
	int i = 0;
	TupleDesc base_desc = RelationGetDescr(cvscan->base_rel);
	ColumnVector *cv;

	//TODO:process del bitmap
	//if (del[rowid])
	//	return false;

	if (cvscan->bms_proj)
	{
		i = -1;
		while ((i = bms_next_member(cvscan->bms_proj, i)) >= 0)
		{
			int attno = i + FirstLowInvalidHeapAttributeNumber;
			if (!cvtable_load_cv(cvscan, rgid, attno))
				return false;

			cv = &cvscan->rg->cvs[attno - 1];
			slot->tts_values[attno - 1] = cv->values[rowid - 1];
			if (CVIsNonNull(cv))
			{
				slot->tts_isnull[attno - 1] = false;
			}
			else
			{
				slot->tts_isnull[attno - 1] = cv->isnull[rowid - 1];
			}
		}
	}
	else
	{
		for (i = 0; i < base_desc->natts; i++)
		{
			if (!cvtable_load_cv(cvscan, rgid, i + 1))
				return false;

			cv = &cvscan->rg->cvs[i];
			slot->tts_values[i] = cv->values[rowid - 1];
			if (CVIsNonNull(cv))
			{
				slot->tts_isnull[i] = false;
			}
			else
			{
				slot->tts_isnull[i] = cv->isnull[rowid - 1];
			}

		}
	}

	ExecStoreVirtualTuple(slot);

	return true;
}

void
cvtable_load_delbitmap(CVScanDesc cvscan, uint32 rgid)
{
	HeapTuple delbitmap_tuple = cvtable_get_delbitmap_tuple(cvscan->cv_rel,
			cvscan->cv_index_rel->rd_id,
			cvscan->snapshot, rgid);

	if (delbitmap_tuple != NULL)
	{
		Datum datum;
		bool isnull;
		text *text_data;
		int32 data_len;
		bool *delbitmap;

		datum = heap_getattr(delbitmap_tuple, Anum_gamma_rowgroup_values,
				RelationGetDescr(cvscan->cv_rel), &isnull);
		text_data = DatumGetTextPP(datum);  
		data_len = VARSIZE_ANY_EXHDR(text_data);
		delbitmap = (bool *)VARDATA(text_data);

		memcpy(cvscan->rg->delbitmap, delbitmap, data_len);
		RGSetDelBitmap(cvscan->rg);
	}

	return;
}

bool
cvtable_loadnext_rg(CVScanDesc cvscan, ScanDirection direction)
{
	uint32 rgid = 0;
	bool result = false;
	bool backward = ScanDirectionIsBackward(direction);
	uint32 max_rg_id = 0;
	uint32 cur_rg_id = 0;

	if (ScanDirectionIsForward(direction))
	{
		if (!cvscan->inited)
		{
			max_rg_id = pg_atomic_read_u32(&cvscan->p_rg->max_rg_id);
			cur_rg_id = pg_atomic_read_u32(&cvscan->p_rg->cur_rg_id);
			if (max_rg_id == 1 || max_rg_id <= cur_rg_id)
			{
				return false;
			}
		}
			
		rgid = pg_atomic_add_fetch_u32(&cvscan->p_rg->cur_rg_id, 1);
	}
	else if (backward)
	{
		if (!cvscan->inited)
		{
			max_rg_id = pg_atomic_read_u32(&cvscan->p_rg->max_rg_id);
			pg_atomic_write_u32(&cvscan->p_rg->cur_rg_id, max_rg_id);
			if (max_rg_id == 1)
			{
				return false;
			}
		}

		rgid = pg_atomic_sub_fetch_u32(&cvscan->p_rg->cur_rg_id, 1);
	}
	else
	{
		if (!cvscan->inited)
		{
			max_rg_id = pg_atomic_read_u32(&cvscan->p_rg->max_rg_id);
			cur_rg_id = pg_atomic_read_u32(&cvscan->p_rg->cur_rg_id);
			if (max_rg_id == 1 || max_rg_id <= cur_rg_id)
			{
				return false;
			}
		}

		rgid = pg_atomic_add_fetch_u32(&cvscan->p_rg->cur_rg_id, 1);
	}

	while (!(result = cvtable_load_rg(cvscan, rgid)))
	{
		if (ScanDirectionIsForward(direction))
		{
			max_rg_id = pg_atomic_read_u32(&cvscan->p_rg->max_rg_id);
			cur_rg_id = pg_atomic_read_u32(&cvscan->p_rg->cur_rg_id);
			if (max_rg_id <= cur_rg_id)
			{
				break;
			}

			rgid = pg_atomic_add_fetch_u32(&cvscan->p_rg->cur_rg_id, 1);

			/* check again */
			if (max_rg_id <= rgid)
			{
				break;
			}
		}
		else if (backward)
		{
			max_rg_id = pg_atomic_read_u32(&cvscan->p_rg->max_rg_id);
			cur_rg_id = pg_atomic_read_u32(&cvscan->p_rg->cur_rg_id);
			if (cur_rg_id < 1)
			{
				break;
			}

			rgid = pg_atomic_sub_fetch_u32(&cvscan->p_rg->cur_rg_id, 1);

			/* check again */
			if (rgid < 1)
			{
				break;
			}
		}
		else
		{
			max_rg_id = pg_atomic_read_u32(&cvscan->p_rg->max_rg_id);
			cur_rg_id = pg_atomic_read_u32(&cvscan->p_rg->cur_rg_id);
			if (max_rg_id <= cur_rg_id)
			{
				break;
			}

			rgid = pg_atomic_add_fetch_u32(&cvscan->p_rg->cur_rg_id, 1);

			/* check again */
			if (max_rg_id <= rgid)
			{
				break;
			}
		}
	}

	if (result)
		cvtable_load_delbitmap(cvscan, rgid);

	return result;
}

bool
cvtable_getnextslot(CVScanDesc cvscan, ScanDirection direction,
		TupleTableSlot * slot)
{
	int32 i;
	uint32 rows = 0;
	uint32 rgid = 0;
	TupleDesc base_desc = RelationGetDescr(cvscan->base_rel);
	TupleTableSlot *cv_slot = cvscan->cv_slot;

	//TODO: process del bitmap

	for (i = 0; i < base_desc->natts; i++)
	{
		bool isnull = false;
		bool non_nulls = false;
		Datum datum_rows;
		Datum datum_data;
		Datum datum_nulls;
		Datum datum_rgid;

		text *text_data;
		uint32 data_len = 0;
		text *text_nulls;

		/*
		 * Get the tuples in the cv table one by one until all tuples
		 * are exhausted.
		 */
		if (!index_getnext_slot(cvscan->scan, ForwardScanDirection, cv_slot))
		{
			Assert(i == 0); /* first row */
			ExecClearTuple(cv_slot);

			return false;
		}

		/* Extract values */
		slot_getallattrs(cv_slot);
		datum_rgid = slot_getattr(cv_slot, Anum_gamma_rowgroup_rgid, &isnull);
		datum_rows = slot_getattr(cv_slot, Anum_gamma_rowgroup_count, &isnull);
		datum_data = slot_getattr(cv_slot, Anum_gamma_rowgroup_values, &isnull);
		datum_nulls = slot_getattr(cv_slot, Anum_gamma_rowgroup_nulls, &non_nulls);

		/* TODO: check if text_data/text_nulls should be freed */
		rgid = DatumGetObjectId(datum_rgid);
		rows = DatumGetInt32(datum_rows);
		text_data = DatumGetTextPP(datum_data);  
		data_len = VARSIZE_ANY_EXHDR(text_data);
		if (!non_nulls)
		{
			text_nulls = DatumGetTextPP(datum_nulls);
			gamma_cv_fill_data(&cvscan->rg->cvs[i], text_to_cstring(text_data),
					data_len, (bool *)text_to_cstring(text_nulls), rows);
		}
		else
		{
			gamma_cv_fill_data(&cvscan->rg->cvs[i], text_to_cstring(text_data),
					data_len, (bool *)NULL, rows);
		}
	}

	/* the dim of Row Group */
	cvscan->rg->dim = rows;
	cvscan->rg->rgid = rgid;

	return true;
}

void
cvtable_rescan(CVScanDesc scan, struct ScanKeyData * key,
		bool set_params, bool allow_strat, bool allow_sync,
		bool allow_pagemode)
{
	if (scan->ref_cv_list != NULL)
		cvtable_release_ref_cv_list(scan);

	return;
}

void
cvtable_endscan(CVScanDesc cvscan)
{
	if (cvscan->cv_slot != NULL)
		ExecDropSingleTupleTableSlot(cvscan->cv_slot);

	if (cvscan->ref_cv_list != NULL)
		cvtable_release_ref_cv_list(cvscan);

	if (cvscan->rg)
		gamma_rg_free(cvscan->rg);

	if (cvscan->scan)
		index_endscan(cvscan->scan);

	if (cvscan->cv_index_rel)
		index_close(cvscan->cv_index_rel, NoLock);

	if (cvscan->cv_rel)
		table_close(cvscan->cv_rel, NoLock);
}

TM_Result
cvtable_delete_tuple(Relation relation, ItemPointer tid,
			CommandId cid, Snapshot snapshot, Snapshot crosscheck, bool wait,
			TM_FailureData *tmfd, bool changingPart)
{
	HeapTuple delbitmap_tuple;
	List *index_oid_list;
	Oid cv_index_oid = InvalidOid;
	bool *delbitmap;
	MemoryContext del_context;
	MemoryContext old_context;

	uint32 rgid = gamma_meta_ptid_get_rgid(tid);
	int32 rowid = gamma_meta_ptid_get_rowid(tid);

	Oid cv_rel_oid = gamma_meta_get_cv_table_rel(relation);

	Relation cv_rel = table_open(cv_rel_oid, RowExclusiveLock);

	/*
	 * Repeated consecutive DELETE/UPDATE of the same transaction will set
	 * multiple delete bitmaps. During this period, the memory blocks are
	 * relatively large, causing a sharp increase in memory usage.
	 * By setting an independent Memory Context, memory can be regularly cleared.
	 */
	del_context = AllocSetContextCreate(TopMemoryContext,
										"Gamma Cvtable Delete",
										ALLOCSET_DEFAULT_SIZES);

	old_context = MemoryContextSwitchTo(del_context);
	index_oid_list = RelationGetIndexList(cv_rel);
	Assert (list_lenght(index_oid_list) == 1);
	cv_index_oid = list_nth_oid(index_oid_list, 0);

	delbitmap_tuple = cvtable_get_delbitmap_tuple(cv_rel, cv_index_oid,
												  snapshot, rgid);


	if (delbitmap_tuple == NULL)
	{
		/* insert new delete bitmap tuple */
		delbitmap = (bool *)palloc0(sizeof(bool) * GAMMA_COLUMN_VECTOR_SIZE);

		/* rowid start with 1 */
		delbitmap[rowid - 1] = true;
		gamma_meta_insert_delbitmap(cv_rel, rgid, delbitmap,
									GAMMA_COLUMN_VECTOR_SIZE);

		pfree(delbitmap);
	}
	else
	{
		/* update delete bitmap tuple */
		HeapTuple tuple = delbitmap_tuple;
		Datum values[Natts_gamma_rowgroup];
		bool nulls[Natts_gamma_rowgroup];
		bool replace[Natts_gamma_rowgroup];
		Datum datum;
		bool isnull;
		text *text_data;
		text *new_text_data;
		int32 data_len;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));
		memset(replace, false, sizeof(replace));

		datum = heap_getattr(tuple, Anum_gamma_rowgroup_values,
									RelationGetDescr(cv_rel), &isnull);
		text_data = DatumGetTextPP(datum);  
		data_len = VARSIZE_ANY_EXHDR(text_data);
		delbitmap = (bool *)VARDATA(text_data);
	
		Assert(datalen >= rowid);

		delbitmap[rowid - 1] = true;

		new_text_data = cstring_to_text_with_len((char*)delbitmap, data_len);
		values[Anum_gamma_rowgroup_values - 1] = PointerGetDatum(new_text_data);
		replace[Anum_gamma_rowgroup_values - 1] = true;

		tuple = heap_modify_tuple(tuple, RelationGetDescr(cv_rel),
									values, nulls, replace);
		CatalogTupleUpdate(cv_rel, &delbitmap_tuple->t_self, tuple);

		if ((void *)text_data == DatumGetPointer(datum))
			pfree(text_data);

		pfree(new_text_data);

		heap_freetuple(delbitmap_tuple);
	}


	table_close(cv_rel, RowExclusiveLock);

	/* 
	 * When multiple rows are deleted in the same transaction, the delete
	 * bitmap is updated multiple times. It is necessary to ensure that the
	 * latest delete bitmap is obtained each time a read operation is
	 * performed in this transaction, so Command++.
	 */
	CommandCounterIncrement();

	MemoryContextSwitchTo(old_context);

	MemoryContextDelete(del_context);
	return TM_Ok;
}

HeapTuple
cvtable_get_delbitmap_tuple(Relation cvrel, Oid indexoid,
											Snapshot snapshot, Oid rgid)
{
	HeapTuple tuple ;
	ScanKeyData scankey[2];
	SysScanDesc delbitmapscan;

	/* Set up a scan key to fetch from the index. */
	ScanKeyInit(&scankey[0],
				(AttrNumber) 1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(rgid));

	ScanKeyInit(&scankey[1],
			(AttrNumber) 2,
			BTEqualStrategyNumber, F_INT4EQ,
			Int32GetDatum(GammaDelBitmapAttributeNumber));

	/*
	 * Here, "transaction MVCC snapshot" is used, which combines "command++"
	 * to ensure the visibility of new tuples when updated multiple times
	 * in same transaction.
	 *
	 * GAMMA_NOTE: The delete snapshot is different from the transaction snapshot?
	 */
	delbitmapscan = systable_beginscan(cvrel, indexoid, true,
										   GetTransactionSnapshot(),
										   2, scankey);

	tuple = systable_getnext(delbitmapscan);
	if (tuple != NULL)
		tuple = heap_copytuple(tuple);

	systable_endscan(delbitmapscan);

	return tuple;
}

uint64
cvtable_get_rows(Relation cvrel)
{
	HeapTuple tuple ;
	ScanKeyData scankey[1];
	SysScanDesc sscan;
	uint64 rows = 0;
	Datum datum_rows;
	bool isnull;
	TupleDesc cv_desc = RelationGetDescr(cvrel);

	ScanKeyInit(&scankey[0],
				Anum_gamma_rowgroup_attno,
				BTEqualStrategyNumber, F_INT4EQ,
				Int32GetDatum(1));

	sscan = systable_beginscan(cvrel, InvalidOid, false,
										   GetTransactionSnapshot(),
										   1, scankey);

	while ((tuple = systable_getnext(sscan)) != NULL)
	{
		datum_rows = heap_getattr(tuple, Anum_gamma_rowgroup_count, cv_desc, &isnull);
		rows += DatumGetInt32(datum_rows);
	}

	systable_endscan(sscan);
	return rows;
}

void
cvtable_update_delete_bitmap(Relation relation, Snapshot snapshot, uint32 rgid,
								bool *vacuum_delbitmap, int count)
{
	HeapTuple delbitmap_tuple;
	List *index_oid_list;
	Oid rg_index_oid = InvalidOid;
	bool *delbitmap;

	index_oid_list = RelationGetIndexList(relation);
	Assert (list_lenght(index_oid_list) == 1);
	rg_index_oid = list_nth_oid(index_oid_list, 0);

	delbitmap_tuple = cvtable_get_delbitmap_tuple(relation, rg_index_oid,
												  snapshot, rgid);

	if (delbitmap_tuple == NULL)
	{
		gamma_meta_insert_delbitmap(relation, rgid, vacuum_delbitmap,
									GAMMA_COLUMN_VECTOR_SIZE);

	}
	else
	{
		/* update delete bitmap tuple */
		int i;
		HeapTuple tuple = delbitmap_tuple;
		Datum values[Natts_gamma_rowgroup];
		bool nulls[Natts_gamma_rowgroup];
		bool replace[Natts_gamma_rowgroup];
		Datum datum;
		bool isnull;
		text *text_data;
		int32 data_len;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));
		memset(replace, false, sizeof(replace));

		datum = heap_getattr(tuple, Anum_gamma_rowgroup_values,
									RelationGetDescr(relation), &isnull);
		text_data = DatumGetTextPP(datum);  
		data_len = VARSIZE_ANY_EXHDR(text_data);
		delbitmap = (bool *)VARDATA(text_data);
	
		Assert(datalen >= rowid);

		for (i = 0; i < count; i++)
		{
			if (vacuum_delbitmap[i])
				delbitmap[i] = vacuum_delbitmap[i];
		}

		text_data = cstring_to_text_with_len((char*)delbitmap, data_len);
		values[Anum_gamma_rowgroup_values - 1] = PointerGetDatum(text_data);
		replace[Anum_gamma_rowgroup_values - 1] = true;

		tuple = heap_modify_tuple(tuple, RelationGetDescr(relation),
									values, nulls, replace);
		CatalogTupleUpdate(relation, &delbitmap_tuple->t_self, tuple);

		heap_freetuple(delbitmap_tuple);
	}

	return;
}

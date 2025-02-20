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

#include "access/heapam.h"
#include "access/tableam.h"
#include "access/sysattr.h"
#include "executor/tuptable.h"
#include "storage/bufmgr.h"
#include "utils/expandeddatum.h"
#include "utils/datum.h"
#include "utils/rel.h"

#include "storage/gamma_meta.h"
#include "executor/vector_tuple_slot.h"
#include "utils/utils.h"

static void tts_vector_slot_init(TupleTableSlot *slot);
static void tts_vector_slot_clear(TupleTableSlot *slot);
static void tts_vector_slot_copy(TupleTableSlot *dstslot,
								 TupleTableSlot *srcslot);

TupleTableSlotOps TTSOpsVector;

static void
tts_vector_slot_init(TupleTableSlot *slot)
{
	int i;
	TupleDesc desc = slot->tts_tupleDescriptor;
	VectorTupleSlot *vslot = (VectorTupleSlot *)slot;

	/* vectorized initialize */
	vslot->dim = 0;
	vslot->row_indexarr = NULL;
	memset(vslot->skip, true, sizeof(vslot->skip));
	
	/* initailize column in vector slot */
	for (i = 0; i < desc->natts; i++)
	{
		Oid typid = desc->attrs[i].atttypid;
		vdatum *column = buildvdatum(typid, VECTOR_SIZE, vslot->skip);
		column->dim = 0;
		slot->tts_values[i]  = PointerGetDatum(column);
		slot->tts_isnull[i] = false;
	}

	return;
}

static void
tts_vector_slot_clear(TupleTableSlot *slot)	
{
	VectorTupleSlot *vecslot = (VectorTupleSlot *)slot;
	VirtualTupleTableSlot *vslot = (VirtualTupleTableSlot *) slot;

	if (unlikely(TTS_SHOULDFREE(slot)))
	{
		pfree(vslot->data);
		vslot->data = NULL;

		slot->tts_flags &= ~TTS_FLAG_SHOULDFREE;
	}

	slot->tts_nvalid = 0;
	slot->tts_flags |= TTS_FLAG_EMPTY;
	ItemPointerSetInvalid(&slot->tts_tid);

	vecslot->dim = 0;
	vecslot->row_indexarr = NULL;
	memset(vecslot->skip, true, sizeof(vecslot->skip));

	return;
}

static void
tts_vector_slot_copy(TupleTableSlot *dstslot, TupleTableSlot *srcslot)
{
	VectorTupleSlot *vdstslot; 
	VectorTupleSlot *vsrcslot;
	TupleDesc srcdesc = dstslot->tts_tupleDescriptor;

	TTSOpsVirtual.copyslot(dstslot, srcslot);
	
	Assert (dstslot->tts_ops == srcslot->tts_ops &&
			dstslot->tts_ops == &TTSOpsVector);

	vdstslot = (VectorTupleSlot *) dstslot;
	vsrcslot = (VectorTupleSlot *) srcslot;

	vdstslot->dim = vsrcslot->dim;
	vdstslot->row_indexarr = vsrcslot->row_indexarr;
	memcpy(vdstslot->skip, vsrcslot->skip, sizeof(bool) * VECTOR_SIZE);
	if (VSlotHasNonSkip(vsrcslot))
			VSlotSetNonSkip(vdstslot);

	/* deep copy */
	for (int natt = 0; natt < srcdesc->natts; natt++)
	{
		if (is_vec_type(srcdesc->attrs[natt].atttypid))
		{
			dstslot->tts_values[natt] = PointerGetDatum(
										copyvdatum((vdatum *)srcslot->tts_values[natt],
										vdstslot->skip));
		}
		else
		{
			dstslot->tts_values[natt] = datumCopy(srcslot->tts_values[natt],
												  srcdesc->attrs[natt].attbyval,
												  srcdesc->attrs[natt].attlen);
			dstslot->tts_isnull[natt] = srcslot->tts_isnull[natt];
		}
	}

	return;
}

void
ttsops_vector_init(void)
{
	TTSOpsVector = TTSOpsVirtual;

	TTSOpsVector.base_slot_size = sizeof(VectorTupleSlot);
	TTSOpsVector.init = tts_vector_slot_init;
	TTSOpsVector.clear = tts_vector_slot_clear;
	TTSOpsVector.copyslot = tts_vector_slot_copy;
	
	return;
}

const TupleTableSlotOps *
ttsops_vector_slot_callbacks(Relation relation)
{
	return &TTSOpsVector;
}

/*
 * we don't want set the pin_buffers/pin_tuples in the slot, so call 
 * tts_vector_slot_deform_tuple directly, it is not a callback function.
 */
static void
tts_vector_slot_deform_tuple(TupleTableSlot *slot, int natts,
			HeapTupleData *pin_tuples, Buffer *pin_buffers)
{
	VectorTupleSlot	*vslot = (VectorTupleSlot *)slot;
	TupleDesc	tupledesc = slot->tts_tupleDescriptor;
	HeapTuple	tuple;
	HeapTupleHeader tup;
	bool		hasnulls;
	//Form_pg_attribute *att = &(tupledesc->attrs);
	int			attnum;
	char	   *tp;				/* ptr to tuple data */
	long		off;			/* offset in tuple data */
	bits8	   *bp;		/* ptr to null bitmap in tuple */
	bool		slow;			/* can we use/set attcacheoff? */
	int			row;
	vdatum		*column;

	for (row = 0; row < vslot->dim; row++)
	{
		tuple = &pin_tuples[row];
		tup = tuple->t_data;
		bp = tup->t_bits;
		hasnulls = HeapTupleHasNulls(tuple);

		attnum = slot->tts_nvalid;
		/*
		 * Check whether the first call for this tuple, and initialize or restore
		 * loop state.
		 */
		/* vectorize engine deform once for now */
		off = 0;
		slow = false;

		tp = (char *) tup + tup->t_hoff;

		for (; attnum < natts; attnum++)
		{
			Form_pg_attribute thisatt = TupleDescAttr(tupledesc, attnum);
			//Form_pg_attribute thisatt = att[attnum];
			column = (vdatum *)slot->tts_values[attnum];
			column->ref = false;

			if (hasnulls && att_isnull(attnum, bp))
			{
				VDATUM_SET_DATUM(column, row, ((Datum)0));
				VDATUM_SET_ISNULL(column, row, true);
				slow = true;		/* can't use attcacheoff anymore */
				continue;
			}

			VDATUM_SET_ISNULL(column, row, false);

			if (!slow && thisatt->attcacheoff >= 0)
				off = thisatt->attcacheoff;
			else if (thisatt->attlen == -1)
			{
				/*
				 * We can only cache the offset for a varlena attribute if the
				 * offset is already suitably aligned, so that there would be no
				 * pad bytes in any case: then the offset will be valid for either
				 * an aligned or unaligned value.
				 */
				if (!slow &&
						off == att_align_nominal(off, thisatt->attalign))
					thisatt->attcacheoff = off;
				else
				{
					off = att_align_pointer(off, thisatt->attalign, -1,
							tp + off);
					slow = true;
				}
			}
			else
			{
				/* not varlena, so safe to use att_align_nominal */
				off = att_align_nominal(off, thisatt->attalign);

				if (!slow)
					thisatt->attcacheoff = off;
			}

			VDATUM_SET_DATUM(column, row, fetchatt(thisatt, tp + off));

			off = att_addlength_pointer(off, thisatt->attlen, tp + off);

			if (thisatt->attlen <= 0)
				slow = true;		/* can't use attcacheoff anymore */
		}
	}


	attnum = slot->tts_nvalid;
	for (; attnum < natts; attnum++)
	{
		column = (vdatum *)slot->tts_values[attnum];
		column->dim = vslot->dim;
	}

	/*
	 * Save state for next execution
	 */
	slot->tts_nvalid = attnum;
}


/*
 * we don't want set the pin_buffers/pin_tuples in the slot, so call 
 * tts_vector_slot_getallattrs directly, it is not a callback function.
 */
void
tts_vector_slot_getallattrs(TupleTableSlot *slot, HeapTupleData *pin_tuples,
							Buffer *pin_buffers)
{
	VectorTupleSlot	*vslot = (VectorTupleSlot *)slot;
	int			tdesc_natts = slot->tts_tupleDescriptor->natts;
	int			attnum;
	HeapTuple	tuple;
	int			i;

	/* Quick out if we have 'em all already */
	if (slot->tts_nvalid == tdesc_natts)
		return;

	if (vslot->dim == 0)
		return;

	//TODO: zsj Assert?
	for (i = 0; i < vslot->dim; i++)
	{
		tuple = &pin_tuples[i];
		if (tuple == NULL)			/* internal error */
			elog(ERROR, "cannot extract attribute from empty tuple slot");
	}

	/*
	 * load up any slots available from physical tuple
	 */
	attnum = HeapTupleHeaderGetNatts(pin_tuples[0].t_data);
	attnum = Min(attnum, tdesc_natts);

	tts_vector_slot_deform_tuple(slot, attnum, pin_tuples, pin_buffers);

	/*
	 * If tuple doesn't have all the atts indicated by tupledesc, read the
	 * rest as null
	 */
	for (; attnum < tdesc_natts; attnum++)
	{
		slot->tts_values[attnum] = (Datum) 0;
		slot->tts_isnull[attnum] = true;
	}

	slot->tts_nvalid = tdesc_natts;
}

uint32
tts_vector_slot_from_rg(TupleTableSlot *slot, RowGroup *rg,
						Bitmapset *bms_proj, uint32 offset)
{
	VectorTupleSlot	*vslot = (VectorTupleSlot *)slot;
	TupleDesc tupledesc = slot->tts_tupleDescriptor;
	uint16 natts = tupledesc->natts;
	int attnum;
	vdatum *column;
	ColumnVector *cv;
	uint32 count;

	count = rg->dim - offset > VECTOR_SIZE ? VECTOR_SIZE : rg->dim - offset;

	if (bms_proj != NULL)
	{
		attnum = -1;
		while ((attnum = bms_next_member(bms_proj, attnum)) >= 0)
		{
			int attno = attnum + FirstLowInvalidHeapAttributeNumber;
			attno -= 1;
			column = (vdatum *)slot->tts_values[attno];
			cv = &rg->cvs[attno];

			column->ref = true;
			column->ref_values = &cv->values[offset];
			column->ref_isnull = CVIsNonNull(cv) ? NULL : &cv->isnull[offset];
			column->dim = count;
		}
	}
	else
	{
		for (attnum = 0; attnum < natts; attnum++)
		{
			column = (vdatum *)slot->tts_values[attnum];
			cv = &rg->cvs[attnum];

			column->ref = true;
			column->ref_values = &cv->values[offset];
			column->ref_isnull = CVIsNonNull(cv) ? NULL : &cv->isnull[offset];
			column->dim = count;
		}
	}

	slot->tts_nvalid = natts;
	vslot->dim = count;

	if (RGHasDelBitmap(rg))
	{
		memcpy(vslot->skip, &rg->delbitmap[offset], sizeof(bool) * count);
	}
	else
	{
		VSlotSetNonSkip(vslot);
		memset(vslot->skip, false, sizeof(bool) * count);
	}

	ExecStoreVirtualTuple(slot);

	return count;
}

uint32
tts_slot_from_rg(TupleTableSlot *slot, RowGroup *rg,
					Bitmapset *bms_proj, uint32 offset)
{
	TupleDesc tupledesc = slot->tts_tupleDescriptor;
	uint16 natts = tupledesc->natts;
	int attnum;
	ColumnVector *cv;
	int count = 0;

	if (bms_proj != NULL)
	{
		attnum = -1;
		while ((attnum = bms_next_member(bms_proj, attnum)) >= 0)
		{
			int attno = attnum + FirstLowInvalidHeapAttributeNumber;
			attno -= 1;
			cv = &rg->cvs[attno];
			slot->tts_values[attno] = cv->values[offset];
			slot->tts_isnull[attno] = CVIsNonNull(cv) ? false : cv->isnull[offset];
		}
	}
	else
	{
		for (attnum = 0; attnum < natts; attnum++)
		{
			cv = &rg->cvs[attnum];
			slot->tts_values[attnum] = cv->values[offset];
			slot->tts_isnull[attnum] = CVIsNonNull(cv) ? false : cv->isnull[offset];
		}
	}

	offset++;
	count++;

	slot->tts_nvalid = natts;
	slot->tts_tid = gamma_meta_cv_convert_tid(rg->rgid, offset);

	ExecStoreVirtualTuple(slot);

	return count;
}

/*
 * copy values/nulls from Buffer Tuple Slot to Virtual Slot
 */
void
tts_slot_copy_values(TupleTableSlot *slot, TupleTableSlot *src_slot)
{
	TupleDesc tupledesc = slot->tts_tupleDescriptor;
	uint16 natts = tupledesc->natts;
	int attnum;

	for (attnum = 0; attnum < natts; attnum++)
	{
		slot->tts_values[attnum] = src_slot->tts_values[attnum];
		slot->tts_isnull[attnum] = src_slot->tts_isnull[attnum];
	}

	slot->tts_nvalid = natts;

	ExecStoreVirtualTuple(slot);

	return;
}

/*
 * copy vector values/nulls from vector slot to another vector slot
 */
void
tts_vector_slot_copy_values(TupleTableSlot *slot, TupleTableSlot *src_slot)
{
	VectorTupleSlot *vslot = (VectorTupleSlot *)slot;
	VectorTupleSlot *vsrc_slot = (VectorTupleSlot *)src_slot;

	TupleDesc tupledesc = slot->tts_tupleDescriptor;
	uint16 natts = tupledesc->natts;
	int attnum;
	
	for (attnum = 0; attnum < natts; attnum++)
	{
		slot->tts_values[attnum] = src_slot->tts_values[attnum];
		slot->tts_isnull[attnum] = src_slot->tts_isnull[attnum];
	}

	slot->tts_nvalid = natts;
	vslot->dim = vsrc_slot->dim;

	ExecStoreVirtualTuple(slot);

	return;
}

bool
tts_vector_slot_fill_tuple(TableScanDesc scandesc, ScanDirection direction,
							TupleTableSlot *slot)
{
	bool scan_over = false;
	HeapTuple tuple;
	TupleDesc tupledesc;

	VectorTupleSlot	*vslot;
	TupleTableSlot *scanslot;
	int row;

	Buffer prev_buf = InvalidBuffer;
	HeapTupleData pin_tuples[VECTOR_SIZE];
	Buffer pin_buffers[VECTOR_SIZE] = {0};

	vslot = (VectorTupleSlot *)slot;

	ExecClearTuple(slot);

	/* use to scan heap rows */
	tupledesc = RelationGetDescr(scandesc->rs_rd);
	scanslot = MakeSingleTupleTableSlot(tupledesc, &TTSOpsBufferHeapTuple);

	/* fetch a batch of rows and fill them into VectorTupleSlot */
	for (row = 0 ; row < VECTOR_SIZE; row++)
	{
		if (heap_getnextslot(scandesc, direction, scanslot))
		{
			bool shouldFree;
			HeapScanDesc hscandesc = (HeapScanDesc)scandesc;
			Buffer buffer = hscandesc->rs_cbuf;

			tuple = ExecFetchSlotHeapTuple(scanslot, false, &shouldFree);

			memcpy(&pin_tuples[row], tuple, sizeof(HeapTupleData));

			if (BufferIsValid(buffer) &&
				(row == 0 || prev_buf != buffer))
			{
				prev_buf = buffer;
				if (BufferIsValid(pin_buffers[row]))
					ReleaseBuffer(pin_buffers[row]);
				pin_buffers[row] = buffer;
				IncrBufferRefCount(buffer);
			}

			if (shouldFree)
				heap_freetuple(tuple);
		}
		else
		{
			/* scan finish, but we still need to emit current vslot */
			scan_over = true;
			break;
		}
	}

	if (row > 0)
	{
		int i = 0;
		vslot->dim = row;
		memset(vslot->skip, false, sizeof(bool) * row);
		
		/* deform the vector slot now */
		tts_vector_slot_getallattrs(slot, pin_tuples, pin_buffers);
		ExecStoreVirtualTuple(slot);

		for(i = 0; i < row; i++)
		{
			if(BufferIsValid(pin_buffers[i]))
			{
				ReleaseBuffer(pin_buffers[i]);
				pin_buffers[i] = InvalidBuffer;
			}
		}
	}

	ExecDropSingleTupleTableSlot(scanslot);
	return scan_over;
}

/*
 * copy vector values/nulls from vector slot to a row scan slot
 */
void
tts_vector_slot_copy_one_row(TupleTableSlot *slot,
									TupleTableSlot *src_slot, int row)
{
	//VectorTupleSlot *vsrc_slot = (VectorTupleSlot *)src_slot;

	TupleDesc tupledesc = slot->tts_tupleDescriptor;
	TupleDesc srcdesc = src_slot->tts_tupleDescriptor;
	uint16 natts = tupledesc->natts;
	int attnum;
	
	for (attnum = 0; attnum < natts; attnum++)
	{
		if (is_vec_type(srcdesc->attrs[attnum].atttypid))
		{
			vdatum *vec_value = (vdatum *) src_slot->tts_values[attnum];
			slot->tts_values[attnum] = VDATUM_DATUM(vec_value, row);
			slot->tts_isnull[attnum] = VDATUM_ISNULL(vec_value, row);
		}
		else
		{
			slot->tts_values[attnum] = datumCopy(src_slot->tts_values[attnum],
												  srcdesc->attrs[attnum].attbyval,
												  srcdesc->attrs[attnum].attlen);
			slot->tts_isnull[attnum] = src_slot->tts_isnull[attnum];
		}
	}

	slot->tts_nvalid = natts;
	ExecStoreVirtualTuple(slot);

	return;
}

void
tts_vector_slot_fill_vector(TupleTableSlot *slot, TupleTableSlot *src_slot, int row)
{
	VectorTupleSlot	*vslot = (VectorTupleSlot *) slot;
	TupleDesc tupledesc = src_slot->tts_tupleDescriptor;
	TupleDesc destdesc = slot->tts_tupleDescriptor;
	uint16 natts = tupledesc->natts;
	int attnum;

	vslot->dim = vslot->dim > (row + 1) ? vslot->dim : (row + 1);
	
	for (attnum = 0; attnum < natts; attnum++)
	{
		if (is_vec_type(destdesc->attrs[attnum].atttypid))
		{
			vdatum *vec_value = (vdatum *) slot->tts_values[attnum];
			VDATUM_SET_DATUM(vec_value, row, (src_slot->tts_values[attnum]));
			VDATUM_SET_ISNULL(vec_value, row, (src_slot->tts_isnull[attnum]));
			vec_value->dim = vslot->dim;
		}
		else
		{
			slot->tts_values[attnum] = datumCopy(src_slot->tts_values[attnum],
												 tupledesc->attrs[attnum].attbyval,
												 tupledesc->attrs[attnum].attlen);
			slot->tts_isnull[attnum] = src_slot->tts_isnull[attnum];
		}
	}

	vslot->skip[row] = false;

	return;
}

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

#include "datatype/timestamp.h"
#include "catalog/pg_type.h"
#include "utils/date.h"
#include "utils/lsyscache.h"

#include "executor/gamma_vec_tablescan.h"
#include "storage/gamma_scankeys.h"
#include "storage/gamma_toc.h"

bool gammadb_storage_skip_keys = true;

static GammaSKStrategy
gamma_sk_strategy(OpExpr *op_expr)
{
	char *op = get_opname(op_expr->opno);

	if (pg_strcasecmp(op, ">") == 0)
		return GammaSKGreater;
	else if (pg_strcasecmp(op, ">=") == 0)
		return GammaSKGreaterEqual;
	else if (pg_strcasecmp(op, ">=") == 0)
		return GammaSKGreaterEqual;
	else if (pg_strcasecmp(op, "=") == 0)
		return GammaSKEqual;
	else if (pg_strcasecmp(op, "<=") == 0)
		return GammaSKLessEqual;
	else if (pg_strcasecmp(op, "<") == 0)
		return GammaSKLess;
	else if (pg_strcasecmp(op, "<>") == 0)
		return GammaSKNotEqual;
	else
		return GammaSKNone;
}

static GammaSKStrategy
gamma_sk_commute(GammaSKStrategy sk_strategy)
{
	if (sk_strategy == GammaSKLess)
		return GammaSKGreater;
	else if (sk_strategy == GammaSKLessEqual)
		return GammaSKGreaterEqual;
	else if (sk_strategy == GammaSKGreaterEqual)
		return GammaSKLessEqual;
	else if (sk_strategy == GammaSKGreater)
		return GammaSKLess;

	return sk_strategy;
}
static bool
gamma_sk_is_scankey(OpExpr *op_expr)
{
	Node *left;
	Node *right;

	if (!IsA(op_expr, OpExpr))
		return false;

	if (list_length(op_expr->args) != 2)
		return false;

	left = (Node *) linitial(op_expr->args);
	right = (Node *) lsecond(op_expr->args);
	if (IsA(left, Var) && IsA(right, Const))
	{}
	else if (IsA(left, Const) && IsA(right, Var))
	{}
	else
		return false;
	
	return true;
}

void
gamma_sk_init_scankeys(SeqScanState *scanstate, SeqScan *node)
{
	ListCell *lc;
	OpExpr *op_expr = NULL;
	GammaScanKey sk;
	GammaSkipKey skip;
	VecSeqScanState *vscanstate = (VecSeqScanState *) scanstate;
	uint16 sk_count;
	uint16 skip_count;
	vscanstate->scankeys = NULL;
	vscanstate->sk_count = 0;

	if (((Plan *)node)->qual == NULL)
		return;

	if (!IsA(((Plan *)node)->qual, List))
		return;

	sk_count = list_length(((Plan *)node)->qual);
	sk = (GammaScanKey) palloc0(sizeof(GammaScanKeyData) * sk_count);

	skip_count = list_length(((Plan *)node)->qual);
	skip = (GammaSkipKey) palloc0(sizeof(GammaSkipKeyData) * skip_count);

	sk_count = 0;
	skip_count = 0;
	foreach (lc, ((Plan *)node)->qual)
	{
		Var *var;
		Const *con;
		GammaSKStrategy sk_strategy;
		op_expr = (OpExpr *) lfirst(lc);
		if (!gamma_sk_is_scankey(op_expr))
			continue;

		sk_strategy = gamma_sk_strategy(op_expr);
		if (sk_strategy == GammaSKNone)
			continue;

		if (IsA(linitial(op_expr->args), Var))
		{
			var = (Var *) linitial(op_expr->args);
			con = (Const *) lsecond(op_expr->args);
		}
		else
		{
			var = (Var *) lsecond(op_expr->args);
			con = (Const *) linitial(op_expr->args);

			sk_strategy = gamma_sk_commute(sk_strategy);
		}

		if (con->constisnull)
			continue;//TODO:need check?

		if (sk_strategy != GammaSKNotEqual)
		{
			sk[sk_count].sk_attno = var->varattno;
			sk[sk_count].sk_collation = op_expr->opcollid;
			sk[sk_count].sk_argument = con->constvalue;
			sk[sk_count].sk_strategy = sk_strategy;
			sk_count++;
		}

		if (gammadb_storage_skip_keys &&
				(sk_strategy == GammaSKNotEqual || sk_strategy == GammaSKEqual))
		{
			skip[skip_count].skip_attno = var->varattno;
			skip[skip_count].skip_collation = op_expr->opcollid;
			skip[skip_count].skip_argument = con->constvalue;
			skip[skip_count].skip_strategy = sk_strategy;
			skip_count++;
		}
	}

	if (sk_count == 0)
	{
		Assert(vscanstate->scankeys == NULL);
		Assert(vscanstate->sk_count == 0);
		pfree(sk);
		sk = NULL;
		//return;
	}

	if (skip_count == 0)
	{
		Assert(vscanstate->skipkeys == NULL);
		Assert(vscanstate->skip_count == 0);
		pfree(skip);
		skip = NULL;

	}

	vscanstate->scankeys = sk;
	vscanstate->sk_count = sk_count;

	vscanstate->skipkeys = skip;
	vscanstate->skip_count = skip_count;
	return;
}

bool
gamma_sk_run_scankeys(CVScanDesc cvscan, uint32 rgid)
{
	ListCell *lc;

	foreach(lc, cvscan->sk_attno_list)
	{
		AttrNumber attno = lfirst_int(lc);

		if (!cvtable_load_scankey_cv(cvscan, rgid, attno, true))
			return false;
	}

	return true;
}

bool
gamma_sk_attr_check(CVScanDesc cvscan, AttrNumber attno, char *min, char *max)
{
	int i;
	uint16 sk_count = cvscan->sk_count;

	for (i = 0; i < sk_count; i++)
	{
		GammaScanKey scankey = &cvscan->scankeys[i];
		if (scankey->sk_attno != attno)
			continue;

		Assert(scankey->sk_cmp != NULL);
		if (!scankey->sk_cmp(scankey->sk_strategy, scankey->sk_argument, min, max))
			return false;
	}

	return true;
}

#define GAMMA_SK_DEFINE_CMP(type, ctype, dtype) \
static bool \
gamma_sk_cmp_##type(GammaSKStrategy strategy, Datum con, char *min, char *max) \
{ \
	ctype dcon = DatumGet##dtype(con); \
	ctype dmin = DatumGet##dtype(*(Datum *) min); \
	ctype dmax = DatumGet##dtype(*(Datum *) max); \
	switch(strategy) \
	{ \
		case GammaSKLess: \
			{ \
				if (dmin >= dcon) \
					return false; \
				return true; \
			} \
		case GammaSKLessEqual: \
			{ \
				if (dmin > dcon) \
					return false; \
				return true; \
			} \
		case GammaSKEqual: \
			{ \
				if (dcon < dmin || dcon > dmax) \
					return false; \
				return true; \
			} \
		case GammaSKGreaterEqual: \
			{ \
				if (dmax < dcon) \
					return false; \
				return true; \
			} \
		case GammaSKGreater: \
			{ \
				if (dmax <= dcon) \
					return false; \
				return true; \
			} \
		case GammaSKNotEqual: \
		case GammaSKNone: \
		default: \
			{ \
				return true; \
			} \
	} \
	return true; \
}

GAMMA_SK_DEFINE_CMP(int16, int16, Int16)
GAMMA_SK_DEFINE_CMP(int32, int32, Int32)
GAMMA_SK_DEFINE_CMP(int64, int64, Int64)
GAMMA_SK_DEFINE_CMP(float4, float4, Float4)
GAMMA_SK_DEFINE_CMP(float8, float8, Float8)

GAMMA_SK_DEFINE_CMP(date, DateADT, DateADT)
GAMMA_SK_DEFINE_CMP(timestamp, Timestamp, Timestamp)

/*(text, char*, char*)*/
static bool
gamma_sk_cmp_text(GammaSKStrategy strategy, Datum con, char *min, char *max)
{
	char *dcon = DatumGetCString(con);
	char *dmin = min + 1;
	char *dmax = max + 1;
	uint8 min_len = *(uint8 *) min;
	uint8 max_len = *(uint8 *) max;

	switch(strategy) 
	{ 
		case GammaSKLess:
		case GammaSKLessEqual:
			{
				if (memcmp(dmin, dcon, min_len) > 0)
					return false;

				return true;
			}
		case GammaSKEqual:
			{
				 if (memcmp(dmin, dcon, min_len) > 0 ||
					 memcmp(dmax, dcon, max_len) < 0)
					return false;

				return true;
			}
		case GammaSKGreaterEqual:
		case GammaSKGreater:
			{
				if (memcmp(dmax, dcon, max_len) < 0) 
					return false;

				return true;
			}
		case GammaSKNotEqual:
		case GammaSKNone:
		default:
			{
				return true;
			}
	}
	return true;
}

static gamma_sk_cmp_callback
gamma_sk_get_func(Oid typeId)
{
	switch(typeId)
	{
		case INT2OID:
			{
				return gamma_sk_cmp_int16;
			}
		case INT4OID:
			{
				return gamma_sk_cmp_int32;
			}
		case INT8OID:
			{
				return gamma_sk_cmp_int64;
			}
		case FLOAT4OID:
			{
				return gamma_sk_cmp_float4;
			}
		case FLOAT8OID:
			{
				return gamma_sk_cmp_float8;
			}
		case DATEOID:
			{
				return gamma_sk_cmp_date;
			}
		case TIMESTAMPOID:
			{
				return gamma_sk_cmp_timestamp;
			}
		case TEXTOID:
			{
				return gamma_sk_cmp_text;
			}
		default:
			return NULL;
	}

	return NULL;
}

#define GAMMA_SKIP_DEFINE_CMP(type, ctype, dtype) \
static bool \
gamma_skip_cmp_##type(GammaSKStrategy strategy, Datum con, Datum val) \
{ \
	ctype dcon = DatumGet##dtype(con); \
	ctype dval = DatumGet##dtype(val); \
	switch(strategy) \
	{ \
		case GammaSKEqual: \
			{ \
				return (dcon == dval); \
			} \
		case GammaSKNotEqual: \
			{ \
				return (dcon != dval); \
			} \
		default: \
			{ \
				return true; \
			} \
	} \
	return true; \
}

GAMMA_SKIP_DEFINE_CMP(int16, int16, Int16)
GAMMA_SKIP_DEFINE_CMP(int32, int32, Int32)
GAMMA_SKIP_DEFINE_CMP(int64, int64, Int64)
GAMMA_SKIP_DEFINE_CMP(float4, float4, Float4)
GAMMA_SKIP_DEFINE_CMP(float8, float8, Float8)

GAMMA_SKIP_DEFINE_CMP(date, DateADT, DateADT)
GAMMA_SKIP_DEFINE_CMP(timestamp, Timestamp, Timestamp)

/*(text, char*, char*)*/
static bool
gamma_skip_cmp_text(GammaSKStrategy strategy, Datum con, Datum val)
{
	char *dcon = DatumGetCString(con);
	char *dval = VARDATA_ANY(DatumGetPointer(val));
	int32 len = VARSIZE_ANY_EXHDR(DatumGetPointer(val));
	int32 min_len = len > GAMMA_MINMAX_LENGTH ? GAMMA_MINMAX_LENGTH : len;

	switch(strategy) 
	{ 
		case GammaSKEqual:
			{
				 if (memcmp(dval, dcon, min_len) == 0)
					return true;

				return false;
			}
		case GammaSKNotEqual:
			{
				if (memcmp(dval, dcon, min_len) != 0) 
					return true;

				return false;
			}
		default:
			{
				return true;
			}
	}
	return true;
}

static gamma_skip_cmp_callback
gamma_skip_get_func(Oid typeId)
{
	switch(typeId)
	{
		case INT2OID:
			{
				return gamma_skip_cmp_int16;
			}
		case INT4OID:
			{
				return gamma_skip_cmp_int32;
			}
		case INT8OID:
			{
				return gamma_skip_cmp_int64;
			}
		case FLOAT4OID:
			{
				return gamma_skip_cmp_float4;
			}
		case FLOAT8OID:
			{
				return gamma_skip_cmp_float8;
			}
		case DATEOID:
			{
				return gamma_skip_cmp_date;
			}
		case TIMESTAMPOID:
			{
				return gamma_skip_cmp_timestamp;
			}
		case TEXTOID:
			{
				return gamma_skip_cmp_text;
			}
		default:
			return NULL;
	}

	return NULL;
}

bool
gamma_sk_set_scankeys(CVScanDesc cvscan, SeqScanState *scanstate)
{
	int i;
	VecSeqScanState *vstate = (VecSeqScanState *) scanstate;
	uint16 sk_count = vstate->sk_count;

	Relation base_rel = cvscan->base_rel;
	TupleDesc tupdesc = RelationGetDescr(base_rel);

	cvscan->scankeys = vstate->scankeys;
	cvscan->sk_count = sk_count;
	cvscan->sk_preloaded = (bool*)palloc0(sizeof(bool) * (tupdesc->natts + 1));

	for (i = 0; i < sk_count; i++)
	{
		AttrNumber sk_attno = vstate->scankeys[i].sk_attno;
		Form_pg_attribute attr = &tupdesc->attrs[sk_attno - 1];
		cvscan->scankeys[i].sk_cmp = gamma_sk_get_func(attr->atttypid);

		cvscan->sk_preloaded[sk_attno] = true;
		cvscan->sk_attno_list = list_append_unique_int(cvscan->sk_attno_list, sk_attno);
	}

	cvscan->skipkeys = vstate->skipkeys;
	cvscan->skip_count = vstate->skip_count;
	for (i = 0; i < vstate->skip_count; i++)
	{
		AttrNumber skip_attno = vstate->skipkeys[i].skip_attno;
		Form_pg_attribute attr = &tupdesc->attrs[skip_attno - 1];
		cvscan->skipkeys[i].skip_cmp = gamma_skip_get_func(attr->atttypid);
	}

	return true;
}

uint32
gamma_skip_run_scankeys(CVScanDesc cvscan, RowGroup *rg, uint32 offset)
{
	int i;
	GammaSkipKey skipkeys = cvscan->skipkeys;
	int16 skip_count = cvscan->skip_count;

	for (i = 0; i < skip_count; i++)
	{
		GammaSkipKey key = &skipkeys[i];
		AttrNumber attno = key->skip_attno;
		ColumnVector *cv = &rg->cvs[attno - 1];
		bool non_null = CVIsNonNull(cv);

		for (; offset < rg->dim; offset++)
		{
			if (!non_null && cv->isnull[i])
				continue;

			if (!key->skip_cmp(key->skip_strategy, key->skip_argument, cv->values[offset]))
				continue;

			break;
		}
	}

	return offset;
}

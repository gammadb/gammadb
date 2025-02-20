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
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "common/int.h"
#include "utils/date.h"

#include "utils/vdatum/vdatum.h"


#define MAX_NUM_LEN 64

const char canary = 0xe7;

vdatum* buildvdatum(Oid elemtype,int dim,bool *skip)
{
	vdatum *res;
	res = palloc0(sizeof(vdatum));
	res->dim = dim;
	res->elemtype = elemtype;
	res->skipref = skip;
	res->ref = false;

	return res;
}

void destroyvdatum(vdatum** vt)
{
	pfree((*vt));
	*vt = NULL;
}

void clearvdatum(vdatum *vt)
{
	vt->ref = false;
}

vdatum* copyvdatum(vdatum* src, bool *skip)
{
	vdatum *dest;
	dest = palloc0(sizeof(vdatum));
	dest->dim = src->dim;
	dest->elemtype = src->elemtype;
	dest->skipref = skip;
	dest->ref = false;

	if (src->ref)
	{
		memcpy(dest->values, src->ref_values, VECTOR_SIZE * sizeof(Datum));
		memcpy(dest->isnull, src->ref_isnull, VECTOR_SIZE * sizeof(bool));
	}
	else
	{
		memcpy(dest->values, src->values, VECTOR_SIZE * sizeof(Datum));
		memcpy(dest->isnull, src->isnull, VECTOR_SIZE * sizeof(bool));
	}

	return dest;
}

#define _FUNCTION_BUILD(type, typeoid) \
v##type* buildv##type(int dim, bool *skip) \
{ \
	return buildvdatum(typeoid, dim, skip); \
}

/*
 * IN function for the abstract data types
 * e.g. Datum vint2in(PG_FUNCTION_ARGS)
 */
#define _FUNCTION_IN(type, fname, typeoid) \
PG_FUNCTION_INFO_V1(v##fname##in); \
Datum \
v##fname##in(PG_FUNCTION_ARGS) \
{ \
	char *intString = PG_GETARG_CSTRING(0); \
	vdatum *res = NULL; \
	char tempstr[MAX_NUM_LEN] = {0}; \
	int n = 0; \
	res = buildvdatum(typeoid,VECTOR_SIZE,NULL);\
	for (n = 0; *intString && n < VECTOR_SIZE; n++) \
	{ \
			char *start = NULL;\
		while (*intString && isspace((unsigned char) *intString)) \
			intString++; \
		if (*intString == '\0') \
			break; \
		start = intString; \
		while ((*intString && !isspace((unsigned char) *intString)) && *intString != '\0') \
			intString++; \
		Assert(intString - start < MAX_NUM_LEN); \
		strncpy(tempstr, start, intString - start); \
		tempstr[intString - start] = 0; \
		VDATUM_SET_DATUM(res, n, DirectFunctionCall1(fname##in, CStringGetDatum(tempstr))); \
		while (*intString && !isspace((unsigned char) *intString)) \
			intString++; \
	} \
	while (*intString && isspace((unsigned char) *intString)) \
		intString++; \
	if (*intString) \
		ereport(ERROR, \
		(errcode(ERRCODE_INVALID_PARAMETER_VALUE), \
				errmsg("int2vector has too many elements"))); \
	res->elemtype = typeoid; \
	res->dim = n; \
	SET_VARSIZE(res, VDATUMSIZE(n)); \
	PG_RETURN_POINTER(res); \
}

/*
 * OUT function for the abstract data types
 * e.g. Datum vint2out(PG_FUNCTION_ARGS)
 */
#define _FUNCTION_OUT(type, fname, typeoid) \
PG_FUNCTION_INFO_V1(v##fname##out); \
Datum \
v##fname##out(PG_FUNCTION_ARGS) \
{ \
	vdatum * arg1 = (v##type *) PG_GETARG_POINTER(0); \
	int len = arg1->dim; \
	int i = 0; \
	char *rp; \
	char *result; \
	rp = result = (char *) palloc0(len * MAX_NUM_LEN + 1); \
	for (i = 0; i < len; i++) \
	{ \
		if (i != 0) \
			*rp++ = ' '; \
		strcat(rp, DatumGetCString(DirectFunctionCall1(fname##out, VDATUM_DATUM(arg1, i))));\
		while (*++rp != '\0'); \
	} \
	*rp = '\0'; \
	PG_RETURN_CSTRING(result); \
}

/*
 * Operator function for the abstract data types, this MACRO is used for the 
 * V-types OP V-types.
 * e.g. extern Datum vint2vint2pl(PG_FUNCTION_ARGS);
 * NOTE:we assum that return type is same with the type of arg1,
 * we have not processed the overflow so far.
 */
#define __FUNCTION_OP(type1, XTYPE1, type2, XTYPE2, opsym, opstr) \
PG_FUNCTION_INFO_V1(v##type1##v##type2##opstr); \
Datum \
v##type1##v##type2##opstr(PG_FUNCTION_ARGS) \
{ \
	int size = 0; \
	int i = 0; \
	v##type1 *arg1 = (v##type1*)PG_GETARG_POINTER(0); \
	v##type2 *arg2 = (v##type2*)PG_GETARG_POINTER(1); \
	v##type1 *res = buildv##type1(VECTOR_SIZE, arg1->skipref); \
	Assert(arg1->dim == arg2->dim); \
	size = arg1->dim; \
	while(i < size) \
	{ \
		VDATUM_SET_ISNULL(res, i, (VDATUM_ISNULL(arg1, i)||VDATUM_ISNULL(arg2, i))); \
		i++; \
	} \
	i=0; \
	while(i < size) \
	{ \
		if(!VDATUM_ISNULL(res, i)) \
			VDATUM_SET_DATUM(res, i, XTYPE1##GetDatum((DatumGet##XTYPE1(VDATUM_DATUM(arg1, i))) opsym (DatumGet##XTYPE2(VDATUM_DATUM(arg2,i))))); \
		i++; \
	} \
	res->dim = arg1->dim; \
	PG_RETURN_POINTER(res); \
}

/*
 * Operator function for the abstract data types, this MACRO is used for the 
 * V-types OP Consts.
 * e.g. extern Datum vint2int2pl(PG_FUNCTION_ARGS);
 */
#define __FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, opsym, opstr) \
PG_FUNCTION_INFO_V1(v##type##const_type##opstr); \
Datum \
v##type##const_type##opstr(PG_FUNCTION_ARGS) \
{ \
	int size = 0; \
	int i = 0; \
	v##type *arg1 = (v##type*)PG_GETARG_POINTER(0); \
	const_type arg2 = CONST_ARG_MACRO(1); \
	v##type *res = buildv##type(VECTOR_SIZE, arg1->skipref); \
	size = arg1->dim;\
	while(i < size) \
	{ \
		VDATUM_SET_ISNULL(res, i, (VDATUM_ISNULL(arg1, i))); \
		if(!VDATUM_ISNULL(res, i)) \
			VDATUM_SET_DATUM(res, i, XTYPE##GetDatum((DatumGet##XTYPE(VDATUM_DATUM(arg1, i))) opsym ((type)arg2))); \
		i ++ ;\
	} \
	res->dim = arg1->dim; \
	PG_RETURN_POINTER(res); \
}

/*
 * Operator function for the abstract data types, this MACRO is used for the
 * Consts OP V-types.
 * e.g. extern Datum int2vint2pl(PG_FUNCTION_ARGS);
 */
#define __FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, opsym, opstr) \
PG_FUNCTION_INFO_V1(const_type##v##type##opstr); \
Datum \
const_type##v##type##opstr(PG_FUNCTION_ARGS) \
{ \
	int size = 0; \
	int i = 0; \
	const_type arg1 = CONST_ARG_MACRO(0); \
	v##type *arg2 = (v##type*)PG_GETARG_POINTER(1); \
	v##type *res = buildv##type(VECTOR_SIZE, arg2->skipref); \
	size = arg2->dim;\
	while(i < size) \
	{ \
		VDATUM_SET_ISNULL(res, i, (VDATUM_ISNULL(arg2, i))); \
		i++; \
	} \
	i=0; \
	while(i < size) \
	{ \
		if(!VDATUM_ISNULL(res, i)) \
			VDATUM_SET_DATUM(res, i, XTYPE##GetDatum(((type)arg1) opsym (DatumGet##XTYPE(VDATUM_DATUM(arg2, i))))); \
		i ++ ;\
	} \
	res->dim = arg2->dim; \
	PG_RETURN_POINTER(res); \
}


/*
 * Comparision function for the abstract data types, this MACRO is used for the 
 * V-types OP V-types.
 * e.g. extern Datum vint2vint2eq(PG_FUNCTION_ARGS);
 */
#define __FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, cmpsym, cmpstr) \
PG_FUNCTION_INFO_V1(v##type1##v##type2##cmpstr); \
Datum \
v##type1##v##type2##cmpstr(PG_FUNCTION_ARGS) \
{ \
	vbool *res; \
	int size = 0; \
	int i = 0; \
	v##type1 *arg1 = (v##type1*)PG_GETARG_POINTER(0); \
	v##type2 *arg2 = (v##type2*)PG_GETARG_POINTER(1); \
	Assert(arg1->dim == arg2->dim); \
	res = buildvdatum(BOOLOID, VECTOR_SIZE, arg1->skipref); \
	size = arg1->dim; \
	while(i < size) \
	{ \
		VDATUM_SET_ISNULL(res, i, (VDATUM_ISNULL(arg1, i)||VDATUM_ISNULL(arg2, i))); \
		i++; \
	} \
	i=0; \
	while(i < size) \
	{ \
		if(!VDATUM_ISNULL(res, i)) \
			VDATUM_SET_DATUM(res, i, BoolGetDatum(DatumGet##XTYPE1(VDATUM_DATUM(arg1, i)) cmpsym (DatumGet##XTYPE2(VDATUM_DATUM(arg2, i))))); \
		i++; \
	} \
	res->dim = arg1->dim; \
	PG_RETURN_POINTER(res); \
}

/*
 * Comparision function for the abstract data types, this MACRO is used for the 
 * V-types OP Consts.
 * e.g. extern Datum vint2int2eq(PG_FUNCTION_ARGS);
 */
#define __FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, cmpsym, cmpstr) \
PG_FUNCTION_INFO_V1(v##type##const_type##cmpstr); \
Datum \
v##type##const_type##cmpstr(PG_FUNCTION_ARGS) \
{ \
	int size = 0; \
	int i = 0; \
	v##type *arg1 = (v##type*)PG_GETARG_POINTER(0); \
	const_type arg2 = CONST_ARG_MACRO(1); \
	vbool *res = buildvdatum(BOOLOID, VECTOR_SIZE, arg1->skipref); \
	size = arg1->dim; \
	if (!VDATUM_IS_REF(arg1)) \
	{ \
		memcpy(res->isnull, arg1->isnull, sizeof(bool) * size); \
		while(i < size) \
		{ \
			res->values[i] = BoolGetDatum(DatumGet##XTYPE(arg1->values[i]) cmpsym arg2); \
			i++; \
		} \
	} \
	else \
	{ \
		if (arg1->ref_isnull != NULL) \
			memcpy(res->isnull, arg1->ref_isnull, sizeof(bool) * size); \
		else \
			memset(res->isnull, false, sizeof(bool) * size); \
		while(i < size) \
		{ \
			res->values[i] = BoolGetDatum(DatumGet##XTYPE(arg1->ref_values[i]) cmpsym arg2); \
			i++; \
		} \
	} \
	res->dim = arg1->dim; \
	PG_RETURN_POINTER(res); \
}

//Macro Level 3
/* These MACRO will be expanded when the code is compiled. */
#define _FUNCTION_OP(type1, XTYPE1, type2, XTYPE2) \
	__FUNCTION_OP(type1, XTYPE1, type2, XTYPE2, +, pl)  \
	__FUNCTION_OP(type1, XTYPE1, type2, XTYPE2, -, mi)  \
	__FUNCTION_OP(type1, XTYPE1, type2, XTYPE2, *, mul) \
	__FUNCTION_OP(type1, XTYPE1, type2, XTYPE2, /, div)

#define _FUNCTION_DATE_OP_CONST(type, XTYPE, const_type, CONST_ARG_MACRO) \
	__FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, +, pl)  \
	__FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, -, mi)  \
	__FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, +, pl)  \
	__FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, -, mi)

#define _FUNCTION_OP_CONST(type, XTYPE, const_type, CONST_ARG_MACRO) \
	__FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, +, pl)  \
	__FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, -, mi)  \
	__FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, *, mul) \
	__FUNCTION_OP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, /, div) \
	__FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, +, pl)  \
	__FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, -, mi)  \
	__FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, *, mul) \
	__FUNCTION_OP_LCONST(type, XTYPE, const_type, CONST_ARG_MACRO, /, div)

#define _FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2) \
	__FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, ==, eq) \
	__FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, !=, ne) \
	__FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, >, gt) \
	__FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, >=, ge) \
	__FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, <, lt) \
	__FUNCTION_CMP(type1, XTYPE1, type2, XTYPE2, <=, le)

#define _FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO) \
	__FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, ==, eq)  \
	__FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, !=, ne)  \
	__FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO,  >, gt) \
	__FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, >=, ge) \
	__FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO,  <, lt) \
	__FUNCTION_CMP_RCONST(type, XTYPE, const_type, CONST_ARG_MACRO, <=, le) \

//Macro Level 2
#define FUNCTION_OP(type, XTYPE1) \
	_FUNCTION_OP(type, XTYPE1, int2, Int16) \
	_FUNCTION_OP(type, XTYPE1, int4, Int32) \
	_FUNCTION_OP(type, XTYPE1, int8, Int64) \
	_FUNCTION_OP(type, XTYPE1, float4, Float4) \
	_FUNCTION_OP(type, XTYPE1, float8, Float8)

#define FUNCTION_DATE_OP_RCONST(type, XTYPE) \
	_FUNCTION_DATE_OP_CONST(type, XTYPE, DateADT, PG_GETARG_DATEADT) \
	_FUNCTION_DATE_OP_CONST(type, XTYPE, Interval, PG_GETARG_INTERVAL_P) \
	_FUNCTION_DATE_OP_CONST(type, XTYPE, int4, PG_GETARG_INT32)

#define FUNCTION_OP_RCONST(type, XTYPE) \
	_FUNCTION_OP_CONST(type, XTYPE, int2, PG_GETARG_INT16) \
	_FUNCTION_OP_CONST(type, XTYPE, int4, PG_GETARG_INT32) \
	_FUNCTION_OP_CONST(type, XTYPE, int8, PG_GETARG_INT64) \
	_FUNCTION_OP_CONST(type, XTYPE, float4, PG_GETARG_FLOAT4) \
	_FUNCTION_OP_CONST(type, XTYPE, float8, PG_GETARG_FLOAT8)

#define FUNCTION_CMP(type1, XTYPE1) \
	_FUNCTION_CMP(type1, XTYPE1, int2, Int16) \
	_FUNCTION_CMP(type1, XTYPE1, int4, Int32) \
	_FUNCTION_CMP(type1, XTYPE1, int8, Int64) \
	_FUNCTION_CMP(type1, XTYPE1, float4, Float4) \
	_FUNCTION_CMP(type1, XTYPE1, float8, Float8)

#define FUNCTION_CMP_RCONST(type, XTYPE) \
	_FUNCTION_CMP_RCONST(type, XTYPE, int2, PG_GETARG_INT16) \
	_FUNCTION_CMP_RCONST(type, XTYPE, int4, PG_GETARG_INT32) \
	_FUNCTION_CMP_RCONST(type, XTYPE, int8, PG_GETARG_INT64) \
	_FUNCTION_CMP_RCONST(type, XTYPE, float4, PG_GETARG_FLOAT4) \
	_FUNCTION_CMP_RCONST(type, XTYPE, float8, PG_GETARG_FLOAT8)

//Macro Level 1
#define FUNCTION_OP_ALL(type, XTYPE1) \
	FUNCTION_OP(type, XTYPE1) \
	FUNCTION_OP_RCONST(type, XTYPE1) \
	FUNCTION_CMP(type, XTYPE1) \
	FUNCTION_CMP_RCONST(type, XTYPE1)

#define FUNCTION_BUILD(type,fname, typeoid) \
	_FUNCTION_BUILD(type, typeoid) \
	_FUNCTION_IN(type,fname, typeoid) \
	_FUNCTION_OUT(type, fname, typeoid)

//Macro Level 0
FUNCTION_BUILD(int2, int2, INT2OID)
FUNCTION_BUILD(int4, int4, INT4OID)
FUNCTION_BUILD(int8, int8, INT8OID)
FUNCTION_BUILD(float4, float4, FLOAT4OID)
FUNCTION_BUILD(float8, float8, FLOAT8OID)
//FUNCTION_BUILD(bool, bool, BOOLOID)
FUNCTION_BUILD(text, text, TEXTOID)
FUNCTION_BUILD(bpchar, bpchar, BPCHAROID)

FUNCTION_OP_ALL(int2, Int16)
FUNCTION_OP_ALL(int4, Int32)
FUNCTION_OP_ALL(int8, Int64)
FUNCTION_OP_ALL(float4, Float4)
FUNCTION_OP_ALL(float8, Float8)
//FUNCTION_OP_ALL(bool, Bool)

//#define _FUNCTION_BUILD(type, typeoid)
vbool* buildvbool(int dim, bool *skip)
{
	return buildvdatum(BOOLOID, dim, skip);
}

/*
 * IN function for the abstract data types
 * e.g. Datum vint2in(PG_FUNCTION_ARGS)
 */
//#define _FUNCTION_IN(type, fname, typeoid) 
PG_FUNCTION_INFO_V1(vboolin);
Datum
vboolin(PG_FUNCTION_ARGS)
{
	char *intString = PG_GETARG_CSTRING(0);
	vdatum *res = NULL;
	char tempstr[MAX_NUM_LEN] = {0};
	int n = 0;
	res = buildvdatum(BOOLOID,VECTOR_SIZE,NULL);
	for (n = 0; *intString && n < VECTOR_SIZE; n++)
	{
			char *start = NULL;
		while (*intString && isspace((unsigned char) *intString))
			intString++; 
		if (*intString == '\0')
			break;
		start = intString;
		while ((*intString &&
				!isspace((unsigned char) *intString)) &&
				*intString != '\0')
			intString++;
		Assert(intString - start < MAX_NUM_LEN);
		strncpy(tempstr, start, intString - start);
		tempstr[intString - start] = 0;
		VDATUM_SET_DATUM(res, n, DirectFunctionCall1(boolin, CStringGetDatum(tempstr)));
		while (*intString && !isspace((unsigned char) *intString))
			intString++;
	}
	while (*intString && isspace((unsigned char) *intString))
		intString++;
	if (*intString)
		ereport(ERROR,
		(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("int2vector has too many elements")));
	res->elemtype = BOOLOID;
	res->dim = n;
	SET_VARSIZE(res, VDATUMSIZE(n));
	PG_RETURN_POINTER(res);
}

/*
 * OUT function for the abstract data types
 * e.g. Datum vint2out(PG_FUNCTION_ARGS)
 */
//#define _FUNCTION_OUT(type, fname, typeoid)
PG_FUNCTION_INFO_V1(vboolout); \
Datum
vboolout(PG_FUNCTION_ARGS)
{
	vdatum * arg1 = (vbool *) PG_GETARG_POINTER(0);
	int len = arg1->dim;
	int i = 0;
	char *rp;
	char *result;
	rp = result = (char *) palloc0(len * MAX_NUM_LEN + 1);
	for (i = 0; i < len; i++)
	{
		if (i != 0)
			*rp++ = ' ';
		strcat(rp, DatumGetCString(DirectFunctionCall1(boolout, VDATUM_DATUM(arg1, i))));
		while (*++rp != '\0');
	}
	*rp = '\0';
	PG_RETURN_CSTRING(result);
}

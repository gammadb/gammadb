--create namespace
CREATE SCHEMA gammadb_namespace;
GRANT all privileges ON SCHEMA gammadb_namespace TO PUBLIC;


--create ctable handler
CREATE OR REPLACE FUNCTION gammadb_namespace.gammadb_ctable_am_handler(internal)
	RETURNS table_am_handler
	LANGUAGE C
	AS 'MODULE_PATHNAME', 'ctable_handler';

CREATE ACCESS METHOD gamma
	TYPE TABLE
	HANDLER gammadb_namespace.gammadb_ctable_am_handler;

-- switch to public namespace
SET search_path = public;

--drop agg functions
--drop the previous funcitons
DROP AGGREGATE IF EXISTS sum(vint4);
DROP AGGREGATE IF EXISTS sum(vfloat8);

DROP AGGREGATE IF EXISTS avg(vfloat8);

DROP AGGREGATE IF EXISTS count(vany);

-- DROP TYPEs first
DROP TYPE IF EXISTS vint2 cascade;
DROP TYPE IF EXISTS vint4 cascade;
DROP TYPE IF EXISTS vint8 cascade;
DROP TYPE IF EXISTS vfloat8 cascade;
DROP TYPE IF EXISTS vfloat4 cascade;
DROP TYPE IF EXISTS vbool cascade;
DROP TYPE IF EXISTS vtext cascade;
DROP TYPE IF EXISTS vdate cascade;
DROP TYPE IF EXISTS vtimestamp cascade;
DROP TYPE IF EXISTS vnumeric cascade;
DROP TYPE IF EXISTS vbpchar cascade;
DROP TYPE IF EXISTS vtid cascade;
--DROP TYPE IF EXISTS vvarchar cascade;




-- create vectorized types

CREATE TYPE vint2;
CREATE FUNCTION vint2in(cstring) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vint2out(vint2) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vint2 ( INPUT = vint2in, OUTPUT = vint2out, storage = plain, internallength = 2, alignment = int2, PASSEDBYVALUE );


CREATE TYPE vint4;
CREATE FUNCTION vint4in(cstring) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vint4out(vint4) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vint4 ( INPUT = vint4in, OUTPUT = vint4out, storage = plain, internallength = 4, alignment = int4, PASSEDBYVALUE );

CREATE TYPE vint8;
CREATE FUNCTION vint8in(cstring) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vint8out(vint8) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vint8 ( INPUT = vint8in, OUTPUT = vint8out, storage = plain, internallength = 8, alignment = double, PASSEDBYVALUE);


CREATE TYPE vfloat4;
CREATE FUNCTION vfloat4in(cstring) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vfloat4out(vfloat4) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vfloat4 ( INPUT = vfloat4in, OUTPUT = vfloat4out, storage = plain, internallength = 4, alignment = int4, PASSEDBYVALUE );


CREATE TYPE vfloat8;
CREATE FUNCTION vfloat8in(cstring) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vfloat8out(vfloat8) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vfloat8 ( INPUT = vfloat8in, OUTPUT = vfloat8out, storage = plain, internallength = 8, alignment = double, PASSEDBYVALUE );


CREATE TYPE vbool;
CREATE FUNCTION vboolin(cstring) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vboolout(vbool) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vbool ( INPUT = vboolin, OUTPUT = vboolout, storage=plain );

CREATE TYPE vtext;
CREATE FUNCTION vtextin(cstring) RETURNS vtext AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vtextout(vtext) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vtext ( INPUT = vtextin, OUTPUT = vtextout, storage=extended, internallength = -1, alignment = int4);

CREATE TYPE vdate;
CREATE FUNCTION vdate_in(cstring) RETURNS vdate AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vdate_out(vdate) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vdate ( INPUT = vdate_in, OUTPUT = vdate_out, storage=plain, internallength = 4, alignment = int4, PASSEDBYVALUE );

CREATE TYPE vtimestamp;
CREATE FUNCTION vtimestamp_in(cstring) RETURNS vtimestamp AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vtimestamp_out(vtimestamp) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vtimestamp ( INPUT = vtimestamp_in, OUTPUT = vtimestamp_out, storage=plain, internallength = 8, alignment = double, PASSEDBYVALUE );

CREATE TYPE vbpchar;
CREATE FUNCTION vbpcharin(cstring) RETURNS vbpchar AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vbpcharout(vbpchar) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vbpchar ( INPUT = vbpcharin, OUTPUT = vbpcharout, storage=plain );

CREATE TYPE vnumeric;
CREATE FUNCTION vnumeric_in(cstring) RETURNS vnumeric AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vnumeric_out(vnumeric) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vnumeric ( INPUT = vnumeric_in, OUTPUT = vnumeric_out, TYPMOD_IN = numerictypmodin, TYPMOD_OUT = numerictypmodout, storage=main, internallength = -1, alignment = int4 );

CREATE TYPE vtid;
CREATE FUNCTION vtidin(cstring) RETURNS vtid AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vtidout(vtid) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vtid ( INPUT = vtidin, OUTPUT = vtidout, storage = plain, internallength=6,  alignment = int2);

--CREATE TYPE vvarchar;
--CREATE FUNCTION vvarcharin(cstring) RETURNS vvarchar AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
--CREATE FUNCTION vvarcharout(vvarchar) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
--CREATE TYPE vvarchar ( INPUT = vvarcharin, OUTPUT = vvarcharout, element = varchar, storage=plain );

CREATE TYPE vany;
CREATE FUNCTION vany_in(cstring) RETURNS vany AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vany_out(vany) RETURNS cstring AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE vany ( INPUT = vany_in, OUTPUT = vany_out, storage=plain );

-- create operators for the vectorized types

CREATE FUNCTION vint2vint2gt(vint2, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2gt, commutator = <= );
CREATE FUNCTION vint2vint2ge(vint2, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2ge, commutator = < );
CREATE FUNCTION vint2vint2eq(vint2, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2eq, commutator = <> );
CREATE FUNCTION vint2vint2ne(vint2, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2ne, commutator = = );
CREATE FUNCTION vint2vint2lt(vint2, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2lt, commutator = >= );
CREATE FUNCTION vint2vint2le(vint2, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2le, commutator = > );
CREATE FUNCTION vint2vint4gt(vint2, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = vint4, procedure = vint2vint4gt, commutator = <= );
CREATE FUNCTION vint2vint4ge(vint2, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = vint4, procedure = vint2vint4ge, commutator = < );
CREATE FUNCTION vint2vint4eq(vint2, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = vint4, procedure = vint2vint4eq, commutator = <> );
CREATE FUNCTION vint2vint4ne(vint2, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = vint4, procedure = vint2vint4ne, commutator = = );
CREATE FUNCTION vint2vint4lt(vint2, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = vint4, procedure = vint2vint4lt, commutator = >= );
CREATE FUNCTION vint2vint4le(vint2, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = vint4, procedure = vint2vint4le, commutator = > );
CREATE FUNCTION vint2vint8gt(vint2, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = vint8, procedure = vint2vint8gt, commutator = <= );
CREATE FUNCTION vint2vint8ge(vint2, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = vint8, procedure = vint2vint8ge, commutator = < );
CREATE FUNCTION vint2vint8eq(vint2, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = vint8, procedure = vint2vint8eq, commutator = <> );
CREATE FUNCTION vint2vint8ne(vint2, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = vint8, procedure = vint2vint8ne, commutator = = );
CREATE FUNCTION vint2vint8lt(vint2, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = vint8, procedure = vint2vint8lt, commutator = >= );
CREATE FUNCTION vint2vint8le(vint2, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = vint8, procedure = vint2vint8le, commutator = > );
CREATE FUNCTION vint2vfloat4gt(vint2, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = vfloat4, procedure = vint2vfloat4gt, commutator = <= );
CREATE FUNCTION vint2vfloat4ge(vint2, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = vfloat4, procedure = vint2vfloat4ge, commutator = < );
CREATE FUNCTION vint2vfloat4eq(vint2, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = vfloat4, procedure = vint2vfloat4eq, commutator = <> );
CREATE FUNCTION vint2vfloat4ne(vint2, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = vfloat4, procedure = vint2vfloat4ne, commutator = = );
CREATE FUNCTION vint2vfloat4lt(vint2, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = vfloat4, procedure = vint2vfloat4lt, commutator = >= );
CREATE FUNCTION vint2vfloat4le(vint2, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = vfloat4, procedure = vint2vfloat4le, commutator = > );
CREATE FUNCTION vint2vfloat8gt(vint2, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = vfloat8, procedure = vint2vfloat8gt, commutator = <= );
CREATE FUNCTION vint2vfloat8ge(vint2, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = vfloat8, procedure = vint2vfloat8ge, commutator = < );
CREATE FUNCTION vint2vfloat8eq(vint2, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = vfloat8, procedure = vint2vfloat8eq, commutator = <> );
CREATE FUNCTION vint2vfloat8ne(vint2, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = vfloat8, procedure = vint2vfloat8ne, commutator = = );
CREATE FUNCTION vint2vfloat8lt(vint2, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = vfloat8, procedure = vint2vfloat8lt, commutator = >= );
CREATE FUNCTION vint2vfloat8le(vint2, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = vfloat8, procedure = vint2vfloat8le, commutator = > );
CREATE FUNCTION vint2int2gt(vint2, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = int2, procedure = vint2int2gt, commutator = <= );
CREATE FUNCTION vint2int2ge(vint2, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = int2, procedure = vint2int2ge, commutator = < );
CREATE FUNCTION vint2int2eq(vint2, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = int2, procedure = vint2int2eq, commutator = <> );
CREATE FUNCTION vint2int2ne(vint2, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = int2, procedure = vint2int2ne, commutator = = );
CREATE FUNCTION vint2int2lt(vint2, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = int2, procedure = vint2int2lt, commutator = >= );
CREATE FUNCTION vint2int2le(vint2, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = int2, procedure = vint2int2le, commutator = > );
CREATE FUNCTION vint2int4gt(vint2, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = int4, procedure = vint2int4gt, commutator = <= );
CREATE FUNCTION vint2int4ge(vint2, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = int4, procedure = vint2int4ge, commutator = < );
CREATE FUNCTION vint2int4eq(vint2, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = int4, procedure = vint2int4eq, commutator = <> );
CREATE FUNCTION vint2int4ne(vint2, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = int4, procedure = vint2int4ne, commutator = = );
CREATE FUNCTION vint2int4lt(vint2, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = int4, procedure = vint2int4lt, commutator = >= );
CREATE FUNCTION vint2int4le(vint2, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = int4, procedure = vint2int4le, commutator = > );
CREATE FUNCTION vint2int8gt(vint2, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = int8, procedure = vint2int8gt, commutator = <= );
CREATE FUNCTION vint2int8ge(vint2, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = int8, procedure = vint2int8ge, commutator = < );
CREATE FUNCTION vint2int8eq(vint2, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = int8, procedure = vint2int8eq, commutator = <> );
CREATE FUNCTION vint2int8ne(vint2, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = int8, procedure = vint2int8ne, commutator = = );
CREATE FUNCTION vint2int8lt(vint2, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = int8, procedure = vint2int8lt, commutator = >= );
CREATE FUNCTION vint2int8le(vint2, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = int8, procedure = vint2int8le, commutator = > );
CREATE FUNCTION vint2float4gt(vint2, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = float4, procedure = vint2float4gt, commutator = <= );
CREATE FUNCTION vint2float4ge(vint2, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = float4, procedure = vint2float4ge, commutator = < );
CREATE FUNCTION vint2float4eq(vint2, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = float4, procedure = vint2float4eq, commutator = <> );
CREATE FUNCTION vint2float4ne(vint2, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = float4, procedure = vint2float4ne, commutator = = );
CREATE FUNCTION vint2float4lt(vint2, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = float4, procedure = vint2float4lt, commutator = >= );
CREATE FUNCTION vint2float4le(vint2, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = float4, procedure = vint2float4le, commutator = > );
CREATE FUNCTION vint2float8gt(vint2, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint2, rightarg = float8, procedure = vint2float8gt, commutator = <= );
CREATE FUNCTION vint2float8ge(vint2, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint2, rightarg = float8, procedure = vint2float8ge, commutator = < );
CREATE FUNCTION vint2float8eq(vint2, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint2, rightarg = float8, procedure = vint2float8eq, commutator = <> );
CREATE FUNCTION vint2float8ne(vint2, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint2, rightarg = float8, procedure = vint2float8ne, commutator = = );
CREATE FUNCTION vint2float8lt(vint2, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint2, rightarg = float8, procedure = vint2float8lt, commutator = >= );
CREATE FUNCTION vint2float8le(vint2, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint2, rightarg = float8, procedure = vint2float8le, commutator = > );

CREATE FUNCTION vint4vint2gt(vint4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = vint2, procedure = vint4vint2gt, commutator = <= );
CREATE FUNCTION vint4vint2ge(vint4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = vint2, procedure = vint4vint2ge, commutator = < );
CREATE FUNCTION vint4vint2eq(vint4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = vint2, procedure = vint4vint2eq, commutator = <> );
CREATE FUNCTION vint4vint2ne(vint4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = vint2, procedure = vint4vint2ne, commutator = = );
CREATE FUNCTION vint4vint2lt(vint4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = vint2, procedure = vint4vint2lt, commutator = >= );
CREATE FUNCTION vint4vint2le(vint4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = vint2, procedure = vint4vint2le, commutator = > );
CREATE FUNCTION vint4vint4gt(vint4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4gt, commutator = <= );
CREATE FUNCTION vint4vint4ge(vint4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4ge, commutator = < );
CREATE FUNCTION vint4vint4eq(vint4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4eq, commutator = <> );
CREATE FUNCTION vint4vint4ne(vint4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4ne, commutator = = );
CREATE FUNCTION vint4vint4lt(vint4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4lt, commutator = >= );
CREATE FUNCTION vint4vint4le(vint4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4le, commutator = > );
CREATE FUNCTION vint4vint8gt(vint4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = vint8, procedure = vint4vint8gt, commutator = <= );
CREATE FUNCTION vint4vint8ge(vint4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = vint8, procedure = vint4vint8ge, commutator = < );
CREATE FUNCTION vint4vint8eq(vint4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = vint8, procedure = vint4vint8eq, commutator = <> );
CREATE FUNCTION vint4vint8ne(vint4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = vint8, procedure = vint4vint8ne, commutator = = );
CREATE FUNCTION vint4vint8lt(vint4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = vint8, procedure = vint4vint8lt, commutator = >= );
CREATE FUNCTION vint4vint8le(vint4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = vint8, procedure = vint4vint8le, commutator = > );
CREATE FUNCTION vint4vfloat4gt(vint4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = vfloat4, procedure = vint4vfloat4gt, commutator = <= );
CREATE FUNCTION vint4vfloat4ge(vint4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = vfloat4, procedure = vint4vfloat4ge, commutator = < );
CREATE FUNCTION vint4vfloat4eq(vint4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = vfloat4, procedure = vint4vfloat4eq, commutator = <> );
CREATE FUNCTION vint4vfloat4ne(vint4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = vfloat4, procedure = vint4vfloat4ne, commutator = = );
CREATE FUNCTION vint4vfloat4lt(vint4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = vfloat4, procedure = vint4vfloat4lt, commutator = >= );
CREATE FUNCTION vint4vfloat4le(vint4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = vfloat4, procedure = vint4vfloat4le, commutator = > );
CREATE FUNCTION vint4vfloat8gt(vint4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = vfloat8, procedure = vint4vfloat8gt, commutator = <= );
CREATE FUNCTION vint4vfloat8ge(vint4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = vfloat8, procedure = vint4vfloat8ge, commutator = < );
CREATE FUNCTION vint4vfloat8eq(vint4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = vfloat8, procedure = vint4vfloat8eq, commutator = <> );
CREATE FUNCTION vint4vfloat8ne(vint4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = vfloat8, procedure = vint4vfloat8ne, commutator = = );
CREATE FUNCTION vint4vfloat8lt(vint4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = vfloat8, procedure = vint4vfloat8lt, commutator = >= );
CREATE FUNCTION vint4vfloat8le(vint4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = vfloat8, procedure = vint4vfloat8le, commutator = > );
CREATE FUNCTION vint4int2gt(vint4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = int2, procedure = vint4int2gt, commutator = <= );
CREATE FUNCTION vint4int2ge(vint4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = int2, procedure = vint4int2ge, commutator = < );
CREATE FUNCTION vint4int2eq(vint4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = int2, procedure = vint4int2eq, commutator = <> );
CREATE FUNCTION vint4int2ne(vint4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = int2, procedure = vint4int2ne, commutator = = );
CREATE FUNCTION vint4int2lt(vint4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = int2, procedure = vint4int2lt, commutator = >= );
CREATE FUNCTION vint4int2le(vint4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = int2, procedure = vint4int2le, commutator = > );
CREATE FUNCTION vint4int4gt(vint4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = int4, procedure = vint4int4gt, commutator = <= );
CREATE FUNCTION vint4int4ge(vint4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = int4, procedure = vint4int4ge, commutator = < );
CREATE FUNCTION vint4int4eq(vint4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = int4, procedure = vint4int4eq, commutator = <> );
CREATE FUNCTION vint4int4ne(vint4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = int4, procedure = vint4int4ne, commutator = = );
CREATE FUNCTION vint4int4lt(vint4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = int4, procedure = vint4int4lt, commutator = >= );
CREATE FUNCTION vint4int4le(vint4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = int4, procedure = vint4int4le, commutator = > );
CREATE FUNCTION vint4int8gt(vint4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = int8, procedure = vint4int8gt, commutator = <= );
CREATE FUNCTION vint4int8ge(vint4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = int8, procedure = vint4int8ge, commutator = < );
CREATE FUNCTION vint4int8eq(vint4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = int8, procedure = vint4int8eq, commutator = <> );
CREATE FUNCTION vint4int8ne(vint4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = int8, procedure = vint4int8ne, commutator = = );
CREATE FUNCTION vint4int8lt(vint4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = int8, procedure = vint4int8lt, commutator = >= );
CREATE FUNCTION vint4int8le(vint4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = int8, procedure = vint4int8le, commutator = > );
CREATE FUNCTION vint4float4gt(vint4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = float4, procedure = vint4float4gt, commutator = <= );
CREATE FUNCTION vint4float4ge(vint4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = float4, procedure = vint4float4ge, commutator = < );
CREATE FUNCTION vint4float4eq(vint4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = float4, procedure = vint4float4eq, commutator = <> );
CREATE FUNCTION vint4float4ne(vint4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = float4, procedure = vint4float4ne, commutator = = );
CREATE FUNCTION vint4float4lt(vint4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = float4, procedure = vint4float4lt, commutator = >= );
CREATE FUNCTION vint4float4le(vint4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = float4, procedure = vint4float4le, commutator = > );
CREATE FUNCTION vint4float8gt(vint4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint4, rightarg = float8, procedure = vint4float8gt, commutator = <= );
CREATE FUNCTION vint4float8ge(vint4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint4, rightarg = float8, procedure = vint4float8ge, commutator = < );
CREATE FUNCTION vint4float8eq(vint4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint4, rightarg = float8, procedure = vint4float8eq, commutator = <> );
CREATE FUNCTION vint4float8ne(vint4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint4, rightarg = float8, procedure = vint4float8ne, commutator = = );
CREATE FUNCTION vint4float8lt(vint4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint4, rightarg = float8, procedure = vint4float8lt, commutator = >= );
CREATE FUNCTION vint4float8le(vint4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint4, rightarg = float8, procedure = vint4float8le, commutator = > );

CREATE FUNCTION vint8vint2gt(vint8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = vint2, procedure = vint8vint2gt, commutator = <= );
CREATE FUNCTION vint8vint2ge(vint8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = vint2, procedure = vint8vint2ge, commutator = < );
CREATE FUNCTION vint8vint2eq(vint8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = vint2, procedure = vint8vint2eq, commutator = <> );
CREATE FUNCTION vint8vint2ne(vint8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = vint2, procedure = vint8vint2ne, commutator = = );
CREATE FUNCTION vint8vint2lt(vint8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = vint2, procedure = vint8vint2lt, commutator = >= );
CREATE FUNCTION vint8vint2le(vint8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = vint2, procedure = vint8vint2le, commutator = > );
CREATE FUNCTION vint8vint4gt(vint8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = vint4, procedure = vint8vint4gt, commutator = <= );
CREATE FUNCTION vint8vint4ge(vint8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = vint4, procedure = vint8vint4ge, commutator = < );
CREATE FUNCTION vint8vint4eq(vint8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = vint4, procedure = vint8vint4eq, commutator = <> );
CREATE FUNCTION vint8vint4ne(vint8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = vint4, procedure = vint8vint4ne, commutator = = );
CREATE FUNCTION vint8vint4lt(vint8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = vint4, procedure = vint8vint4lt, commutator = >= );
CREATE FUNCTION vint8vint4le(vint8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = vint4, procedure = vint8vint4le, commutator = > );
CREATE FUNCTION vint8vint8gt(vint8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8gt, commutator = <= );
CREATE FUNCTION vint8vint8ge(vint8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8ge, commutator = < );
CREATE FUNCTION vint8vint8eq(vint8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8eq, commutator = <> );
CREATE FUNCTION vint8vint8ne(vint8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8ne, commutator = = );
CREATE FUNCTION vint8vint8lt(vint8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8lt, commutator = >= );
CREATE FUNCTION vint8vint8le(vint8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8le, commutator = > );
CREATE FUNCTION vint8vfloat4gt(vint8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = vfloat4, procedure = vint8vfloat4gt, commutator = <= );
CREATE FUNCTION vint8vfloat4ge(vint8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = vfloat4, procedure = vint8vfloat4ge, commutator = < );
CREATE FUNCTION vint8vfloat4eq(vint8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = vfloat4, procedure = vint8vfloat4eq, commutator = <> );
CREATE FUNCTION vint8vfloat4ne(vint8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = vfloat4, procedure = vint8vfloat4ne, commutator = = );
CREATE FUNCTION vint8vfloat4lt(vint8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = vfloat4, procedure = vint8vfloat4lt, commutator = >= );
CREATE FUNCTION vint8vfloat4le(vint8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = vfloat4, procedure = vint8vfloat4le, commutator = > );
CREATE FUNCTION vint8vfloat8gt(vint8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = vfloat8, procedure = vint8vfloat8gt, commutator = <= );
CREATE FUNCTION vint8vfloat8ge(vint8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = vfloat8, procedure = vint8vfloat8ge, commutator = < );
CREATE FUNCTION vint8vfloat8eq(vint8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = vfloat8, procedure = vint8vfloat8eq, commutator = <> );
CREATE FUNCTION vint8vfloat8ne(vint8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = vfloat8, procedure = vint8vfloat8ne, commutator = = );
CREATE FUNCTION vint8vfloat8lt(vint8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = vfloat8, procedure = vint8vfloat8lt, commutator = >= );
CREATE FUNCTION vint8vfloat8le(vint8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = vfloat8, procedure = vint8vfloat8le, commutator = > );
CREATE FUNCTION vint8int2gt(vint8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = int2, procedure = vint8int2gt, commutator = <= );
CREATE FUNCTION vint8int2ge(vint8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = int2, procedure = vint8int2ge, commutator = < );
CREATE FUNCTION vint8int2eq(vint8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = int2, procedure = vint8int2eq, commutator = <> );
CREATE FUNCTION vint8int2ne(vint8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = int2, procedure = vint8int2ne, commutator = = );
CREATE FUNCTION vint8int2lt(vint8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = int2, procedure = vint8int2lt, commutator = >= );
CREATE FUNCTION vint8int2le(vint8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = int2, procedure = vint8int2le, commutator = > );
CREATE FUNCTION vint8int4gt(vint8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = int4, procedure = vint8int4gt, commutator = <= );
CREATE FUNCTION vint8int4ge(vint8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = int4, procedure = vint8int4ge, commutator = < );
CREATE FUNCTION vint8int4eq(vint8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = int4, procedure = vint8int4eq, commutator = <> );
CREATE FUNCTION vint8int4ne(vint8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = int4, procedure = vint8int4ne, commutator = = );
CREATE FUNCTION vint8int4lt(vint8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = int4, procedure = vint8int4lt, commutator = >= );
CREATE FUNCTION vint8int4le(vint8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = int4, procedure = vint8int4le, commutator = > );
CREATE FUNCTION vint8int8gt(vint8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = int8, procedure = vint8int8gt, commutator = <= );
CREATE FUNCTION vint8int8ge(vint8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = int8, procedure = vint8int8ge, commutator = < );
CREATE FUNCTION vint8int8eq(vint8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = int8, procedure = vint8int8eq, commutator = <> );
CREATE FUNCTION vint8int8ne(vint8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = int8, procedure = vint8int8ne, commutator = = );
CREATE FUNCTION vint8int8lt(vint8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = int8, procedure = vint8int8lt, commutator = >= );
CREATE FUNCTION vint8int8le(vint8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = int8, procedure = vint8int8le, commutator = > );
CREATE FUNCTION vint8float4gt(vint8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = float4, procedure = vint8float4gt, commutator = <= );
CREATE FUNCTION vint8float4ge(vint8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = float4, procedure = vint8float4ge, commutator = < );
CREATE FUNCTION vint8float4eq(vint8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = float4, procedure = vint8float4eq, commutator = <> );
CREATE FUNCTION vint8float4ne(vint8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = float4, procedure = vint8float4ne, commutator = = );
CREATE FUNCTION vint8float4lt(vint8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = float4, procedure = vint8float4lt, commutator = >= );
CREATE FUNCTION vint8float4le(vint8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = float4, procedure = vint8float4le, commutator = > );
CREATE FUNCTION vint8float8gt(vint8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vint8, rightarg = float8, procedure = vint8float8gt, commutator = <= );
CREATE FUNCTION vint8float8ge(vint8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vint8, rightarg = float8, procedure = vint8float8ge, commutator = < );
CREATE FUNCTION vint8float8eq(vint8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vint8, rightarg = float8, procedure = vint8float8eq, commutator = <> );
CREATE FUNCTION vint8float8ne(vint8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vint8, rightarg = float8, procedure = vint8float8ne, commutator = = );
CREATE FUNCTION vint8float8lt(vint8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vint8, rightarg = float8, procedure = vint8float8lt, commutator = >= );
CREATE FUNCTION vint8float8le(vint8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vint8, rightarg = float8, procedure = vint8float8le, commutator = > );

CREATE FUNCTION vfloat4vint2gt(vfloat4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = vint2, procedure = vfloat4vint2gt, commutator = <= );
CREATE FUNCTION vfloat4vint2ge(vfloat4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = vint2, procedure = vfloat4vint2ge, commutator = < );
CREATE FUNCTION vfloat4vint2eq(vfloat4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = vint2, procedure = vfloat4vint2eq, commutator = <> );
CREATE FUNCTION vfloat4vint2ne(vfloat4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = vint2, procedure = vfloat4vint2ne, commutator = = );
CREATE FUNCTION vfloat4vint2lt(vfloat4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = vint2, procedure = vfloat4vint2lt, commutator = >= );
CREATE FUNCTION vfloat4vint2le(vfloat4, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = vint2, procedure = vfloat4vint2le, commutator = > );
CREATE FUNCTION vfloat4vint4gt(vfloat4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = vint4, procedure = vfloat4vint4gt, commutator = <= );
CREATE FUNCTION vfloat4vint4ge(vfloat4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = vint4, procedure = vfloat4vint4ge, commutator = < );
CREATE FUNCTION vfloat4vint4eq(vfloat4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = vint4, procedure = vfloat4vint4eq, commutator = <> );
CREATE FUNCTION vfloat4vint4ne(vfloat4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = vint4, procedure = vfloat4vint4ne, commutator = = );
CREATE FUNCTION vfloat4vint4lt(vfloat4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = vint4, procedure = vfloat4vint4lt, commutator = >= );
CREATE FUNCTION vfloat4vint4le(vfloat4, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = vint4, procedure = vfloat4vint4le, commutator = > );
CREATE FUNCTION vfloat4vint8gt(vfloat4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = vint8, procedure = vfloat4vint8gt, commutator = <= );
CREATE FUNCTION vfloat4vint8ge(vfloat4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = vint8, procedure = vfloat4vint8ge, commutator = < );
CREATE FUNCTION vfloat4vint8eq(vfloat4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = vint8, procedure = vfloat4vint8eq, commutator = <> );
CREATE FUNCTION vfloat4vint8ne(vfloat4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = vint8, procedure = vfloat4vint8ne, commutator = = );
CREATE FUNCTION vfloat4vint8lt(vfloat4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = vint8, procedure = vfloat4vint8lt, commutator = >= );
CREATE FUNCTION vfloat4vint8le(vfloat4, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = vint8, procedure = vfloat4vint8le, commutator = > );
CREATE FUNCTION vfloat4vfloat4gt(vfloat4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4gt, commutator = <= );
CREATE FUNCTION vfloat4vfloat4ge(vfloat4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4ge, commutator = < );
CREATE FUNCTION vfloat4vfloat4eq(vfloat4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4eq, commutator = <> );
CREATE FUNCTION vfloat4vfloat4ne(vfloat4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4ne, commutator = = );
CREATE FUNCTION vfloat4vfloat4lt(vfloat4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4lt, commutator = >= );
CREATE FUNCTION vfloat4vfloat4le(vfloat4, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4le, commutator = > );
CREATE FUNCTION vfloat4vfloat8gt(vfloat4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = vfloat8, procedure = vfloat4vfloat8gt, commutator = <= );
CREATE FUNCTION vfloat4vfloat8ge(vfloat4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = vfloat8, procedure = vfloat4vfloat8ge, commutator = < );
CREATE FUNCTION vfloat4vfloat8eq(vfloat4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = vfloat8, procedure = vfloat4vfloat8eq, commutator = <> );
CREATE FUNCTION vfloat4vfloat8ne(vfloat4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = vfloat8, procedure = vfloat4vfloat8ne, commutator = = );
CREATE FUNCTION vfloat4vfloat8lt(vfloat4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = vfloat8, procedure = vfloat4vfloat8lt, commutator = >= );
CREATE FUNCTION vfloat4vfloat8le(vfloat4, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = vfloat8, procedure = vfloat4vfloat8le, commutator = > );
CREATE FUNCTION vfloat4int2gt(vfloat4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2gt, commutator = <= );
CREATE FUNCTION vfloat4int2ge(vfloat4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2ge, commutator = < );
CREATE FUNCTION vfloat4int2eq(vfloat4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2eq, commutator = <> );
CREATE FUNCTION vfloat4int2ne(vfloat4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2ne, commutator = = );
CREATE FUNCTION vfloat4int2lt(vfloat4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2lt, commutator = >= );
CREATE FUNCTION vfloat4int2le(vfloat4, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2le, commutator = > );
CREATE FUNCTION vfloat4int4gt(vfloat4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4gt, commutator = <= );
CREATE FUNCTION vfloat4int4ge(vfloat4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4ge, commutator = < );
CREATE FUNCTION vfloat4int4eq(vfloat4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4eq, commutator = <> );
CREATE FUNCTION vfloat4int4ne(vfloat4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4ne, commutator = = );
CREATE FUNCTION vfloat4int4lt(vfloat4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4lt, commutator = >= );
CREATE FUNCTION vfloat4int4le(vfloat4, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4le, commutator = > );
CREATE FUNCTION vfloat4int8gt(vfloat4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8gt, commutator = <= );
CREATE FUNCTION vfloat4int8ge(vfloat4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8ge, commutator = < );
CREATE FUNCTION vfloat4int8eq(vfloat4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8eq, commutator = <> );
CREATE FUNCTION vfloat4int8ne(vfloat4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8ne, commutator = = );
CREATE FUNCTION vfloat4int8lt(vfloat4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8lt, commutator = >= );
CREATE FUNCTION vfloat4int8le(vfloat4, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8le, commutator = > );
CREATE FUNCTION vfloat4float4gt(vfloat4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4gt, commutator = <= );
CREATE FUNCTION vfloat4float4ge(vfloat4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4ge, commutator = < );
CREATE FUNCTION vfloat4float4eq(vfloat4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4eq, commutator = <> );
CREATE FUNCTION vfloat4float4ne(vfloat4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4ne, commutator = = );
CREATE FUNCTION vfloat4float4lt(vfloat4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4lt, commutator = >= );
CREATE FUNCTION vfloat4float4le(vfloat4, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4le, commutator = > );
CREATE FUNCTION vfloat4float8gt(vfloat4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8gt, commutator = <= );
CREATE FUNCTION vfloat4float8ge(vfloat4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8ge, commutator = < );
CREATE FUNCTION vfloat4float8eq(vfloat4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8eq, commutator = <> );
CREATE FUNCTION vfloat4float8ne(vfloat4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8ne, commutator = = );
CREATE FUNCTION vfloat4float8lt(vfloat4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8lt, commutator = >= );
CREATE FUNCTION vfloat4float8le(vfloat4, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8le, commutator = > );

CREATE FUNCTION vfloat8vint2gt(vfloat8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = vint2, procedure = vfloat8vint2gt, commutator = <= );
CREATE FUNCTION vfloat8vint2ge(vfloat8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = vint2, procedure = vfloat8vint2ge, commutator = < );
CREATE FUNCTION vfloat8vint2eq(vfloat8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = vint2, procedure = vfloat8vint2eq, commutator = <> );
CREATE FUNCTION vfloat8vint2ne(vfloat8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = vint2, procedure = vfloat8vint2ne, commutator = = );
CREATE FUNCTION vfloat8vint2lt(vfloat8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = vint2, procedure = vfloat8vint2lt, commutator = >= );
CREATE FUNCTION vfloat8vint2le(vfloat8, vint2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = vint2, procedure = vfloat8vint2le, commutator = > );
CREATE FUNCTION vfloat8vint4gt(vfloat8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = vint4, procedure = vfloat8vint4gt, commutator = <= );
CREATE FUNCTION vfloat8vint4ge(vfloat8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = vint4, procedure = vfloat8vint4ge, commutator = < );
CREATE FUNCTION vfloat8vint4eq(vfloat8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = vint4, procedure = vfloat8vint4eq, commutator = <> );
CREATE FUNCTION vfloat8vint4ne(vfloat8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = vint4, procedure = vfloat8vint4ne, commutator = = );
CREATE FUNCTION vfloat8vint4lt(vfloat8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = vint4, procedure = vfloat8vint4lt, commutator = >= );
CREATE FUNCTION vfloat8vint4le(vfloat8, vint4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = vint4, procedure = vfloat8vint4le, commutator = > );
CREATE FUNCTION vfloat8vint8gt(vfloat8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = vint8, procedure = vfloat8vint8gt, commutator = <= );
CREATE FUNCTION vfloat8vint8ge(vfloat8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = vint8, procedure = vfloat8vint8ge, commutator = < );
CREATE FUNCTION vfloat8vint8eq(vfloat8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = vint8, procedure = vfloat8vint8eq, commutator = <> );
CREATE FUNCTION vfloat8vint8ne(vfloat8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = vint8, procedure = vfloat8vint8ne, commutator = = );
CREATE FUNCTION vfloat8vint8lt(vfloat8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = vint8, procedure = vfloat8vint8lt, commutator = >= );
CREATE FUNCTION vfloat8vint8le(vfloat8, vint8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = vint8, procedure = vfloat8vint8le, commutator = > );
CREATE FUNCTION vfloat8vfloat4gt(vfloat8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = vfloat4, procedure = vfloat8vfloat4gt, commutator = <= );
CREATE FUNCTION vfloat8vfloat4ge(vfloat8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = vfloat4, procedure = vfloat8vfloat4ge, commutator = < );
CREATE FUNCTION vfloat8vfloat4eq(vfloat8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = vfloat4, procedure = vfloat8vfloat4eq, commutator = <> );
CREATE FUNCTION vfloat8vfloat4ne(vfloat8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = vfloat4, procedure = vfloat8vfloat4ne, commutator = = );
CREATE FUNCTION vfloat8vfloat4lt(vfloat8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = vfloat4, procedure = vfloat8vfloat4lt, commutator = >= );
CREATE FUNCTION vfloat8vfloat4le(vfloat8, vfloat4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = vfloat4, procedure = vfloat8vfloat4le, commutator = > );
CREATE FUNCTION vfloat8vfloat8gt(vfloat8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8gt, commutator = <= );
CREATE FUNCTION vfloat8vfloat8ge(vfloat8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8ge, commutator = < );
CREATE FUNCTION vfloat8vfloat8eq(vfloat8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8eq, commutator = <> );
CREATE FUNCTION vfloat8vfloat8ne(vfloat8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8ne, commutator = = );
CREATE FUNCTION vfloat8vfloat8lt(vfloat8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8lt, commutator = >= );
CREATE FUNCTION vfloat8vfloat8le(vfloat8, vfloat8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8le, commutator = > );
CREATE FUNCTION vfloat8int2gt(vfloat8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2gt, commutator = <= );
CREATE FUNCTION vfloat8int2ge(vfloat8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2ge, commutator = < );
CREATE FUNCTION vfloat8int2eq(vfloat8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2eq, commutator = <> );
CREATE FUNCTION vfloat8int2ne(vfloat8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2ne, commutator = = );
CREATE FUNCTION vfloat8int2lt(vfloat8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2lt, commutator = >= );
CREATE FUNCTION vfloat8int2le(vfloat8, int2) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2le, commutator = > );
CREATE FUNCTION vfloat8int4gt(vfloat8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4gt, commutator = <= );
CREATE FUNCTION vfloat8int4ge(vfloat8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4ge, commutator = < );
CREATE FUNCTION vfloat8int4eq(vfloat8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4eq, commutator = <> );
CREATE FUNCTION vfloat8int4ne(vfloat8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4ne, commutator = = );
CREATE FUNCTION vfloat8int4lt(vfloat8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4lt, commutator = >= );
CREATE FUNCTION vfloat8int4le(vfloat8, int4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4le, commutator = > );
CREATE FUNCTION vfloat8int8gt(vfloat8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8gt, commutator = <= );
CREATE FUNCTION vfloat8int8ge(vfloat8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8ge, commutator = < );
CREATE FUNCTION vfloat8int8eq(vfloat8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8eq, commutator = <> );
CREATE FUNCTION vfloat8int8ne(vfloat8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8ne, commutator = = );
CREATE FUNCTION vfloat8int8lt(vfloat8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8lt, commutator = >= );
CREATE FUNCTION vfloat8int8le(vfloat8, int8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8le, commutator = > );
CREATE FUNCTION vfloat8float4gt(vfloat8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4gt, commutator = <= );
CREATE FUNCTION vfloat8float4ge(vfloat8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4ge, commutator = < );
CREATE FUNCTION vfloat8float4eq(vfloat8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4eq, commutator = <> );
CREATE FUNCTION vfloat8float4ne(vfloat8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4ne, commutator = = );
CREATE FUNCTION vfloat8float4lt(vfloat8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4lt, commutator = >= );
CREATE FUNCTION vfloat8float4le(vfloat8, float4) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4le, commutator = > );
CREATE FUNCTION vfloat8float8gt(vfloat8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8gt, commutator = <= );
CREATE FUNCTION vfloat8float8ge(vfloat8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8ge, commutator = < );
CREATE FUNCTION vfloat8float8eq(vfloat8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8eq, commutator = <> );
CREATE FUNCTION vfloat8float8ne(vfloat8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8ne, commutator = = );
CREATE FUNCTION vfloat8float8lt(vfloat8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8lt, commutator = >= );
CREATE FUNCTION vfloat8float8le(vfloat8, float8) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8le, commutator = > );

/* text */
CREATE FUNCTION vtext_ne_const(vtext, text) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <> ( leftarg = vtext, rightarg = text, procedure = vtext_ne_const, commutator = = );

CREATE FUNCTION vint2vint2pl(vint2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2pl, commutator = - );
CREATE FUNCTION vint2vint2mi(vint2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2mi, commutator = + );
CREATE FUNCTION vint2vint2mul(vint2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2mul, commutator = / );
CREATE FUNCTION vint2vint2div(vint2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint2, rightarg = vint2, procedure = vint2vint2div, commutator = * );

CREATE FUNCTION vint4vint4pl(vint4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4pl, commutator = - );
CREATE FUNCTION vint4vint4mi(vint4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4mi, commutator = + );
CREATE FUNCTION vint4vint4mul(vint4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4mul, commutator = / );
CREATE FUNCTION vint4vint4div(vint4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint4, rightarg = vint4, procedure = vint4vint4div, commutator = * );

CREATE FUNCTION vint8vint8pl(vint8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8pl, commutator = - );
CREATE FUNCTION vint8vint8mi(vint8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8mi, commutator = + );
CREATE FUNCTION vint8vint8mul(vint8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8mul, commutator = / );
CREATE FUNCTION vint8vint8div(vint8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint8, rightarg = vint8, procedure = vint8vint8div, commutator = * );


CREATE FUNCTION vfloat4vfloat4pl(vfloat4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4pl, commutator = - );
CREATE FUNCTION vfloat4vfloat4mi(vfloat4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4mi, commutator = + );
CREATE FUNCTION vfloat4vfloat4mul(vfloat4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4mul, commutator = / );
CREATE FUNCTION vfloat4vfloat4div(vfloat4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat4, rightarg = vfloat4, procedure = vfloat4vfloat4div, commutator = * );



CREATE FUNCTION vfloat8vfloat8pl(vfloat8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8pl, commutator = - );
CREATE FUNCTION vfloat8vfloat8mi(vfloat8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8mi, commutator = + );
CREATE FUNCTION vfloat8vfloat8mul2(vfloat8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8mul2, commutator = / );
CREATE FUNCTION vfloat8vfloat8div(vfloat8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat8, rightarg = vfloat8, procedure = vfloat8vfloat8div, commutator = * );





CREATE FUNCTION vint2int2pl(vint2, int2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint2, rightarg = int2, procedure = vint2int2pl, commutator = - );
CREATE FUNCTION vint2int2mi(vint2, int2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint2, rightarg = int2, procedure = vint2int2mi, commutator = + );
CREATE FUNCTION vint2int2mul(vint2, int2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint2, rightarg = int2, procedure = vint2int2mul, commutator = / );
CREATE FUNCTION vint2int2div(vint2, int2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint2, rightarg = int2, procedure = vint2int2div, commutator = * );
--CREATE FUNCTION vint2int4pl(vint2, int4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
--CREATE OPERATOR + ( leftarg = vint2, rightarg = int4, procedure = vint2int4pl, commutator = - );
CREATE FUNCTION vint2int4pl_u(vint2, int4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint2, rightarg = int4, procedure = vint2int4pl_u, commutator = - );
CREATE FUNCTION vint2int4mi(vint2, int4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint2, rightarg = int4, procedure = vint2int4mi, commutator = + );
CREATE FUNCTION vint2int4mul(vint2, int4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint2, rightarg = int4, procedure = vint2int4mul, commutator = / );
CREATE FUNCTION vint2int4div(vint2, int4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint2, rightarg = int4, procedure = vint2int4div, commutator = * );
CREATE FUNCTION vint2int8pl(vint2, int8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint2, rightarg = int8, procedure = vint2int8pl, commutator = - );
CREATE FUNCTION vint2int8mi(vint2, int8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint2, rightarg = int8, procedure = vint2int8mi, commutator = + );
CREATE FUNCTION vint2int8mul(vint2, int8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint2, rightarg = int8, procedure = vint2int8mul, commutator = / );
CREATE FUNCTION vint2int8div(vint2, int8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint2, rightarg = int8, procedure = vint2int8div, commutator = * );
CREATE FUNCTION vint2float4pl(vint2, float4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint2, rightarg = float4, procedure = vint2float4pl, commutator = - );
CREATE FUNCTION vint2float4mi(vint2, float4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint2, rightarg = float4, procedure = vint2float4mi, commutator = + );
CREATE FUNCTION vint2float4mul(vint2, float4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint2, rightarg = float4, procedure = vint2float4mul, commutator = / );
CREATE FUNCTION vint2float4div(vint2, float4) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint2, rightarg = float4, procedure = vint2float4div, commutator = * );
CREATE FUNCTION vint2float8pl(vint2, float8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint2, rightarg = float8, procedure = vint2float8pl, commutator = - );
CREATE FUNCTION vint2float8mi(vint2, float8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint2, rightarg = float8, procedure = vint2float8mi, commutator = + );
CREATE FUNCTION vint2float8mul(vint2, float8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint2, rightarg = float8, procedure = vint2float8mul, commutator = / );
CREATE FUNCTION vint2float8div(vint2, float8) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint2, rightarg = float8, procedure = vint2float8div, commutator = * );

CREATE FUNCTION vint4int2pl(vint4, int2) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint4, rightarg = int2, procedure = vint4int2pl, commutator = - );
CREATE FUNCTION vint4int2mi(vint4, int2) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint4, rightarg = int2, procedure = vint4int2mi, commutator = + );
CREATE FUNCTION vint4int2mul(vint4, int2) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint4, rightarg = int2, procedure = vint4int2mul, commutator = / );
CREATE FUNCTION vint4int2div(vint4, int2) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint4, rightarg = int2, procedure = vint4int2div, commutator = * );
CREATE FUNCTION vint4int4pl(vint4, int4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint4, rightarg = int4, procedure = vint4int4pl, commutator = - );
CREATE FUNCTION vint4int4mi(vint4, int4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint4, rightarg = int4, procedure = vint4int4mi, commutator = + );
CREATE FUNCTION vint4int4mul(vint4, int4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint4, rightarg = int4, procedure = vint4int4mul, commutator = / );
CREATE FUNCTION vint4int4div(vint4, int4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint4, rightarg = int4, procedure = vint4int4div, commutator = * );
CREATE FUNCTION vint4int8pl(vint4, int8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint4, rightarg = int8, procedure = vint4int8pl, commutator = - );
CREATE FUNCTION vint4int8mi(vint4, int8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint4, rightarg = int8, procedure = vint4int8mi, commutator = + );
CREATE FUNCTION vint4int8mul(vint4, int8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint4, rightarg = int8, procedure = vint4int8mul, commutator = / );
CREATE FUNCTION vint4int8div(vint4, int8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint4, rightarg = int8, procedure = vint4int8div, commutator = * );
CREATE FUNCTION vint4float4pl(vint4, float4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint4, rightarg = float4, procedure = vint4float4pl, commutator = - );
CREATE FUNCTION vint4float4mi(vint4, float4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint4, rightarg = float4, procedure = vint4float4mi, commutator = + );
CREATE FUNCTION vint4float4mul(vint4, float4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint4, rightarg = float4, procedure = vint4float4mul, commutator = / );
CREATE FUNCTION vint4float4div(vint4, float4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint4, rightarg = float4, procedure = vint4float4div, commutator = * );
CREATE FUNCTION vint4float8pl(vint4, float8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint4, rightarg = float8, procedure = vint4float8pl, commutator = - );
CREATE FUNCTION vint4float8mi(vint4, float8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint4, rightarg = float8, procedure = vint4float8mi, commutator = + );
CREATE FUNCTION vint4float8mul(vint4, float8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint4, rightarg = float8, procedure = vint4float8mul, commutator = / );
CREATE FUNCTION vint4float8div(vint4, float8) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint4, rightarg = float8, procedure = vint4float8div, commutator = * );

CREATE FUNCTION vint8int2pl(vint8, int2) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint8, rightarg = int2, procedure = vint8int2pl, commutator = - );
CREATE FUNCTION vint8int2mi(vint8, int2) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint8, rightarg = int2, procedure = vint8int2mi, commutator = + );
CREATE FUNCTION vint8int2mul(vint8, int2) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint8, rightarg = int2, procedure = vint8int2mul, commutator = / );
CREATE FUNCTION vint8int2div(vint8, int2) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint8, rightarg = int2, procedure = vint8int2div, commutator = * );
CREATE FUNCTION vint8int4pl(vint8, int4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint8, rightarg = int4, procedure = vint8int4pl, commutator = - );
CREATE FUNCTION vint8int4mi(vint8, int4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint8, rightarg = int4, procedure = vint8int4mi, commutator = + );
CREATE FUNCTION vint8int4mul(vint8, int4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint8, rightarg = int4, procedure = vint8int4mul, commutator = / );
CREATE FUNCTION vint8int4div(vint8, int4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint8, rightarg = int4, procedure = vint8int4div, commutator = * );
CREATE FUNCTION vint8int8pl(vint8, int8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint8, rightarg = int8, procedure = vint8int8pl, commutator = - );
CREATE FUNCTION vint8int8mi(vint8, int8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint8, rightarg = int8, procedure = vint8int8mi, commutator = + );
CREATE FUNCTION vint8int8mul(vint8, int8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint8, rightarg = int8, procedure = vint8int8mul, commutator = / );
CREATE FUNCTION vint8int8div(vint8, int8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint8, rightarg = int8, procedure = vint8int8div, commutator = * );
CREATE FUNCTION vint8float4pl(vint8, float4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint8, rightarg = float4, procedure = vint8float4pl, commutator = - );
CREATE FUNCTION vint8float4mi(vint8, float4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint8, rightarg = float4, procedure = vint8float4mi, commutator = + );
CREATE FUNCTION vint8float4mul(vint8, float4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint8, rightarg = float4, procedure = vint8float4mul, commutator = / );
CREATE FUNCTION vint8float4div(vint8, float4) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint8, rightarg = float4, procedure = vint8float4div, commutator = * );
CREATE FUNCTION vint8float8pl(vint8, float8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vint8, rightarg = float8, procedure = vint8float8pl, commutator = - );
CREATE FUNCTION vint8float8mi(vint8, float8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vint8, rightarg = float8, procedure = vint8float8mi, commutator = + );
CREATE FUNCTION vint8float8mul(vint8, float8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vint8, rightarg = float8, procedure = vint8float8mul, commutator = / );
CREATE FUNCTION vint8float8div(vint8, float8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vint8, rightarg = float8, procedure = vint8float8div, commutator = * );



CREATE FUNCTION vfloat4int2pl(vfloat4, int2) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2pl, commutator = - );
CREATE FUNCTION vfloat4int2mi(vfloat4, int2) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2mi, commutator = + );
CREATE FUNCTION vfloat4int2mul(vfloat4, int2) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2mul, commutator = / );
CREATE FUNCTION vfloat4int2div(vfloat4, int2) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat4, rightarg = int2, procedure = vfloat4int2div, commutator = * );
CREATE FUNCTION vfloat4int4pl(vfloat4, int4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4pl, commutator = - );
CREATE FUNCTION vfloat4int4mi(vfloat4, int4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4mi, commutator = + );
CREATE FUNCTION vfloat4int4mul(vfloat4, int4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4mul, commutator = / );
CREATE FUNCTION vfloat4int4div(vfloat4, int4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat4, rightarg = int4, procedure = vfloat4int4div, commutator = * );
CREATE FUNCTION vfloat4int8pl(vfloat4, int8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8pl, commutator = - );
CREATE FUNCTION vfloat4int8mi(vfloat4, int8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8mi, commutator = + );
CREATE FUNCTION vfloat4int8mul(vfloat4, int8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8mul, commutator = / );
CREATE FUNCTION vfloat4int8div(vfloat4, int8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat4, rightarg = int8, procedure = vfloat4int8div, commutator = * );
CREATE FUNCTION vfloat4float4pl(vfloat4, float4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4pl, commutator = - );
CREATE FUNCTION vfloat4float4mi(vfloat4, float4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4mi, commutator = + );
CREATE FUNCTION vfloat4float4mul(vfloat4, float4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4mul, commutator = / );
CREATE FUNCTION vfloat4float4div(vfloat4, float4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat4, rightarg = float4, procedure = vfloat4float4div, commutator = * );
CREATE FUNCTION vfloat4float8pl(vfloat4, float8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8pl, commutator = - );
CREATE FUNCTION vfloat4float8mi(vfloat4, float8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8mi, commutator = + );
CREATE FUNCTION vfloat4float8mul(vfloat4, float8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8mul, commutator = / );
CREATE FUNCTION vfloat4float8div(vfloat4, float8) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat4, rightarg = float8, procedure = vfloat4float8div, commutator = * );




CREATE FUNCTION vfloat8int2pl(vfloat8, int2) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2pl, commutator = - );
CREATE FUNCTION vfloat8int2mi(vfloat8, int2) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2mi, commutator = + );
CREATE FUNCTION vfloat8int2mul(vfloat8, int2) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2mul, commutator = / );
CREATE FUNCTION vfloat8int2div(vfloat8, int2) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat8, rightarg = int2, procedure = vfloat8int2div, commutator = * );
CREATE FUNCTION vfloat8int4pl(vfloat8, int4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4pl, commutator = - );
CREATE FUNCTION vfloat8int4mi(vfloat8, int4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4mi, commutator = + );
CREATE FUNCTION vfloat8int4mul(vfloat8, int4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4mul, commutator = / );
CREATE FUNCTION vfloat8int4div(vfloat8, int4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat8, rightarg = int4, procedure = vfloat8int4div, commutator = * );
CREATE FUNCTION vfloat8int8pl(vfloat8, int8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8pl, commutator = - );
CREATE FUNCTION vfloat8int8mi(vfloat8, int8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8mi, commutator = + );
CREATE FUNCTION vfloat8int8mul(vfloat8, int8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8mul, commutator = / );
CREATE FUNCTION vfloat8int8div(vfloat8, int8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat8, rightarg = int8, procedure = vfloat8int8div, commutator = * );
CREATE FUNCTION vfloat8float4pl(vfloat8, float4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4pl, commutator = - );
CREATE FUNCTION vfloat8float4mi(vfloat8, float4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4mi, commutator = + );
CREATE FUNCTION vfloat8float4mul(vfloat8, float4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4mul, commutator = / );
CREATE FUNCTION vfloat8float4div(vfloat8, float4) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat8, rightarg = float4, procedure = vfloat8float4div, commutator = * );
CREATE FUNCTION vfloat8float8pl(vfloat8, float8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8pl, commutator = - );
CREATE FUNCTION vfloat8float8mi(vfloat8, float8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8mi, commutator = + );
CREATE FUNCTION vfloat8float8mul(vfloat8, float8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8mul, commutator = / );
CREATE FUNCTION vfloat8float8div(vfloat8, float8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = vfloat8, rightarg = float8, procedure = vfloat8float8div, commutator = * );

CREATE FUNCTION int2vint2pl(int2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = int2, rightarg = vint2, procedure = int2vint2pl, commutator = - );
CREATE FUNCTION int2vint2mi(int2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = int2, rightarg = vint2, procedure = int2vint2mi, commutator = + );
CREATE FUNCTION int2vint2mul(int2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = int2, rightarg = vint2, procedure = int2vint2mul, commutator = / );
CREATE FUNCTION int2vint2div(int2, vint2) RETURNS vint2 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = int2, rightarg = vint2, procedure = int2vint2div, commutator = * );

CREATE FUNCTION int4vint4pl(int4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = int4, rightarg = vint4, procedure = int4vint4pl, commutator = - );
CREATE FUNCTION int4vint4mi(int4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = int4, rightarg = vint4, procedure = int4vint4mi, commutator = + );
CREATE FUNCTION int4vint4mul(int4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = int4, rightarg = vint4, procedure = int4vint4mul, commutator = / );
CREATE FUNCTION int4vint4div(int4, vint4) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = int4, rightarg = vint4, procedure = int4vint4div, commutator = * );

CREATE FUNCTION int8vint8pl(int8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = int8, rightarg = vint8, procedure = int8vint8pl, commutator = - );
CREATE FUNCTION int8vint8mi(int8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = int8, rightarg = vint8, procedure = int8vint8mi, commutator = + );
CREATE FUNCTION int8vint8mul(int8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = int8, rightarg = vint8, procedure = int8vint8mul, commutator = / );
CREATE FUNCTION int8vint8div(int8, vint8) RETURNS vint8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = int8, rightarg = vint8, procedure = int8vint8div, commutator = * );

CREATE FUNCTION float4vfloat4pl(float4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = float4, rightarg = vfloat4, procedure = float4vfloat4pl, commutator = - );
CREATE FUNCTION float4vfloat4mi(float4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = float4, rightarg = vfloat4, procedure = float4vfloat4mi, commutator = + );
CREATE FUNCTION float4vfloat4mul(float4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = float4, rightarg = vfloat4, procedure = float4vfloat4mul, commutator = / );
CREATE FUNCTION float4vfloat4div(float4, vfloat4) RETURNS vfloat4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = float4, rightarg = vfloat4, procedure = float4vfloat4div, commutator = * );


CREATE FUNCTION float8vfloat8pl(float8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR + ( leftarg = float8, rightarg = vfloat8, procedure = float8vfloat8pl, commutator = - );
CREATE FUNCTION float8vfloat8mi(float8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = float8, rightarg = vfloat8, procedure = float8vfloat8mi, commutator = + );
CREATE FUNCTION float8vfloat8mul(float8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR * ( leftarg = float8, rightarg = vfloat8, procedure = float8vfloat8mul, commutator = / );
CREATE FUNCTION float8vfloat8div(float8, vfloat8) RETURNS vfloat8 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR / ( leftarg = float8, rightarg = vfloat8, procedure = float8vfloat8div, commutator = * );

CREATE FUNCTION vdate_le_timestamp(vdate, timestamp) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vdate, rightarg = timestamp, procedure = vdate_le_timestamp, commutator = <= );
CREATE FUNCTION vdate_mi_interval(vdate, interval) RETURNS vtimestamp AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR - ( leftarg = vdate, rightarg = interval, procedure = vdate_mi_interval, commutator = - );
CREATE FUNCTION vdate_le(vdate, date) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= ( leftarg = vdate, rightarg = date, procedure = vdate_le, commutator = <= );
CREATE FUNCTION vdate_ge_const(vdate, date) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= ( leftarg = vdate, rightarg = date, procedure = vdate_ge_const, commutator = >= );

CREATE FUNCTION vextract_time(text, vtimestamp) RETURNS vnumeric AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vtext_length(vtext) RETURNS vint4 AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION vtext_like_const(vtext, text) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR ~~ ( leftarg = vtext, rightarg = text, procedure = vtext_like_const, commutator = !~~ );

CREATE FUNCTION vtext_nlike_const(vtext, text) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR !~~ ( leftarg = vtext, rightarg = text, procedure = vtext_nlike_const, commutator = ~~ );

CREATE FUNCTION vtextregexreplace_noopt(vtext, text, text) RETURNS vtext AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION vtextregexreplace(vtext, text, text, text) RETURNS vtext AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION vtimestamp_trunc(text, vtimestamp) RETURNS vtimestamp AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;

--create count aggregate functions

CREATE FUNCTION vint8inc_any(int8, vany) returns int8 as '$libdir/gammadb' language c immutable;
create AGGREGATE count(vany) ( 
	sfunc = vint8inc_any, 
	INITCOND = '0',
	stype = int8);

CREATE FUNCTION vint4_sum(int8, vint4) returns int8 as '$libdir/gammadb' language c immutable;
CREATE AGGREGATE sum(vint4) ( 
	sfunc = vint4_sum,
	combinefunc = int8pl,
	stype = int8);

CREATE FUNCTION vint2_sum(int8, vint2) returns int8 as '$libdir/gammadb' language c immutable;
CREATE AGGREGATE sum(vint2) ( 
	sfunc = vint2_sum,
	combinefunc = int8pl,
	stype = int8);

CREATE FUNCTION vfloat8pl(float8, vfloat8) returns float8 as '$libdir/gammadb' language c immutable;
create AGGREGATE sum(vfloat8) ( 
	sfunc = vfloat8pl, 
	stype = float8);

CREATE FUNCTION vfloat8_accum(_float8, vfloat8) returns _float8 as '$libdir/gammadb' language c immutable strict;
--CREATE FUNCTION vfloat8_avg(_float8) returns float8 as '$libdir/gammadb' language c immutable;
create AGGREGATE avg(vfloat8) ( 
	sfunc = vfloat8_accum, 
	finalfunc = float8_avg, 
	INITCOND = '{0,0,0}',
	stype = _float8);

CREATE FUNCTION vint8_avg_accum(internal, vint8) returns internal as '$libdir/gammadb' language c immutable;
CREATE AGGREGATE sum(vint8) ( 
	sfunc = vint8_avg_accum,
	finalfunc = numeric_poly_sum,
	combinefunc = int8_avg_combine,
	serialfunc = int8_avg_serialize,
	deserialfunc = int8_avg_deserialize,
	sspace = 48,
	stype = internal);

CREATE FUNCTION vint2_avg_accum(_int8, vint2) returns _int8 as '$libdir/gammadb' language c immutable;
CREATE FUNCTION vint4_avg_accum(_int8, vint4) returns _int8 as '$libdir/gammadb' language c immutable;
CREATE AGGREGATE avg(vint2) (
	sfunc = vint2_avg_accum,
	finalfunc = int8_avg,
	combinefunc = int4_avg_combine,
	initcond = '{0,0}',
	stype = _int8);
CREATE AGGREGATE avg(vint4) (
	sfunc = vint4_avg_accum,
	finalfunc = int8_avg,
	combinefunc = int4_avg_combine,
	initcond = '{0,0}',
	stype = _int8);
CREATE AGGREGATE avg(vint8) (
	sfunc = vint8_avg_accum,
	finalfunc = numeric_poly_avg,
	combinefunc = int8_avg_combine,
	serialfunc = int8_avg_serialize,
	deserialfunc = int8_avg_deserialize,
	sspace = 48,
	stype = internal);

-- min/max aggregate functions
CREATE FUNCTION vdate_larger(date, vdate) returns date as '$libdir/gammadb' language c immutable;
CREATE FUNCTION vdate_smaller(date, vdate) returns date as '$libdir/gammadb' language c immutable;
CREATE FUNCTION vtext_larger(text, vtext) returns text as '$libdir/gammadb' language c immutable;
CREATE FUNCTION vtext_smaller(text, vtext) returns text as '$libdir/gammadb' language c immutable;

CREATE AGGREGATE max(vdate) (
	sfunc = vdate_larger,
	combinefunc = date_larger,
	--sortop = '>(date,date)',
	--initcond = '1',
	stype = date);

CREATE AGGREGATE min(vdate) (
	sfunc = vdate_smaller,
	combinefunc = date_smaller,
	--sortop = '<(date,date)',
	initcond = '5874897-12-31',
	stype = date);

CREATE AGGREGATE max(vtext) (
	sfunc = vtext_larger,
	combinefunc = text_larger,
	stype = text);

CREATE AGGREGATE min(vtext) (
	sfunc = vtext_smaller,
	combinefunc = text_smaller,
	stype = text);

CREATE OR REPLACE FUNCTION gamma_count_distinct_transition(internal, anyelement)
RETURNS internal
AS '$libdir/gammadb'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION gamma_count_distinct_final(internal)
RETURNS bigint
AS '$libdir/gammadb'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION gamma_count_distinct_serial(p_pointer internal)
RETURNS bytea
AS '$libdir/gammadb'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gamma_count_distinct_deserial(p_value bytea, p_dummy internal)
RETURNS internal
AS '$libdir/gammadb'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gamma_count_distinct_combine(p_state_1 internal, p_state_2 internal)
RETURNS internal
AS '$libdir/gammadb'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE count_distinct(anyelement) (
	SFUNC = gamma_count_distinct_transition,
	STYPE = internal,
	FINALFUNC = gamma_count_distinct_final,
	COMBINEFUNC = gamma_count_distinct_combine,
	SERIALFUNC = gamma_count_distinct_serial,
	DESERIALFUNC = gamma_count_distinct_deserial,
	PARALLEL = SAFE
);

-- Replacing BoolExpr in vectorized mode
CREATE FUNCTION gamma_vec_bool_expr_and(VARIADIC vbool[]) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION gamma_vec_bool_expr_or(VARIADIC vbool[]) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION gamma_vec_bool_expr_not(vbool) RETURNS vbool AS '$libdir/gammadb' LANGUAGE C IMMUTABLE STRICT;

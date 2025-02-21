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

#include "fmgr.h"
#include "optimizer/planner.h"
#include "executor/nodeCustom.h"
#include "utils/guc.h"

#include "executor/gamma_copy.h"
#include "executor/gamma_vec_agg.h"
#include "executor/gamma_devectorize.h"
#include "executor/gamma_indexscan.h"
#include "executor/gamma_indexonlyscan.h"
#include "executor/gamma_vec_result.h"
#include "executor/gamma_vec_sort.h"
#include "executor/gamma_vec_tablescan.h"
#include "executor/vector_tuple_slot.h"
#include "optimizer/gamma_paths.h"
#include "storage/gamma_buffer.h"
#include "storage/gamma_rg.h"
#include "tcop/gamma_utility.h"
#include "utils/gamma_cache.h"
#include "utils/nodes/gamma_nodes.h"

#ifdef _GAMMAX_
#include "executor/gamma_vec_hashjoin.h"
#include "executor/gamma_colindex_scan.h"
#endif

#ifdef _DBINAI_
#include "aidb/gamma_copilot.h"
#endif

PG_MODULE_MAGIC;

extern bool					enable_gammadb;
extern bool					enable_gammadb_notice;

extern double				gammadb_delta_table_factor;
extern int					gammadb_delta_table_nblocks;
extern bool					gammadb_delta_table_merge_all;
extern int					gammadb_buffers;

extern double				gammadb_stats_analyze_tuple_factor;


extern int					gammadb_cv_compress_method;
extern bool					gammadb_storage_skip_keys;

extern bool					gammadb_rewrite_grouping_const;
extern bool					gammadb_rewrite_simplify_grouping;
extern bool					gammadb_rewrite_extract_agg;

void _PG_init(void);

static void
gamma_guc_init(void)
{
	DefineCustomBoolVariable("enable_gammadb",
							 "Enables gammadb engine.",
							 NULL,
							 &enable_gammadb,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);

	DefineCustomBoolVariable("enable_gammadb_notice",
							 "Enables gammadb notice engine.",
							 NULL,
							 &enable_gammadb_notice,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomRealVariable("gammadb_delta_table_factor",
							 "vacuum factor for delta table",
							 NULL,
							 &gammadb_delta_table_factor,
							 0.5,
							 0.0,
							 1.0,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomIntVariable("gammadb_delta_table_nblocks",
							"max gammadb block counts",
							NULL,
							&gammadb_delta_table_nblocks,
							134217728,
							5,
							INT_MAX,
							PGC_USERSET,
							GUC_NOT_IN_SAMPLE,
							NULL, NULL, NULL);
	DefineCustomIntVariable("gammadb_buffers",
							"buffer size for gamma tables",
							NULL,
							&gammadb_buffers,
							1024,
							0,
							INT_MAX,
							PGC_USERSET,
							GUC_NOT_IN_SAMPLE | GUC_UNIT_MB,
							NULL, NULL, NULL);
	DefineCustomBoolVariable("gammadb_delta_table_merge_all",
							 "Merging all rows to column store.",
							 NULL,
							 &gammadb_delta_table_merge_all,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomRealVariable("gammadb_stats_analyze_tuple_factor",
							 "#count of sampling tuples",
							 NULL,
							 &gammadb_stats_analyze_tuple_factor,
							 0.01,
							 0.0,
							 1.0,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomIntVariable("gammadb_cv_compress_method",
							"none/pglz/lz4 for column vector",
							NULL,
							&gammadb_cv_compress_method,
							1, /* GAMMA_CV_COMPRESS_PGLZ*/
							0,
							INT_MAX,
							PGC_USERSET,
							GUC_NOT_IN_SAMPLE,
							NULL, NULL, NULL);
	DefineCustomBoolVariable("gammadb_copy_to_cvtable",
							 "Copy data to cvtable directly.",
							 NULL,
							 &gammadb_copy_to_cvtable,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("gammadb_rewrite_grouping_const",
							 "Remove const in Grouping Clause.",
							 NULL,
							 &gammadb_rewrite_grouping_const,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("gammadb_rewrite_simplify_grouping",
							 "Simplify Grouping Clause.",
							 NULL,
							 &gammadb_rewrite_simplify_grouping,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("gammadb_rewrite_extract_agg",
							 "sum(c1+1) => sum(c1) + 1 * count(*)",
							 NULL,
							 &gammadb_rewrite_extract_agg,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("gammadb_storage_skip_keys",
							 "gammadb skip keys",
							 NULL,
							 &gammadb_storage_skip_keys,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
}

void
_PG_init(void)
{
	elog(LOG, "GammaDB");

	/* Init GUCs */
	gamma_guc_init();

	/* Init Extensible nodes*/
	gamma_register_nodes();

	/* Init the custom path functions */
	gamma_path_scan_methods();
	gamma_path_join_methods();
	gamma_path_upper_methods();
	gamma_path_planner_methods();

	/* Initialize gamma buffers for row group */
	gamma_buffer_startup();

	/* Initialize the Vector Tuple Slot ops */
	ttsops_vector_init();

	/* cache some oid for performance */
	gamma_cache_startup();

	/* Register vectorized execution nodes */
	gamma_vec_tablescan_init();
	gamma_vec_devector_init();
	gamma_vec_agg_init();
	gamma_vec_result_init();
	gamma_vec_sort_init();
	gamma_indexscan_init();
	gamma_indexonlyscan_init();

	/* do some utility works */
	gamma_utility_startup();

#ifdef _GAMMAX_
	gamma_colindex_scan_init();
	gamma_vec_hashjoin_init();
#endif

#ifdef _DBINAI_
	gamma_init_copilot();
#endif
}

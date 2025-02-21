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

#ifndef GAMMA_SCANKEYS_H
#define GAMMA_SCANKEYS_H

#include "executor/gamma_vec_tablescan.h"
#include "nodes/execnodes.h"
#include "storage/gamma_cvtable_am.h"

extern bool					gammadb_storage_skip_keys;

extern void gamma_sk_init_scankeys(SeqScanState *scanstate, SeqScan *node);
extern bool gamma_sk_set_scankeys(CVScanDesc cvscan, SeqScanState *scanstate);
extern bool gamma_sk_run_scankeys(CVScanDesc cvscan, uint32 rgid);
extern bool gamma_sk_attr_check(CVScanDesc cvscan, AttrNumber attno,
									char *min, char *max);

extern uint32 gamma_skip_run_scankeys(CVScanDesc cvscan, RowGroup *rg, uint32 offset);

#endif /* GAMMA_SCANKEYS_H */

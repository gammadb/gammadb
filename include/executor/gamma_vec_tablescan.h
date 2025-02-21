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

#ifndef GAMMA_VEC_TABLESCAN_H
#define GAMMA_VEC_TABLESCAN_H

#include "access/skey.h"
#include "nodes/execnodes.h"
#include "nodes/extensible.h"
#include "nodes/plannodes.h"

#include "storage/gamma_cvtable_am.h"

typedef struct VecSeqScanState
{
	SeqScanState sss;

	/* row group scankeys*/
	GammaScanKey scankeys;
	uint16 sk_count;
	bool scan_over;

	/* column vector skip keys */
	GammaSkipKey skipkeys;
	uint16 skip_count;
}VecSeqScanState;

extern const CustomPathMethods* gamma_vec_tablescan_path_methods(void);
extern const CustomExecMethods* gamma_vec_tablescan_exec_methods(void);
extern void gamma_vec_tablescan_init(void);

#endif   /* GAMMA_VEC_TABLESCAN_H */

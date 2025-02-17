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

#ifndef GAMMA_BUFFER_H
#define GAMMA_BUFFER_H

#include "access/attnum.h"
#include "utils/resowner.h"

#include "storage/gamma_toc.h"

typedef struct GammaBufferTag
{
	Oid relid;
	Oid rgid;
	AttrNumber attno;
} GammaBufferTag;

extern void gamma_buffer_startup(void);
extern bool gamma_buffer_add_cv(Oid relid, Oid rgid, int16 attno, gamma_buffer_cv *cv);
extern bool gamma_buffer_get_cv(Oid relid, Oid rgid, int16 attno, gamma_buffer_cv *cv);
extern void gamma_buffer_invalid_rel(Oid relid);

extern void gamma_buffer_register_cv(Oid relid, Oid rgid, int16 attno);
extern void gamma_buffer_release_cv(Oid relid, Oid rgid, int16 attno);

#endif

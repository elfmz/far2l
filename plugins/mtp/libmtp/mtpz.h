/**
 * \file mtpz.h
 *
 * Copyright (C) 2011-2012 Sajid Anwar <sajidanwar94@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef MTPZ_H_INCLUSION_GUARD
#define MTPZ_H_INCLUSION_GUARD

#include "config.h" /* USE_MTPZ or not */
#include "ptp.h" /* PTPParams */

#ifdef USE_MTPZ

uint16_t ptp_mtpz_handshake (PTPParams* params);
int mtpz_loaddata(void);

#else

/* Stubs if mtpz is unused */
static inline uint16_t ptp_mtpz_handshake (PTPParams* params)
{
  return PTP_RC_OperationNotSupported;
}

static inline int mtpz_loaddata(void)
{
  return -1;
}

#endif

extern int use_mtpz;

#endif /* LIBMTP_H_INCLUSION_GUARD */


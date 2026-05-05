/*
 * \file playlist-spl.h
 * Playlist to .spl conversion functions.
 *
 * Copyright (C) 2008 Alistair Boyle <alistair.js.boyle@gmail.com>
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
 */

#ifndef __MTP__PLAYLIST_SPL__H
#define __MTP__PLAYLIST_SPL__H

int is_spl_playlist(PTPObjectInfo *oi);

void spl_to_playlist_t(LIBMTP_mtpdevice_t* device, PTPObjectInfo *oi,
                       const uint32_t id, LIBMTP_playlist_t * const pl);
int playlist_t_to_spl(LIBMTP_mtpdevice_t *device,
                      LIBMTP_playlist_t * const metadata);
int update_spl_playlist(LIBMTP_mtpdevice_t *device,
			  LIBMTP_playlist_t * const newlist);

#endif //__MTP__PLAYLIST_SPL__H

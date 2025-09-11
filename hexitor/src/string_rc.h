/**************************************************************************
 *  Hexitor plug-in for FAR 3.0 modifed by m32 2024 for far2l             *
 *  Copyright (C) 2010-2014 by Artem Senichev <artemsen@gmail.com>        *
 *  https://sourceforge.net/projects/farplugs/                            *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#pragma once


//! Plug-in strings
enum plugin_string {
	ps_title,
	ps_helptopic,

	ps_ok,
	ps_cancel,
	ps__yes,
	ps__no,
	ps__cancel,
	ps__retry,

	ps_cfg_add_pm,
	ps_cfg_add_em,
	ps_cfg_add_vm,
	ps_cfg_prefix,
	ps_cfg_save_pos,
	ps_cfg_move_ib,
	ps_cfg_std_csize,
	ps_cfg_show_dd,
	ps_cfg_clr_title,
	ps_cfg_clr_offset,
	ps_cfg_clr_updated,
	ps_cfg_clr_active,

	ps_fn_help,
	ps_fn_save,
	ps_fn_saveas,
	ps_fn_mode_ro,
	ps_fn_mode_rw,
	ps_fn_find,
	ps_fn_prev,
	ps_fn_next,
	ps_fn_goto,
	ps_fn_codepage,
	ps_fn_setup,
	ps_fn_exit,

	ps_sav_title,
	ps_sav_file,
	ps_sav_modifq,
	ps_sav_saveas,
	ps_sav_cancelq,
	ps_sav_overwrq,
	ps_sav_alrexist,
	ps_sav_readonly,

	ps_swmod_warn,
	ps_swmod_quest,

	ps_goto_title,
	ps_goto_hex,
	ps_goto_percent,
	ps_goto_out_ofr,
	ps_goto_spec,

	ps_find_title,
	ps_find_backward,
	ps_find_empty,
	ps_find_not_found,

	ps_err_open_file,
	ps_err_read_file,
	ps_err_save_file,
};

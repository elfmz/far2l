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

#include "screen_ctl.h"


class keybar_ctl : public screen_ctl
{
public:
	keybar_ctl();

	//From screen_ctl
	void initialize();

	/**
	 * Resize screen control buffer
	 * \param width new width size
	 */
	void resize(const size_t width);

	/**
	 * Update state (handle Ctrl/Alt/Shift press)
	 * \param rw_mode true for RW, false for RO file modes
	 * \return true if state updated
	 */
	bool update(const bool rw_mode, const bool initialize, const int keystate);

	/**
	 * Get button's number from position
	 * \param x_coord screen coordinate
	 * \return button's number (1...12) or 0 if undefined
	 */
	WORD get_button(const SHORT x_coord) const;

private:
	/**
	 * Set labels for key bar
	 */
	void set_labels();

	/**
	 * Set label for button
	 * \param num button number
	 * \param label button label (nullptr to clear)
	 */
	void set_label(const size_t num, const wchar_t* label);

	/**
	 * Get button position
	 * \param num button number (1...12)
	 * \param for_label true to get button's label position
	 * \return position for button
	 */
	size_t get_btn_pos(const size_t num, const bool for_label = true) const;

	/**
	 * Get button width size
	 * \return button width size
	 */
	size_t get_btn_width() const;

public:
	//! Key bar state
	enum state {
		st_normal,
		st_ctrl,
		st_alt,
		st_shift,
		st_undef
	};
	state get_state() const { return _state; }
private:

	state		_state;
	bool		_rw_mode;
	FarColor	_clr_num;
	FarColor	_clr_txt;
};

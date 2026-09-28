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

#include "common.h"


class progress
{
public:
	/**
	 * Constructor.
	 * \param title window title
	 * \param min_value minimal progress value
	 * \param max_value maximal progress value
	 */
	progress(const wchar_t* title, const uint64_t min_value, const uint64_t max_value);

	~progress();

	/**
	 * Show progress window.
	 */
	void show();

	/**
	 * Hide progress window.
	 */
	void hide();

	/**
	 * Set progress value.
	 * \param val new progress value
	 */
	void update(const uint64_t val);

	/**
	 * Check for abort request.
	 * \return true if user requested abort
	 */
	static bool aborted();

private:
	const wchar_t*	_title;		///< Window title
	bool			_visible;	///< Visible flag
	uint64_t		_min_value;	///< Minimum progress value
	uint64_t		_max_value;	///< Maximum progress value
	wstring			_bar;		///< Progress bar
};

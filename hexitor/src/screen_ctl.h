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

static constexpr size_t MIN_WIDTH_SIZE = 80;		//Minimal width size of the screen control
static constexpr size_t MIN_HEIGHT_SIZE = 1;		//Minimal height size of the screen control

class screen_ctl
{
public:
	screen_ctl()
		: _width(0), _height(0)
	{
	}

	/**
	 * Initialization
	 */
	virtual void initialize()
	{
	}

	/**
	 * Get pointer to buffer
	 * \return pointer to buffer
	 */
	FAR_CHAR_INFO* buffer()
	{
		assert(!_buffer.empty());
		return _buffer.empty() ? nullptr : &_buffer.front();
	}

	/**
	 * Get width dimension of the buffer
	 * \return buffer width
	 */
	inline int width() const	{ return static_cast<int>(_width); }

	/**
	 * Get height dimension of the buffer
	 * \return buffer height
	 */
	inline int height() const	{ return static_cast<int>(_height); }

protected:
	/**
	 * Resize screen control buffer
	 * \param width new width size
	 * \param height new height size
	 */
	virtual void resize_buffer(const size_t width, const size_t height)
	{
		_width = std::max(width, MIN_WIDTH_SIZE);
		_height = std::max(height, MIN_HEIGHT_SIZE);

		FAR_CHAR_INFO fill_data;
		fill_data.Char.UnicodeChar = L' ';
		fill_data.Attributes = _default_color;
		_buffer.resize(_width * _height, fill_data);
	}


	/**
	 * Write string to character buffer
	 * \param row start position (row)
	 * \param col start position (col)
	 * \param val source string
	 * \param len source string length
	 */
	void write(const size_t row, const size_t col, const wchar_t* val, const size_t len)
	{
		assert(val);
		assert(len);
		assert(row < _height);
		assert(col < _width);
		assert(row * _width + col + len <= _buffer.size());

		const size_t start_pos = row * _width + col;
		for (size_t i = 0; i < len; ++i)
			_buffer[start_pos + i].Char.UnicodeChar = val[i];
	}

	/**
	 * Write symbol to character buffer
	 * \param row position (row)
	 * \param col position (col)
	 * \param val char value
	 */
	void write(const size_t row, const size_t col, const wchar_t val)
	{
		assert(row < _height);
		assert(col < _width);
		_buffer[row * _width + col].Char.UnicodeChar = val;
	}

	/**
	 * Write color attribute to character buffer
	 * \param row position (row)
	 * \param col position (col)
	 * \param val color attribute value
	 */
	void write(const size_t row, const size_t col, const FarColor& val)
	{
		assert(row < _height);
		assert(col < _width);
		_buffer[row * _width + col].Attributes = val;
	}

protected:
	typedef vector<FAR_CHAR_INFO> screen_buf;

	screen_buf	_buffer;		///< Screen buffer
	size_t		_width;			///< Buffer width dimension
	size_t		_height;		///< Buffer height dimension

	FarColor	_default_color;	///< Default color
};

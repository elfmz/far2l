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

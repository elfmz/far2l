#include "unicode/uchar.h"
#include "unicode/utypes.h"
#include "unicode/stringoptions.h"
#include "../include/CharClasses.h"

bool CharClasses::FullWidth()
{
	if (_prop_EAST_ASIAN_WIDTH == -1) {
		_prop_EAST_ASIAN_WIDTH = u_getIntPropertyValue(_c, UCHAR_EAST_ASIAN_WIDTH);
	}
	return _prop_EAST_ASIAN_WIDTH == U_EA_FULLWIDTH || _prop_EAST_ASIAN_WIDTH == U_EA_WIDE;
}

bool CharClasses::Prefix()
{
	if (_prop_GENERAL_CATEGORY == -1) {
		_prop_GENERAL_CATEGORY = u_getIntPropertyValue(_c, UCHAR_GENERAL_CATEGORY);
	}
	if (_prop_GENERAL_CATEGORY == U_SURROGATE) {
		return true;
	}
	if (_prop_GENERAL_CATEGORY == U_OTHER_LETTER || _prop_GENERAL_CATEGORY == U_OTHER_NUMBER) {
		return false;
	}
	if (_prop_JOINING_TYPE == -1) {
		_prop_JOINING_TYPE = u_getIntPropertyValue(_c, UCHAR_JOINING_TYPE);
	}
	return _prop_JOINING_TYPE == U_JT_RIGHT_JOINING;
}

bool CharClasses::Suffix()
{
	if (_prop_GENERAL_CATEGORY == -1) {
		_prop_GENERAL_CATEGORY = u_getIntPropertyValue(_c, UCHAR_GENERAL_CATEGORY);
	}
	if (_prop_GENERAL_CATEGORY == U_COMBINING_SPACING_MARK
			|| _prop_GENERAL_CATEGORY == U_NON_SPACING_MARK
			|| _prop_GENERAL_CATEGORY == U_ENCLOSING_MARK
			|| _prop_GENERAL_CATEGORY == U_FORMAT_CHAR) {
		return true;
	}
	if (_prop_GENERAL_CATEGORY != U_MODIFIER_LETTER
			&& _prop_GENERAL_CATEGORY != U_OTHER_LETTER
			&& _prop_GENERAL_CATEGORY != U_OTHER_NUMBER
			&& _prop_GENERAL_CATEGORY != U_OTHER_PUNCTUATION) {
		if (_prop_JOINING_TYPE == -1) {
			_prop_JOINING_TYPE = u_getIntPropertyValue(_c, UCHAR_JOINING_TYPE);
		}
		if (_prop_JOINING_TYPE != U_JT_NON_JOINING
			&& _prop_JOINING_TYPE != U_JT_TRANSPARENT
			&& _prop_JOINING_TYPE != U_JT_RIGHT_JOINING
			&& _prop_JOINING_TYPE != U_JT_DUAL_JOINING) {
			return true;
		}
	}

	if (_prop_BLOCK == -1) {
		_prop_BLOCK = u_getIntPropertyValue(_c, UCHAR_BLOCK);
	}

	return _prop_BLOCK == UBLOCK_COMBINING_DIACRITICAL_MARKS
		|| _prop_BLOCK == UBLOCK_COMBINING_MARKS_FOR_SYMBOLS
		|| _prop_BLOCK == UBLOCK_COMBINING_HALF_MARKS
		|| _prop_BLOCK == UBLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT
		|| _prop_BLOCK == UBLOCK_COMBINING_DIACRITICAL_MARKS_EXTENDED;
}

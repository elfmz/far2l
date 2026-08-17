#pragma once

/*
macrobrowser.hpp

Просмотр списка клавиатурных макросов - общие элементы только macro.cpp и macrobrowser.cpp
*/
/*
Copyright (c) 2026- Far2l People
*/

// return values for KeyMacro::MacroReplaceAdd()
enum MacroReplaceAddRes
{
	Success = 0,         //  0: successfully
	Busy,                //  1: prevent modification during macro execution or recording
	InvalidMacroIndex,   //  2: MacroLIB not initialized or invalid macro index (imacro)
	InvalidAreaIndex,    //  3: invalid macroarea index (iarea)
	AreaRequired,        //  4: for Add iarea are mandatory, but iarea < 0
	InvalidKey,          //  5: Некорректная комбинация клавиш
	EmptySequence,       //  6: Пустая последовательность макрокоманды
	InvalidSequence,     //  7: Некорректная последовательность макрокоманды
	NoChanges,           //  8: Изменения полность эквивалентны исходному => не меняем
	DuplicateAreaKey,    //  9: В выбранной области уже присутствует другой не удаленный макрос с такой же комбинацией клавиш
	ReallocationFailed,  // 10: Error memory reallocation
	DeleteFailed,        // 11: Delete error
};

// declare from macro.cpp
extern TVarTable glbVarTable, glbConstTable;

const wchar_t *MacroLib_GetFunctionType(const TMacroFunction *tmf);

size_t MacroLib_KeywordsFunctions2Items(class KeyMacro *KMacro,
	std::vector<FarListItem> &Items, std::vector<FARString> &Descriptions);

/*
FileMasksProcessor.cpp

Класс для работы с простыми масками файлов (не учитывается наличие масок
исключения).
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "FileMasksProcessor.hpp"
#include "processname.hpp"
#include "StackHeapArray.hpp"

FileMasksProcessor::FileMasksProcessor()
	:
	BaseFileMask(), re(nullptr), n(0)
{}

void FileMasksProcessor::Free()
{
	Masks.Free();

	re.reset();
	n = 0;
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/

bool FileMasksProcessor::Set(const wchar_t *masks, DWORD Flags)
{
	Free();
	// разделителем масок является не только запятая, но и точка с запятой!
	DWORD flags = ULF_PACKASTERISKS | ULF_PROCESSBRACKETS | ULF_SORT | ULF_UNIQUE;

	if (Flags & FMPF_ADDASTERISK)
		flags|= ULF_ADDASTERISK;

	if (masks && *masks == L'/') {
		re.reset(new (std::nothrow) RegExp);
		if (re && re->Compile(masks, OP_PERLSTYLE | OP_OPTIMIZE)) {
			n = re->GetBracketsCount();
			return true;
		}
		re.reset();
		return false;
	}

	Masks.SetParameters(L',', L';', flags);
	return Masks.Set(masks);
}

bool FileMasksProcessor::IsEmpty() const
{
	if (re) {
		return !n;
	}

	return Masks.IsEmpty();
}

/*
	сравнить имя файла со списком масок
	Возвращает TRUE в случае успеха.
	Путь к файлу в FileName НЕ игнорируется
*/
bool FileMasksProcessor::Compare(const wchar_t *FileName, bool ignorecase) const
{
	if (re) {
		StackHeapArray<RegExpMatch> m(n);
		int i = n;
		return re->Search(ReStringView(FileName), m.Get(), i);
	}

	const wchar_t *MaskPtr;		// указатель на текущую маску в списке
	for (size_t MI = 0; nullptr != (MaskPtr = Masks.Get(MI)); ++MI) {
		// SkipPath=FALSE, т.к. в CFileMask вызывается PointToName
		if (CmpName(MaskPtr, FileName, false, ignorecase))
			return true;
	}

	return false;
}

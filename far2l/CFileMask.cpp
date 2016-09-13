/*
CFileMask.cpp

�������� ����� ��� ������ � ������� ������. ������������ ����� ������ ���.
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

#include "CFileMask.hpp"
#include "FileMasksProcessor.hpp"
#include "FileMasksWithExclude.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "message.hpp"
#include "pathmix.hpp"

CFileMask::CFileMask():
	FileMask(nullptr)
{
}

void CFileMask::Free()
{
	if (FileMask)
		delete FileMask;

	FileMask=nullptr;
}

/*
 �������������� ������ �����. ��������� ������, ����������� ������� ��� ������
 � �������. ����������� ��������� ����� ����������, ������� �� �� ��������
 �������� '|' ���������� FALSE ��� ������� (��������, ���� �� ����� ����� ��
 ����� ����� 0).
*/

bool CFileMask::Set(const wchar_t *Masks, DWORD Flags)
{
	Free();
	bool Result=false;
	int Silent=Flags & FMF_SILENT;
	DWORD flags=0;

	if (Flags & FMF_ADDASTERISK)
		flags|=FMPF_ADDASTERISK;

	if (Masks && *Masks)
	{
		if (FileMasksWithExclude::IsExcludeMask(Masks))
		{
			if (!(Flags&FMF_FORBIDEXCLUDE))
				FileMask=new FileMasksWithExclude;
		}
		else
		{
			FileMask=new FileMasksProcessor;
		}

		if (FileMask)
			Result=FileMask->Set(Masks, flags);

		if (!Result)
			Free();
	}

	if (!Silent && !Result)
		Message(MSG_WARNING,1,MSG(MWarning),MSG(MIncorrectMask), MSG(MOk));

	return Result;
}

// ���������� TRUE, ���� ������ ����� ������
bool CFileMask::IsEmpty()
{
	return FileMask?FileMask->IsEmpty():true;
}

/* �������� ��� ����� �� ������� �����
   ���������� TRUE � ������ ������.
   ���� � ����� ����� ������������.
*/
bool CFileMask::Compare(const wchar_t *FileName)
{
	return FileMask?FileMask->Compare(PointToName((wchar_t*)FileName)):false;
}

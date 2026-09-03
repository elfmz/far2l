/*
scrobj.cpp

Parent class для всех screen objects
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

#include "scrobj.hpp"
#include "savescr.hpp"
#include "interf.hpp"

ThinScreenObject::ThinScreenObject()
	: X1(0), Y1(0), X2(0), Y2(0)
{
	//  _OT(SysLog(L"[%p] ThinScreenObject::ThinScreenObject()", this));
}

ThinScreenObject::~ThinScreenObject()
{
}


bool ThinScreenObject::Locked() const
{
	return Flags.Check(FSCROBJ_LOCKED) || (pOwner && pOwner->Locked());
}

void ThinScreenObject::Lock()
{
	Flags.Set(FSCROBJ_LOCKED);
}

void ThinScreenObject::Unlock()
{
	Flags.Clear(FSCROBJ_LOCKED);
}

int ThinScreenObject::ObjWidth() const
{
	return X2 >= X1 ? X2 + 1 - X1 : 0;
}

int ThinScreenObject::ObjHeight() const
{
	return Y2 >= Y1 ? Y2 + 1 - Y1 : 0;
}

void ThinScreenObject::SetOwner(ThinScreenObject *pOwner)
{
	ThinScreenObject::pOwner = pOwner;
}

ThinScreenObject *ThinScreenObject::GetOwner()
{
	return pOwner;
}

void ThinScreenObject::SetPosition(int newX1, int newY1, int newX2, int newY2)
{
	X1 = newX1;
	Y1 = newY1;
	X2 = newX2;
	Y2 = newY2;
	Flags.Set(FSCROBJ_SETPOSITIONDONE);
}

void ThinScreenObject::SetScreenPosition()
{
	Flags.Clear(FSCROBJ_SETPOSITIONDONE);
}

void ThinScreenObject::GetPosition(int &outX1, int &outY1, int &outX2, int &outY2) const
{
	outX1 = X1;
	outY1 = Y1;
	outX2 = X2;
	outY2 = Y2;
}

void ThinScreenObject::Hide()
{
	//  _tran(SysLog(L"[%p] ThinScreenObject::Hide()",this));
	Flags.Clear(FSCROBJ_VISIBLE);
}

/*
	$ 15.07.2000 tran
	add ugly new method
*/
void ThinScreenObject::Hide0()
{
	Flags.Clear(FSCROBJ_VISIBLE);
}
/* tran 15.07.2000 $ */

void ThinScreenObject::Show()
{
	if (Locked() || !Flags.Check(FSCROBJ_SETPOSITIONDONE))
		return;

	//	if (Flags.Check(FSCROBJ_ISREDRAWING))
	//		return;
	//	Flags.Set(FSCROBJ_ISREDRAWING);
	Flags.Set(FSCROBJ_VISIBLE);
	DisplayObject();
	//	Flags.Clear(FSCROBJ_ISREDRAWING);
}

void ThinScreenObject::Redraw()
{
	//  _tran(SysLog(L"[%p] ThinScreenObject::Redraw()",this));
	if (Flags.Check(FSCROBJ_VISIBLE))
		Show();
}

///
ScreenObject::~ScreenObject()
{
	//  _OT(SysLog(L"[%p] ThinScreenObject::~ThinScreenObject()", this));
	if (!Flags.Check(FSCROBJ_ENABLERESTORESCREEN)) {
		if (ShadowSaveScr)
			ShadowSaveScr->Discard();

		if (SaveScr)
			SaveScr->Discard();
	}

	if (ShadowSaveScr)
		delete ShadowSaveScr;

	if (SaveScr)
		delete SaveScr;
}


void ScreenObject::SetPosition(int X1, int Y1, int X2, int Y2)
{
	/*
		$ 13.04.2002 KM
		- Раз меняем позицию объекта на экране, то тогда
		перед этим восстановим изображение под ним для
		предотвращения восстановления ранее сохранённого
		изображения в новом месте.
	*/
	if (SaveScr) {
		delete SaveScr;
		SaveScr = nullptr;
	}
	ThinScreenObject::SetPosition(X1, Y1, X2, Y2);
}

void ScreenObject::Hide()
{
	//  _tran(SysLog(L"[%p] ThinScreenObject::Hide()",this));
	if (!Flags.Check(FSCROBJ_VISIBLE))
		return;

	Flags.Clear(FSCROBJ_VISIBLE);

	if (ShadowSaveScr) {
		delete ShadowSaveScr;
		ShadowSaveScr = nullptr;
	}

	if (SaveScr) {
		delete SaveScr;
		SaveScr = nullptr;
	}
}

void ScreenObject::Show()
{
	if (Locked() || !Flags.Check(FSCROBJ_SETPOSITIONDONE))
		return;

	//	if (Flags.Check(FSCROBJ_ISREDRAWING))
	//		return;
	//	Flags.Set(FSCROBJ_ISREDRAWING);
	if (!Flags.Check(FSCROBJ_VISIBLE)) {
		Flags.Set(FSCROBJ_VISIBLE);
		if (Flags.Check(FSCROBJ_ENABLERESTORESCREEN) && !SaveScr)
			SaveScr = new SaveScreen(X1, Y1, X2, Y2);
	}

	DisplayObject();
	//	Flags.Clear(FSCROBJ_ISREDRAWING);
}

void ScreenObject::Shadow(bool Full)
{
	if (Flags.Check(FSCROBJ_VISIBLE)) {
		if (Full) {
			if (!ShadowSaveScr)
				ShadowSaveScr = new SaveScreen(0, 0, ScrX, ScrY);

			MakeShadow(0, 0, ScrX, ScrY, ShadowSaveScr);

		} else {
			if (!ShadowSaveScr)
				ShadowSaveScr = new SaveScreen(X1, Y1, X2 + 2, Y2 + 1);

			MakeShadow(X1 + 2, Y2 + 1, X2, Y2 + 1, ShadowSaveScr);
			MakeShadow(X2 + 1, Y1 + 1, X2 + 2, Y2 + 1, ShadowSaveScr);
		}
	}
}

void ScreenObject::Lock()
{
	if (++nLockCount == 1) {
		ThinScreenObject::Lock();
	}
}

void ScreenObject::Unlock()
{
	if (nLockCount > 1) {
		--nLockCount;
	} else {
		ThinScreenObject::Unlock();
		if (UNLIKELY(nLockCount <= 0)) {
			fprintf(stderr, "ScreenObject::Unlock: unexpected nLockCount=%d\n", nLockCount);
		}
		nLockCount = 0;
	}
}

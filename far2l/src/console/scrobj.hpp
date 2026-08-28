#pragma once

/*
scrobj.hpp

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

#include "bitflags.hpp"
#include <WinCompat.h>
#include "keys.hpp"
#include "macroopcode.hpp"


class SaveScreen;

// можно использовать только младший байт (т.е. маска 0x000000FF), остальное отдается порожденным классам
enum
{
	FSCROBJ_VISIBLE             = 0x00000001,
	FSCROBJ_ENABLERESTORESCREEN = 0x00000002,
	FSCROBJ_SETPOSITIONDONE     = 0x00000004,
	FSCROBJ_ISREDRAWING         = 0x00000008,		// идет процесс Show?
	FSCROBJ_LOCKED              = 0x00000010,
};

#if defined(__LP64__) || defined(_LP64)
# pragma pack(push,16)
#else
# pragma pack(push,8)
#endif
class ThinScreenObject
{
	friend class LockThinObject;
protected:
	ThinScreenObject *pOwner{nullptr};

protected:
	BitFlags Flags;
	// this 24 bits per coordinate allows to work in terminals up to 8388607 x 8388607 cells
	// until such terminals become avaiable, let save some bytes on sizeof(ThinScreenObject)
	// more exactly 8 bytes saved: sizeof(ThinScreenObject)=32 and if remove :24 its =40
	int X1:24, Y1:24, X2:24, Y2:24;

	virtual void DisplayObject(){};

public:
	ThinScreenObject();
	virtual ~ThinScreenObject();

public:
	virtual int ProcessKey(FarKey Key) { return 0; };
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) { return 0; };

	virtual void Hide();
	virtual void Hide0();	// 15.07.2000 tran - dirty hack :(  // 0 mean - Don't purge saved screen
	virtual void Show();
	virtual void ShowConsoleTitle(){};

	virtual void SetPosition(int newX1, int newY1, int newX2, int newY2);
	virtual void GetPosition(int &outX1, int &outY1, int &outX2, int &outY2) const;
	virtual int ObjWidth() const;
	virtual int ObjHeight() const;

	virtual void SetScreenPosition();
	virtual void ResizeConsole(){};

	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0) { return 0; };

	bool Locked() const;
	virtual void Lock();
	virtual void Unlock();

	void SetOwner(ThinScreenObject *pOwner);
	ThinScreenObject *GetOwner();

	void Redraw();
	bool IsVisible() const { return Flags.Check(FSCROBJ_VISIBLE) != 0; };
	void SetVisible(bool Visible) { Flags.Change(FSCROBJ_VISIBLE, Visible); };
};
#pragma pack(pop) 

/// temporary locks given thin object if it wasnt locked
/// intended for ThinScreenObject-s as they dont have lock counter, but only one bit
class LockThinObject
{
	ThinScreenObject &_obj;
	bool _locked{false};

public:
	LockThinObject(ThinScreenObject &obj) : _obj(obj)
	{
		if (!_obj.Flags.Check(FSCROBJ_LOCKED)) {
			_obj.Lock();
			_locked = true;
		}
	}
	~LockThinObject() { Unlock(); }

	void Unlock()
	{
		if (_locked) {
			_obj.Unlock();
			_locked = false;
		}
	}
};

class ScreenObject : public ThinScreenObject
{
protected:
	SaveScreen *ShadowSaveScr = nullptr;
	int nLockCount{0};

public:
	SaveScreen *SaveScr = nullptr;

	virtual ~ScreenObject();

	virtual void SetPosition(int X1, int Y1, int X2, int Y2);
	virtual void Hide();
	virtual void Show();
	void Lock();
	void Unlock();

	void SetRestoreScreenMode(int Mode) { Flags.Change(FSCROBJ_ENABLERESTORESCREEN, Mode); };
	void Shadow(bool Full = false);
};

/// temporary locks given object that have lock counter
class LockObject
{
	ScreenObject &_obj;
	bool _locked;

public:
	LockObject(ScreenObject &obj) : _obj(obj), _locked(true)
	{
		_obj.Lock();
	}

	~LockObject() { Unlock(); }

	void Unlock()
	{
		if (_locked) {
			_obj.Unlock();
			_locked = false;
		}
	}
};

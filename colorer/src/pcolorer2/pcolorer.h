#ifndef _PCOLORER_H_
#define _PCOLORER_H_

#include <farplug-wide.h>
#include <farcolor.h>
#include <farkeys.h>
#include "colorer/strings/legacy/UnicodeString.h"

extern PluginStartupInfo Info;
extern FarStandardFunctions FSF;
extern UnicodeString* PluginPath;

/** FAR .lng file identifiers. */
enum {
  mName,
  mSetup,
  mTurnOff,
  mTrueMod,
  mCross,
  mPairs,
  mSyntax,
  mOldOutline,
  mOk,
  mReloadAll,
  mCancel,
  mCatalogFile,
  mHRDName,
  mHRDNameTrueMod,
  mListTypes,
  mMatchPair,
  mSelectBlock,
  mSelectPair,
  mListFunctions,
  mFindErrors,
  mSelectRegion,
  mRegionInfo,
  mLocateFunction,
  mUpdateHighlight,
  mReloadBase,
  mConfigure,
  mTotalTypes,
  mSelectSyntax,
  mOutliner,
  mNothingFound,
  mGotcha,
  mChoose,
  mReloading,
  mCantLoad,
  mCantOpenFile,
  mDie,
  mTry,
  mFatal,
  mSelectHRD,
  mChangeBackgroundEditor,
  mTrueModSetting,
  mNoFarTM,
  mUserHrdFile,
  mUserHrcFile,
  mUserHrcSetting,
  mUserHrcSettingDialog,
  mListSyntax,
  mParamList,
  mParamValue,
  mAutoDetect,
  mFavorites,
  mKeyAssignDialogTitle,
  mKeyAssignTextTitle
};

UnicodeString* GetConfigPath(const UnicodeString& sub);

#endif

/* ***** BEGIN LICENSE BLOCK *****
 * Copyright (C) 1999-2009 Cail Lomecb <irusskih at gmail dot com>.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * ***** END LICENSE BLOCK ***** */

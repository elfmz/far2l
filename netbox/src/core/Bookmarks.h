#pragma once

#include <Classes.hpp>

class THierarchicalStorage;
class TBookmarkList;
class TShortCuts;

class TBookmarks : public TObject
{
NB_DISABLE_COPY(TBookmarks)
public:
  TBookmarks();
  virtual ~TBookmarks();

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage, bool All);
  void ModifyAll(bool Modify);
  void Clear();

/*  __property TBookmarkList * Bookmarks[UnicodeString Index] = { read = GetBookmarks, write = SetBookmarks };
  __property TBookmarkList * SharedBookmarks = { read = GetSharedBookmarks, write = SetSharedBookmarks };*/

private:
  TStringList * FBookmarkLists;
  UnicodeString FSharedKey;
  static UnicodeString Keys[];

public:
  TBookmarkList * GetBookmarks(const UnicodeString & Index);
  void SetBookmarks(const UnicodeString & Index, TBookmarkList * Value);
  TBookmarkList * GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList * Value);

private:
  void LoadLevel(THierarchicalStorage * Storage, const UnicodeString & Key,
    intptr_t AIndex, TBookmarkList * BookmarkList);
};

class TBookmark;
class TBookmarkList : public TPersistent
{
friend class TBookmarks;
friend class TBookmark;
NB_DISABLE_COPY(TBookmarkList)
NB_DECLARE_CLASS(TBookmarkList)
public:
  TBookmarkList();
  virtual ~TBookmarkList();

  void Clear();
  void Add(TBookmark * Bookmark);
  void Insert(intptr_t Index, TBookmark * Bookmark);
  void InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark);
  void MoveTo(TBookmark * ToBookmark, TBookmark * Bookmark, bool Before);
  void Delete(TBookmark *& Bookmark);
  TBookmark * FindByName(const UnicodeString & Node, const UnicodeString & Name);
  TBookmark * FindByShortCut(const TShortCut & ShortCut);
  virtual void Assign(const TPersistent * Source);
  void LoadOptions(THierarchicalStorage * Storage);
  void SaveOptions(THierarchicalStorage * Storage);
  void ShortCuts(TShortCuts & ShortCuts);

/*  __property int Count = { read = GetCount };
  __property TBookmark * Bookmarks[int Index] = { read = GetBookmarks };
  __property bool NodeOpened[UnicodeString Index] = { read = GetNodeOpened, write = SetNodeOpened };*/

protected:
  intptr_t IndexOf(TBookmark * Bookmark);
  void KeyChanged(intptr_t Index);

//  __property bool Modified = { read = FModified, write = FModified };
  bool GetModified() const { return FModified; }
  void SetModified(bool Value) { FModified = Value; }

private:
  TStringList * FBookmarks;
  TStringList * FOpenedNodes;
  bool FModified;

public:

  intptr_t GetCount() const;
  TBookmark * GetBookmarks(intptr_t Index);
  bool GetNodeOpened(const UnicodeString & Index);
  void SetNodeOpened(const UnicodeString & Index, bool Value);
};

class TBookmark : public TPersistent
{
friend class TBookmarkList;
NB_DISABLE_COPY(TBookmark)
NB_DECLARE_CLASS(TBookmark)
public:
  TBookmark();

  virtual void Assign(const TPersistent * Source);

/*  __property UnicodeString Name = { read = FName, write = SetName };
  __property UnicodeString Local = { read = FLocal, write = SetLocal };
  __property UnicodeString Remote = { read = FRemote, write = SetRemote };
  __property UnicodeString Node = { read = FNode, write = SetNode };
  __property TShortCut ShortCut = { read = FShortCut, write = SetShortCut };*/

protected:
  TBookmarkList * FOwner;

  static UnicodeString BookmarkKey(const UnicodeString & Node, const UnicodeString & Name);
  // __property UnicodeString Key = { read = GetKey };

private:
  UnicodeString FName;
  UnicodeString FLocal;
  UnicodeString FRemote;
  UnicodeString FNode;
  TShortCut FShortCut;

public:
  void SetName(const UnicodeString & Value);
  UnicodeString GetName() const { return FName; }
  void SetLocal(const UnicodeString & Value);
  UnicodeString GetLocal() const { return FLocal; }
  void SetRemote(const UnicodeString & Value);
  UnicodeString GetRemote() const { return FRemote; }
  void SetNode(const UnicodeString & Value);
  UnicodeString GetNode() const { return FNode; }
  void SetShortCut(const TShortCut & Value);
  TShortCut GetShortCut() const { return FShortCut; }
  UnicodeString GetKey() const;

private:
  void Modify(intptr_t OldIndex);
};


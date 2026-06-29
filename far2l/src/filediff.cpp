#include "headers.hpp"

#include "filediff.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

#include "config.hpp"
#include "codepage.hpp"
#include "colors.hpp"
#include "ctrlobj.hpp"
#include "edit.hpp"
#include "editor.hpp"
#include "exitcode.hpp"
#include "farcolors.hpp"
#include "farwinapi.hpp"
#include "filepanels.hpp"
#include "filestr.hpp"
#include "format.hpp"
#include "frame.hpp"
#include "interf.hpp"
#include "lang.hpp"
#include "manager.hpp"
#include "message.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panel.hpp"

namespace
{
enum class DiffKind
{
	Equal,
	Changed,
	Added,
	Deleted
};

struct DiffRow
{
	int Left = -1;
	int Right = -1;
	DiffKind Kind = DiffKind::Equal;
};

struct DiffHunk
{
	size_t FirstRow = 0;
	size_t LastRow = 0;
};

constexpr size_t InvalidIndex = std::numeric_limits<size_t>::max();
constexpr size_t MaxStoredHistogramPositions = 64;
constexpr size_t MaxUsefulHistogramFrequency = 64;

struct ScreenRow
{
	size_t Row = 0;
	size_t Part = 0;
};

struct InlineRange
{
	int Start = 0;
	int End = 0;
};

struct InlineDiff
{
	std::vector<InlineRange> Left;
	std::vector<InlineRange> Right;
};

void StripEol(FARString &Line)
{
	while (!Line.IsEmpty()) {
		const wchar_t Ch = Line.At(Line.GetLength() - 1);
		if (Ch != L'\r' && Ch != L'\n')
			break;
		Line.Truncate(Line.GetLength() - 1);
	}
}

bool LoadTextFile(const FARString &Path, std::vector<FARString> &Lines,
		std::vector<FARString> *EditorLines = nullptr, UINT *DetectedCodePage = nullptr,
		bool *DetectedSignature = nullptr)
{
	File Src;
	if (!Src.Open(Path.CPtr(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
				OPEN_EXISTING)) {
		return false;
	}

	Lines.clear();
	if (EditorLines)
		EditorLines->clear();

	UINT CodePage = 0;
	bool SignatureFound = false;
	if (!GetFileFormat(Src, CodePage, &SignatureFound, Opt.EdOpt.AutoDetectCodePage != 0) || !IsCodePageSupported(CodePage))
		CodePage = Opt.EdOpt.DefaultCodePage;
	if (DetectedCodePage)
		*DetectedCodePage = CodePage;
	if (DetectedSignature)
		*DetectedSignature = SignatureFound;

	if (!IsUnicodeOrUtfCodePage(CodePage))
		Src.SetPointer(0, nullptr, FILE_BEGIN);

	GetFileString Reader(Src);
	wchar_t *RawLine = nullptr;
	int Length = 0;
	int Result = 0;
	while ((Result = Reader.GetString(&RawLine, CodePage, Length))) {
		if (Result == -1)
			return false;

		FARString LineForEditor(RawLine, Length);
		if (EditorLines)
			EditorLines->emplace_back(LineForEditor);

		FARString Line(LineForEditor);
		StripEol(Line);
		Lines.emplace_back(Line);
	}

	return true;
}

void AppendRawLines(const wchar_t *Data, int Length, std::vector<FARString> &Lines)
{
	Lines.clear();
	if (!Data || Length <= 0)
		return;

	const wchar_t *Ptr = Data;
	const wchar_t *End = Data + Length;
	while (Ptr < End) {
		const wchar_t *LineStart = Ptr;
		while (Ptr < End && *Ptr != L'\r' && *Ptr != L'\n')
			++Ptr;

		if (Ptr < End) {
			if (*Ptr == L'\r' && Ptr + 2 < End && Ptr[1] == L'\r' && Ptr[2] == L'\n')
				Ptr+= 3;
			else if (*Ptr == L'\r' && Ptr + 1 < End && Ptr[1] == L'\n')
				Ptr+= 2;
			else
				++Ptr;
		}

		FARString Line(LineStart, static_cast<int>(Ptr - LineStart));
		StripEol(Line);
		Lines.emplace_back(Line);
	}
}

bool ShouldWriteSignature(UINT CodePage, bool SignatureFound)
{
	switch (CodePage) {
		case CP_UTF32LE:
		case CP_UTF32BE:
		case CP_UTF16LE:
		case CP_UTF16BE:
		case CP_UTF8:
			return SignatureFound;
		default:
			return false;
	}
}

DWORD SignatureForCodePage(UINT CodePage, DWORD &Length)
{
	Length = 0;
	switch (CodePage) {
		case CP_UTF32LE:
			Length = 4;
			return SIGN_UTF32LE;
		case CP_UTF32BE:
			Length = 4;
			return SIGN_UTF32BE;
		case CP_UTF16LE:
			Length = 2;
			return SIGN_UTF16LE;
		case CP_UTF16BE:
			Length = 2;
			return SIGN_UTF16BE;
		case CP_UTF8:
			Length = 3;
			return SIGN_UTF8;
		default:
			return 0;
	}
}

bool WriteAll(File &Dst, const void *Data, size_t Length)
{
	const char *Ptr = static_cast<const char *>(Data);
	while (Length != 0) {
		const DWORD Chunk = static_cast<DWORD>(std::min<size_t>(Length, std::numeric_limits<DWORD>::max()));
		DWORD Written = 0;
		if (!Dst.Write(Ptr, Chunk, &Written) || Written == 0)
			return false;
		Ptr+= Written;
		Length-= Written;
	}
	return true;
}

bool WriteEncoded(File &Dst, UINT CodePage, const wchar_t *Data, int Length)
{
	if (!Length)
		return true;

	if (CodePage == CP_WIDE_LE)
		return WriteAll(Dst, Data, static_cast<size_t>(Length) * sizeof(wchar_t));

	std::string Utf8;
	if (CodePage == CP_UTF8) {
		Wide2MB(Data, Length, Utf8);
		return WriteAll(Dst, Utf8.data(), Utf8.size());
	}

	const int Bytes = WINPORT(WideCharToMultiByte)(CodePage, 0, Data, Length, nullptr, 0, nullptr, nullptr);
	if (Bytes <= 0)
		return false;

	std::vector<char> Buffer(Bytes);
	const int Written = WINPORT(WideCharToMultiByte)(CodePage, 0, Data, Length, Buffer.data(),
			static_cast<int>(Buffer.size()), nullptr, nullptr);
	return Written > 0 && WriteAll(Dst, Buffer.data(), static_cast<size_t>(Written));
}

bool IsEditorContentChangeKey(FarKey Key)
{
	switch (Key) {
		case KEY_BS:
		case KEY_CTRLBS:
		case KEY_DEL:
		case KEY_NUMDEL:
		case KEY_ENTER:
		case KEY_NUMENTER:
		case KEY_SHIFTENTER:
		case KEY_CTRLV:
		case KEY_SHIFTINS:
		case KEY_SHIFTNUMPAD0:
		case KEY_CTRLP:
		case KEY_CTRLM:
		case KEY_CTRLX:
		case KEY_SHIFTDEL:
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL:
		case KEY_CTRLD:
		case KEY_CTRLY:
		case KEY_CTRLT:
		case KEY_CTRLDEL:
		case KEY_CTRLNUMDEL:
		case KEY_CTRLDECIMAL:
		case KEY_CTRLQ:
		case KEY_OP_PLAINTEXT:
		case KEY_ALTBS:
		case KEY_CTRLZ:
		case KEY_CTRLSHIFTZ:
		case KEY_CTRLF7:
		case KEY_SHIFTF7:
		case KEY_ALTU:
		case KEY_ALTI:
		case KEY_CTRLK:
		case KEY_CTRLI:
		case KEY_OP_XLAT:
			return true;
		default:
			break;
	}

	FarKey InsertKey = Key;
	return TranslateInsertKey(InsertKey);
}

bool IsEditorSearchKey(FarKey Key)
{
	switch (Key) {
		case KEY_F7:
		case KEY_CTRLF7:
		case KEY_SHIFTF7:
		case KEY_ALTF7:
			return true;
		default:
			return false;
	}
}

bool ResolvePanelFile(Panel *Source, FARString &Path)
{
	if (!Source || !Source->IsVisible() || Source->GetType() != FILE_PANEL || Source->GetMode() != NORMAL_PANEL)
		return false;

	FARString Name;
	if (!Source->GetCurName(Name) || Name.IsEmpty())
		return false;

	Source->GetCurDir(Path);
	AddEndSlash(Path);
	Path+= Name;

	const DWORD Attr = apiGetFileAttributes(Path.CPtr());
	return Attr != INVALID_FILE_ATTRIBUTES && !(Attr & FILE_ATTRIBUTE_DIRECTORY);
}

constexpr size_t MaxInlineLcsCells = 65536;

bool BuildInlineLcs(const FARString &Left, const FARString &Right, std::vector<int> &Lcs, size_t &Columns)
{
	const size_t LeftLength = Left.GetLength();
	const size_t RightLength = Right.GetLength();
	if (LeftLength == 0 || RightLength == 0
			|| (RightLength != 0 && LeftLength > MaxInlineLcsCells / RightLength)) {
		return false;
	}

	Columns = RightLength + 1;
	Lcs.assign((LeftLength + 1) * Columns, 0);
	for (size_t I = LeftLength; I-- > 0;) {
		for (size_t J = RightLength; J-- > 0;) {
			Lcs[I * Columns + J] = Left.At(I) == Right.At(J)
					? Lcs[(I + 1) * Columns + J + 1] + 1
					: std::max(Lcs[(I + 1) * Columns + J], Lcs[I * Columns + J + 1]);
		}
	}
	return true;
}

bool LinesSimilar(const FARString &Left, const FARString &Right)
{
	if (Left == Right)
		return true;

	const size_t LeftLength = Left.GetLength();
	const size_t RightLength = Right.GetLength();
	if (LeftLength == 0 || RightLength == 0)
		return false;

	const size_t MaxLength = std::max(LeftLength, RightLength);
	const size_t MinLength = std::min(LeftLength, RightLength);
	if (MinLength * 2 < MaxLength)
		return false;

	std::vector<int> Lcs;
	size_t Columns = 0;
	if (BuildInlineLcs(Left, Right, Lcs, Columns))
		return static_cast<size_t>(Lcs[0]) * 3 >= MaxLength * 2;

	size_t Prefix = 0;
	while (Prefix < LeftLength && Prefix < RightLength && Left.At(Prefix) == Right.At(Prefix))
		++Prefix;

	size_t LeftSuffix = LeftLength;
	size_t RightSuffix = RightLength;
	while (LeftSuffix > Prefix && RightSuffix > Prefix && Left.At(LeftSuffix - 1) == Right.At(RightSuffix - 1)) {
		--LeftSuffix;
		--RightSuffix;
	}

	const size_t Common = Prefix + (LeftLength - LeftSuffix);
	return Common * 3 >= MaxLength * 2;
}

bool RowsSimilar(const DiffRow &Deleted, const DiffRow &Added, const std::vector<FARString> &Left,
		const std::vector<FARString> &Right)
{
	return Deleted.Left >= 0 && Added.Right >= 0 && LinesSimilar(Left[Deleted.Left], Right[Added.Right]);
}

std::vector<DiffRow> CoalesceChanges(std::vector<DiffRow> Rows,
		const std::vector<FARString> &Left, const std::vector<FARString> &Right)
{
	size_t Write = 0;
	for (size_t I = 0; I < Rows.size();) {
		if (Rows[I].Kind != DiffKind::Deleted) {
			Rows[Write++] = Rows[I++];
			continue;
		}

		size_t J = I;
		while (J < Rows.size() && Rows[J].Kind == DiffKind::Deleted)
			++J;

		size_t K = J;
		while (K < Rows.size() && Rows[K].Kind == DiffKind::Added)
			++K;

		const size_t DeletedCount = J - I;
		const size_t AddedCount = K - J;
		const size_t ChangedCount = std::min(DeletedCount, AddedCount);
		std::vector<bool> Similar(ChangedCount);
		for (size_t N = 0; N < ChangedCount; ++N) {
			Similar[N] = RowsSimilar(Rows[I + N], Rows[J + N], Left, Right);
			if (Similar[N])
				Rows[Write++] = {Rows[I + N].Left, Rows[J + N].Right, DiffKind::Changed};
			else
				Rows[Write++] = Rows[I + N];
		}
		for (size_t N = ChangedCount; N < DeletedCount; ++N)
			Rows[Write++] = Rows[I + N];
		for (size_t N = 0; N < ChangedCount; ++N) {
			if (!Similar[N])
				Rows[Write++] = Rows[J + N];
		}
		for (size_t N = ChangedCount; N < AddedCount; ++N)
			Rows[Write++] = Rows[J + N];

		I = K;
	}
	Rows.resize(Write);
	return Rows;
}

void EmitFallbackRange(const std::vector<FARString> &Left, const std::vector<FARString> &Right,
		size_t LeftBegin, size_t LeftEnd, size_t RightBegin, size_t RightEnd, std::vector<DiffRow> &Rows)
{
	const size_t LeftCount = LeftEnd - LeftBegin;
	const size_t RightCount = RightEnd - RightBegin;
	const size_t Count = std::max(LeftCount, RightCount);
	for (size_t I = 0; I < Count; ++I) {
		const int L = I < LeftCount ? static_cast<int>(LeftBegin + I) : -1;
		const int R = I < RightCount ? static_cast<int>(RightBegin + I) : -1;
		if (L < 0)
			Rows.push_back({L, R, DiffKind::Added});
		else if (R < 0)
			Rows.push_back({L, R, DiffKind::Deleted});
		else if (Left[L] == Right[R])
			Rows.push_back({L, R, DiffKind::Equal});
		else if (LinesSimilar(Left[L], Right[R]))
			Rows.push_back({L, R, DiffKind::Changed});
		else {
			Rows.push_back({L, -1, DiffKind::Deleted});
			Rows.push_back({-1, R, DiffKind::Added});
		}
	}
}

struct HistogramEntry
{
	size_t Count = 0;
	size_t FirstPosition = InvalidIndex;
	std::vector<size_t> MorePositions;

	void AddPosition(size_t Position)
	{
		++Count;
		if (FirstPosition == InvalidIndex)
			FirstPosition = Position;
		else if (MorePositions.size() + 1 < MaxStoredHistogramPositions)
			MorePositions.emplace_back(Position);
	}

	size_t StoredPositionCount() const
	{
		return FirstPosition == InvalidIndex ? 0 : MorePositions.size() + 1;
	}

	size_t StoredPosition(size_t Index) const
	{
		return Index == 0 ? FirstPosition : MorePositions[Index - 1];
	}
};

struct DiffAnchor
{
	size_t LeftBegin = 0;
	size_t RightBegin = 0;
	size_t Length = 0;
	size_t Frequency = 0;
	size_t UnmatchedPairs = 0;

	bool Valid() const { return Length != 0; }
};

uint64_t HashLine(const FARString &Line)
{
	constexpr uint64_t Offset = 1469598103934665603ull;
	constexpr uint64_t Prime = 1099511628211ull;

	uint64_t Hash = Offset;
	const wchar_t *Data = Line.CPtr();
	for (size_t I = 0, End = Line.GetLength(); I < End; ++I) {
		Hash^= static_cast<uint64_t>(static_cast<uint32_t>(Data[I]));
		Hash*= Prime;
	}
	return Hash;
}

DiffAnchor FindHistogramAnchor(const std::vector<FARString> &Left, const std::vector<FARString> &Right,
		size_t LeftBegin, size_t LeftEnd, size_t RightBegin, size_t RightEnd)
{
	std::unordered_map<uint64_t, size_t> LeftCounts;
	LeftCounts.reserve(LeftEnd - LeftBegin);
	for (size_t I = LeftBegin; I < LeftEnd; ++I)
		++LeftCounts[HashLine(Left[I])];

	std::unordered_map<uint64_t, HistogramEntry> RightHistogram;
	RightHistogram.reserve(RightEnd - RightBegin);
	for (size_t I = RightBegin; I < RightEnd; ++I) {
		HistogramEntry &Entry = RightHistogram[HashLine(Right[I])];
		Entry.AddPosition(I);
	}

	DiffAnchor Best;
	Best.Frequency = InvalidIndex;

	for (size_t LeftPos = LeftBegin; LeftPos < LeftEnd; ++LeftPos) {
		const uint64_t Hash = HashLine(Left[LeftPos]);
		const auto LeftCount = LeftCounts.find(Hash);
		const auto RightEntry = RightHistogram.find(Hash);
		if (LeftCount == LeftCounts.end() || RightEntry == RightHistogram.end())
			continue;

		const size_t Frequency = LeftCount->second + RightEntry->second.Count;
		if (Frequency > MaxUsefulHistogramFrequency || Frequency > Best.Frequency)
			continue;

		for (size_t PositionIndex = 0; PositionIndex < RightEntry->second.StoredPositionCount(); ++PositionIndex) {
			const size_t RightPos = RightEntry->second.StoredPosition(PositionIndex);
			if (Left[LeftPos] != Right[RightPos])
				continue;

			size_t LB = LeftPos;
			size_t RB = RightPos;
			while (LB > LeftBegin && RB > RightBegin && Left[LB - 1] == Right[RB - 1]) {
				--LB;
				--RB;
			}

			size_t LE = LeftPos + 1;
			size_t RE = RightPos + 1;
			while (LE < LeftEnd && RE < RightEnd && Left[LE] == Right[RE]) {
				++LE;
				++RE;
			}

			const size_t Length = LE - LB;
			const size_t UnmatchedPairs = std::min(LB - LeftBegin, RB - RightBegin)
					+ std::min(LeftEnd - LE, RightEnd - RE);
			if (!Best.Valid() || Frequency < Best.Frequency
					|| (Frequency == Best.Frequency
							&& (Length > Best.Length
									|| (Length == Best.Length && UnmatchedPairs < Best.UnmatchedPairs)))) {
				Best.LeftBegin = LB;
				Best.RightBegin = RB;
				Best.Length = Length;
				Best.Frequency = Frequency;
				Best.UnmatchedPairs = UnmatchedPairs;
			}
		}
	}

	return Best;
}

void BuildHistogramDiff(const std::vector<FARString> &Left, const std::vector<FARString> &Right,
		size_t LeftBegin, size_t LeftEnd, size_t RightBegin, size_t RightEnd, std::vector<DiffRow> &Rows,
		int Depth = 0)
{
	while (LeftBegin < LeftEnd && RightBegin < RightEnd && Left[LeftBegin] == Right[RightBegin]) {
		Rows.push_back({static_cast<int>(LeftBegin++), static_cast<int>(RightBegin++), DiffKind::Equal});
	}

	size_t Suffix = 0;
	while (LeftBegin + Suffix < LeftEnd && RightBegin + Suffix < RightEnd
			&& Left[LeftEnd - Suffix - 1] == Right[RightEnd - Suffix - 1]) {
		++Suffix;
	}
	LeftEnd-= Suffix;
	RightEnd-= Suffix;

	if (LeftBegin == LeftEnd || RightBegin == RightEnd || Depth > 64) {
		EmitFallbackRange(Left, Right, LeftBegin, LeftEnd, RightBegin, RightEnd, Rows);
	} else {
		const DiffAnchor Anchor = FindHistogramAnchor(Left, Right, LeftBegin, LeftEnd, RightBegin, RightEnd);
		if (Anchor.Valid()) {
			BuildHistogramDiff(Left, Right, LeftBegin, Anchor.LeftBegin, RightBegin, Anchor.RightBegin, Rows, Depth + 1);
			for (size_t I = 0; I < Anchor.Length; ++I) {
				Rows.push_back({static_cast<int>(Anchor.LeftBegin + I), static_cast<int>(Anchor.RightBegin + I),
						DiffKind::Equal});
			}
			BuildHistogramDiff(Left, Right, Anchor.LeftBegin + Anchor.Length, LeftEnd,
					Anchor.RightBegin + Anchor.Length, RightEnd, Rows, Depth + 1);
		} else {
			EmitFallbackRange(Left, Right, LeftBegin, LeftEnd, RightBegin, RightEnd, Rows);
		}
	}

	const size_t LeftSuffixBegin = LeftEnd;
	const size_t RightSuffixBegin = RightEnd;
	for (size_t I = 0; I < Suffix; ++I) {
		Rows.push_back({static_cast<int>(LeftSuffixBegin + I), static_cast<int>(RightSuffixBegin + I),
				DiffKind::Equal});
	}
}

std::vector<DiffRow> BuildLineDiff(const std::vector<FARString> &Left, const std::vector<FARString> &Right)
{
	std::vector<DiffRow> Rows;
	Rows.reserve(Left.size() + Right.size());
	BuildHistogramDiff(Left, Right, 0, Left.size(), 0, Right.size(), Rows);
	return CoalesceChanges(std::move(Rows), Left, Right);
}

uint32_t ComposeRgb(int R, int G, int B)
{
	return static_cast<uint32_t>(R) | (static_cast<uint32_t>(G) << 8) | (static_cast<uint32_t>(B) << 16);
}

uint32_t TintRgb(DiffKind Kind)
{
	switch (Kind) {
		case DiffKind::Added:
			return ComposeRgb(90, 170, 110);
		case DiffKind::Deleted:
			return ComposeRgb(220, 90, 90);
		case DiffKind::Changed:
			return ComposeRgb(215, 175, 70);
		case DiffKind::Equal:
		default:
			return 0;
	}
}

uint64_t FallbackDiffBackground(DiffKind Kind, bool Inline)
{
	switch (Kind) {
		case DiffKind::Added:
			return Inline ? B_LIGHTGREEN : B_GREEN;
		case DiffKind::Deleted:
			return Inline ? B_LIGHTRED : B_RED;
		case DiffKind::Changed:
			return Inline ? B_YELLOW : B_BROWN;
		case DiffKind::Equal:
		default:
			return 0;
	}
}

uint64_t ApplyFallbackDiffBackground(uint64_t Attr, DiffKind Kind, bool Inline)
{
	constexpr uint64_t BackgroundTrueColorMask = 0xffffff0000000000ull | BACKGROUND_TRUECOLOR;
	return (Attr & ~(static_cast<uint64_t>(B_MASK) | BackgroundTrueColorMask))
			| FallbackDiffBackground(Kind, Inline);
}

uint32_t IndexedBackgroundRgb(uint64_t Attr)
{
	return Palette::FARPalette[(Attr & B_MASK) >> 4] & 0x00ffffff;
}

bool CanUseTrueColorFallback()
{
	return WINPORT(GetConsoleColorPalette)(NULL) >= 24;
}

uint32_t BlendRgb(uint32_t Base, uint32_t Tint, int Alpha)
{
	const int InvAlpha = 256 - Alpha;
	const int R = (((Base & 0xff) * InvAlpha) + ((Tint & 0xff) * Alpha)) >> 8;
	const int G = ((((Base >> 8) & 0xff) * InvAlpha) + (((Tint >> 8) & 0xff) * Alpha)) >> 8;
	const int B = ((((Base >> 16) & 0xff) * InvAlpha) + (((Tint >> 16) & 0xff) * Alpha)) >> 8;
	return ComposeRgb(R, G, B);
}

uint64_t ApplyDiffTint(uint64_t Attr, DiffKind Kind, bool Inline, bool UseTrueColorFallback)
{
	if (Kind == DiffKind::Equal)
		return Attr;

	uint64_t Result = ApplyFallbackDiffBackground(Attr, Kind, Inline);
	if ((Attr & BACKGROUND_TRUECOLOR) == 0 && !UseTrueColorFallback)
		return Result;

	const uint32_t Base = (Attr & BACKGROUND_TRUECOLOR) ? GET_RGB_BACK(Attr) : IndexedBackgroundRgb(Attr);
	const uint32_t Blended = BlendRgb(Base, TintRgb(Kind), Inline ? 136 : 76);
	SET_RGB_BACK(Result, Blended);
	return Result;
}

void ApplyDiffOverlay(int X, int Y, int Width, DiffKind Kind)
{
	if (Kind == DiffKind::Equal || Width <= 0)
		return;

	std::vector<CHAR_INFO> Buffer(Width);
	GetText(X, Y, X + Width - 1, Y, Buffer.data(), static_cast<int>(Buffer.size() * sizeof(Buffer.front())));
	const bool UseTrueColorFallback = CanUseTrueColorFallback();
	for (CHAR_INFO &Cell : Buffer)
		Cell.Attributes = ApplyDiffTint(Cell.Attributes, Kind, false, UseTrueColorFallback);
	PutText(X, Y, X + Width - 1, Y, Buffer.data());
}

void ApplyInlineDiffOverlay(int X1, int X2, int Y, DiffKind Kind)
{
	if (X1 > X2)
		return;

	const int Width = X2 - X1 + 1;
	std::vector<CHAR_INFO> Buffer(Width);
	GetText(X1, Y, X2, Y, Buffer.data(), static_cast<int>(Buffer.size() * sizeof(Buffer.front())));
	const bool UseTrueColorFallback = CanUseTrueColorFallback();
	for (CHAR_INFO &Cell : Buffer)
		Cell.Attributes = ApplyDiffTint(Cell.Attributes, Kind, true, UseTrueColorFallback);
	PutText(X1, Y, X2, Y, Buffer.data());
}

std::vector<InlineRange> InlineRangesFromMarks(const std::vector<bool> &Marks)
{
	std::vector<InlineRange> Ranges;
	for (size_t I = 0; I < Marks.size();) {
		if (!Marks[I]) {
			++I;
			continue;
		}

		const size_t Start = I;
		while (I < Marks.size() && Marks[I])
			++I;
		Ranges.push_back({static_cast<int>(Start), static_cast<int>(I)});
	}
	return Ranges;
}

void BuildSimpleInlineRanges(const FARString &Left, const FARString &Right,
		std::vector<InlineRange> &LeftRanges, std::vector<InlineRange> &RightRanges)
{
	const int LeftLength = static_cast<int>(Left.GetLength());
	const int RightLength = static_cast<int>(Right.GetLength());
	int Prefix = 0;
	while (Prefix < LeftLength && Prefix < RightLength && Left.At(Prefix) == Right.At(Prefix))
		++Prefix;

	int LeftSuffix = LeftLength;
	int RightSuffix = RightLength;
	while (LeftSuffix > Prefix && RightSuffix > Prefix && Left.At(LeftSuffix - 1) == Right.At(RightSuffix - 1)) {
		--LeftSuffix;
		--RightSuffix;
	}

	if (Prefix < LeftSuffix)
		LeftRanges.push_back({Prefix, LeftSuffix});
	if (Prefix < RightSuffix)
		RightRanges.push_back({Prefix, RightSuffix});
}

void BuildInlineDiffRanges(const FARString &Left, const FARString &Right,
		std::vector<InlineRange> &LeftRanges, std::vector<InlineRange> &RightRanges)
{
	const size_t LeftLength = Left.GetLength();
	const size_t RightLength = Right.GetLength();
	std::vector<int> Lcs;
	size_t Columns = 0;
	if (!BuildInlineLcs(Left, Right, Lcs, Columns)) {
		BuildSimpleInlineRanges(Left, Right, LeftRanges, RightRanges);
		return;
	}

	std::vector<bool> LeftChanged(LeftLength);
	std::vector<bool> RightChanged(RightLength);
	size_t I = 0;
	size_t J = 0;
	while (I < LeftLength && J < RightLength) {
		if (Left.At(I) == Right.At(J)) {
			++I;
			++J;
		} else if (Lcs[(I + 1) * Columns + J] >= Lcs[I * Columns + J + 1]) {
			LeftChanged[I++] = true;
		} else {
			RightChanged[J++] = true;
		}
	}
	while (I < LeftLength)
		LeftChanged[I++] = true;
	while (J < RightLength)
		RightChanged[J++] = true;

	LeftRanges = InlineRangesFromMarks(LeftChanged);
	RightRanges = InlineRangesFromMarks(RightChanged);
}

class DiffEditorPane
{
	ScreenObject *m_owner = nullptr;
	FARString m_path;
	UINT m_codepage = CP_AUTODETECT;
	bool m_signatureFound = false;
	std::vector<FARString> m_lines;
	std::unique_ptr<Editor> m_editor;
	bool m_colorerOpened = false;
	int m_syncedTopLine = -1;
	int m_syncedTopVisualLine = -1;
	int m_x1 = -1;
	int m_y1 = -1;
	int m_x2 = -1;
	int m_y2 = -1;

	class PluginEditorScope
	{
		Editor *m_prev = nullptr;

	public:
		PluginEditorScope(Editor *EditorPtr)
		{
			if (CtrlObject) {
				m_prev = CtrlObject->Plugins.CurDialogEditor;
				CtrlObject->Plugins.CurDialogEditor = EditorPtr;
			}
		}

		~PluginEditorScope()
		{
			if (CtrlObject)
				CtrlObject->Plugins.CurDialogEditor = m_prev;
		}
	};

public:
	DiffEditorPane(ScreenObject *Owner, const FARString &Path)
		: m_owner(Owner), m_path(Path)
	{
	}

	~DiffEditorPane()
	{
		CloseColorer();
	}

	bool Load()
	{
		m_editor.reset(new (std::nothrow) Editor(m_owner, true));
		if (!m_editor)
			return false;

		m_editor->SetVirtualFileName(m_path.CPtr());

		std::vector<FARString> EditorLines;
		if (!LoadTextFile(m_path, m_lines, &EditorLines, &m_codepage, &m_signatureFound))
			return false;

		m_editor->FreeAllocatedData(false);
		m_editor->SetCodePage(m_codepage);
		m_editor->BeginBulkLoad();

		if (EditorLines.empty()) {
			m_editor->InsertString(L"", 0);
		} else {
			for (const auto &Line : EditorLines)
				m_editor->InsertString(Line.CPtr(), static_cast<int>(Line.GetLength()));
		}

		m_editor->EndBulkLoad();
		m_editor->MarkSaved();
		m_editor->SetReadOnly(FALSE);
		m_editor->SetShowCursor(false);
		m_editor->SetShowScrollBar(FALSE);
		m_editor->SetShowLineNumbers(TRUE);
		m_editor->SetWordWrap(TRUE);
		return true;
	}

	void SetPosition(int X1, int Y1, int X2, int Y2)
	{
		if (X2 < X1)
			X2 = X1;
		const bool Changed = X1 != m_x1 || Y1 != m_y1 || X2 != m_x2 || Y2 != m_y2;
		m_x1 = X1;
		m_y1 = Y1;
		m_x2 = X2;
		m_y2 = Y2;
		if (m_editor && Changed) {
			m_editor->SetPosition(X1, Y1, X2, Y2);
			UpdateColorer();
		}
	}

	const FARString &Path() const { return m_path; }
	const std::vector<FARString> &Lines() const { return m_lines; }
	bool Modified() const { return m_editor && m_editor->IsFileModified(); }
	void SetActive(bool Active)
	{
		if (m_editor)
			m_editor->SetShowCursor(Active);
	}
	int VisualLineCount(int Line) const
	{
		return m_editor && Line >= 0 ? m_editor->GetVisualLineCount(Line) : 1;
	}
	int VisualOffset(int FirstLine, int FirstVisualLine, int Line, int VisualLine) const
	{
		if (!m_editor || FirstLine < 0 || FirstVisualLine < 0 || Line < FirstLine || VisualLine < 0)
			return -1;

		int Offset = -FirstVisualLine;
		for (int I = FirstLine; I < Line; ++I)
			Offset+= VisualLineCount(I);
		return Offset + VisualLine;
	}
	bool RenderLine(int Line, int VisualLine, int X1, int Y, int X2) const
	{
		return m_editor && Line >= 0 && m_editor->RenderVisualLine(Line, VisualLine, X1, Y, X2);
	}
	void ApplyInlineHighlight(int Line, int VisualLine, int DrawX1, int DrawX2, int DrawY,
			const std::vector<InlineRange> &Ranges, DiffKind Kind) const
	{
		if (!m_editor || Line < 0 || Ranges.empty())
			return;

		for (const InlineRange &Range : Ranges) {
			int CellX1 = 0;
			int CellX2 = 0;
			if (!m_editor->GetVisualLineHighlightCells(Line, VisualLine, Range.Start, Range.End,
						DrawX1, CellX1, CellX2))
				continue;

			CellX1 = std::max(CellX1, DrawX1);
			CellX2 = std::min(CellX2, DrawX2);
			ApplyInlineDiffOverlay(CellX1, CellX2, DrawY, Kind);
		}
	}
	void SyncViewport(int FirstVisibleLine, int FirstVisibleVisualLine)
	{
		if (!m_editor || FirstVisibleLine < 0 || FirstVisibleVisualLine < 0)
			return;

		const bool Changed = FirstVisibleLine != m_syncedTopLine || FirstVisibleVisualLine != m_syncedTopVisualLine;
		if (Changed) {
			m_editor->SetTopScreenLine(FirstVisibleLine, FirstVisibleVisualLine);
			m_syncedTopLine = FirstVisibleLine;
			m_syncedTopVisualLine = FirstVisibleVisualLine;
		}
		if (Changed || !m_colorerOpened)
			UpdateColorer();
	}
	bool ProcessKey(FarKey Key)
	{
		if (!m_editor)
			return false;

		PluginEditorScope Scope(m_editor.get());
		return m_editor->ProcessKey(Key) != 0;
	}
	bool RefreshLinesFromEditor()
	{
		if (!m_editor)
			return false;

		wchar_t *RawData = nullptr;
		int RawLength = 0;
		if (!m_editor->GetRawData(&RawData, RawLength))
			return false;

		std::vector<FARString> NewLines;
		AppendRawLines(RawData, RawLength, NewLines);
		free(RawData);

		if (NewLines == m_lines)
			return false;

		m_lines = std::move(NewLines);
		return true;
	}
	bool Save()
	{
		if (!m_editor)
			return false;

		wchar_t *RawData = nullptr;
		int RawLength = 0;
		if (!m_editor->GetRawData(&RawData, RawLength))
			return false;

		File Dst;
		bool Ok = Dst.Open(m_path.CPtr(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
				FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN);
		if (Ok) {
			if (ShouldWriteSignature(m_codepage, m_signatureFound)) {
				DWORD SignatureLength = 0;
				const DWORD Signature = SignatureForCodePage(m_codepage, SignatureLength);
				Ok = SignatureLength == 0 || WriteAll(Dst, &Signature, SignatureLength);
			}

			Ok = Ok && WriteEncoded(Dst, m_codepage, RawData, RawLength);
			Ok = Ok && Dst.SetEnd();
		}

		free(RawData);
		if (Ok) {
			RefreshLinesFromEditor();
			m_editor->MarkSaved();
		}
		return Ok;
	}
	bool ReplaceLines(int StartLine, int DeleteCount, const std::vector<FARString> &NewLines)
	{
		if (!m_editor)
			return false;

		PluginEditorScope Scope(m_editor.get());
		EditorInfo Info{};
		if (!m_editor->EditorControl(ECTL_GETINFO, &Info))
			return false;

		const int TotalLines = std::max(1, Info.TotalLines);
		StartLine = std::clamp(StartLine, 0, TotalLines);
		DeleteCount = std::clamp(DeleteCount, 0, TotalLines - StartLine);

		if (NewLines.empty()) {
			if (!DeleteLines(StartLine, DeleteCount))
				return false;
			RefreshLinesFromEditor();
			return true;
		}

		const bool ReuseFirstLine = DeleteCount > 0 || (m_lines.empty() && TotalLines == 1);
		size_t FirstInsertedLine = 0;
		if (ReuseFirstLine) {
			if (!SetLine(StartLine, NewLines.front()))
				return false;
			if (!DeleteLines(StartLine + 1, DeleteCount > 0 ? DeleteCount - 1 : 0))
				return false;
			FirstInsertedLine = 1;
			++StartLine;
		} else if (!DeleteLines(StartLine, DeleteCount)) {
			return false;
		}

		for (size_t I = FirstInsertedLine; I < NewLines.size(); ++I) {
			if (!InsertLine(StartLine + static_cast<int>(I - FirstInsertedLine), NewLines[I]))
				return false;
		}

		RefreshLinesFromEditor();
		return true;
	}
	void SetCursorByVisualLine(int Line, int VisualLine, int CellOffset)
	{
		if (!m_editor || Line < 0)
			return;

		m_editor->SetCursorByVisualLineCellOffset(Line, VisualLine, CellOffset);
	}
	bool ProcessMouse(const MOUSE_EVENT_RECORD &MouseEvent)
	{
		if (!m_editor)
			return false;

		PluginEditorScope Scope(m_editor.get());
		MOUSE_EVENT_RECORD Translated = MouseEvent;
		Translated.dwMousePosition.X = std::clamp<SHORT>(Translated.dwMousePosition.X, m_x1, m_x2);
		Translated.dwMousePosition.Y = std::clamp<SHORT>(Translated.dwMousePosition.Y, m_y1, m_y2);
		return m_editor->ProcessMouse(&Translated) != 0;
	}
	bool ProcessMouseAtLine(const MOUSE_EVENT_RECORD &MouseEvent, int Line, int VisualLine,
			int FirstVisibleLine, int FirstVisibleVisualLine)
	{
		if (!m_editor || Line < 0 || VisualLine < 0)
			return false;

		MOUSE_EVENT_RECORD Translated = MouseEvent;
		int Offset = VisualOffset(FirstVisibleLine, FirstVisibleVisualLine, Line, VisualLine);
		if (Offset < 0 || m_y1 + Offset > m_y2) {
			m_editor->SetTopScreenLine(Line, VisualLine);
			m_syncedTopLine = Line;
			m_syncedTopVisualLine = VisualLine;
			Offset = 0;
		}
		Translated.dwMousePosition.Y = static_cast<SHORT>(m_y1 + Offset);

		PluginEditorScope Scope(m_editor.get());
		return m_editor->ProcessMouse(&Translated) != 0;
	}
	int CursorLine() const { return m_editor ? m_editor->GetCursorLine() : 0; }
	int CursorVisualLine() const { return m_editor ? m_editor->GetCursorVisualLine() : 0; }
	int CursorCol() const { return m_editor ? m_editor->GetCurCol() : 0; }

private:
	bool SetEditorPosition(int Line, int Pos)
	{
		EditorSetPosition Position{};
		Position.CurLine = Line;
		Position.CurPos = Pos;
		Position.CurTabPos = -1;
		Position.TopScreenLine = -1;
		Position.LeftPos = -1;
		Position.Overtype = -1;
		return m_editor->EditorControl(ECTL_SETPOSITION, &Position) != 0;
	}

	int LineLength(int Line)
	{
		EditorGetString String{};
		String.StringNumber = Line;
		return m_editor->EditorControl(ECTL_GETSTRING, &String) && String.StringLength > 0 ? String.StringLength : 0;
	}

	bool SetInsertPosition(int Line)
	{
		EditorInfo Info{};
		if (!m_editor->EditorControl(ECTL_GETINFO, &Info))
			return false;

		const int TotalLines = std::max(1, Info.TotalLines);
		if (Line >= TotalLines)
			return SetEditorPosition(TotalLines - 1, LineLength(TotalLines - 1));
		return SetEditorPosition(std::max(0, Line), 0);
	}

	bool SetLine(int Line, const FARString &Text)
	{
		EditorSetString String{};
		String.StringNumber = Line;
		String.StringText = Text.CPtr();
		String.StringEOL = nullptr;
		String.StringLength = static_cast<int>(Text.GetLength());
		return m_editor->EditorControl(ECTL_SETSTRING, &String) != 0;
	}

	bool InsertLine(int Line, const FARString &Text)
	{
		if (!SetInsertPosition(Line))
			return false;
		if (!m_editor->EditorControl(ECTL_INSERTSTRING, nullptr))
			return false;
		return SetLine(Line, Text);
	}

	bool DeleteLines(int StartLine, int Count)
	{
		if (Count <= 0)
			return true;
		if (!SetEditorPosition(StartLine, 0))
			return false;
		for (int I = 0; I < Count; ++I) {
			if (!m_editor->EditorControl(ECTL_DELETESTRING, nullptr))
				return false;
		}
		return true;
	}

	void UpdateColorer()
	{
		if (!CtrlObject || !m_editor)
			return;

		PluginEditorScope Scope(m_editor.get());
		if (!m_colorerOpened) {
			CtrlObject->Plugins.ProcessEditorEvent(EE_READ, nullptr);
			int EditorID = m_editor->GetEditorID();
			CtrlObject->Plugins.ProcessEditorEvent(EE_GOTFOCUS, &EditorID);
			m_colorerOpened = true;
		}
		CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW, EEREDRAW_ALL);
	}

	void CloseColorer()
	{
		if (!CtrlObject || !m_editor || !m_colorerOpened)
			return;

		PluginEditorScope Scope(m_editor.get());
		int EditorID = m_editor->GetEditorID();
		CtrlObject->Plugins.ProcessEditorEvent(EE_CLOSE, &EditorID);
		m_colorerOpened = false;
	}
};

class FileDiffFrame : public Frame
{
	enum class ActivePane
	{
		Left,
		Right
	};

	enum class MergeDirection
	{
		LeftToRight,
		RightToLeft
	};

	enum class StatusAction
	{
		Save,
		Merge,
		PrevDiff,
		NextDiff
	};

	struct LineRange
	{
		int Begin = 0;
		int End = 0;
		bool HasLines = false;
	};

	struct StatusButton
	{
		StatusAction Action = StatusAction::Save;
		bool Enabled = false;
		int X1 = 0;
		int X2 = 0;
	};

	static constexpr int PreferredGutterWidth = 3;
	static constexpr DWORD DiffRefreshDelay = 150;

	FARString m_leftPath;
	FARString m_rightPath;
	DiffEditorPane m_leftPane;
	DiffEditorPane m_rightPane;
	std::vector<DiffRow> m_rows;
	std::vector<DiffHunk> m_hunks;
	std::vector<std::unique_ptr<InlineDiff>> m_inlineDiffs;
	std::vector<ScreenRow> m_screenRows;
	size_t m_top = 0;
	int m_leftWidth = 0;
	int m_gutterWidth = 0;
	int m_rightWidth = 0;
	ActivePane m_activePane = ActivePane::Left;
	bool m_gutterActive = false;
	size_t m_selectedHunk = InvalidIndex;
	MergeDirection m_selectedDirection = MergeDirection::LeftToRight;
	bool m_pendingDiffRefresh = false;
	DWORD m_lastEditTick = 0;
	std::vector<StatusButton> m_statusButtons;

public:
	FileDiffFrame(const FARString &LeftPath, const FARString &RightPath)
		: m_leftPath(LeftPath), m_rightPath(RightPath), m_leftPane(this, m_leftPath), m_rightPane(this, m_rightPath)
	{
		SetCanLoseFocus(TRUE);
		SetRestoreScreenMode(TRUE);
		KeyBarVisible = FALSE;
		SetPosition(0, 0, ScrX, ScrY);

		if (!m_leftPane.Load() || !m_rightPane.Load()) {
			SetExitCode(XC_OPEN_ERROR);
			Message(MSG_WARNING, 1, L"Compare files", L"Cannot open one of the selected files.", Msg::Ok);
			return;
		}
		UpdateActivePane();
		RebuildDiffModel();
		SetExitCode(TRUE);
		FrameManager->InsertFrame(this);
	}

	virtual const wchar_t *GetTypeName() { return L"[FileDiff]"; }
	virtual int GetType() { return MODALTYPE_USER; }
	virtual int GetTypeAndName(FARString &strType, FARString &strName)
	{
		strType = L"FileDiff";
		strName = m_leftPath;
		return MODALTYPE_USER;
	}

	virtual FARString &GetTitle(FARString &Title, int SubLen = -1, int TruncSize = 0)
	{
		Title = L"Compare files";
		return Title;
	}

	virtual void ResizeConsole()
	{
		SetPosition(0, 0, ScrX, ScrY);
		RebuildScreenRows();
	}

	virtual void OnChangeFocus(int focus)
	{
		Frame::OnChangeFocus(focus);
	}

	virtual int ProcessKey(FarKey Key)
	{
		switch (Key) {
			case KEY_IDLE:
				if (FlushPendingDiffRefresh(false))
					Show();
				return TRUE;
			case KEY_ESC:
			case KEY_F10:
				Close();
				return TRUE;
			case KEY_F2:
				ExecuteStatusAction(StatusAction::Save);
				return TRUE;
			case KEY_F5:
				ExecuteStatusAction(StatusAction::Merge);
				return TRUE;
			case KEY_TAB:
			case KEY_SHIFTTAB:
				SwitchFocus(Key == KEY_SHIFTTAB ? -1 : 1);
				return TRUE;
			case KEY_ENTER:
			case KEY_NUMENTER:
				if (m_gutterActive) {
					MergeCurrentSelection();
					return TRUE;
				}
				break;
			case KEY_LEFT:
			case KEY_NUMPAD4:
				if (m_gutterActive) {
					SelectGutterDirection(MergeDirection::RightToLeft);
					Show();
					return TRUE;
				}
				break;
			case KEY_RIGHT:
			case KEY_NUMPAD6:
				if (m_gutterActive) {
					SelectGutterDirection(MergeDirection::LeftToRight);
					Show();
					return TRUE;
				}
				break;
			case KEY_F12:
				return FrameManager->ProcessKey(KEY_F12);
			case KEY_MSWHEEL_UP:
				Scroll(-1);
				return TRUE;
			case KEY_MSWHEEL_DOWN:
				Scroll(1);
				return TRUE;
			case KEY_CTRLUP:
			case KEY_CTRLNUMPAD8:
				ExecuteStatusAction(StatusAction::PrevDiff);
				return TRUE;
			case KEY_CTRLDOWN:
			case KEY_CTRLNUMPAD2:
				ExecuteStatusAction(StatusAction::NextDiff);
				return TRUE;
		}
		if (m_gutterActive)
			return TRUE;

		const bool ContentChange = IsEditorContentChangeKey(Key);
		const bool SearchKey = IsEditorSearchKey(Key);
		if (ActiveEditorPane().ProcessKey(Key)) {
			if (ContentChange)
				ScheduleDiffRefresh();

			if (SearchKey) {
				FlushPendingDiffRefresh(true);
				PlaceActiveCursorForSearch();
			} else {
				if (ContentChange)
					return TRUE;
				EnsureActiveCursorVisible();
			}
			Show();
			return TRUE;
		}
		return FALSE;
	}

	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
	{
		const int X = MouseEvent->dwMousePosition.X;
		const int Y = MouseEvent->dwMousePosition.Y;

		if (MouseEvent->dwEventFlags == MOUSE_WHEELED) {
			const short Delta = HIWORD(MouseEvent->dwButtonState);
			Scroll(Delta > 0 ? -3 : 3);
			return TRUE;
		}

		if (ProcessStatusMouse(*MouseEvent))
			return TRUE;

		if (MouseInsideGutter(X) && Y >= Y1 + 1 && Y <= Y2 - 1)
			return ProcessGutterMouse(*MouseEvent);

		if (IsFocusClick(*MouseEvent) && Y >= Y1 + 1 && Y <= Y2 - 1 && !MouseInsideGutter(X)) {
			const ActivePane NewPane = X >= RightPaneX1() ? ActivePane::Right : ActivePane::Left;
			if (NewPane != m_activePane || m_gutterActive) {
				const size_t CursorIndex = ActiveCursorScreenIndex();
				m_activePane = NewPane;
				m_gutterActive = false;
				SyncActiveCursorToScreenIndex(CursorIndex);
				UpdateActivePane();
			}
		}

		if (!MouseInsideActivePane(*MouseEvent))
			return FALSE;

		if (ProcessPaneMouse(*MouseEvent)) {
			EnsureActiveCursorVisible();
			Show();
			return TRUE;
		}
		ActiveEditorPane().ProcessMouse(*MouseEvent);
		EnsureActiveCursorVisible();
		Show();
		return TRUE;
	}

private:
	size_t VisibleRows() const
	{
		return ObjHeight > 2 ? static_cast<size_t>(ObjHeight - 2) : 0;
	}

	size_t MaxTop() const
	{
		const size_t Page = VisibleRows();
		return m_screenRows.size() > Page ? m_screenRows.size() - Page : 0;
	}

	int GutterX1() const
	{
		return X1 + m_leftWidth;
	}

	int GutterX2() const
	{
		return GutterX1() + m_gutterWidth - 1;
	}

	int RightPaneX1() const
	{
		return GutterX2() + 1;
	}

	bool MouseInsideGutter(int X) const
	{
		return m_gutterWidth > 0 && X >= GutterX1() && X <= GutterX2();
	}

	bool IsFocusClick(const MOUSE_EVENT_RECORD &MouseEvent) const
	{
		return (MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
				&& MouseEvent.dwEventFlags != MOUSE_MOVED;
	}

	void Scroll(int Delta)
	{
		const size_t OldTop = m_top;
		const size_t Page = VisibleRows();
		const size_t CursorIndex = ActiveCursorScreenIndex();
		const size_t CursorOffset = CursorIndex != InvalidIndex && CursorIndex >= OldTop && Page != 0
				? std::min(CursorIndex - OldTop, Page - 1)
				: 0;

		const int64_t NewTop = static_cast<int64_t>(m_top) + Delta;
		m_top = static_cast<size_t>(std::max<int64_t>(0, std::min<int64_t>(NewTop, static_cast<int64_t>(MaxTop()))));
		if (!m_screenRows.empty() && Page != 0 && m_top != OldTop)
			SyncActiveCursorToScreenIndex(std::min(m_top + CursorOffset, m_screenRows.size() - 1));
		Show();
	}

	void NavigateDiff(int Direction)
	{
		if (m_hunks.empty() || Direction == 0)
			return;

		const size_t TargetHunk = FindNavigationHunk(CurrentDiffRow(), Direction);
		if (TargetHunk == InvalidIndex)
			return;

		JumpToHunk(TargetHunk);
	}

	size_t FindNavigationHunk(size_t Row, int Direction) const
	{
		if (m_hunks.empty() || Row == InvalidIndex)
			return InvalidIndex;

		const size_t CurrentHunk = HunkIndexForRow(Row);
		if (Direction > 0) {
			if (CurrentHunk != InvalidIndex)
				return CurrentHunk + 1 < m_hunks.size() ? CurrentHunk + 1 : InvalidIndex;

			for (size_t I = 0; I < m_hunks.size(); ++I) {
				if (m_hunks[I].FirstRow > Row)
					return I;
			}
			return InvalidIndex;
		}

		if (CurrentHunk != InvalidIndex) {
			if (Row > m_hunks[CurrentHunk].FirstRow)
				return CurrentHunk;
			return CurrentHunk > 0 ? CurrentHunk - 1 : InvalidIndex;
		}

		for (size_t I = m_hunks.size(); I > 0; --I) {
			if (m_hunks[I - 1].FirstRow < Row)
				return I - 1;
		}
		return InvalidIndex;
	}

	void JumpToHunk(size_t HunkIndex)
	{
		if (HunkIndex >= m_hunks.size())
			return;

		const size_t RowIndex = m_hunks[HunkIndex].FirstRow;
		const size_t TargetScreenIndex = FirstScreenIndexForRow(RowIndex);
		if (TargetScreenIndex == InvalidIndex || RowIndex >= m_rows.size())
			return;

		m_top = std::min(TargetScreenIndex > 0 ? TargetScreenIndex - 1 : TargetScreenIndex, MaxTop());
		if (m_gutterActive) {
			SelectGutterHunk(HunkIndex, m_selectedDirection);
			EnsureSelectedHunkVisible();
		} else {
			SelectPaneForRow(m_rows[RowIndex]);
			SyncActiveCursorToScreenIndex(TargetScreenIndex);
		}
		UpdateActivePane();
		Show();
	}

	void SwitchFocus(int Direction)
	{
		const size_t CursorIndex = ActiveCursorScreenIndex();
		if (Direction >= 0) {
			if (!m_gutterActive && m_activePane == ActivePane::Left) {
				if (!ActivateGutter(MergeDirection::LeftToRight)) {
					m_activePane = ActivePane::Right;
					SyncActiveCursorToScreenIndex(CursorIndex);
				}
			} else if (m_gutterActive) {
				m_gutterActive = false;
				m_activePane = ActivePane::Right;
				SyncActiveCursorToSelectedHunk(CursorIndex);
			} else {
				m_activePane = ActivePane::Left;
				SyncActiveCursorToScreenIndex(CursorIndex);
			}
		} else {
			if (!m_gutterActive && m_activePane == ActivePane::Right) {
				if (!ActivateGutter(MergeDirection::RightToLeft)) {
					m_activePane = ActivePane::Left;
					SyncActiveCursorToScreenIndex(CursorIndex);
				}
			} else if (m_gutterActive) {
				m_gutterActive = false;
				m_activePane = ActivePane::Left;
				SyncActiveCursorToSelectedHunk(CursorIndex);
			} else {
				m_activePane = ActivePane::Right;
				SyncActiveCursorToScreenIndex(CursorIndex);
			}
		}
		UpdateActivePane();
		Show();
	}

	void UpdateActivePane()
	{
		m_leftPane.SetActive(!m_gutterActive && m_activePane == ActivePane::Left);
		m_rightPane.SetActive(!m_gutterActive && m_activePane == ActivePane::Right);
		if (m_gutterActive)
			SetCursorType(FALSE, 0);
	}

	DiffEditorPane &ActiveEditorPane()
	{
		return m_activePane == ActivePane::Left ? m_leftPane : m_rightPane;
	}

	const DiffEditorPane &ActiveEditorPane() const
	{
		return m_activePane == ActivePane::Left ? m_leftPane : m_rightPane;
	}

	DiffEditorPane &Pane(ActivePane Pane)
	{
		return Pane == ActivePane::Left ? m_leftPane : m_rightPane;
	}

	const DiffEditorPane &Pane(ActivePane Pane) const
	{
		return Pane == ActivePane::Left ? m_leftPane : m_rightPane;
	}

	ActivePane SourcePane(MergeDirection Direction) const
	{
		return Direction == MergeDirection::LeftToRight ? ActivePane::Left : ActivePane::Right;
	}

	ActivePane TargetPane(MergeDirection Direction) const
	{
		return Direction == MergeDirection::LeftToRight ? ActivePane::Right : ActivePane::Left;
	}

	int RowLine(const DiffRow &Row, ActivePane Pane) const
	{
		return Pane == ActivePane::Left ? Row.Left : Row.Right;
	}

	LineRange HunkRange(size_t HunkIndex, ActivePane PaneSide) const
	{
		LineRange Range;
		if (HunkIndex >= m_hunks.size())
			return Range;

		const DiffHunk &Hunk = m_hunks[HunkIndex];
		for (size_t I = Hunk.FirstRow; I < Hunk.LastRow; ++I) {
			const int Line = RowLine(m_rows[I], PaneSide);
			if (Line < 0)
				continue;
			if (!Range.HasLines) {
				Range.Begin = Line;
				Range.HasLines = true;
			}
			Range.End = Line + 1;
		}

		if (!Range.HasLines) {
			Range.Begin = InsertionLineForHunk(Hunk, PaneSide);
			Range.End = Range.Begin;
		}
		return Range;
	}

	int InsertionLineForHunk(const DiffHunk &Hunk, ActivePane PaneSide) const
	{
		for (size_t I = Hunk.LastRow; I < m_rows.size(); ++I) {
			const int Line = RowLine(m_rows[I], PaneSide);
			if (Line >= 0)
				return Line;
		}
		return static_cast<int>(Pane(PaneSide).Lines().size());
	}

	bool CanMergeDirection(size_t HunkIndex, MergeDirection Direction) const
	{
		return HunkRange(HunkIndex, SourcePane(Direction)).HasLines;
	}

	bool CanMergeHunk(size_t HunkIndex) const
	{
		return CanMergeDirection(HunkIndex, MergeDirection::LeftToRight)
				|| CanMergeDirection(HunkIndex, MergeDirection::RightToLeft);
	}

	void ExtractHunkLines(size_t HunkIndex, ActivePane PaneSide, std::vector<FARString> &Lines) const
	{
		Lines.clear();
		const LineRange Range = HunkRange(HunkIndex, PaneSide);
		if (!Range.HasLines)
			return;

		const std::vector<FARString> &PaneLines = Pane(PaneSide).Lines();
		for (int I = Range.Begin; I < Range.End && I < static_cast<int>(PaneLines.size()); ++I)
			Lines.emplace_back(PaneLines[I]);
	}

	bool SelectGutterHunk(size_t HunkIndex, MergeDirection PreferredDirection)
	{
		if (HunkIndex >= m_hunks.size()) {
			m_selectedHunk = InvalidIndex;
			return false;
		}

		m_selectedHunk = HunkIndex;
		if (CanMergeDirection(HunkIndex, PreferredDirection)) {
			m_selectedDirection = PreferredDirection;
			return true;
		}

		if (CanMergeDirection(HunkIndex, MergeDirection::LeftToRight)) {
			m_selectedDirection = MergeDirection::LeftToRight;
			return true;
		}

		if (CanMergeDirection(HunkIndex, MergeDirection::RightToLeft)) {
			m_selectedDirection = MergeDirection::RightToLeft;
			return true;
		}

		return false;
	}

	size_t VisibleMergeHunk(MergeDirection PreferredDirection) const
	{
		const size_t CurrentHunk = HunkIndexForRow(CurrentDiffRow());
		if (CurrentHunk != InvalidIndex && VisibleHunkActionScreenIndex(CurrentHunk) != InvalidIndex
				&& CanMergeHunk(CurrentHunk)) {
			return CurrentHunk;
		}

		for (size_t I = 0; I < m_hunks.size(); ++I) {
			if (VisibleHunkActionScreenIndex(I) != InvalidIndex && CanMergeDirection(I, PreferredDirection))
				return I;
		}

		for (size_t I = 0; I < m_hunks.size(); ++I) {
			if (VisibleHunkActionScreenIndex(I) != InvalidIndex && CanMergeHunk(I))
				return I;
		}
		return InvalidIndex;
	}

	bool ActivateGutter(MergeDirection PreferredDirection)
	{
		const size_t HunkIndex = VisibleMergeHunk(PreferredDirection);
		if (HunkIndex == InvalidIndex) {
			m_gutterActive = false;
			m_selectedHunk = InvalidIndex;
			return false;
		}

		m_gutterActive = true;
		return SelectGutterHunk(HunkIndex, PreferredDirection);
	}

	void SelectGutterDirection(MergeDirection Direction)
	{
		if (m_selectedHunk != InvalidIndex && CanMergeDirection(m_selectedHunk, Direction))
			m_selectedDirection = Direction;
	}

	bool CanMergeCurrentSelection() const
	{
		const size_t HunkIndex = m_gutterActive ? m_selectedHunk : HunkIndexForRow(CurrentDiffRow());
		if (HunkIndex == InvalidIndex || HunkIndex >= m_hunks.size())
			return false;

		const MergeDirection Direction = m_gutterActive ? m_selectedDirection :
				m_activePane == ActivePane::Left ? MergeDirection::LeftToRight : MergeDirection::RightToLeft;
		return CanMergeDirection(HunkIndex, Direction);
	}

	bool CanNavigateDiff(int Direction) const
	{
		return Direction != 0 && FindNavigationHunk(CurrentDiffRow(), Direction) != InvalidIndex;
	}

	bool StatusActionEnabled(StatusAction Action) const
	{
		switch (Action) {
			case StatusAction::Save:
				return CanSaveActivePane();
			case StatusAction::Merge:
				return CanMergeCurrentSelection();
			case StatusAction::PrevDiff:
				return CanNavigateDiff(-1);
			case StatusAction::NextDiff:
				return CanNavigateDiff(1);
		}
		return false;
	}

	void SyncActiveCursorToSelectedHunk(size_t FallbackScreenIndex)
	{
		size_t ScreenIndex = InvalidIndex;
		if (m_selectedHunk != InvalidIndex && m_selectedHunk < m_hunks.size()) {
			ScreenIndex = VisibleHunkActionScreenIndex(m_selectedHunk);
			if (ScreenIndex == InvalidIndex)
				ScreenIndex = FirstScreenIndexForRow(m_hunks[m_selectedHunk].FirstRow);
		}
		SyncActiveCursorToScreenIndex(ScreenIndex != InvalidIndex ? ScreenIndex : FallbackScreenIndex);
	}

	void MergeCurrentSelection()
	{
		FlushPendingDiffRefresh(true);
		const size_t HunkIndex = m_gutterActive ? m_selectedHunk : HunkIndexForRow(CurrentDiffRow());
		if (HunkIndex == InvalidIndex || HunkIndex >= m_hunks.size())
			return;

		const MergeDirection Direction = m_gutterActive ? m_selectedDirection :
				m_activePane == ActivePane::Left ? MergeDirection::LeftToRight : MergeDirection::RightToLeft;
		MergeHunk(HunkIndex, Direction);
	}

	void MergeHunk(size_t HunkIndex, MergeDirection Direction)
	{
		if (!CanMergeDirection(HunkIndex, Direction))
			return;

		const bool GutterWasActive = m_gutterActive;
		const ActivePane ActivePaneBefore = m_activePane;
		const int CursorLine = ActiveEditorPane().CursorLine();
		const int CursorVisualLine = ActiveEditorPane().CursorVisualLine();
		const int CursorCol = ActiveEditorPane().CursorCol();
		const LineRange TargetRange = HunkRange(HunkIndex, TargetPane(Direction));
		std::vector<FARString> NewLines;
		ExtractHunkLines(HunkIndex, SourcePane(Direction), NewLines);

		DiffEditorPane &Target = Pane(TargetPane(Direction));
		if (!Target.ReplaceLines(TargetRange.Begin, TargetRange.End - TargetRange.Begin, NewLines)) {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, L"Compare files", L"Cannot merge hunk.",
					Target.Path().CPtr(), Msg::Ok);
			return;
		}

		const size_t NextHunkHint = HunkIndex;
		m_leftPane.RefreshLinesFromEditor();
		m_rightPane.RefreshLinesFromEditor();
		m_pendingDiffRefresh = false;
		RebuildDiffModel();

		if (GutterWasActive) {
			if (!m_hunks.empty()) {
				m_gutterActive = true;
				SelectGutterHunk(std::min(NextHunkHint, m_hunks.size() - 1), Direction);
				EnsureSelectedHunkVisible();
			} else {
				m_gutterActive = false;
				m_selectedHunk = InvalidIndex;
				m_activePane = TargetPane(Direction);
				EnsureActiveCursorVisible();
			}
		} else {
			m_gutterActive = false;
			m_activePane = ActivePaneBefore;
			SyncActiveCursorByLine(CursorLine, CursorVisualLine, CursorCol);
		}

		UpdateActivePane();
		Show();
	}

	void RebuildDiffModel()
	{
		m_rows = BuildLineDiff(m_leftPane.Lines(), m_rightPane.Lines());
		BuildDiffHunks();
		ResetInlineDiffs();
		RebuildScreenRows();
	}

	void ScheduleDiffRefresh()
	{
		m_pendingDiffRefresh = true;
		m_lastEditTick = WINPORT(GetTickCount)();
	}

	bool FlushPendingDiffRefresh(bool Force)
	{
		if (!m_pendingDiffRefresh)
			return false;

		if (!Force && WINPORT(GetTickCount)() - m_lastEditTick < DiffRefreshDelay)
			return false;

		const int CursorLine = ActiveEditorPane().CursorLine();
		const int CursorVisualLine = ActiveEditorPane().CursorVisualLine();
		const int CursorCol = ActiveEditorPane().CursorCol();
		const bool LeftChanged = m_leftPane.RefreshLinesFromEditor();
		const bool RightChanged = m_rightPane.RefreshLinesFromEditor();
		m_pendingDiffRefresh = false;
		if (!LeftChanged && !RightChanged)
			return false;

		RebuildDiffModel();
		SyncActiveCursorByLine(CursorLine, CursorVisualLine, CursorCol);
		EnsureActiveCursorVisible();
		return true;
	}

	void RebuildDiffFromEditors()
	{
		const int CursorLine = ActiveEditorPane().CursorLine();
		const int CursorVisualLine = ActiveEditorPane().CursorVisualLine();
		const int CursorCol = ActiveEditorPane().CursorCol();
		m_leftPane.RefreshLinesFromEditor();
		m_rightPane.RefreshLinesFromEditor();
		m_pendingDiffRefresh = false;
		RebuildDiffModel();
		SyncActiveCursorByLine(CursorLine, CursorVisualLine, CursorCol);
		EnsureActiveCursorVisible();
	}

	void SaveActivePane()
	{
		if (!CanSaveActivePane())
			return;

		FARString SaveFile = m_activePane == ActivePane::Left ? L"Left file: " : L"Right file: ";
		SaveFile+= ActiveEditorPane().Path();
		const int Choice = Message(MSG_WARNING, 2, L"Compare files", L"Save file?",
				SaveFile.CPtr(), Msg::HYes, Msg::HNo);
		if (Choice != 0)
			return;

		if (!ActiveEditorPane().Save()) {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, L"Compare files", L"Cannot save file.",
					ActiveEditorPane().Path().CPtr(), Msg::Ok);
			return;
		}

		RebuildDiffFromEditors();
		Show();
	}

	bool CanSaveActivePane() const
	{
		return ActiveEditorPane().Modified();
	}

	FARString BuildStatusInfo() const
	{
		FARString HunkStatus;
		const size_t CurrentHunk = HunkIndexForRow(CurrentDiffRow());
		if (CurrentHunk != InvalidIndex) {
			HunkStatus.Format(L"Hunk: %u/%u", static_cast<unsigned>(CurrentHunk + 1),
					static_cast<unsigned>(m_hunks.size()));
		} else {
			HunkStatus.Format(L"Hunks: %u", static_cast<unsigned>(m_hunks.size()));
		}

		const wchar_t *ActiveName = m_gutterActive ? L"gutter" :
				m_activePane == ActivePane::Left ? L"left" : L"right";
		FormatString Info;
		Info << L"Active: " << ActiveName << (!m_gutterActive && ActiveEditorPane().Modified() ? L"*" : L"")
				<< L"  " << HunkStatus
				<< L"  Modified: " << (m_leftPane.Modified() ? L"L" : L"-")
				<< (m_rightPane.Modified() ? L"R" : L"-")
				<< L"  Lines: " << static_cast<UINT64>(m_leftPane.Lines().size()) << L'/'
				<< static_cast<UINT64>(m_rightPane.Lines().size())
				<< L"  Diff rows: " << static_cast<UINT64>(m_rows.size());
		return std::move(Info.strValue());
	}

	bool SaveModifiedPanes()
	{
		if (m_leftPane.Modified() && !m_leftPane.Save()) {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, L"Compare files", L"Cannot save file.",
					m_leftPane.Path().CPtr(), Msg::Ok);
			return false;
		}

		if (m_rightPane.Modified() && !m_rightPane.Save()) {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, L"Compare files", L"Cannot save file.",
					m_rightPane.Path().CPtr(), Msg::Ok);
			return false;
		}

		RebuildDiffFromEditors();
		return true;
	}

	void Close()
	{
		if (!m_leftPane.Modified() && !m_rightPane.Modified()) {
			FrameManager->DeleteFrame();
			return;
		}

		FARString LeftModified, RightModified;
		Messager MessageBuilder(L"Compare files");
		MessageBuilder.Add(L"Save changed files before closing?");
		if (m_leftPane.Modified()) {
			LeftModified = L"Left file: ";
			LeftModified+= m_leftPane.Path();
			MessageBuilder.Add(LeftModified.CPtr());
		}
		if (m_rightPane.Modified()) {
			RightModified = L"Right file: ";
			RightModified+= m_rightPane.Path();
			MessageBuilder.Add(RightModified.CPtr());
		}

		const int Choice = MessageBuilder.Add(Msg::HYes, Msg::HNo, Msg::HCancel).Show(MSG_WARNING, 3);
		if (Choice == 0) {
			if (!SaveModifiedPanes())
				return;
		} else if (Choice != 1) {
			return;
		}

		FrameManager->DeleteFrame();
	}

	void SelectPaneForRow(const DiffRow &Row)
	{
		if (m_activePane == ActivePane::Left && Row.Left < 0 && Row.Right >= 0)
			m_activePane = ActivePane::Right;
		else if (m_activePane == ActivePane::Right && Row.Right < 0 && Row.Left >= 0)
			m_activePane = ActivePane::Left;
		UpdateActivePane();
	}

	void GetFirstVisiblePositions(int &LeftLine, int &LeftVisualLine, int &RightLine, int &RightVisualLine) const
	{
		LeftLine = -1;
		LeftVisualLine = -1;
		RightLine = -1;
		RightVisualLine = -1;
		const size_t Page = VisibleRows();
		for (size_t I = 0; I < Page && m_top + I < m_screenRows.size(); ++I) {
			const ScreenRow &Screen = m_screenRows[m_top + I];
			const DiffRow &Row = m_rows[Screen.Row];
			if (LeftLine < 0 && Row.Left >= 0) {
				LeftLine = Row.Left;
				LeftVisualLine = static_cast<int>(Screen.Part);
			}
			if (RightLine < 0 && Row.Right >= 0) {
				RightLine = Row.Right;
				RightVisualLine = static_cast<int>(Screen.Part);
			}
			if (LeftLine >= 0 && RightLine >= 0)
				break;
		}
	}

	void FirstVisiblePosition(ActivePane Pane, int &Line, int &VisualLine) const
	{
		int LeftLine = -1;
		int LeftVisualLine = -1;
		int RightLine = -1;
		int RightVisualLine = -1;
		GetFirstVisiblePositions(LeftLine, LeftVisualLine, RightLine, RightVisualLine);
		Line = Pane == ActivePane::Left ? LeftLine : RightLine;
		VisualLine = Pane == ActivePane::Left ? LeftVisualLine : RightVisualLine;
	}

	bool GutterActionAt(const MOUSE_EVENT_RECORD &MouseEvent, size_t &HunkIndex, MergeDirection &Direction) const
	{
		const int X = MouseEvent.dwMousePosition.X;
		const int Y = MouseEvent.dwMousePosition.Y;
		if (!MouseInsideGutter(X) || Y < Y1 + 1 || Y > Y2 - 1)
			return false;

		const size_t ScreenIndex = m_top + static_cast<size_t>(Y - (Y1 + 1));
		if (ScreenIndex >= m_screenRows.size())
			return false;

		const ScreenRow &Screen = m_screenRows[ScreenIndex];
		HunkIndex = HunkIndexForRow(Screen.Row);
		if (HunkIndex == InvalidIndex)
			return false;

		const bool CanRightToLeft = CanMergeDirection(HunkIndex, MergeDirection::RightToLeft);
		const bool CanLeftToRight = CanMergeDirection(HunkIndex, MergeDirection::LeftToRight);
		if (m_gutterWidth >= 3) {
			if (X == GutterX1() && CanRightToLeft) {
				Direction = MergeDirection::RightToLeft;
				return true;
			}
			if (X == GutterX2() && CanLeftToRight) {
				Direction = MergeDirection::LeftToRight;
				return true;
			}
			return false;
		}

		if (CanLeftToRight != CanRightToLeft) {
			Direction = CanLeftToRight ? MergeDirection::LeftToRight : MergeDirection::RightToLeft;
			return true;
		}
		return false;
	}

	bool SelectGutterAction(size_t HunkIndex, MergeDirection Direction, bool Activate)
	{
		const bool Changed = (Activate && !m_gutterActive) || m_selectedHunk != HunkIndex
				|| m_selectedDirection != Direction;
		if (Activate)
			m_gutterActive = true;
		m_selectedHunk = HunkIndex;
		m_selectedDirection = Direction;
		if (Activate)
			UpdateActivePane();
		return Changed;
	}

	int ProcessGutterMouse(const MOUSE_EVENT_RECORD &MouseEvent)
	{
		size_t HunkIndex = InvalidIndex;
		MergeDirection Direction = MergeDirection::LeftToRight;
		if (!GutterActionAt(MouseEvent, HunkIndex, Direction))
			return TRUE;

		const bool Click = IsFocusClick(MouseEvent);
		const bool Changed = SelectGutterAction(HunkIndex, Direction, Click);
		if (Click) {
			MergeCurrentSelection();
			return TRUE;
		}

		if (Changed)
			Show();
		return TRUE;
	}

	void ExecuteStatusAction(StatusAction Action)
	{
		switch (Action) {
			case StatusAction::Save:
				if (CanSaveActivePane())
					SaveActivePane();
				break;
			case StatusAction::Merge:
				if (CanMergeCurrentSelection())
					MergeCurrentSelection();
				break;
			case StatusAction::PrevDiff:
				FlushPendingDiffRefresh(true);
				if (CanNavigateDiff(-1))
					NavigateDiff(-1);
				break;
			case StatusAction::NextDiff:
				FlushPendingDiffRefresh(true);
				if (CanNavigateDiff(1))
					NavigateDiff(1);
				break;
		}
	}

	bool LayoutStatusButton(int &X, int MaxX, const wchar_t *Label, bool &NeedSpace, int &ButtonX1,
			int &ButtonX2) const
	{
		const FARString TextLabel = Label;
		const int LabelWidth = static_cast<int>(TextLabel.CellsCount()) + 2;
		const int SpaceWidth = NeedSpace ? 1 : 0;
		if (LabelWidth <= 0 || X + SpaceWidth + LabelWidth - 1 > MaxX)
			return false;

		if (NeedSpace)
			++X;

		ButtonX1 = X;
		ButtonX2 = ButtonX1 + LabelWidth - 1;
		X+= LabelWidth;
		NeedSpace = true;
		return true;
	}

	int ProcessStatusMouse(const MOUSE_EVENT_RECORD &MouseEvent)
	{
		if (MouseEvent.dwMousePosition.Y != Y2 || MouseEvent.dwEventFlags != 0
				|| !(MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
			return FALSE;
		}

		const int X = MouseEvent.dwMousePosition.X;
		for (const StatusButton &Button : m_statusButtons) {
			if (X < Button.X1 || X > Button.X2)
				continue;

			if (Button.Enabled)
				ExecuteStatusAction(Button.Action);
			return TRUE;
		}
		return FALSE;
	}

	bool ProcessPaneMouse(const MOUSE_EVENT_RECORD &MouseEvent)
	{
		const int Y = MouseEvent.dwMousePosition.Y;
		if (Y < Y1 + 1 || Y > Y2 - 1)
			return false;

		const size_t ScreenIndex = m_top + static_cast<size_t>(Y - (Y1 + 1));
		if (ScreenIndex >= m_screenRows.size())
			return false;

		const ScreenRow &Screen = m_screenRows[ScreenIndex];
		const DiffRow &Row = m_rows[Screen.Row];
		const int Line = m_activePane == ActivePane::Left ? Row.Left : Row.Right;
		if (Line < 0)
			return false;

		int FirstLine = -1;
		int FirstVisualLine = -1;
		FirstVisiblePosition(m_activePane, FirstLine, FirstVisualLine);
		return ActiveEditorPane().ProcessMouseAtLine(MouseEvent, Line, static_cast<int>(Screen.Part),
				FirstLine, FirstVisualLine);
	}

	bool MouseInsideActivePane(const MOUSE_EVENT_RECORD &MouseEvent) const
	{
		const int X = MouseEvent.dwMousePosition.X;
		const int Y = MouseEvent.dwMousePosition.Y;
		if (Y < Y1 + 1 || Y > Y2 - 1)
			return false;
		return m_activePane == ActivePane::Left ? X >= X1 && X < GutterX1() : X >= RightPaneX1() && X <= X2;
	}

	void EnsureActiveCursorVisible()
	{
		const size_t Candidate = ActiveCursorScreenIndex();

		if (Candidate == InvalidIndex)
			return;

		const size_t Page = VisibleRows();
		if (Candidate < m_top)
			m_top = Candidate;
		else if (Page != 0 && Candidate >= m_top + Page)
			m_top = Candidate - Page + 1;
		m_top = std::min(m_top, MaxTop());
	}

	void EnsureSelectedHunkVisible()
	{
		if (m_selectedHunk == InvalidIndex || m_selectedHunk >= m_hunks.size())
			return;

		size_t Candidate = VisibleHunkActionScreenIndex(m_selectedHunk);
		if (Candidate == InvalidIndex)
			Candidate = FirstScreenIndexForRow(m_hunks[m_selectedHunk].FirstRow);
		if (Candidate == InvalidIndex)
			return;

		const size_t Page = VisibleRows();
		if (Candidate < m_top)
			m_top = Candidate;
		else if (Page != 0 && Candidate >= m_top + Page)
			m_top = Candidate - Page + 1;
		m_top = std::min(m_top, MaxTop());
	}

	void PlaceActiveCursorForSearch()
	{
		const size_t Candidate = ActiveCursorScreenIndex();
		const size_t Page = VisibleRows();

		if (Candidate == InvalidIndex || Page == 0)
			return;

		const size_t FromTop = Page / 4;
		m_top = Candidate > FromTop ? Candidate - FromTop : 0;
		m_top = std::min(m_top, MaxTop());
	}

	void SyncActiveCursorToScreenIndex(size_t ScreenIndex)
	{
		if (ScreenIndex == InvalidIndex || ScreenIndex >= m_screenRows.size())
			return;

		ScreenRow Screen = m_screenRows[ScreenIndex];
		DiffRow Row = m_rows[Screen.Row];
		int Line = m_activePane == ActivePane::Left ? Row.Left : Row.Right;

		if (Line < 0) {
			const size_t Page = VisibleRows();
			const size_t ViewEnd = std::min(m_top + Page, m_screenRows.size());
			for (int Pass = 0; Line < 0 && Pass < 2; ++Pass) {
				for (size_t Distance = 1; Distance < m_screenRows.size(); ++Distance) {
					bool Found = false;
					for (const int Direction : {-1, 1}) {
						const int64_t CandidateIndex = static_cast<int64_t>(ScreenIndex)
								+ static_cast<int64_t>(Direction) * static_cast<int64_t>(Distance);
						if (CandidateIndex < 0 || CandidateIndex >= static_cast<int64_t>(m_screenRows.size()))
							continue;
						if (Pass == 0
								&& (Page == 0 || static_cast<size_t>(CandidateIndex) < m_top
										|| static_cast<size_t>(CandidateIndex) >= ViewEnd)) {
							continue;
						}

						Screen = m_screenRows[CandidateIndex];
						Row = m_rows[Screen.Row];
						Line = m_activePane == ActivePane::Left ? Row.Left : Row.Right;
						if (Line >= 0) {
							Found = true;
							break;
						}
					}
					if (Found)
						break;
				}
			}
		}

		if (Line < 0)
			return;

		const int VisualLine = std::min(static_cast<int>(Screen.Part), ActiveEditorPane().VisualLineCount(Line) - 1);
		ActiveEditorPane().SetCursorByVisualLine(Line, VisualLine, 0);
		EnsureActiveCursorVisible();
	}

	void SyncActiveCursorByLine(int CursorLine, int CursorVisualLine, int CursorCol)
	{
		size_t Candidate = InvalidIndex;
		size_t CandidatePart = 0;
		for (size_t I = 0; I < m_screenRows.size(); ++I) {
			const ScreenRow &Screen = m_screenRows[I];
			const DiffRow &Row = m_rows[Screen.Row];
			const int Line = m_activePane == ActivePane::Left ? Row.Left : Row.Right;
			if (Line != CursorLine)
				continue;

			if (static_cast<int>(Screen.Part) == CursorVisualLine) {
				Candidate = I;
				CandidatePart = Screen.Part;
				break;
			}
			if (Candidate == InvalidIndex) {
				Candidate = I;
				CandidatePart = Screen.Part;
			}
		}

		if (Candidate != InvalidIndex) {
			ActiveEditorPane().SetCursorByVisualLine(CursorLine, static_cast<int>(CandidatePart), CursorCol);
			EnsureActiveCursorVisible();
		}
	}

	size_t ActiveCursorScreenIndex() const
	{
		const int CursorLine = ActiveEditorPane().CursorLine();
		const int CursorVisualLine = ActiveEditorPane().CursorVisualLine();
		size_t Candidate = InvalidIndex;

		for (size_t I = 0; I < m_screenRows.size(); ++I) {
			const ScreenRow &Screen = m_screenRows[I];
			const DiffRow &Row = m_rows[Screen.Row];
			const int Line = m_activePane == ActivePane::Left ? Row.Left : Row.Right;
			if (Line != CursorLine)
				continue;

			if (static_cast<int>(Screen.Part) == CursorVisualLine) {
				Candidate = I;
				break;
			}
			if (Candidate == InvalidIndex)
				Candidate = I;
		}

		return Candidate;
	}

	bool ActiveCursorVisible() const
	{
		const size_t CursorIndex = ActiveCursorScreenIndex();
		if (CursorIndex == InvalidIndex)
			return false;
		const size_t Page = VisibleRows();
		return CursorIndex >= m_top && CursorIndex < m_top + Page;
	}

	void UpdateCursorVisibilityForViewport()
	{
		if (m_gutterActive) {
			m_leftPane.SetActive(false);
			m_rightPane.SetActive(false);
			SetCursorType(FALSE, 0);
			return;
		}

		const bool Visible = ActiveCursorVisible();
		m_leftPane.SetActive(Visible && m_activePane == ActivePane::Left);
		m_rightPane.SetActive(Visible && m_activePane == ActivePane::Right);
		if (!Visible)
			SetCursorType(FALSE, 0);
	}

	void RebuildScreenRows()
	{
		const int TotalWidth = std::max(1, ObjWidth);
		m_gutterWidth = TotalWidth >= 5 ? PreferredGutterWidth : TotalWidth >= 3 ? 1 : 0;
		const int PanesWidth = std::max(1, TotalWidth - m_gutterWidth);
		m_leftWidth = std::max(1, PanesWidth / 2);
		m_rightWidth = std::max(1, PanesWidth - m_leftWidth);
		m_leftPane.SetPosition(X1, Y1 + 1, X1 + m_leftWidth - 1, Y2 - 1);
		m_rightPane.SetPosition(RightPaneX1(), Y1 + 1, X2, Y2 - 1);

		m_screenRows.clear();
		for (size_t I = 0; I < m_rows.size(); ++I) {
			const DiffRow &Row = m_rows[I];
			const size_t Parts = std::max(static_cast<size_t>(m_leftPane.VisualLineCount(Row.Left)),
					static_cast<size_t>(m_rightPane.VisualLineCount(Row.Right)));
			for (size_t Part = 0; Part < Parts; ++Part)
				m_screenRows.push_back({I, Part});
		}
		if (m_screenRows.empty())
			m_screenRows.push_back({});
		m_top = std::min(m_top, MaxTop());
	}

	void BuildDiffHunks()
	{
		m_hunks.clear();
		for (size_t I = 0; I < m_rows.size();) {
			if (m_rows[I].Kind == DiffKind::Equal) {
				++I;
				continue;
			}

			const size_t FirstRow = I;
			while (I < m_rows.size() && m_rows[I].Kind != DiffKind::Equal)
				++I;
			m_hunks.push_back({FirstRow, I});
		}
	}

	void ResetInlineDiffs()
	{
		m_inlineDiffs.clear();
		m_inlineDiffs.resize(m_rows.size());
	}

	const InlineDiff &InlineDiffForRow(size_t RowIndex)
	{
		static const InlineDiff Empty;
		if (RowIndex >= m_rows.size() || RowIndex >= m_inlineDiffs.size())
			return Empty;

		const DiffRow &Row = m_rows[RowIndex];
		if (Row.Kind != DiffKind::Changed || Row.Left < 0 || Row.Right < 0)
			return Empty;

		std::unique_ptr<InlineDiff> &Inline = m_inlineDiffs[RowIndex];
		if (!Inline) {
			Inline.reset(new InlineDiff);
			BuildInlineDiffRanges(m_leftPane.Lines()[Row.Left], m_rightPane.Lines()[Row.Right],
					Inline->Left, Inline->Right);
		}

		return *Inline;
	}

	size_t FirstScreenIndexForRow(size_t Row) const
	{
		for (size_t I = 0; I < m_screenRows.size(); ++I) {
			if (m_screenRows[I].Row == Row)
				return I;
		}
		return InvalidIndex;
	}

	size_t CurrentDiffRow() const
	{
		if (m_gutterActive && m_selectedHunk != InvalidIndex && m_selectedHunk < m_hunks.size())
			return m_hunks[m_selectedHunk].FirstRow;

		const size_t CursorIndex = ActiveCursorScreenIndex();
		if (CursorIndex != InvalidIndex && CursorIndex < m_screenRows.size())
			return m_screenRows[CursorIndex].Row;
		if (m_top < m_screenRows.size())
			return m_screenRows[m_top].Row;
		return InvalidIndex;
	}

	size_t HunkIndexForRow(size_t Row) const
	{
		for (size_t I = 0; I < m_hunks.size(); ++I) {
			if (Row >= m_hunks[I].FirstRow && Row < m_hunks[I].LastRow)
				return I;
		}
		return InvalidIndex;
	}

	size_t VisibleHunkActionScreenIndex(size_t HunkIndex) const
	{
		if (HunkIndex >= m_hunks.size())
			return InvalidIndex;

		const DiffHunk &Hunk = m_hunks[HunkIndex];
		const size_t ViewEnd = std::min(m_top + VisibleRows(), m_screenRows.size());
		size_t First = InvalidIndex;
		size_t Last = InvalidIndex;
		for (size_t I = m_top; I < ViewEnd; ++I) {
			const size_t Row = m_screenRows[I].Row;
			if (Row < Hunk.FirstRow)
				continue;
			if (Row >= Hunk.LastRow) {
				if (First != InvalidIndex)
					break;
				continue;
			}

			if (First == InvalidIndex)
				First = I;
			Last = I;
		}

		return First == InvalidIndex ? InvalidIndex : First + (Last - First) / 2;
	}

	void DrawHeader()
	{
		SetScreen(X1, Y1, X2, Y1, L' ', FarColorToReal(COL_VIEWERSTATUS));
		DrawHeaderSide(X1, m_leftWidth, m_leftPath + (m_leftPane.Modified() ? L"*" : L""));
		DrawGutterHeader();
		DrawHeaderSide(RightPaneX1(), m_rightWidth, m_rightPath + (m_rightPane.Modified() ? L"*" : L""));
	}

	void DrawHeaderSide(int X, int Width, FARString Path)
	{
		if (Width <= 0)
			return;
		TruncPathStr(Path, Width);
		Text(X, Y1, FarColorToReal(COL_VIEWERSTATUS), Path.CPtr(), Path.GetLength());
	}

	void DrawGutterHeader()
	{
		if (m_gutterWidth <= 0)
			return;

		SetScreen(GutterX1(), Y1, GutterX2(), Y1, L' ', FarColorToReal(COL_VIEWERSTATUS));
		if (m_gutterWidth >= 3) {
			const wchar_t Gutter[] = {L' ', BoxSymbols[BS_V1], L' '};
			Text(GutterX1(), Y1, FarColorToReal(COL_VIEWERSTATUS), Gutter, ARRAYSIZE(Gutter));
		} else {
			Text(GutterX1(), Y1, FarColorToReal(COL_VIEWERSTATUS), &BoxSymbols[BS_V1], 1);
		}
	}

	bool DrawStatusButton(int &X, int MaxX, const wchar_t *Label, StatusAction Action, bool Enabled,
			bool &NeedSpace)
	{
		const int SpaceX = X;
		const bool HadSpace = NeedSpace;
		int ButtonX1 = 0;
		int ButtonX2 = 0;
		if (!LayoutStatusButton(X, MaxX, Label, NeedSpace, ButtonX1, ButtonX2))
			return false;

		if (HadSpace)
			Text(SpaceX, Y2, FarColorToReal(COL_VIEWERSTATUS), L" ", 1);

		FARString TextLabel = Label;
		const wchar_t LeftBracket = L'[';
		const wchar_t RightBracket = L']';
		const uint64_t PrefixColor = FarColorToReal(Enabled ? COL_MENUPREFIX : COL_MENUDISABLEDTEXT);
		const uint64_t TextColor = FarColorToReal(Enabled ? COL_MENUTEXT : COL_MENUDISABLEDTEXT);
		Text(ButtonX1, Y2, PrefixColor, &LeftBracket, 1);
		Text(ButtonX1 + 1, Y2, TextColor, TextLabel.CPtr(), TextLabel.GetLength());
		Text(ButtonX2, Y2, PrefixColor, &RightBracket, 1);
		m_statusButtons.push_back({Action, Enabled, ButtonX1, ButtonX2});
		return true;
	}

	void DrawStatusHelp(int X, int Width)
	{
		if (Width <= 0)
			return;

		int DrawX = X;
		const int MaxX = X + Width - 1;
		bool NeedSpace = false;
		DrawStatusButton(DrawX, MaxX, L"F2 Save", StatusAction::Save,
				StatusActionEnabled(StatusAction::Save), NeedSpace);
		DrawStatusButton(DrawX, MaxX, L"F5 Merge", StatusAction::Merge,
				StatusActionEnabled(StatusAction::Merge), NeedSpace);
		DrawStatusButton(DrawX, MaxX, L"Ctrl-^ Prev", StatusAction::PrevDiff,
				StatusActionEnabled(StatusAction::PrevDiff), NeedSpace);
		DrawStatusButton(DrawX, MaxX, L"Ctrl-v Next", StatusAction::NextDiff,
				StatusActionEnabled(StatusAction::NextDiff), NeedSpace);

		const int TailWidth = MaxX - DrawX + 1;
		if (TailWidth > 0) {
			FARString Tail = NeedSpace ? L"  Tab Focus" : L"Tab Focus";
			GotoXY(DrawX, Y2);
			SetFarColor(COL_VIEWERSTATUS);
			FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(TailWidth) << Tail;
		}
	}

	void DrawStatus()
	{
		m_statusButtons.clear();
		const int StatusWidth = X2 - X1 + 1;
		if (StatusWidth == 0)
			return;

		SetFarColor(COL_VIEWERSTATUS);
		GotoXY(X1, Y2);
		FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(StatusWidth) << L"";

		const FARString Info = BuildStatusInfo();
		const int InfoWidth = std::min(StatusWidth, static_cast<int>(Info.CellsCount()));
		const int HelpWidth = InfoWidth == StatusWidth ? 0 : StatusWidth - InfoWidth - 1;
		DrawStatusHelp(X1, HelpWidth);

		if (InfoWidth > 0) {
			GotoXY(X2 - InfoWidth + 1, Y2);
			FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(InfoWidth) << Info;
		}
	}

	void DrawPane(int X, int Y, int Width, const DiffEditorPane &Pane, int LineIndex, DiffKind Kind,
			size_t Part, const std::vector<InlineRange> &InlineRanges, DiffKind InlineKind)
	{
		const uint64_t Color = FarColorToReal(COL_VIEWERTEXT);
		SetScreen(X, Y, X + Width - 1, Y, L' ', Color);

		if (!Pane.RenderLine(LineIndex, static_cast<int>(Part), X, Y, X + Width - 1))
			SetScreen(X, Y, X + Width - 1, Y, L' ', Color);
		ApplyDiffOverlay(X, Y, Width, Kind);
		Pane.ApplyInlineHighlight(LineIndex, static_cast<int>(Part), X, X + Width - 1, Y, InlineRanges, InlineKind);
	}

	void DrawGutter(int Y, size_t ScreenIndex, DiffKind Kind)
	{
		if (m_gutterWidth <= 0)
			return;

		const uint64_t Color = FarColorToReal(COL_VIEWERTEXT);
		SetScreen(GutterX1(), Y, GutterX2(), Y, L' ', Color);
		if (m_gutterWidth >= 3) {
			wchar_t Gutter[] = {L' ', BoxSymbols[BS_V1], L' '};
			const ScreenRow &Screen = m_screenRows[ScreenIndex];
			const size_t HunkIndex = HunkIndexForRow(Screen.Row);
			const bool DrawAction = ScreenIndex == VisibleHunkActionScreenIndex(HunkIndex);
			if (DrawAction) {
				if (CanMergeDirection(HunkIndex, MergeDirection::RightToLeft))
					Gutter[0] = L'\x25C2';
				if (CanMergeDirection(HunkIndex, MergeDirection::LeftToRight))
					Gutter[2] = L'\x25B8';
			}
			Text(GutterX1(), Y, Color, Gutter, ARRAYSIZE(Gutter));
			ApplyDiffOverlay(GutterX1(), Y, m_gutterWidth, Kind);
			if (HunkIndex == m_selectedHunk
					&& CanMergeDirection(HunkIndex, m_selectedDirection)) {
				const int Offset = m_selectedDirection == MergeDirection::RightToLeft ? 0 : 2;
				Text(GutterX1() + Offset, Y, FarColorToReal(COL_EDITORSELECTEDTEXT), &Gutter[Offset], 1);
			}
			return;
		} else {
			Text(GutterX1(), Y, Color, &BoxSymbols[BS_V1], 1);
		}
		ApplyDiffOverlay(GutterX1(), Y, m_gutterWidth, Kind);
	}

	virtual void DisplayObject()
	{
		RebuildScreenRows();
		SyncPaneViewports();
		UpdateCursorVisibilityForViewport();
		DrawHeader();

		const size_t Page = VisibleRows();
		for (size_t I = 0; I < Page; ++I) {
			const int Y = Y1 + 1 + static_cast<int>(I);
			const size_t ScreenIndex = m_top + I;
			if (ScreenIndex >= m_screenRows.size()) {
				SetScreen(X1, Y, X2, Y, L' ', FarColorToReal(COL_VIEWERTEXT));
				continue;
			}

			const ScreenRow &Screen = m_screenRows[ScreenIndex];
			const DiffRow &Row = m_rows[Screen.Row];
			const InlineDiff &Inline = InlineDiffForRow(Screen.Row);
			DrawPane(X1, Y, m_leftWidth, m_leftPane, Row.Left, Row.Kind, Screen.Part,
					Inline.Left, DiffKind::Deleted);
			DrawGutter(Y, ScreenIndex, Row.Kind);
			DrawPane(RightPaneX1(), Y, m_rightWidth, m_rightPane, Row.Right, Row.Kind, Screen.Part,
					Inline.Right, DiffKind::Added);
		}

		DrawStatus();
	}

	void SyncPaneViewports()
	{
		int LeftLine = -1;
		int LeftVisualLine = -1;
		int RightLine = -1;
		int RightVisualLine = -1;
		GetFirstVisiblePositions(LeftLine, LeftVisualLine, RightLine, RightVisualLine);
		m_leftPane.SyncViewport(LeftLine, LeftVisualLine);
		m_rightPane.SyncViewport(RightLine, RightVisualLine);
	}
};
}

void PresentFileDiff()
{
	if (!CtrlObject || !CtrlObject->Cp())
		return;

	Panel *Active = CtrlObject->Cp()->ActivePanel;
	Panel *Passive = CtrlObject->Cp()->GetAnotherPanel(Active);

	FARString LeftPath, RightPath;
	if (!ResolvePanelFile(Active, LeftPath) || !ResolvePanelFile(Passive, RightPath)) {
		Message(MSG_WARNING, 1, L"Compare files", L"Select regular local files on both file panels.", Msg::Ok);
		return;
	}

	FileDiffFrame *Diff = new (std::nothrow) FileDiffFrame(LeftPath, RightPath);
	if (!Diff) {
		Message(MSG_WARNING, 1, L"Compare files", L"Cannot allocate compare view.", Msg::Ok);
		return;
	}
	if (Diff->GetExitCode() == XC_OPEN_ERROR)
		delete Diff;
}

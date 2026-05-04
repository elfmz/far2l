#pragma once

#include "ADBDialogs.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// Single driver for multi-item ops. RunBatch walks vector<WorkUnit>, snapping state to truth between units.
// ALL progress arithmetic lives in ProgressTracker — nothing else writes state.all_complete.

struct WorkUnit
{
	std::wstring display_name;
	uint64_t total_bytes = 0;
	uint64_t total_files = 0;
	bool is_directory = false;
	std::function<int(class ProgressTracker&)> execute;
};

// Sub-progress reporter. Tick advances bytes/count on path-change; TickDisplayOnly only updates filename + bar.
class ProgressTracker
{
public:
	ProgressTracker(ProgressState& state,
	                uint64_t bytes_before, uint64_t files_before,
	                uint64_t unit_bytes, uint64_t unit_files);

	void Tick(int percent, const std::string& sub_path, uint64_t sub_size);
	void TickDisplayOnly(int percent, const std::string& sub_path);
	void Reset();
	bool Aborted() const;
	// Diagnostic — # of unique paths seen via Tick (emit-derived file count).
	uint64_t EmittedPaths() const { return _emitted_paths; }
	uint64_t FilesDone() const { return _files_done; }
	uint64_t BytesDone() const { return _bytes_done; }
	// Marks unit as intentionally skipped (Skip on overwrite prompt) — no success/fail credit.
	void MarkSkipped() { _skipped = true; }
	bool Skipped() const { return _skipped; }
	// Keeps modal alive at near-100% during opaque shell ops (cp/mv) without progress.
	static constexpr int kPinPercent = 99;
	void PinNearDone() { _state.file_complete = kPinPercent; }
	// Escape hatch: raw state access for UI helpers like CheckOverwrite.
	ProgressState& StateRef() { return _state; }

private:
	ProgressState& _state;
	uint64_t _bytes_before;
	uint64_t _files_before;
	uint64_t _unit_bytes;
	uint64_t _unit_files;
	std::string _cur_path;
	uint64_t _cur_size = 0;
	uint64_t _bytes_done = 0;
	uint64_t _files_done = 0;
	int _last_pct = -1;
	bool _skipped = false;
	bool _cur_counted = false;  // current file already credited (reached 100%)
	uint64_t _emitted_paths = 0; // # of unique sub_paths seen — reconcile vs unit_files
};

struct BatchResult
{
	int success_count = 0;
	int last_error = 0;
	bool aborted = false;
};

BatchResult RunBatch(const std::wstring& title,
                     const std::wstring& source_label,
                     const std::wstring& dest_label,
                     std::vector<WorkUnit> units);

// Resolve adb's emitted path → size via basename → [(rel_path, size)] index, suffix-match on collisions. 0 on miss.
uint64_t LookupSubitemSize(
	const std::unordered_map<std::string, std::vector<std::pair<std::string, uint64_t>>>& index,
	const std::string& adb_path);

// Build the lookup index from a flat path → size map (used in pre-scan).
std::unordered_map<std::string, std::vector<std::pair<std::string, uint64_t>>>
	BuildSubitemIndex(const std::unordered_map<std::string, uint64_t>& file_sizes);

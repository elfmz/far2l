#pragma once

#include "MTPDialogs.h"
#include <functional>
#include <string>
#include <vector>

// Multi-item driver: RunBatch walks WorkUnits; all progress arithmetic lives in ProgressTracker (sole writer of state.all_complete / count_complete).
// MTP convention: file_complete / file_total are BYTES (libmtp's progressfunc reports bytes natively); the dialog computes the percent.

struct WorkUnit
{
	std::wstring display_name;
	uint64_t total_bytes = 0;
	uint64_t total_files = 0;
	bool is_directory = false;
	std::function<int(class ProgressTracker&)> execute;
};

class ProgressTracker
{
public:
	ProgressTracker(ProgressState& state,
	                uint64_t bytes_before, uint64_t files_before,
	                uint64_t unit_bytes, uint64_t unit_files);

	// Per-byte-progress callback into a single sub-file; file_id change = file boundary; bytes_done/file_size in bytes, dialog → %.
	void Tick(uint64_t bytes_done, uint64_t file_size, const std::string& file_id);

	// Display only: updates current_file + per-file bar, no global advancement; for "preparation" phases (host-fallback download half) to avoid double-counting bytes.
	void TickDisplayOnly(uint64_t bytes_done, uint64_t file_size, const std::string& file_id);

	// Reset between phases of the same unit (e.g. after pull, before push).
	void Reset();

	bool Aborted() const;
	void MarkSkipped() { _skipped = true; }
	bool Skipped() const { return _skipped; }

	// Keeps modal alive at near-100% during opaque libmtp ops (CopyObject/MoveObject — no progressfunc); else file bar sits at 0% for entire device-side op.
	void PinNearDone(const std::string& file_id);

	ProgressState& StateRef() { return _state; }

	// While display_only=true, Tick→TickDisplayOnly and CompleteFile is no-op — for prep phase of 2-phase unit (pull-then-push fallback) so DownloadRecursive drives dialog visually without crediting bytes/count twice.
	void SetDisplayOnly(bool v) { _display_only = v; }
	bool DisplayOnly() const { return _display_only; }

	// Tells RunBatch to stop loop after this unit (no aborted=true); for unrecoverable errors — equivalent to old `break` after a hard error.
	void HaltBatch() { _halt = true; }
	bool HaltRequested() const { return _halt; }

	// Direct increment for recursive walkers that report per-file completion in one shot (DownloadRecursive style).
	void CompleteFile(uint64_t file_size);

private:
	ProgressState& _state;
	uint64_t _bytes_before;
	uint64_t _files_before;
	uint64_t _unit_bytes;
	uint64_t _unit_files;
	std::string _cur_file_id;
	uint64_t _cur_size = 0;
	uint64_t _bytes_done = 0;
	uint64_t _files_done = 0;
	bool _skipped = false;
	bool _display_only = false;
	bool _halt = false;
};

struct BatchResult
{
	int success_count = 0;
	int last_error = 0;
	bool aborted = false;
};

// is_move/is_upload affect ProgressDialog title; on_abort fires once on Esc so caller wakes libmtp CancellationToken (nullptr = none).
// THREADING CONTRACT (load-bearing): RunBatch is SYNCHRONOUS — spawns worker thread, joins, then returns; any pointer/ref captured by WorkUnit::execute lambda is valid for entire RunBatch call. Callers depend: ctx structs w/ &local_var, [&tr,...] callbacks, on_abort closure. If ever made async, re-audit every call site for UAF.
BatchResult RunBatch(bool is_move, bool is_upload,
                     const std::wstring& source_label,
                     const std::wstring& dest_label,
                     std::vector<WorkUnit> units,
                     std::function<void()> on_abort = nullptr);

// Silent variant: same per-unit accounting + tracker semantics, no dialog; sync, single-threaded — for OPM_SILENT op_mode (Far wants no UI).
BatchResult RunBatchSilent(std::vector<WorkUnit> units);

#include "ProgressBatch.h"
#include "ADBDevice.h"
#include "ADBLog.h"
#include <WideMB.h>
#include <farplug-wide.h>
#include <windows.h>
#include <algorithm>

using ADBUtils::PathBasename;

namespace {

void NudgeDialog()
{
	INPUT_RECORD ir = {};
	ir.EventType = NOOP_EVENT;
	DWORD dw = 0;
	WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
}

}  // namespace

uint64_t LookupSubitemSize(
	const std::unordered_map<std::string, std::vector<std::pair<std::string, uint64_t>>>& index,
	const std::string& adb_path)
{
	if (adb_path.empty()) return 0;
	const std::string bn = PathBasename(adb_path);
	auto it = index.find(bn);
	if (it == index.end() || it->second.empty()) return 0;
	if (it->second.size() == 1) return it->second[0].second;
	// Basename collision: suffix-match in either direction (adb path may be longer or shorter).
	for (const auto& kv : it->second) {
		const auto& rel_path = kv.first;
		if (adb_path == rel_path) return kv.second;
		if (adb_path.size() > rel_path.size()
		    && adb_path.compare(adb_path.size() - rel_path.size(), rel_path.size(), rel_path) == 0
		    && adb_path[adb_path.size() - rel_path.size() - 1] == '/') return kv.second;
		if (rel_path.size() > adb_path.size()
		    && rel_path.compare(rel_path.size() - adb_path.size(), adb_path.size(), adb_path) == 0
		    && rel_path[rel_path.size() - adb_path.size() - 1] == '/') return kv.second;
	}
	return 0;
}

std::unordered_map<std::string, std::vector<std::pair<std::string, uint64_t>>>
BuildSubitemIndex(const std::unordered_map<std::string, uint64_t>& file_sizes)
{
	std::unordered_map<std::string, std::vector<std::pair<std::string, uint64_t>>> idx;
	idx.reserve(file_sizes.size());
	for (const auto& kv : file_sizes) {
		idx[PathBasename(kv.first)].emplace_back(kv.first, kv.second);
	}
	return idx;
}

ProgressTracker::ProgressTracker(ProgressState& state,
                                 uint64_t bytes_before, uint64_t files_before,
                                 uint64_t unit_bytes, uint64_t unit_files)
	: _state(state),
	  _bytes_before(bytes_before),
	  _files_before(files_before),
	  _unit_bytes(unit_bytes),
	  _unit_files(unit_files)
{}

void ProgressTracker::Tick(int percent, const std::string& sub_path, uint64_t sub_size)
{
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;

	bool path_changed = false;
	bool credited_100 = false;

	// Path change = new file. Credit prev file's bytes if it didn't reach 100%.
	if (!sub_path.empty() && sub_path != _cur_path) {
		path_changed = true;
		++_emitted_paths;
		if (!_cur_path.empty() && !_cur_counted) {
			_bytes_done += _cur_size;
			++_files_done;
			DBG("PATH-CHG credit prev='%s' size=%llu → _files_done=%llu _bytes_done=%llu\n",
				PathBasename(_cur_path).c_str(), (unsigned long long)_cur_size,
				(unsigned long long)_files_done, (unsigned long long)_bytes_done);
		}
		_cur_path = sub_path;
		_cur_size = sub_size;
		_cur_counted = false;
		std::lock_guard<std::mutex> lk(_state.mtx_strings);
		_state.current_file = StrMB2Wide(PathBasename(sub_path));
	}

	// One-shot 100% credit — last/only file emits 100% with no follow-up path-change.
	if (percent >= 100 && !_cur_path.empty() && !_cur_counted) {
		_bytes_done += _cur_size;
		++_files_done;
		_cur_counted = true;
		credited_100 = true;
		DBG("100%% credit '%s' size=%llu → _files_done=%llu _bytes_done=%llu\n",
			PathBasename(_cur_path).c_str(), (unsigned long long)_cur_size,
			(unsigned long long)_files_done, (unsigned long long)_bytes_done);
	}

	_state.file_complete = percent;
	if (_cur_size > 0) _state.file_total = _cur_size;

	// Zero contribution if already credited — avoids double-count on 100% Tick.
	uint64_t in_progress = (!_cur_counted && _cur_size > 0) ? (_cur_size * percent) / 100 : 0;
	uint64_t this_unit = _bytes_done + in_progress;
	if (_unit_bytes > 0 && this_unit > _unit_bytes) this_unit = _unit_bytes;
	const uint64_t total_bytes_now = _bytes_before + this_unit;

	// Bytes-derived file count smooths the counter when adb -p skips per-file emits.
	uint64_t bytes_est_files = 0;
	if (_unit_bytes > 0 && _unit_files > 0) {
		bytes_est_files = (_unit_files * this_unit) / _unit_bytes;
		if (bytes_est_files > _unit_files) bytes_est_files = _unit_files;
	}
	uint64_t cnt = _files_before + std::max<uint64_t>(_files_done, bytes_est_files);
	if (_unit_files > 0 && cnt > _files_before + _unit_files) cnt = _files_before + _unit_files;

	// All three monotonic — CAS upward only (no jumps back from glitches/stale values).
	uint64_t prev_count = _state.count_complete.load();
	bool count_advanced = false;
	while (cnt > prev_count && !_state.count_complete.compare_exchange_weak(prev_count, cnt)) {}
	if (cnt > prev_count) count_advanced = true;  // we won the CAS
	// Bump count_total only when we have a real total — otherwise the "of N" grows in lockstep with count_complete.
	if (_state.all_total.load() > 0) {
		uint64_t total_prev = _state.count_total.load();
		while (cnt > total_prev && !_state.count_total.compare_exchange_weak(total_prev, cnt)) {}
	}
	uint64_t bytes_prev = _state.all_complete.load();
	bool bytes_advanced = false;
	while (total_bytes_now > bytes_prev && !_state.all_complete.compare_exchange_weak(bytes_prev, total_bytes_now)) {}
	if (total_bytes_now > bytes_prev) bytes_advanced = true;

	// Log only on real state movement — keeps log readable on busy adb -p emit streams.
	if (path_changed || credited_100 || count_advanced || bytes_advanced) {
		DBG("Tick pct=%d path='%s' sz=%llu | _files=%llu/%llu(unit) bytes=%llu/%llu in_prog=%llu | "
			"cnt=%llu state.cnt=%llu/%llu state.bytes=%llu/%llu\n",
			percent, PathBasename(_cur_path).c_str(), (unsigned long long)_cur_size,
			(unsigned long long)_files_done, (unsigned long long)_unit_files,
			(unsigned long long)_bytes_done, (unsigned long long)_unit_bytes,
			(unsigned long long)in_progress,
			(unsigned long long)cnt,
			(unsigned long long)_state.count_complete.load(),
			(unsigned long long)_state.count_total.load(),
			(unsigned long long)_state.all_complete.load(),
			(unsigned long long)_state.all_total.load());
	}

	if (percent != _last_pct) {
		_last_pct = percent;
		NudgeDialog();
	}
}

void ProgressTracker::TickDisplayOnly(int percent, const std::string& sub_path)
{
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;
	if (!sub_path.empty() && sub_path != _cur_path) {
		_cur_path = sub_path;
		std::lock_guard<std::mutex> lk(_state.mtx_strings);
		_state.current_file = StrMB2Wide(PathBasename(sub_path));
	}
	_state.file_complete = percent;
	if (percent != _last_pct) {
		_last_pct = percent;
		NudgeDialog();
	}
}

void ProgressTracker::Reset()
{
	_cur_path.clear();
	_cur_size = 0;
	_last_pct = -1;
	_cur_counted = false;
}

bool ProgressTracker::Aborted() const
{
	return _state.ShouldAbort();
}

BatchResult RunBatch(const std::wstring& title,
                     const std::wstring& source_label,
                     const std::wstring& dest_label,
                     std::vector<WorkUnit> units)
{
	BatchResult result;
	if (units.empty()) return result;

	uint64_t totalBytes = 0, totalFiles = 0;
	bool any_dir = false;
	for (const auto& u : units) {
		totalBytes += u.total_bytes;
		totalFiles += u.total_files;
		if (u.is_directory) any_dir = true;
	}
	const bool is_multi = units.size() > 1 || totalFiles > 1 || any_dir;

	DBG("RunBatch START units=%zu totalBytes=%llu totalFiles=%llu any_dir=%d\n",
		units.size(), (unsigned long long)totalBytes, (unsigned long long)totalFiles, any_dir);

	ProgressOperation op(title, is_multi);
	op.GetState().all_total = totalBytes;
	op.GetState().count_total = totalFiles;
	op.GetState().source_path = source_label;
	op.GetState().dest_path = dest_label;

	op.Run([&](ProgressState& state) {
		uint64_t bytes_done = 0, files_done = 0;
		size_t unit_idx = 0;
		try {
			for (auto& u : units) {
				if (state.ShouldAbort()) { result.aborted = true; break; }

				DBG("UNIT[%zu/%zu] START name='%ls' dir=%d bytes=%llu files=%llu | "
					"before bytes=%llu files=%llu\n",
					++unit_idx, units.size(), u.display_name.c_str(), u.is_directory,
					(unsigned long long)u.total_bytes, (unsigned long long)u.total_files,
					(unsigned long long)bytes_done, (unsigned long long)files_done);

				{
					std::lock_guard<std::mutex> lk(state.mtx_strings);
					state.current_file = u.display_name;
				}
				state.is_directory = u.is_directory;
				state.file_complete = 0;
				state.file_total = u.total_bytes;

				ProgressTracker tr(state, bytes_done, files_done,
				                   u.total_bytes, u.total_files);
				int r = u.execute ? u.execute(tr) : 0;

				if (tr.Skipped()) {
					DBG("UNIT[%zu] SKIP\n", unit_idx);
				} else if (r == 0 && !state.ShouldAbort()) {
					++result.success_count;
					bytes_done += u.total_bytes;
					files_done += u.total_files;
					state.file_complete = 100;
					state.all_complete = bytes_done;
					state.count_complete = files_done;
					DBG("UNIT[%zu] OK expected=%llu emitted=%llu credited=%llu bytes_credited=%llu/%llu | "
						"cumul bytes=%llu/%llu files=%llu/%llu\n",
						unit_idx,
						(unsigned long long)u.total_files,
						(unsigned long long)tr.EmittedPaths(),
						(unsigned long long)tr.FilesDone(),
						(unsigned long long)tr.BytesDone(), (unsigned long long)u.total_bytes,
						(unsigned long long)bytes_done, (unsigned long long)totalBytes,
						(unsigned long long)files_done, (unsigned long long)totalFiles);
					if (tr.EmittedPaths() < u.total_files) {
						DBG("UNIT[%zu] WARN adb -p emitted only %llu of %llu expected files (missing %llu)\n",
							unit_idx, (unsigned long long)tr.EmittedPaths(),
							(unsigned long long)u.total_files,
							(unsigned long long)(u.total_files - tr.EmittedPaths()));
					}
				} else if (r != 0) {
					result.last_error = r;
					DBG("UNIT[%zu] ERR rc=%d emitted=%llu credited=%llu\n", unit_idx, r,
						(unsigned long long)tr.EmittedPaths(), (unsigned long long)tr.FilesDone());
				}
			}
			if (state.ShouldAbort()) result.aborted = true;
			DBG("RunBatch END success=%d aborted=%d last_err=%d final state.bytes=%llu/%llu cnt=%llu/%llu\n",
				result.success_count, result.aborted, result.last_error,
				(unsigned long long)state.all_complete.load(), (unsigned long long)state.all_total.load(),
				(unsigned long long)state.count_complete.load(), (unsigned long long)state.count_total.load());
		} catch (const std::exception& ex) {
			DBG("RunBatch worker exception: %s\n", ex.what());
			result.last_error = EIO;
			state.SetAborting();
		} catch (...) {
			DBG("RunBatch worker: unknown exception\n");
			result.last_error = EIO;
			state.SetAborting();
		}
	});

	return result;
}

#include "ProgressBatch.h"
#include "MTPLog.h"
#include <WideMB.h>
#include <algorithm>

namespace {

std::string FileBasename(const std::string& p)
{
	if (p.empty()) return std::string();
	size_t end = p.size();
	while (end > 0 && (p[end - 1] == '/' || p[end - 1] == '\\')) --end;
	if (end == 0) return std::string();
	size_t start = end;
	while (start > 0 && p[start - 1] != '/' && p[start - 1] != '\\') --start;
	return p.substr(start, end - start);
}

}  // namespace

ProgressTracker::ProgressTracker(ProgressState& state,
                                 uint64_t bytes_before, uint64_t files_before,
                                 uint64_t unit_bytes, uint64_t unit_files)
	: _state(state),
	  _bytes_before(bytes_before),
	  _files_before(files_before),
	  _unit_bytes(unit_bytes),
	  _unit_files(unit_files)
{}

void ProgressTracker::Tick(uint64_t bytes_done, uint64_t file_size, const std::string& file_id)
{
	if (_display_only) { TickDisplayOnly(bytes_done, file_size, file_id); return; }

	if (!file_id.empty() && file_id != _cur_file_id) {
		if (!_cur_file_id.empty()) {
			_bytes_done += _cur_size;
			++_files_done;
		}
		_cur_file_id = file_id;
		_cur_size = file_size;
		std::lock_guard<std::mutex> lk(_state.mtx_strings);
		_state.current_file = StrMB2Wide(FileBasename(file_id));
	} else {
		_cur_size = std::max(_cur_size, file_size);
	}

	_state.file_complete = bytes_done;
	if (file_size > 0) _state.file_total = file_size;

	uint64_t in_progress = bytes_done <= _cur_size ? bytes_done : _cur_size;
	uint64_t this_unit = _bytes_done + in_progress;
	if (this_unit > _unit_bytes && _unit_bytes > 0) this_unit = _unit_bytes;
	_state.all_complete = _bytes_before + this_unit;

	uint64_t cnt = _files_before + _files_done;
	if (cnt > _files_before + _unit_files) cnt = _files_before + _unit_files;
	_state.count_complete = cnt;
}

void ProgressTracker::TickDisplayOnly(uint64_t bytes_done, uint64_t file_size,
                                      const std::string& file_id)
{
	if (!file_id.empty() && file_id != _cur_file_id) {
		_cur_file_id = file_id;
		_cur_size = file_size;
		std::lock_guard<std::mutex> lk(_state.mtx_strings);
		_state.current_file = StrMB2Wide(FileBasename(file_id));
	}
	_state.file_complete = bytes_done;
	if (file_size > 0) _state.file_total = file_size;
}

void ProgressTracker::Reset()
{
	_cur_file_id.clear();
	_cur_size = 0;
}

void ProgressTracker::PinNearDone(const std::string& file_id)
{
	if (!file_id.empty() && file_id != _cur_file_id) {
		_cur_file_id = file_id;
		std::lock_guard<std::mutex> lk(_state.mtx_strings);
		_state.current_file = StrMB2Wide(FileBasename(file_id));
	}
	// Show ~99% via fake byte total/done; dialog computes percent = file_complete / file_total.
	_state.file_total = 100;
	_state.file_complete = 99;
}

void ProgressTracker::CompleteFile(uint64_t file_size)
{
	if (_display_only) { _cur_file_id.clear(); _cur_size = 0; return; }
	_bytes_done += file_size;
	++_files_done;
	uint64_t this_unit = _bytes_done;
	if (this_unit > _unit_bytes && _unit_bytes > 0) this_unit = _unit_bytes;
	_state.all_complete = _bytes_before + this_unit;
	uint64_t cnt = _files_before + _files_done;
	if (cnt > _files_before + _unit_files) cnt = _files_before + _unit_files;
	_state.count_complete = cnt;
	_cur_file_id.clear();
	_cur_size = 0;
}

bool ProgressTracker::Aborted() const
{
	return _state.ShouldAbort();
}

BatchResult RunBatch(bool is_move, bool is_upload,
                     const std::wstring& source_label,
                     const std::wstring& dest_label,
                     std::vector<WorkUnit> units,
                     std::function<void()> on_abort)
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

	ProgressOperation op(is_move, is_multi, is_upload);
	op.GetState().all_total = totalBytes;
	op.GetState().count_total = totalFiles;
	{
		std::lock_guard<std::mutex> lk(op.GetState().mtx_strings);
		op.GetState().source_path = source_label;
		op.GetState().dest_path = dest_label;
	}
	if (on_abort) op.GetState().on_abort = std::move(on_abort);

	op.Run([&](ProgressState& state) {
		uint64_t bytes_done = 0, files_done = 0;
		try {
			for (auto& u : units) {
				if (state.ShouldAbort()) { result.aborted = true; break; }

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
					// no credit
				} else if (r == 0 && !state.ShouldAbort()) {
					++result.success_count;
					bytes_done += u.total_bytes;
					files_done += u.total_files;
					state.file_complete = u.total_bytes;
					state.all_complete = bytes_done;
					state.count_complete = files_done;
				} else if (r != 0) {
					result.last_error = r;
				}
				if (tr.HaltRequested()) break;
			}
			if (state.ShouldAbort()) result.aborted = true;
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

BatchResult RunBatchSilent(std::vector<WorkUnit> units)
{
	BatchResult result;
	if (units.empty()) return result;

	ProgressState dummy;
	dummy.Reset();
	uint64_t bytes_done = 0, files_done = 0;
	for (auto& u : units) {
		ProgressTracker tr(dummy, bytes_done, files_done,
		                   u.total_bytes, u.total_files);
		int r = u.execute ? u.execute(tr) : 0;
		if (tr.Skipped()) {
			// no credit
		} else if (r == 0) {
			++result.success_count;
			bytes_done += u.total_bytes;
			files_done += u.total_files;
		} else {
			result.last_error = r;
		}
		if (tr.HaltRequested()) break;
	}
	return result;
}

#include "Common.h"
#include "ImageView.h"
#include <optional>
#include <mutex>
#include <condition_variable>
#include <Threaded.h>

// quick view processing happens in background thread, that keeps UI fluent,
// even if decoding of some file takes long time - it happens in background
// and seamlessly cancelled when user navigates to another file
class ImageAtQV : public Threaded
{
	std::condition_variable _cond;
	std::mutex _mtx;
	volatile bool _exiting{false};
	volatile bool _changing{true};

	std::string _cur_file;
	SMALL_RECT _cur_area{-1, -1, -1, -1};

	virtual void *ThreadProc()
	{
		fprintf(stderr, "ImageAtQV: thread started\n");

		std::optional<ImageView> iv;
		std::string file;
		SMALL_RECT area;
		for (;;) {
			for (;;) {
				std::unique_lock<std::mutex> lock(_mtx);
				if (_exiting) {
					fprintf(stderr, "ImageAtQV: thread exited\n");
					return nullptr;
				}
				if (_changing) {
					_changing = false;
					file = _cur_file;
					area = _cur_area;
					RectReduce(area);
					break;
				}
				_cond.wait(lock);
			}
			fprintf(stderr, "ImageAtQV: [%d %d %d %d] '%s'\n",
				area.Left, area.Top, area.Right, area.Bottom, file.c_str());

			iv.reset();
			std::vector<std::pair<std::string, bool> > all_files{{file, false}};
			iv.emplace(0, all_files);
			if (!iv->Setup(area, &_changing)) {
				iv.reset();
			}
		}
	}

public:
	ImageAtQV(const std::string &file, const SMALL_RECT &area)
		:
		_cur_file(file),
		_cur_area(area)
	{
		StartThread();
	}

	~ImageAtQV()
	{
		if (!WaitThread(0)) {
			fprintf(stderr, "~ImageAtQV: exiting...\n");
			{
				std::lock_guard<std::mutex> lock(_mtx);
				_exiting = true;
				_changing = true;
				_cond.notify_all();
			}
			DWORD tmout;
			do { // prevent stuck on exit
				if (g_fsf.DispatchInterThreadCalls() > 0) {
					tmout = 1;
				} else {
					tmout = 100;
				}
			} while (!WaitThread(tmout));
		}
		fprintf(stderr, "~ImageAtQV: exited\n");
	}

	void Update(const std::string &file, const SMALL_RECT &area)
	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (_cur_file != file || memcmp(&_cur_area, &area, sizeof(_cur_area)) != 0) {
			_changing = true;
			_cur_file = file;
			_cur_area = area;
			_cond.notify_all();
		}
	}

	bool Alive()
	{
		return !WaitThread(0);
	}
};

///////////////////////////////////////////////////////////////////////////////
static std::optional<ImageAtQV> s_iv_at_qv;

void ShowImageAtQV(const std::string &file, const SMALL_RECT &area)
{
	if (s_iv_at_qv && s_iv_at_qv->Alive()) {
		s_iv_at_qv->Update(file, area);
	} else {
		s_iv_at_qv.emplace(file, area);
	}
}

bool IsShowingImageAtQV()
{
	return (bool)s_iv_at_qv;
}

void DismissImageAtQV()
{
	s_iv_at_qv.reset();
}

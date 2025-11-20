#include "Common.h"
#include "ImageViewer.h"
#include <optional>
#include <Threaded.h>
#include <Event.h>

// there is no notification from far2l so using dummy timer-based polling of panel mode and current file
// and below is period of times that drives that polling
// note that since image loading performed from additional thread - it almost doesnt block panel's UI 
#define TIMER_PERIOD_MSEC      100

class ImageAtQV : public Threaded
{
	Event _exit_event;
	std::string _current_file;

	virtual void *ThreadProc()
	{
		struct PanelInfo pi{};
		std::optional<ImageViewer> iv;
		std::string iv_file, cur_file; //, prev_file
		SMALL_RECT rc{-1, -1, -1, -1};
		fprintf(stderr, "ImageAtQV: thread started\n");
		do {
			g_far.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
			if (!pi.Visible || pi.PanelType != PTYPE_QVIEWPANEL) {
				fprintf(stderr, "ImageAtQV: thread exit due to unappropriate panel\n");
				break;
			}

			cur_file = GetCurrentPanelItem();

			if (iv_file != cur_file // switch_file_periods_counter >= SWITCH_FILE_PERIODS
				|| pi.PanelRect.left != rc.Left || pi.PanelRect.top != rc.Top
				|| pi.PanelRect.right != rc.Right || pi.PanelRect.bottom != rc.Bottom) {

				rc.Left = pi.PanelRect.left;
				rc.Top = pi.PanelRect.top;
				rc.Right = pi.PanelRect.right;
				rc.Bottom = pi.PanelRect.bottom;

				iv_file = std::move(cur_file);

				fprintf(stderr, "ImageAtQV: setup for '%s'\n", iv_file.c_str());

				iv.reset();
				std::set<std::string> single_selection;
				single_selection.emplace(iv_file); // prevent navigation to another file
				iv.emplace(iv_file, single_selection);
				if (!iv->Setup(rc)) {
					iv.reset();
				}
			}
//			fprintf(stderr, "ImageAtQV: loop\n");
		} while (!_exit_event.TimedWait(TIMER_PERIOD_MSEC));

		fprintf(stderr, "ImageAtQV: thread exited\n");
		return nullptr;
	}

public:
	ImageAtQV()
	{
		StartThread();
	}

	~ImageAtQV()
	{
		if (!WaitThread(0)) {
			fprintf(stderr, "ImageAtQV: exiting...\n");
			_exit_event.Signal();
			DWORD tmout;
			do { // prevent stuck on exit
				if (g_fsf.DispatchInterThreadCalls() > 0) {
					tmout = 1;
				} else {
					tmout = 100;
				}
			} while (!WaitThread(tmout));
		}
	}
};

static std::optional<ImageAtQV> s_iv_at_qv;

void ShowImageAtQV()
{
	s_iv_at_qv.reset();
	s_iv_at_qv.emplace();
}

void DismissImageAtQV()
{
	s_iv_at_qv.reset();
}

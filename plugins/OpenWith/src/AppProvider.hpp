#pragma once

#include "common.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class KeyFileReadHelper;
class KeyFileHelper;

namespace openwith
{
	struct ProviderSetting
	{
		std::wstring internal_key;      // persistent INI key and internal identifier
		std::wstring display_name;      // localized UI label
		bool value = false;
		bool disabled = false;          // true if the setting should be grayed out in the UI
		bool affects_candidates = true; // true if changing this setting affects the contents or order of the candidate list
	};


	class AppProvider
	{
	public:
		virtual ~AppProvider() = default;

		AppProvider(const AppProvider&) = delete;
		AppProvider& operator=(const AppProvider&) = delete;
		AppProvider(AppProvider&&) = delete;
		AppProvider& operator=(AppProvider&&) = delete;

		static AppProvider* GetInstance();

		struct GetCandidatesResult
		{
			std::vector<CandidateInfo> candidates;
			bool was_cancelled = false;
		};


		// Carries an optional title and/or status text update from the provider to the progress dialog.
		//   std::nullopt        — leave this field unchanged
		//   empty wstring_view  — clear this field
		//   non-empty           — set this field to the given text
		struct ProgressUpdate
		{
			std::optional<std::wstring_view> title;
			std::optional<std::wstring_view> status;

			// Allows calls like ReportProgress({GetMsg(...), nullptr})
			ProgressUpdate(const wchar_t* title = nullptr, const wchar_t* status = nullptr)
				: title (title ? std::optional<std::wstring_view>{title} : std::nullopt)
				, status(status ? std::optional<std::wstring_view>{status} : std::nullopt)
			{}
		};

		using ProgressCallback = std::function<void(const ProgressUpdate&)>;

		virtual GetCandidatesResult GetAppCandidates(const std::vector<std::wstring>& filepaths, ProgressCallback progress = nullptr,
													 const std::atomic<bool>* cancel_flag = nullptr) = 0;
		virtual std::vector<std::wstring> ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) = 0;
		virtual std::vector<Field> GetCandidateDetails(const CandidateInfo& candidate) = 0;
		virtual std::vector<std::wstring> GetFileTypes() = 0;
		virtual std::vector<CandidateContextLocation> GetCandidateContextLocations(const CandidateInfo& candidate) { return {}; }

		virtual void LoadPlatformSettings(const KeyFileReadHelper& key_reader) {}
		virtual void SavePlatformSettings(KeyFileHelper& key_writer) {}
		virtual std::vector<ProviderSetting> GetPlatformSettings() { return {}; }
		virtual void SetPlatformSettings(const std::vector<ProviderSetting>& settings) {}

	protected:
		AppProvider() = default;

		// Thrown by CheckCancellation() when the user has pressed Cancel.
		// Caught in GetAppCandidates() and converted to GetCandidatesResult::was_cancelled.
		struct OperationCancelledException {};

		// RAII guard that installs the progress callback and cancellation flag on the provider at the start
		// of GetAppCandidates() and clears them on exit (whether normal or via exception).
		struct OperationGuard
		{
			AppProvider& provider;
			OperationGuard(AppProvider& p, ProgressCallback cb, const std::atomic<bool>* cf)
				: provider(p)
			{
				p._progress_cb = std::move(cb);
				p._cancel_flag = cf;
			}
			~OperationGuard()
			{
				provider._cancel_flag = nullptr;
				provider._progress_cb = nullptr;
			}

			OperationGuard(const OperationGuard&) = delete;
			OperationGuard& operator=(const OperationGuard&) = delete;
			OperationGuard(OperationGuard&&) = delete;
			OperationGuard& operator=(OperationGuard&&) = delete;
		};

		void CheckCancellation() const;
		void ReportProgress(const ProgressUpdate& update) const;

	private:
		static std::unique_ptr<AppProvider> CreateAppProvider();
		const std::atomic<bool>* _cancel_flag = nullptr;
		ProgressCallback _progress_cb = nullptr;
	};

} // namespace openwith

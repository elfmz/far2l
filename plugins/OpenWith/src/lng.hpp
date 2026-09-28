#pragma once

namespace openwith
{
	enum class MsgID : int
	{
		PluginTitle,
		ChooseApplication,
		OpenWithFor,
		File_s,

		Error,
		NotRealNames,
		SaveConfigError,
		NoAppsFound,
		CannotExecute,
		UnsupportedPlatform,

		ConfigTitle,
		Ok,
		Cancel,

		UseExternalTerminal,
		NoWaitForCommandCompletion,
		ClearSelection,
		ConfirmLaunchOption,
		DisplayFilename,

		UseXdgMimeTool,
		UseFileTool,
		UseMagikaTool,
		UseGlobRules,
		UseExtensionBasedFallback,
		LoadMimeTypeAliases,
		LoadMimeTypeSubclasses,
		ResolveStructuredSuffixes,
		UseGenericMimeFallbacks,
		ShowUniversalHandlers,
		QueryXdgMimeDefault,
		IgnoreRemovedAssociations,
		UseMimeinfoCache,
		FilterByShowIn,
		ValidateTryExec,
		TreatUrlsAsPaths,
		ShowPackageTags,

		ShowUtiInsteadOfMime,
		RespectSystemRanking,

		SortAlphabetically,

		Details,

		FilesSelected,
		Filepaths,
		FileTypes,
		LaunchCommand,
		Close,
		Launch,

		DesktopFile,
		Source,
		FullScanFor,
		For,
		In,
		GotoDesktop,
		GotoTryExec,
		GotoSource,

		Location,
		BundleDisplayName,
		BundleName,
		BundleShortVersionString,
		BundleVersion,
		BundleExecutable,
		BundleIdentifier,
		GoToBundle,

		Working,
		PleaseWait,
		ProcessingFiles,

		IdentifyingMimes,
		DiscoveringApplications,
		MatchingFilteringRanking,

		IdentifyingUTIsDiscoveringApps,
		FilteringSortingResults,

		ConfirmLaunchTitle,
		ConfirmLaunchMessage,
	};

	const wchar_t* GetMsg(MsgID msg_id);

} // namespace openwith

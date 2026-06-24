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
		SortAlphabetically,
		TreatUrlsAsPaths,
		ShowPackageTags,

		Details,

		FilesSelected,
		Filepaths,
		MimeProfile,
		LaunchCommand,
		Close,
		Launch,
		GotoDesktop,
		GotoSource,

		DesktopFile,
		Source,
		FullScanFor,
		For,
		In,

		AppName,
		FullPath,
		ExecutableFile,
		Version,
		BundleVersion,

		ConfirmLaunchTitle,
		ConfirmLaunchMessage
	};

	const wchar_t* GetMsg(MsgID msg_id);

} // namespace openwith

#pragma once
#include "OpBase.h"
#include "./Utils/Enumer.h"
#include "./Utils/IOBuffer.h"
#include "../UI/Defs.h"
#include "../BackgroundTasks.h"


class OpXfer : protected OpBase, public IBackgroundTask
{
	Path2FileInformation _entries;
	std::shared_ptr<Enumer> _enumer;
	std::shared_ptr<IHost> _dst_host;
	std::string _dst_dir, _diffname_suffix;
	XferOverwriteAction _default_xoa = XOA_ASK;
	XferKind _kind;
	XferDirection _direction;
	IOBuffer _io_buf;
	bool _smart_symlinks_copy;
	bool _on_site_move = false;
	int _use_of_chmod;

	virtual void Process();

	virtual void ForcefullyAbort();	// IAbortableOperationsHost

	bool IsDstPathExists(const std::string &path);

	void Rename(const std::set<std::string> &items);
	void EnsureDstDirExists();
	void Transfer();
	void FileDelete(const std::string &path);
	void DirectoryCopy(const std::string &path_dst, const FileInformation &info);
	bool SymlinkCopy(const std::string &path_src, const std::string &path_dst);
	bool FileCopyLoop(const std::string &path_src, const std::string &path_dst, FileInformation &info);
	void EnsureProgressConsistency();
	void CopyAttributes(const std::string &path_dst, const FileInformation &info);

public:
	OpXfer(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		std::shared_ptr<IHost> &dst_host, const std::string &dst_dir, struct PluginPanelItem *items,
		int items_count, XferKind kind, XferDirection direction);

	virtual ~OpXfer();

	virtual BackgroundTaskStatus GetStatus();
	virtual std::string GetInformation();
	virtual std::string GetDestination(bool &directory);
	virtual void Show();
	virtual void Abort();
};

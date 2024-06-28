/*
Bookrmarks.cpp

Folder shortcuts
*/

#include "headers.hpp"


#include "Bookmarks.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "filelist.hpp"
#include "KeyFileHelper.h"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "dialog.hpp"
#include "DialogBuilder.hpp"
#include <farplug-wide.h>
#include "plugins.hpp"

Bookmarks::Bookmarks()
	: _kfh(InMyConfig("settings/bookmarks.ini").c_str(), true)
{
}

bool Bookmarks::Set(int index, const FARString *path,
	const FARString *plugin, const FARString *plugin_file, const FARString *plugin_data)
{
	if ( (!path || path->IsEmpty()) && (!plugin || plugin->IsEmpty()))
	{
		return Clear(index);
	}

	const auto &sec = ToDec(index);
	_kfh.RemoveSection(sec);

	_kfh.SetString(sec, "Path", path ? path->GetMB().c_str() : "");
	_kfh.SetString(sec, "Plugin", plugin ? plugin->GetMB().c_str() : "");
	_kfh.SetString(sec, "PluginFile", plugin_file ? plugin_file->GetMB().c_str() : "");
	_kfh.SetString(sec, "PluginData", plugin_data ? plugin_data->GetMB().c_str() : "");

	return _kfh.Save();
}

bool Bookmarks::Get(int index, FARString *path,
	FARString *plugin, FARString *plugin_file, FARString *plugin_data)
{
	const auto &sec = ToDec(index);
	FARString strFolder(_kfh.GetString(sec, "Path"));

	if (!strFolder.IsEmpty())
		apiExpandEnvironmentStrings(strFolder, *path);
	else
		path->Clear();

	if (plugin)
		*plugin = _kfh.GetString(sec, "Plugin");

	if (plugin_file)
		*plugin_file = _kfh.GetString(sec, "PluginFile");

	if (plugin_data)
		*plugin_data = _kfh.GetString(sec, "PluginData");

	return (!path->IsEmpty() || (plugin && !plugin->IsEmpty()));
}

bool Bookmarks::Clear(int index)
{
	_kfh.RemoveSection(ToDec(index));
	if (index < 10)
		return _kfh.Save();

	for (int dst_index = index, miss_counter = 0;;)
	{
		FARString path, plugin, plugin_file, plugin_data;
		if (Get(index, &path, &plugin, &plugin_file, &plugin_data))
		{
			if (dst_index != index)
			{
				Set(dst_index, &path, &plugin, &plugin_file, &plugin_data);
			}
			++dst_index;
			miss_counter = 0;
		}
		else if (++miss_counter >= 10)
		{
			for (; dst_index <= index; ++dst_index)
			{
				_kfh.RemoveSection(ToDec(dst_index));
			}
			break;
		}

		++index;
	}

	return _kfh.Save();
}

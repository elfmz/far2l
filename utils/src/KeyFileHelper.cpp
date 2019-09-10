#include "KeyFileHelper.h"

//# define GLIB_VERSION_MIN_REQUIRED      (GLIB_VERSION_2_26)
//# define GLIB_VERSION_MAX_ALLOWED	(G_ENCODE_VERSION (2, 99))
#include <WinCompat.h>
#include <glib.h>
#include <string.h>
#include <mutex>
#include <stdlib.h>

static std::mutex g_key_file_helper_mutex;

KeyFileHelper::KeyFileHelper(const char *filename, bool load)
	: _kf(g_key_file_new()),  _filename(filename), _dirty(!load), _loaded(false)
{
	GError *err = NULL;
	if (load) {
		g_key_file_helper_mutex.lock();
		if (!g_key_file_load_from_file(_kf, _filename.c_str(), G_KEY_FILE_NONE, &err)) {
			//fprintf(stderr, "KeyFileHelper(%s, %d) err=%p\n", _filename.c_str(), load, err);
		} else {
			_loaded = true;
		}
		g_key_file_helper_mutex.unlock();
	}
}

KeyFileHelper::~KeyFileHelper()
{
	if (_dirty) {
		Save();
	}
	g_key_file_free(_kf);
}

bool KeyFileHelper::Save()
{
	GError *err = NULL;
	g_key_file_helper_mutex.lock();
	bool out = !!g_key_file_save_to_file(_kf, _filename.c_str(), &err);
	g_key_file_helper_mutex.unlock();
	if (out) {
		_dirty = false;
	}
	return out;
}

std::vector<std::string> KeyFileHelper::EnumSections()
{
	std::vector<std::string> out;
	
	gchar **r = g_key_file_get_groups (_kf, NULL);
	if (r) {
		for (gchar **p = r; *p; ++p) 
			out.push_back(*p);
		g_strfreev(r);
	}
	
	return out;
}

void KeyFileHelper::RemoveSection(const char *section)
{
	_dirty = true;
	g_key_file_remove_group(_kf, section, NULL);
}

void KeyFileHelper::RemoveKey(const char *section, const char *name)
{
	_dirty = true;
	g_key_file_remove_key(_kf, section, name, NULL);
}

std::vector<std::string> KeyFileHelper::EnumKeys(const char *section)
{
	std::vector<std::string> out;
	
	gchar **r = g_key_file_get_keys(_kf, section, NULL, NULL);
	if (r) {
		for (gchar **p = r; *p; ++p) 
			out.push_back(*p);
		g_strfreev(r);
	}
	
	return out;
}

std::string KeyFileHelper::GetString(const char *section, const char *name, const char *def)
{
	std::string rv;
	char *v = g_key_file_get_string(_kf, section, name, NULL);
	if (v) {
		rv.assign(v);
		free(v);
	} else
		rv.assign(def);
	return rv;
}

void KeyFileHelper::GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def)
{
	std::string rv;
	char *v = g_key_file_get_string(_kf, section, name, NULL);
	if (v) {
		strncpy(buffer, v, buf_size);
		free(v);
	} else if (def && def!=buffer)
		strncpy(buffer, def, buf_size);

	buffer[buf_size - 1] = 0;
}

int KeyFileHelper::GetInt(const char *section, const char *name, int def)
{
	GError *err = NULL;
	int rv = g_key_file_get_integer(_kf, section, name, &err);
	if (rv==0 && err!=NULL) {
		rv = def;
		//TODO? Should I do free(err);  ?
	}
	return rv;
}

///////////////////////////////////////////////
void KeyFileHelper::PutString(const char *section, const char *name, const char *value)
{
	_dirty = true;
	g_key_file_set_string(_kf, section, name, value);
}

void KeyFileHelper::PutInt(const char *section, const char *name, int value)
{
	_dirty = true;
	g_key_file_set_integer(_kf, section, name, value);
}


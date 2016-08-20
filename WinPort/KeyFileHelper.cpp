#include "KeyFileHelper.h"

//# define GLIB_VERSION_MIN_REQUIRED      (GLIB_VERSION_2_26)
//# define GLIB_VERSION_MAX_ALLOWED	(G_ENCODE_VERSION (2, 99))

#include <glib.h>
#include <string.h>
#include <mutex>

static std::mutex g_key_file_helper_mutex;

KeyFileHelper::KeyFileHelper(const char *filename, bool load)
	: _kf(g_key_file_new()),  _filename(filename), _dirty(!load)
{
	GError *err = NULL;
	g_key_file_helper_mutex.lock();
	g_key_file_load_from_file(_kf, _filename.c_str(), G_KEY_FILE_NONE, &err);
	fprintf(stderr, "KeyFileHelper(%s, %d) err=%p\n", _filename.c_str(), load, err);
}

KeyFileHelper::~KeyFileHelper()
{
	if (_dirty) {
		GError *err = NULL;
		g_key_file_save_to_file(_kf, _filename.c_str(), &err);
		fprintf(stderr, "~KeyFileHelper(%s) err=%p\n", _filename.c_str(),  err);
	}
	g_key_file_helper_mutex.unlock();
}

std::string KeyFileHelper::GetString(const char *section, const char *name, const char *def)
{
	std::string rv;
	char *v = g_key_file_get_string(_kf, "FarFTP", "Version", NULL);
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
	} else
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

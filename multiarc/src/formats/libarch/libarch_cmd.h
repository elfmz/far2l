#pragma once

struct LibarchCommandOptions
{
	std::string root_path;
	std::string charset;
};

// cmd  - starts from 'x' 'X' 't'
bool LIBARCH_CommandRead(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[]);

// cmd - starts from 'd' 'D'
bool LIBARCH_CommandDelete(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[]);

// cmd starts from 'a' 'A' 'm' 'M'
bool LIBARCH_CommandAdd(const char *cmd, const char *arc_path, const LibarchCommandOptions &arc_opts, int files_cnt, char *files[]);

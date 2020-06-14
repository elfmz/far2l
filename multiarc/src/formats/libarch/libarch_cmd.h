#pragma once

// cmd  - starts from 'x' 'X' 't'
bool LIBARCH_CommandRead(const char *cmd, const char *arc_path, const char *arc_root_path, int files_cnt, char *files[]);

// cmd - starts from 'd' 'D'
bool LIBARCH_CommandDelete(const char *cmd, const char *arc_path, const char *arc_root_path, int files_cnt, char *files[]);

// cmd starts from 'a' 'A' 'm' 'M'
bool LIBARCH_CommandAdd(const char *cmd, const char *arc_path, const char *arc_root_path, int files_cnt, char *files[]);

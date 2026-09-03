#pragma once

#include <string>

const char *PooledString(const char *s);
const char *PooledString(const wchar_t *s);
const char *PooledString(const std::string &s);


const wchar_t *MB2WidePooled(const std::string &str);
const wchar_t *MB2WidePooled(const char *str);


void PurgePooledStrings();


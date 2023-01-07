#pragma once

size_t RandomStringBuffer(char *out, size_t min_len, size_t max_len, bool only_alnum = true);
void RandomStringAppend(std::string &out, size_t min_len, size_t max_len);

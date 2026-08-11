#pragma once

bool InteractiveLogin(const std::string &display_name, unsigned int retry, std::string &username, std::string &password);
bool InteractivePassphrase(const std::string &display_name, unsigned int retry, const std::string &info, std::string &password);

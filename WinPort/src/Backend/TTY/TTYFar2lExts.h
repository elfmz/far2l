#pragma once
#include <vector>

bool TTYFar2lExt_Negotiate(int std_in, int std_out, bool enable);
bool TTYFar2lExt_Interract(int std_in, int std_out, std::vector<unsigned char> &data, std::vector<unsigned char> &reply);

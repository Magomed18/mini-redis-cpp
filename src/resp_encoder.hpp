#pragma once

#include <string>
#include <string_view>

std::string encode_resp_simple_string(std::string_view value);
std::string encode_resp_error(std::string_view message);
std::string encode_resp_bulk_string(std::string_view value);
std::string encode_resp_integer(long long value);
std::string encode_resp_null_bulk_string();
#pragma once
#include <windows.h>
#include <optional>
#include <string>

std::optional<std::string> select_file_to_open(bool loadImage = true);
std::string getLastErrorAsString();

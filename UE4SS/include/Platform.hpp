#pragma once
#include <filesystem>

std::filesystem::path get_executable_path();
void add_dlsearch_folder(std::filesystem::path& path);

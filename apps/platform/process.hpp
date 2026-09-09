#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace rat {
std::vector<std::string> process_arguments(int argc, char** argv);
std::filesystem::path executable_path();
}

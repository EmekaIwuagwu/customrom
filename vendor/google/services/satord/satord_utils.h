#pragma once

#include <string>
#include <vector>

// logic: Check if package name contains only safe characters
bool validate_package_name(const std::string& pkg);

// logic: Check if path or its parent resolves to a path starting with one of the allowed prefixes
bool validate_path_prefix(const std::string& path, const std::vector<std::string>& allowed);

// logic: Securely execute a command (wrapper around fork/execv)
int secure_exec(const std::vector<std::string>& args);

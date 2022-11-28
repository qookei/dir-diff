/* Directory diff utility - Tree processing
 * Copyright (C) 2022  qookie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class diff_type {
	missing, file_type, contents
};

struct diff {
	diff_type type;
	int n;
	std::string name;

	fs::path a_path = "", b_path = "";

	std::vector<diff> sub_diffs = {};
};

void update_progress(const fs::path &path);
bool should_ignore_file(const fs::path &path, bool a_path);

std::vector<diff> diff_trees(const fs::directory_entry &a_dentry, const fs::directory_entry &b_dentry);

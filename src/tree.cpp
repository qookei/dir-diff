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

#include <tree.hpp>
#include <sys/stat.h>
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <cstdint>

int file_type_i(const fs::directory_entry &dentry) {
	if (dentry.is_block_file()) return 0;
	if (dentry.is_character_file()) return 1;
	if (dentry.is_directory()) return 2;
	if (dentry.is_fifo()) return 3;
	if (dentry.is_other()) return 4;
	if (dentry.is_regular_file()) return 5;
	if (dentry.is_socket()) return 6;
	if (dentry.is_symlink()) return 7;
	return -1;
}

bool are_files_different(const fs::directory_entry &a, const fs::directory_entry &b) {
	// Regular files of different size are bound to be different
	if (a.is_regular_file() && a.file_size() != b.file_size())
		return true;

	// TODO(qookie): Check for stat errors here
	struct stat st_a, st_b;
	stat(a.path().c_str(), &st_a);
	stat(b.path().c_str(), &st_b);

	// Same inode on the same device are always the same
	if (st_a.st_dev == st_b.st_dev && st_a.st_ino == st_b.st_ino)
		return false;

	// Same target means symlinks are the same
	if (a.is_symlink()) {
		return fs::read_symlink(a) != fs::read_symlink(b);
	}

	// Same contents means regular files are the same
	if (a.is_regular_file()) {
		std::ifstream a_ifs{a.path(), std::ios::binary};
		std::ifstream b_ifs{b.path(), std::ios::binary};

		char a_buf[4096], b_buf[4096];
		while (true) {
			a_ifs.read(a_buf, 4096);
			auto a_count = a_ifs.gcount();

			b_ifs.read(b_buf, 4096);
			auto b_count = b_ifs.gcount();

			if (std::string_view{a_buf, static_cast<size_t>(a_count)} !=
					std::string_view{b_buf, static_cast<size_t>(b_count)})
				return true;

			if (a_count < 4096 || b_count < 4096)
				break;
		}

		return false;
	}

	// Only special files (!symlink && !regular) get here
	// Same device numbers of special files means they are the same
	return st_a.st_rdev != st_b.st_rdev;
}

std::vector<diff> diff_trees(const fs::directory_entry &a_dentry, const fs::directory_entry &b_dentry) {
	// Build a union of the sets of children from both directories
	std::unordered_set<std::string> comb_child;
	std::unordered_map<std::string, fs::directory_entry> a_children, b_children;

	for (const auto &child_dentry : fs::directory_iterator{a_dentry}) {
		auto name = child_dentry.path().filename();
		comb_child.emplace(name);
		a_children.emplace(name, child_dentry);
	}

	for (const auto &child_dentry : fs::directory_iterator{b_dentry}) {
		auto name = child_dentry.path().filename();
		comb_child.emplace(name);
		b_children.emplace(name, child_dentry);
	}

	std::vector<diff> diffs;

	// Go through each known file and check if they are the same or not
	for (const auto &name : comb_child) {
		auto a_it = a_children.find(name);
		auto b_it = b_children.find(name);

		if ((a_it == a_children.end()) ^ (b_it == b_children.end())) {
			diffs.push_back({diff_type::missing,
					a_it == a_children.end() ? 0 : 1,
					name});
			continue;
		}

		assert(a_it != a_children.end());
		assert(b_it != b_children.end());

		auto &a_child = a_it->second;
		auto &b_child = b_it->second;

		if (file_type_i(a_child) != file_type_i(b_child)) {
			diffs.push_back({diff_type::file_type, -1, name});
			continue;
		}

		if (a_child.is_directory()) {
			auto sub_diff = diff_trees(a_child, b_child);
			if (sub_diff.size()) {
				diffs.push_back({diff_type::contents, -1,
						name, std::move(sub_diff)});
			}

			continue;
		}

		if (are_files_different(a_child, b_child)) {
			diffs.push_back({diff_type::contents, -1, name});
		}
	}

	return diffs;
}

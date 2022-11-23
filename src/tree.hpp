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

#include <array>
#include <filesystem>
#include <map>
#include <botan/hash.h>
#include <string_view>

namespace fs = std::filesystem;

using hash_output = std::array<uint8_t, 512 / 8>;
using hash_fn = std::unique_ptr<Botan::HashFunction>;

template<>
struct std::hash<hash_output> {
	size_t operator()(const hash_output &h) const noexcept {
		return std::hash<std::string_view>{}(std::string_view{reinterpret_cast<const char *>(h.data()), h.size()});
	}
};

struct node {
	fs::path path{};
	hash_output hash{};

	bool is_dir{false};
	std::unordered_map<hash_output, std::unique_ptr<node>> children{};
	std::map<std::string, hash_output> ordered_hashes{};
};

std::unique_ptr<node> build_node(hash_fn &hash, const fs::directory_entry &dentry, size_t root_width);

void diff_nodes(const std::unique_ptr<node> &a, const std::unique_ptr<node> &b, int depth = 0);

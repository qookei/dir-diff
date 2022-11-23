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

#include <iostream>
#include <fstream>
#include <unordered_set>

int progress_step = 0;
const std::array<std::string, 8> progress_strs{
	"|", "/", "-", "\\", "|", "/", "-", "\\"
};

void update_progress(const fs::path &path, size_t root_width) {
	const auto &indicator = progress_strs[progress_step];
	progress_step = (progress_step + 1) % progress_strs.size();

	std::string path_str = path;
	path_str = path_str.substr(root_width);

	constexpr int width = 72;

	if (path_str.size() > width) {
		path_str = "..." + path_str.substr(path_str.size() - (width - 3), width - 3);
	}

	std::cerr << "\e[2K\e[G " << indicator << " " << path_str << std::flush;
}

std::unique_ptr<node> build_node(hash_fn &hash, const fs::directory_entry &dentry, size_t root_width) {
	auto n = std::make_unique<node>();
	n->path = dentry.path();

	update_progress(dentry.path(), root_width);

	if (dentry.is_symlink()) {
		// Compute hash of link destination
		std::string target = fs::read_symlink(dentry);
		hash->update(reinterpret_cast<uint8_t *>(target.data()), target.size());
	} else if (dentry.is_directory()) {
		n->is_dir = true;
		// Discover all children
		for (const auto &child_dentry : fs::directory_iterator{dentry}) {
			auto child_node = build_node(hash, child_dentry, root_width);
			n->ordered_hashes[child_dentry.path().filename()] = child_node->hash;
			n->children[child_node->hash] = std::move(child_node);
		}

		// Compute hash of all children's names and hashes
		for (const auto &[n, h] : n->ordered_hashes) {
			hash->update(reinterpret_cast<const uint8_t *>(n.data()), n.size());
			hash->update(h.data(), h.size());
		}
	} else if (dentry.is_regular_file()) {
		std::ifstream ifs{dentry.path(), std::ios::binary};

		char buf[4096];
		while (true) {
			ifs.read(buf, 4096);
			auto count = ifs.gcount();

			hash->update(reinterpret_cast<uint8_t *>(buf), count);

			if (count < 4096)
				break;
		}
	}

	hash->final(n->hash.data());

	return n;
}

enum class diff_type {
	missing, kind, mismatch
};

void indent(int depth) {
	for (int i = 0; i <= depth; i++) std::cout << "|  ";
}

struct diff {
	diff_type type;
	int n;
	std::string name;
	const std::unique_ptr<node> *a;
	const std::unique_ptr<node> *b;
};

// Precondition: diff_nodes is only called on directory nodes (node::children::size() > 0)
void diff_nodes(const std::unique_ptr<node> &a, const std::unique_ptr<node> &b, int depth) {
	// Build a union of the sets of children from both nodes
	std::unordered_set<std::string> comb_child;

	if (a->hash != b->hash) {
		for (const auto &[name, _] : a->ordered_hashes)
			comb_child.emplace(name);
		for (const auto &[name, _] : b->ordered_hashes)
			comb_child.emplace(name);
	}

	std::vector<diff> diffs;

	// Go through each known file and check if they are the same or not
	for (const auto &name : comb_child) {
		auto a_it = a->ordered_hashes.find(name);
		auto b_it = b->ordered_hashes.find(name);

		if ((a_it == a->ordered_hashes.end()) ^ (b_it == b->ordered_hashes.end())) {
			diffs.push_back({diff_type::missing,
					a_it == a->ordered_hashes.end() ? 0 : 1,
					name, nullptr, nullptr});
			continue;
		}

		const auto &a_hash = a->ordered_hashes[name];
		const auto &b_hash = b->ordered_hashes[name];
		const auto &a_node = a->children[a_hash];
		const auto &b_node = b->children[b_hash];

		if (a_hash == b_hash)
			continue;

		if (a_node->is_dir != b_node->is_dir) {
			diffs.push_back({diff_type::kind, -1,
					name, nullptr, nullptr});
			continue;
		}

		diffs.push_back({diff_type::mismatch, -1,
				name, &a_node, &b_node});
	}

	if (diffs.size()) {
		if (!depth) {
			std::cout << "Legend:"
				<< "\t\e[31m- foo\e[0m" << " - exists only in 1st tree\n"
				<< "\t\e[32m+ foo\e[0m" << " - exists only in 2nd tree\n"
				<< "\t\e[34m! foo\e[0m" << " - types differ (directory vs file)\n"
				<< "\t\e[33m? foo\e[0m" << " - contents differ\n";
			std::cout << "Diff:\n<root>\n";
		}

		for (const auto &diff : diffs) {
			indent(depth);
			switch (diff.type) {
				using enum diff_type;
				case missing:
					std::cout << (diff.n ? "\e[31m-" : "\e[32m+") << " " << diff.name << "\e[0m\n";
					break;
				case kind:
					std::cout << "\e[34m! " << diff.name << "\e[0m\n";
					break;
				case mismatch:
					if (!(*diff.a)->is_dir) {
						std::cout << "\e[33m? " << diff.name << "\e[0m\n";
					} else if (diff.name == ".git") {
						// Avoid showing contents of .git
						std::cout << "\e[33m? " << diff.name << "\e[0m (not descending)\n";
					} else {
						std::cout << "\e[33m? " << diff.name << "\e[0m:\n";
						diff_nodes(*diff.a, *diff.b, depth + 1);
					}
					break;
			}
		}
	} else if (!depth) {
		std::cout << "No differences.\n";
	}
}

/* Directory diff utility
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

#include <getopt.h>
#include <array>
#include <config.hpp>
#include <filesystem>
#include <iostream>
#include <tree.hpp>

void display_version() {
	std::cout << "dir-diff " << config::version << "\n";

	std::cout << "Copyright (C) 2022 qookie.\n";
	std::cout << "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n";
	std::cout << "This is free software: you are free to change and redistribute it.\n";
	std::cout << "There is NO WARRANTY, to the extent permitted by law.\n";

}

void display_help(const char *progname) {
	std::cout << "Usage: " << progname << " [OPTION]... PATH PATH\n";
	std::cout << "Compute the difference between the specified paths.\n\n";

	std::cout << "Miscellaneous:\n";
	std::cout << "  -v, --version                   display the version information and exit\n";
	std::cout << "  -h, --help                      display this help text and exit\n";
}

int progress_step = 0;
const std::array<std::string, 8> progress_strs{
	"|", "/", "-", "\\", "|", "/", "-", "\\"
};

void update_progress(const fs::path &path) {
	const auto &indicator = progress_strs[progress_step];
	progress_step = (progress_step + 1) % progress_strs.size();

	std::string path_str = path;

	constexpr int width = 72;

	if (path_str.size() > width) {
		path_str = "..." + path_str.substr(path_str.size() - (width - 3), width - 3);
	}

	std::cerr << "\e[2K\e[G " << indicator << " " << path_str << std::flush;
}

void display_diff(const diff &diff, int depth = 0) {
	for (int i = 0; i < depth; i++)
		std::cout << "|  ";

	switch (diff.type) {
		using enum diff_type;
		case missing:
			std::cout << (diff.n ? "\e[31m-" : "\e[32m+") << " " << diff.name << "\e[0m\n";
			break;
		case file_type:
			std::cout << "\e[34m! " << diff.name << "\e[0m\n";
			break;
		case contents:
			if (!diff.sub_diffs.size()) {
				std::cout << "\e[33m? " << diff.name << "\e[0m\n";
			} else if (diff.name == ".git") {
				// Avoid showing contents of .git
				std::cout << "\e[33m? " << diff.name << "\e[0m (not descending)\n";
			} else {
				std::cout << "\e[33m? " << diff.name << "\e[0m:\n";
				for (const auto &sub : diff.sub_diffs)
					display_diff(sub, depth + 1);
			}
			break;
	}
}


int main(int argc, char **argv) {
	fs::path p1, p2;

	const struct option options[] = {
		{"help",	no_argument,	0, 'h'},
		{"version",	no_argument,	0, 'v'},
		{0,		0,		0, 0}
	};

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "hv", options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'h': display_help(argv[0]); return 0;
			case 'v': display_version(); return 0;

			case '?': return 1;
		}
	}

	if (optind < argc && argc - optind >= 2) {
		p1 = argv[optind++];
		p2 = argv[optind++];
	} else {
		std::cerr << "Missing positional argument(s): <path> <path>\n";
		return 1;
	}

	auto diffs = diff_trees(fs::directory_entry{p1}, fs::directory_entry{p2});
	std::cerr << "\e[2K\e[G" << std::flush;

	if (!diffs.size()) {
		std::cout << "No differences.\n";
	} else {
		diff root{diff_type::contents, -1, "<root>", std::move(diffs)};

		std::cout << "Legend:"
			<< "\t\e[31m- foo\e[0m" << " - exists only in 1st tree\n"
			<< "\t\e[32m+ foo\e[0m" << " - exists only in 2nd tree\n"
			<< "\t\e[34m! foo\e[0m" << " - types differ (directory vs file)\n"
			<< "\t\e[33m? foo\e[0m" << " - contents differ\n";
		std::cout << "Diff:\n";

		display_diff(root);
	}
}

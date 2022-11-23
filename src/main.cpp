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

#include <iostream>
#include <getopt.h>

#include <tree.hpp>
#include <config.hpp>

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


	auto blake2b = Botan::HashFunction::create("Blake2b");

	std::cerr << "Processing tree 1\n";
	auto n1 = build_node(blake2b, fs::directory_entry{p1}, static_cast<std::string>(p1).size());
	std::cerr << "\e[2K\e[G" << std::flush;
	std::cerr << "Processing tree 2\n";
	auto n2 = build_node(blake2b, fs::directory_entry{p2}, static_cast<std::string>(p2).size());
	std::cerr << "\e[2K\e[G" << std::flush;

	diff_nodes(n1, n2);
}

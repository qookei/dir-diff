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
#include <print.hpp>
#include <unistd.h>

void display_version() {
	fmtns::print("dir-diff {0}\n", config::version);

	fmtns::print("\
Copyright (C) {0} qookie.\n\
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n", 2022);
}

void display_help(const char *progname) {
	fmtns::print("Usage: {0} [OPTION]... PATH PATH\n", progname);
	fmtns::print("Compute the difference between the specified paths.\n");

	fmtns::print("\n");

	fmtns::print("\
Output control:\n\
  -l, --no-legend                 don't display the legend before the diff\n\
  -q, --quiet                     don't display the progress indicator\n\
  -c, --color=WHEN                force (WHEN is 'force' or 'always'), or\n\
                                  disable (WHEN is 'never' or 'off') the use of colors\n\
                                  by default colors are enabled if the output is a tty");

	fmtns::print("\n");

	fmtns::print("\
Miscellaneous:\n\
  -v, --version                   display the version information and exit\n\
  -h, --help                      display this help text and exit\n");

	fmtns::print("\n");
}

int progress_step = 0;
const std::array<std::string, 8> progress_strs{
	"|", "/", "-", "\\", "|", "/", "-", "\\"
};

fs::path root1, root2;
bool run_quietly = false;

const char *ansi_reset = "\e[0m";
const char *ansi_red = "\e[31m";
const char *ansi_green = "\e[32m";
const char *ansi_yellow = "\e[33m";
const char *ansi_blue = "\e[34m";
const char *ansi_clear_to_beginning_of_line = "\e[2K\e[G";
bool using_color = true;

template <typename ...Args>
void print_in_color(const char *color, fmtns::format_string<Args...> fmt, Args &&...args) {
	fmtns::print("{0}{1}{2}", color, fmtns::format(fmt, std::forward<Args>(args)...), ansi_reset);
}

void update_progress(const fs::path &path) {
	if (run_quietly || !using_color)
		return;

	const auto &indicator = progress_strs[progress_step];
	progress_step = (progress_step + 1) % progress_strs.size();

	std::string path_str = path.string().substr(root1.string().size());

	constexpr int width = 72;

	if (path_str.size() > width) {
		path_str = "..." + path_str.substr(path_str.size() - (width - 3), width - 3);
	}

	fmtns::print(std::cerr, "{0}{1} {2}", ansi_clear_to_beginning_of_line, indicator, path_str);
}

void display_diff(const diff &diff, int depth = 0) {
	for (int i = 0; i < depth; i++)
		fmtns::print("|  ");

	switch (diff.type) {
		using enum diff_type;
		case missing:
			print_in_color(
				diff.n ? ansi_red : ansi_green,
				"{0} {1}\n",
				diff.n ? "-" : "+", diff.name);
			break;
		case file_type:
			print_in_color(ansi_blue, "! {0}\n", diff.name);
			break;
		case contents:
			if (!diff.sub_diffs.size()) {
				print_in_color(ansi_yellow, "? {0}\n", diff.name);
			} else if (diff.name == ".git") {
				// Avoid showing contents of .git
				print_in_color(ansi_yellow, "? {0} (not descending)\n", diff.name);
			} else {
				print_in_color(ansi_yellow, "? {0}:\n", diff.name);
				for (const auto &sub : diff.sub_diffs)
					display_diff(sub, depth + 1);
			}
			break;
	}
}

int main(int argc, char **argv) {
	const struct option options[] = {
		{"help",	no_argument,		0, 'h'},
		{"version",	no_argument,		0, 'v'},
		{"no-legend",	no_argument,		0, 'l'},
		{"quiet",	no_argument,		0, 'q'},
		{"color",	required_argument,	0, 'c'},
		{0,		0,			0, 0}
	};

	bool print_legend = true;

	bool force_color = false, never_color = false;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "hvlqc:", options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'h': display_help(argv[0]); return 0;
			case 'v': display_version(); return 0;
			case 'l': print_legend = false; break;
			case 'q': run_quietly = true; break;
			case 'c': {
				std::string_view v{optarg};
				if (v == "force" || v == "always")
					force_color = true;
				else if (v == "never" || v == "off")
					never_color = true;
				else {
					fmtns::print(std::cerr, "Unknown --color mode: {0}\n", v);
					return 1;
				}
				break;
			}
			case '?': return 1;
		}
	}

	if (optind < argc && argc - optind >= 2) {
		root1 = argv[optind++];
		root2 = argv[optind++];
	} else {
		fmtns::print("Missing positional argument(s): <path> <path>\n");
		return 1;
	}

	if ((!isatty(STDOUT_FILENO) && !force_color) || never_color) {
		using_color = false;

		ansi_reset = ansi_red = ansi_green = ansi_yellow = ansi_blue = "";
	}

	auto diffs = diff_trees(fs::directory_entry{root1}, fs::directory_entry{root2});
	if (!run_quietly && using_color)
		fmtns::print(std::cerr, "{0}", ansi_clear_to_beginning_of_line);

	if (!diffs.size()) {
		fmtns::print("No differences.\n");
	} else {
		diff root{diff_type::contents, -1, "<root>", std::move(diffs)};

		if (print_legend) {
			fmtns::print("Legend:\n");
			fmtns::print("  {0}- foo{1} - exists only in 1st tree\n", ansi_red, ansi_reset);
			fmtns::print("  {0}+ foo{1} - exists only in 2nd tree\n", ansi_green, ansi_reset);
			fmtns::print("  {0}! foo{1} - types differ (directory vs file, etc)\n", ansi_blue, ansi_reset);
			fmtns::print("  {0}? foo{1} - contents differ\n", ansi_yellow, ansi_reset);
		}

		fmtns::print("Diff:\n");

		display_diff(root);
	}
}

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
#include <charconv>
#include <sys/wait.h>
#include <cstring>
#include <fnmatch.h>

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
Input control:\n\
  -i, --ignore=PATTERN            ignore all paths that match the specified pattern,\n\
                                  relative to the source paths (that is, when comparing\n\
                                  /a/ and /b/, these prefixes are not included); can be specified\n\
                                  multiple times to add multiple patterns (a file being ignored\n\
                                  if any of them matches); see glob(7) for pattern syntax\n\
  --paranoid                      check file contents even if files appear to be obviously different\n\
                                  or same, ie. if the sizes differ or if it's the same inode on the\n\
                                  same device\n");

	fmtns::print("\n");

	fmtns::print("\
Output control:\n\
  -l, --no-legend                 don't display the legend before the diff\n\
  -q, --quiet                     don't display the progress indicator\n\
  -c, --color=WHEN                force (WHEN is 'force' or 'always'), or\n\
                                  disable (WHEN is 'never' or 'off') the use of colors;\n\
                                  by default colors are enabled if the output is a tty\n\
  -d, --git-diff=DEPTH            generate a patch file with 'git diff --no-index' for\n\
                                  every pair of differing directories at the given depth\n\
                                  (0 being children of the '<root>' node)\n\
  -p, --prune=PATTERN             do not show the inner differences of directories whose\n\
                                  paths match the specified pattern, relative to the source paths\n\
                                  (that is, when comparing /a/ and /b/, these prefixes are not included);\n\
                                  can be specified multiple times to add multiple patterns (a diff being\n\
                                  pruned if any of them matches); see glob(7) for pattern syntax\n\
  -P, --no-default-prune          do not add default prune patterns (\".git\" and \"**/.git\") to the\n\
                                  prune list\n\
  -m, --max-depth=DEPTH           do not show any inner differences of directories past the specified\n\
                                  depth (depth 0 pruning the '<root>' node itself)\n");

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
std::vector<std::string> ignore_patterns;
std::vector<std::string> prune_patterns;
std::vector<std::string> default_prune_patterns = {".git", "**/.git"};

int max_depth = -1;

bool should_ignore_file(const fs::path &path, bool a_path) {
	auto str = path.string().substr((a_path ? root1 : root2).string().size());

	for (const auto &pat : ignore_patterns) {
		if (!fnmatch(pat.c_str(), str.c_str(), FNM_PATHNAME))
			return true;
	}

	return false;
}

bool should_prune_diff(const diff &diff, int depth) {
	if (max_depth >= 0 && depth > (max_depth - 1))
		return true;

	auto str = diff.a_path.string().substr(root1.string().size());

	for (const auto &pat : prune_patterns) {
		if (!fnmatch(pat.c_str(), str.c_str(), FNM_PATHNAME))
			return true;
	}

	return false;
}

int git_diff_depth = -1;

const char *ansi_reset = "\x1b[0m";
const char *ansi_red = "\x1b[31m";
const char *ansi_green = "\x1b[32m";
const char *ansi_yellow = "\x1b[33m";
const char *ansi_blue = "\x1b[34m";
const char *ansi_clear_to_beginning_of_line = "\x1b[2K\x1b[G";
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

void generate_git_diff(const fs::path &a, const fs::path &b) {
	pid_t pid = fork();
	if (pid < 0) {
		fmtns::print(std::cerr, "Failed to fork: \"{0}\"\n", strerror(errno));
	} else if (pid == 0) {
		auto patch_path = fmtns::format("{0}.{1:x}-{2:x}.patch",
				a.filename().string(),
				fs::hash_value(a), fs::hash_value(b));

		execlp("git",
			"git", "-P",
			"diff",
			"--no-index",
			"--patch-with-stat",
			"--output", patch_path.c_str(),
			a.c_str(), b.c_str(),
			nullptr);

		fmtns::print(std::cerr, "Failed to exec: \"{0}\"\n", strerror(errno));

		_Exit(127);
	} else {
		int wstatus;
		wait(&wstatus);

		if (WEXITSTATUS(wstatus) != 0 && WEXITSTATUS(wstatus) != 1) {
			fmtns::print(std::cerr, "git diff invocation failed\n");
		}
	}
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
			} else if (should_prune_diff(diff, depth)) {
				print_in_color(ansi_yellow, "? {0} (pruned; different)\n", diff.name);
			} else {
				if (git_diff_depth >= 0 && (depth - 1) == git_diff_depth) {
					generate_git_diff(diff.a_path, diff.b_path);
				}

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
		{"git-diff",	required_argument,	0, 'd'},
		{"ignore",	required_argument,	0, 'i'},
		{"prune",	required_argument,	0, 'p'},
		{"no-default-prune",	no_argument,	0, 'P'},
		{"max-depth",	required_argument,	0, 'm'},
		{"paranoid",	no_argument,		0, 300},
		{0,		0,			0, 0}
	};

	bool print_legend = true;

	bool force_color = false, never_color = false;

	bool add_default_prune_patterns = true;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "hvlqc:d:i:p:Pm:", options, &option_index);

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
			case 'd': {
				auto out = std::from_chars(optarg, optarg + strlen(optarg), git_diff_depth);
				if (out.ec != std::errc{}) {
					fmtns::print(std::cerr, "Illegal value for --git-diff: {0}\n", optarg);
					return 1;
				}
				break;
			}
			case 'i': {
				ignore_patterns.push_back(optarg);
				break;
			}
			case 'p': {
				prune_patterns.push_back(optarg);
				break;
			}
			case 'P': add_default_prune_patterns = false; break;
			case 'm': {
				auto out = std::from_chars(optarg, optarg + strlen(optarg), max_depth);
				if (out.ec != std::errc{}) {
					fmtns::print(std::cerr, "Illegal value for --max-depth: {0}\n", optarg);
					return 1;
				}
				break;
			}
			case 300: paranoid = true; break;
			case '?': return 1;
		}
	}

	if (optind < argc && argc - optind >= 2) {
		root1 = argv[optind++];
		root1 /= "";
		root2 = argv[optind++];
		root2 /= "";
	} else {
		fmtns::print("Missing positional argument(s): <path> <path>\n");
		return 1;
	}

	if ((!isatty(STDOUT_FILENO) && !force_color) || never_color) {
		using_color = false;

		ansi_reset = ansi_red = ansi_green = ansi_yellow = ansi_blue = "";
	}

	if (add_default_prune_patterns) {
		for (const auto &pat : default_prune_patterns)
			prune_patterns.push_back(pat);
	}

	auto diffs = diff_trees(fs::directory_entry{root1}, fs::directory_entry{root2});
	if (!run_quietly && using_color)
		fmtns::print(std::cerr, "{0}", ansi_clear_to_beginning_of_line);

	if (!diffs.size()) {
		fmtns::print("No differences.\n");
	} else {
		diff root{diff_type::contents, -1, "<root>", root1, root2, std::move(diffs)};

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

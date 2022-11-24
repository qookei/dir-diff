/* Directory diff utility - std::print support glue
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

#if __has_include(<format>)
#include <format>

#if !__has_include(<print>)
namespace fmtns {
	using std::format;

	template <typename ...Args>
	constexpr void print(std::format_string<Args...> fmt, Args &&...args) {
		std::cout << format(fmt, std::forward<Args>(args)...);
	}

	template <typename ...Args>
	constexpr void print(std::ostream &os, std::format_string<Args...> fmt, Args &&...args) {
		os << format(fmt, std::forward<Args>(args)...);
	}
}
#else
#include <print>
namespace fmtns = std;
#endif
#else
#include <fmt/core.h>
#include <fmt/ostream.h>

namespace fmtns = fmt;
#endif

# Project build file
# Copyright (C) 2022  qookie
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

project('dir-diff', 'cpp',
	license : 'GPL-3.0-or-later',
	version : '0.1',
	default_options : ['cpp_std=c++20'])

deps = []

if not meson.get_compiler('cpp').has_header('format')
	deps += dependency('fmt')
endif

conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())

configure_file(input : 'src/config.hpp.in',
	output : 'config.hpp',
	configuration : conf_data)

install_man('man/dir-diff.1')

executable('dir-diff',
	'src/main.cpp', 'src/tree.cpp',
	include_directories : 'src/',
	dependencies : deps,
	install : true)

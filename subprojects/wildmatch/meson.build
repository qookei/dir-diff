project('wildmatch', 'cpp',
	license : 'BSD-3-Clause',
	version : '0.9.9999')

incs = include_directories('wildmatch/')

wildmatch_lib = static_library('wildmatch', 'wildmatch/wildmatch/wildmatch.cpp',
	include_directories : incs)

wildmatch_dep = declare_dependency(
	link_with : wildmatch_lib,
	include_directories : incs)

meson.override_dependency('wildmatch', wildmatch_dep)

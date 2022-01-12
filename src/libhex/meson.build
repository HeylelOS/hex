luac = find_program('luac')

libhex_luac_out = custom_target('libhex-luac.out',
	input : [
		'env.lua',
		'hex.lua',
		'log.lua',
		'rituals/build.lua',
		'rituals/configure.lua',
		'rituals/install.lua',
		'rituals/cmake/build.lua',
		'rituals/cmake/configure.lua',
		'rituals/cmake/install.lua',
		'rituals/gnu/configure.lua',
		'rituals/kbuild/build.lua',
		'rituals/kbuild/configure.lua',
		'rituals/kbuild/install.lua',
		'rituals/unix/build.lua',
		'rituals/unix/configure.lua',
		'rituals/unix/install.lua',
	],
	output : 'luac.out',
	command : [ luac, '-s', '-o', '@OUTPUT@', '@INPUT@' ]
)

libhex_luac_out_c = custom_target('libhex-luac.out.c',
	input : libhex_luac_out,
	output : 'luac.out.c',
	command : [ bin2src, '-S', 'hex_runtime', '-o', '@OUTPUT@', '--', '@INPUT@' ]
)

libhex = library('hex',
	dependencies : lua,
	include_directories : headers,
	install : true,
	sources : [
		'lua_env.c',
		'lua_fs.c',
		'lua_hex.c',
		'lua_log.c',
		'lua_report_log.c',
		'lua_report_none.c',
		libhex_luac_out_c
	]
)

pkgconfig.generate(libhex, description : 'Lua-based meta-build configuration tool and framework library.')

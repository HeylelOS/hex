hvn_man = find_program('hvn-man', required : false)

if hvn_man.found()
	man_1_hex = custom_target('hex(1)',
		output : 'hex.1', input : 'hex.1.md',
		command : [ hvn_man, '-i', '@INPUT@', '-o', '@OUTPUT@' ],
		install : true, install_dir : get_option('mandir') / 'man1',
		install_tag : 'man'
	)
endif

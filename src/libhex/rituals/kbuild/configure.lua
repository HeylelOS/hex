
hex.rituals['kbuild-configure'] = function(name, material)
	local setup = material.setup.configure
	local source = material.source
	local target = 'defconfig'

	if setup then
		local options = setup.options
		local config = setup.config

		if config then
			hex.cast('cp', '--', config, fs.path(source, '.config'))
		end

		if setup.target then
			target = setup.target
		end

		if options then
			fs.chdir(source)
			return hex.cast('make', options, '--', target)
		end
	end

	fs.chdir(source)
	return hex.cast('make', '--', target)
end


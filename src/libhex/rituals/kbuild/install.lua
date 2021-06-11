
hex.rituals['kbuild-install'] = function(name, material)
	local setup = material.setup.install

	fs.chdir(material.source)

	if setup then
		local options = setup.options

		if options then
			return hex.cast('make', options, '--', 'install')
		end
	end

	return hex.cast('make', 'install')
end


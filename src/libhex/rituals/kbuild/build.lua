
hex.rituals['kbuild-build'] = function(name, material)
	local setup = material.setup.build

	fs.chdir(material.source)

	if setup then
		local options = setup.options
		local targets = setup.targets

		if options then
			if targets then
				return hex.cast('make', options, '--', targets)
			else
				return hex.cast('make', options, '--')
			end
		elseif targets then
			return hex.cast('make', '--', targets)
		end
	end

	return hex.cast('make')
end


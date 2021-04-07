
hex.rituals['unix-build'] = function(name, material)
	local setup = material.setup.build

	fs.chdir(material.build)

	if setup then
		local options = setup.options

		if options then
			local targets = setup.targets

			if targets then
				hex.cast('make', options, '--', targets)
			else
				hex.cast('make', options, '--')
			end
		end
	else
		hex.cast('make')
	end
end


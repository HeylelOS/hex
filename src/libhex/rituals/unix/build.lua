
hex.rituals['unix-build'] = function(name, material)
	local setup = material.setup.build
	local workdir = fs.pwd()

	fs.chdir(material.build)

	if setup then
		local options = setup.options

		if options then
			local targets = setup.targets

			if targets then
				hex.cast('make', table.unpack(options), '--', table.unpack(targets))
			else
				hex.cast('make', table.unpack(options), '--')
			end
		end
	else
		hex.cast('make')
	end

	fs.chdir(workdir)
end


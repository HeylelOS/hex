
hex.rituals['unix-install'] = function(name, material)
	local setup = material.setup.install

	fs.chdir(material.build)

	if setup then
		local options = setup.options

		if options then
			hex.cast('make', table.unpack(options), '--', 'install')
		end
	else
		hex.cast('make', 'install')
	end
end


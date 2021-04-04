
hex.rituals['unix-configure'] = function(name, material)
	local script = fs.path(fs.pwd(), material.source, 'configure')

	fs.chdir(material.build)

	local setup = material.setup.configure

	if setup then
		local options = setup.options

		if options then
			hex.cast(script, table.unpack(options))
		end
	else
		hex.cast(script)
	end
end


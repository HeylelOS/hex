
hex.rituals['unix-configure'] = function(name, material)
	local script = fs.path(fs.pwd(), material.source, 'configure')

	fs.chdir(material.build)

	local setup = material.setup.configure

	if setup then
		local options = setup.options

		if options then
			return hex.cast(script, options)
		end
	end

	return hex.cast(script)
end



hex.rituals['unix-configure'] = function(name, material)
	local workdir = fs.pwd()
	local script = fs.path(workdir, material.source, 'configure')

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

	fs.chdir(workdir)
end


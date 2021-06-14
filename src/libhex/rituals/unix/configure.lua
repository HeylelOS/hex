
hex.rituals['unix-configure'] = function(name, material)
	local setup = material.setup.configure
	local script, options

	if setup then
		script = setup.script
		options = setup.options
	end

	if not script then
		script = fs.path(fs.pwd(), material.source, 'configure')
	end

	fs.chdir(material.build)

	if options then
		return hex.cast(script, options)
	else
		return hex.cast(script)
	end
end



hex.rituals['gnu-configure'] = function(name, material)
	local setup = material.setup.configure
	local autoreconf, script, options

	if setup then
		autoreconf = setup.autoreconf
		script = setup.script
		options = setup.options
	end

	local source = material.source

	if autoreconf then
		hex.cast('autoreconf', autoreconf, '--', source)
	else
		hex.cast('autoreconf', '--', source)
	end

	if not script then
		script = fs.path(fs.pwd(), source, 'configure')
	end

	fs.chdir(material.build)

	if options then
		hex.cast(script, options)
	else
		hex.cast(script)
	end
end


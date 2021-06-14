
hex.rituals['gnu-configure'] = function(name, material)
	local setup = material.setup.configure
	local autooptions, script, scriptoptions

	if setup then
		autooptions = setup.autooptions
		script = setup.script
		scriptoptions = setup.scriptoptions
	end

	local source = material.source

	if autooptions then
		hex.cast('autoreconf', autooptions, '--', source)
	else
		hex.cast('autoreconf', '--', source)
	end

	if not script then
		script = fs.path(fs.pwd(), source, 'configure')
	end

	fs.chdir(material.build)

	if scriptoptions then
		hex.cast(script, scriptoptions)
	else
		hex.cast(script)
	end
end


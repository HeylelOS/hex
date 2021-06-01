
hex.rituals['gnu-configure'] = function(name, material)
	local setup = material.setup.configure
	local source = material.source
	local scriptoptions

	if setup then
		local autooptions = setup.autooptions

		if autooptions then
			hex.cast('autoreconf', autooptions, '--', source)
		end

		scriptoptions = setup.scriptoptions
	else
		hex.cast('autoreconf', source)
	end

	local script = fs.path(fs.pwd(), source, 'configure')
	fs.chdir(material.build)

	if scriptoptions then
		hex.cast(script, scriptoptions)
	else
		hex.cast(script)
	end
end


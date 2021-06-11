
hex.rituals['install'] = function(name, material)
	local ritual = material.override.install

	if not ritual then
		local build = material.build

		if fs.isreg(fs.path(build, 'CMakeCache.txt')) then
			ritual = hex.rituals['cmake-install']
		elseif fs.isreg(fs.path(build, 'Makefile')) then
			ritual = hex.rituals['unix-install']
		else
			local source = material.source

			if fs.isreg(fs.path(source, 'Kbuild')) then
				ritual = hex.rituals['kbuild-install']
			end
		end
	end

	if ritual then
		ritual(name, material)
	else
		error('Unable to determine install ritual for '..name)
	end
end


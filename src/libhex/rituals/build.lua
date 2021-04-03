
hex.rituals['build'] = function(name, material)
	local ritual = material.override['build']

	if not ritual then
		local build = material.build

		if fs.isreg(fs.path(build, 'CMakeCache.txt')) then
			ritual = hex.rituals['cmake-build']
		elseif fs.isreg(fs.path(build, 'Makefile')) then
			ritual = hex.rituals['unix-build']
		end
	end

	if ritual then
		ritual(name, material)
	else
		error('Unable to determine build ritual for '..name)
	end
end


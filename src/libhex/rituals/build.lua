
hex.rituals['build'] = function(name, material)
	local delegate = nil

	do
		local build = material.build

		if fs.isreg(fs.path(build, 'CMakeCache.txt')) then
			delegate = 'cmake-build'
		end
	end

	if delegate then
		hex.rituals[delegate](name, material)
	else
		error('Unable to determine build system for ', name)
	end
end


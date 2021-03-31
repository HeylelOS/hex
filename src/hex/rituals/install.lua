
hex.rituals['install'] = function(name, material)
	local delegate = nil

	do
		local build = material.build

		if fs.isreg(fs.path(build, 'CMakeCache.txt')) then
			delegate = 'cmake-install'
		end
	end

	if delegate then
		hex.rituals[delegate](name, material)
	else
		error('Unable to determine install system for ', name)
	end
end


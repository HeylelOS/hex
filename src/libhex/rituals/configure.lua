
hex.rituals['configure'] = function(name, material)
	local delegate = nil

	do
		local source = material.source

		if fs.isreg(fs.path(source, 'CMakeLists.txt')) then
			delegate = 'cmake-configure'
		end
	end

	if delegate then
		hex.rituals[delegate](name, material)
	else
		error('Unable to determine configuration system for ', name)
	end
end


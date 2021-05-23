
hex.rituals['configure'] = function(name, material)
	local ritual = material.override['configure']

	if not ritual then
		local source = material.source

		if fs.isreg(fs.path(source, 'CMakeLists.txt')) then
			ritual = hex.rituals['cmake-configure']
		elseif fs.isreg(fs.path(source, 'configure.ac')) then
			ritual = hex.rituals['gnu-configure']
		elseif fs.isexe(fs.path(source, 'configure')) then
			ritual = hex.rituals['unix-configure']
		end
	end

	if ritual then
		ritual(name, material)
	else
		error('Unable to determine configuration ritual for '..name)
	end
end


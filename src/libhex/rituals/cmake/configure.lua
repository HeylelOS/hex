
hex.rituals['cmake-configure'] = function(name, material)
	local setup = material.setup.configure

	if setup then
		local options = setup.options

		if options then
			return hex.cast('cmake', options, '-S', material.source, '-B', material.build)
		end
	end

	return hex.cast('cmake', '-S', material.source, '-B', material.build)
end


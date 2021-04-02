
hex.rituals['cmake-configure'] = function(name, material)
	local configure = material.configure

	if configure then
		local options = configure.options

		if options then
			hex.cast('cmake', table.unpack(options), '-S', material.source, '-B', material.build)
			return
		end
	end

	hex.cast('cmake', '-S', material.source, '-B', material.build)
end


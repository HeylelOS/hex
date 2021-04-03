
hex.rituals['cmake-configure'] = function(name, material)
	local setup = material.setup.configure

	if setup then
		local options = setup.options

		if options then
			hex.cast('cmake', table.unpack(options), '-S', material.source, '-B', material.build)
		end
	else
		hex.cast('cmake', '-S', material.source, '-B', material.build)
	end
end



hex.rituals['cmake-configure'] = function(name, material)
	local setup = material.setup.configure

	if setup then
		local variables = setup.variables
		local options = setup.options

		if variables then
			if not options then
				options = { }
			end

			local index = #options + 1

			for k, v in pairs(variables) do
				options[index] = '-D'..k..'='..v
				index = index + 1
			end
		end

		if options then
			return hex.cast('cmake', options, '-S', material.source, '-B', material.build)
		end
	end

	return hex.cast('cmake', '-S', material.source, '-B', material.build)
end


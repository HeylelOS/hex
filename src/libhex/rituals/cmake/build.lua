
hex.rituals['cmake-build'] = function(name, material)
	local setup = material.setup.build

	if setup then
		local options = setup.options

		if options then
			local arguments = setup.arguments

			if arguments then
				return hex.cast('cmake', '--build', material.build, options, '--', arguments)
			else
				return hex.cast('cmake', '--build', material.build, options)
			end
		end
	end

	return hex.cast('cmake', '--build', material.build)
end


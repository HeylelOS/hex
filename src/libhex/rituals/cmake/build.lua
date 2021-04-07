
hex.rituals['cmake-build'] = function(name, material)
	local setup = material.setup.build

	if setup then
		local options = setup.options

		if options then
			local build_tool_options = setup.build_tool_options

			if build_tool_options then
				hex.cast('cmake', '--build', material.build, options, '--', build_tool_options)
			else
				hex.cast('cmake', '--build', material.build, options)
			end
		end
	else
		hex.cast('cmake', '--build', material.build)
	end
end


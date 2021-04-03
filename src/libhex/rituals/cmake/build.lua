
hex.rituals['cmake-build'] = function(name, material)
	local setup = material.setup.build

	if setup then
		local options = setup.options

		if options then
			local build_tool_options = setup.build_tool_options

			if build_tool_options then
				hex.cast('cmake', '--build', material.build, table.unpack(options), '--', table.unpack(build_tool_options))
			else
				hex.cast('cmake', '--build', material.build, table.unpack(options))
			end
		end
	else
		hex.cast('cmake', '--build', material.build)
	end
end


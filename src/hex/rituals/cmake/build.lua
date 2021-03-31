
hex.rituals['cmake-build'] = function(name, material)
	local build = material.build

	if build then
		local options = build.options

		if options then
			local build_tool_options = build.build_tool_options

			if build_tool_options then
				hex.cast('cmake', table.unpack(options), '--build', material.build, '--', table.unpack(build_tool_options))
			else
				hex.cast('cmake', table.unpack(options), '--build', material.build)
			end

			return
		end
	end

	hex.cast('cmake', '--build', material.build)
end


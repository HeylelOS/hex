
hex.rituals['cmake-install'] = function(name, material)
	local setup = material.setup.install

	if setup then
		local options = setup.options

		if options then
			return hex.cast('cmake', '--install', material.build, options)
		end
	end

	return hex.cast('cmake', '--install', material.build)
end


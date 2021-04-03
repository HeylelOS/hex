
hex.rituals['cmake-install'] = function(name, material)
	local setup = material.setup.install

	if setup then
		local options = setup.options

		if options then
			hex.cast('cmake', '--install', material.build, table.unpack(options))
		end
	else
		hex.cast('cmake', '--install', material.build)
	end
end


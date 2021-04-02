
hex.rituals['cmake-install'] = function(name, material)
	local install = material.install

	if install then
		local options = install.options

		if options then
			hex.cast('cmake', '--install', material.build, table.unpack(options))
			return
		end
	end

	hex.cast('cmake', '--install', material.build)
end


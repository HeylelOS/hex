
-- Create the crucible in the 'build' directory
local crucible = hex.crucible('build')

-- Add isolation properties, if we have a complete toolchain
-- we could isolate the build using the 'filesystem' member
crucible.shackle = {
	user = { uid = 0; gid = 0 }; -- Make the program believe root root is doing this
}

-- Create a directory where we'll install everything (eg. to make packages afterwards)
local prefix = fs.path(crucible.molten, 'prefix')
fs.mkdirs(prefix)

do
	-- Fine tune of the material, here we only specify the prefix for install,
	-- but we could modify environment variables, add build options, etc...
	local material = hex.melt(crucible, 'cmake-hello')

	material['install'] = {
		options = { '--prefix', prefix, }
	}

end

-- Finally, perform all rituals
hex.perform(crucible, 'configure', 'build', 'install')



-- If log.level is not set, set it to info, default value doesn't exist but
-- expands to notice. Starting with notice, hex's log reports emit some informations,
-- like exposing when rituals for a material are beginning during hex.perform. Info gives
-- more informations, like uses of fs.copy and fs.remove.
if not log.level then
	log.level = 'info'
end

-- Create the crucible in the 'forge' directory
local crucible = hex.crucible('forge')

-- Add isolation properties, if we have a complete toolchain
-- we could isolate the build using the 'filesystem' member
crucible.shackle = {
	user = { uid = 0; gid = 0 }; -- Make the program believe root root is doing this
	outputs = fs.path(crucible.molten, 'outputs'); -- Specify outputs directory for material's rituals stdout
}

-- Create a directory where we'll stage everything (eg. to make packages afterwards)
local staging = fs.path(crucible.molten, 'staging')
fs.mkdirs(staging)

-- Fine tune of the materials, here we only specify the staged prefix for install,
-- but we could modify environment variables, add build options, etc...

do -- CMake
	local material = hex.melt(crucible, 'cmake-hello')
	material.setup.install = {
		options = { '--prefix', staging, }
	}
end

do -- GNU configure. build & install rituals are shared with UNIX
	local material = hex.melt(crucible, 'gnu-hello')
	material.setup = {
		configure = {
			autoreconf = { '-i', }, -- Should install what's missing, will be done in source tree
			script = fs.path('../../..', material.source, 'configure'), -- Putting relative paths everywhere to keep traces relative
			options = { '--prefix=/', }, -- Install in /, not /usr/local, the default
		};
		install = { options = { 'DESTDIR='..fs.path('../..', fs.basename(staging)), } };
	}
end

do -- UNIX configure/makefile
	local material = hex.melt(crucible, 'unix-hello')
	material.setup = {
		configure = { script = fs.path('../../..', material.source, 'configure'), };
		install = { options = { 'DESTDIR='..fs.path('../..', fs.basename(staging)), } };
	}
end

-- Finally, perform all rituals
hex.perform(crucible, 'configure', 'build', 'install')


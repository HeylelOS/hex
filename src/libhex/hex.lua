
hex.crucible = function(molten)

	fs.mkdirs(molten)

	return {
		molten = molten;
		shackle = { };
		melted = { };
		env = { };
	}
end

hex.melt = function(crucible, source)
	local name = fs.basename(source)
	local check = string.sub(name, 1, 2)

	if check == '/' or check == '.' then
		error('hex.melt: Invalid source (hidden file or root): '..source)
	end

	local build = fs.path(crucible.molten, 'tailings', name)
	fs.mkdirs(build)

	local material = {
		build = build;
		source = source;
		dependencies = { };
		override = { };
		setup = { };
		env = { };
	}

	crucible.melted[name] = material

	return material
end

-- The following function returns the list of a crucible's
-- melted sources in an order satisfying each of their dependencies.
-- The implementation is crude, and the complexity may not be suited for all use cases.
local function resolvedependencies(melted)
	-- Pre-treatment of melted structure
	-- A Set is a table where table[n] is true when n is inside the set
	-- Set of all nodes without incoming edges (noincomingedges)
	local noincomingedges = { }
	local noincomingcount = 0
	-- node -> nodes where all nodes have node as incoming (parents of node)
	local edgesfrom = { }
	-- node -> nodes where node have all nodes as incoming (children of node)
	local edgesto = { }

	-- The complexity of the pre-treatment should be something of O(|V|+|E|)
	for name, material in pairs(melted) do
		local dependencies = material.dependencies
		local count = #dependencies

		-- If we are a leaf node, we are recorded,
		-- else the children set is extended with the dependencies
		-- and the parent set for all dependencies
		-- is extended (and created if non existant) with the node.
		if count == 0 then
			noincomingedges[name] = true
			noincomingcount = noincomingcount + 1
		else
			local children = { }
			for i, node in pairs(dependencies) do
				local parents = edgesfrom[node]
				if not parents then
					parents = { }
					edgesfrom[node] = parents
				end

				children[node] = true
				parents[name] = true
			end
			edgesto[name] = children
		end
	end

	-- Topological sorting (cf. Kahn's algorithm)
	-- The complexity of this one should be of O(|V|+|E|),
	-- Leaving the function with a O(|V|+|E|), so the default
	-- complexity of the base algorithm
	local list = { }
	local listcount = 0

	while noincomingcount > 0 do
		local node = next(noincomingedges)
		-- Removing node
		noincomingedges[node] = nil
		noincomingcount = noincomingcount - 1
		-- Adding to list
		listcount = listcount + 1
		list[listcount] = node

		-- For all nodes on which node is a dependency,
		-- remove the dependency, then check if the parent
		-- still has children, if not, add it to noincomingedges
		local parents = edgesfrom[node]
		if parents then
			edgesfrom[node] = nil
			for parent in pairs(parents) do
				local children = edgesto[parent]
				children[node] = nil

				if next(children) == nil then
					edgesto[parent] = nil
					noincomingedges[parent] = true
					noincomingcount = noincomingcount + 1
				end
			end
		end
	end

	if next(edgesto) ~= nil or next(edgesfrom) ~= nil then
		error('Detected a cycle in dependency graph')
	end

	return list, listcount
end

hex.perform = function(crucible, ...)
	-- Resolve the dependency list
	local list, listcount = resolvedependencies(crucible.melted)
	-- Acquire incantation from arguments
	local incantation = hex.incantation(...)
	local incantationcount = #incantation
	local output = crucible.shackle.output

	if output then
		fs.mkdirs(output)
	end

	for i = 1, listcount do
		local name = list[i]
		local materialout

		if output then
			materialout = fs.path(output, name)
			fs.mkdirs(materialout)
			for _, entry in ipairs(fs.readdir(materialout)) do
				fs.unlink(fs.path(materialout, entry))
			end
		end

		log.info('Starting rituals for ', name)

		for j = 1, incantationcount do
			local invocation = function()
				local material = crucible.melted[name]
				hex.hinder(crucible.shackle)
				env.fill(pairs(crucible.env))
				env.fill(pairs(material.env))
				incantation[j](name, material)
			end

			if materialout then
				hex.invoke(invocation, fs.path(materialout, j))
			else
				hex.invoke(invocation)
			end
		end
	end
end

hex.hinderfilesystem = function(filesystem)
	local mountpoints = filesystem.mountpoints
	local mountpointscount = #mountpoints
	local root = filesystem.root

	for i = 1, mountpointscount do
		local mountpoint = mountpoints[i]
		local source = mountpoint.source or ''
		local target = mountpoint.target or fs.path(root, source)

		fs.mkdirs(target)
		fs.mount(source, target, mountpoint.fstype or '', mountpoint.flags)
	end

	fs.chroot(root)
end

hex.hinder = function(shackle)
	local user = shackle.user
	if user then
		hex.hinderuser(user)
	end

	local filesystem = shackle.filesystem
	if filesystem then
		hex.hinderfilesystem(filesystem)
	end
end


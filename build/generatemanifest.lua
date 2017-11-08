json = require("json")

local function sha1(content)
	local hash = string.sha1(content)
	local newHash = ""
	for i = 1, #hash, 2 do
		newHash = newHash .. hash:sub(i+1, i+1) .. hash:sub(i, i)
	end

	return newHash:lower()
end

newaction {
	trigger = "generatemanifest",
	shortname = "Generate the manifest file",
	description = "Parse all files in the ../Utopia-Game folder and write them in a manifest file",
	execute = function ()
		local gameDirectory = "../../Utopia-Game"

		local manifest = {}
		manifest.Files = {}

		for k,v in pairs(os.matchfiles(gameDirectory .. "/**")) do
			local content = io.readfile(v)
			table.insert(manifest.Files, {path = path.getrelative(gameDirectory, v), size = #content, hash = sha1(content)})
		end
		table.sort(manifest.Files, function (a, b) return a.path < b.path end)

		manifest.Directories = {}
		for k,v in pairs(os.matchdirs(gameDirectory .. "/**")) do
			table.insert(manifest.Directories, path.getrelative(gameDirectory, v))
		end
		table.sort(manifest.Directories)

		io.writefile("manifest", json.encode(manifest))
	end
}

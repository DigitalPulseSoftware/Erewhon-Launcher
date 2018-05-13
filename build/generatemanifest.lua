json = require("json")

local function sha1(content)
	local hash = string.sha1(content)
	local newHash = ""
	for i = 1, #hash, 2 do
		newHash = newHash .. hash:sub(i+1, i+1) .. hash:sub(i, i)
	end

	return newHash:lower()
end

local function GenerateManifest(folder, downloadFolder)
	local manifest = {}
	manifest.DownloadFolder = downloadFolder
	manifest.Files = {}

	for k,v in pairs(os.matchfiles(folder .. "/**")) do
		print("Processing " .. v)
		local content = io.readfile(v)
		table.insert(manifest.Files, {path = path.getrelative(folder, v), size = #content, hash = sha1(content)})
	end
	table.sort(manifest.Files, function (a, b) return a.path < b.path end)

	manifest.Directories = {}
	for k,v in pairs(os.matchdirs(folder .. "/**")) do
		table.insert(manifest.Directories, path.getrelative(folder, v))
	end
	table.sort(manifest.Directories)

	return manifest
end

newaction {
	trigger = "generatemanifest",
	shortname = "Generate the manifest file",
	description = "Parse all files in the ../Utopia-Game folder and write them in a manifest file",
	execute = function (test)
		local osname = os.target()
		local gameDirectory = "../../Utopia-Content"
		local launcherDirectory = "../bin/Release"

		local manifest = {}
		manifest.Version = os.date("%Y_%m_%d_%H_%M_%S")
		manifest.Launcher = GenerateManifest(launcherDirectory, "launcherfiles/" .. osname)
		manifest.Game = GenerateManifest(gameDirectory, "gamefiles/" .. osname)

		io.writefile("manifest." .. osname, json.encode(manifest))
	end
}

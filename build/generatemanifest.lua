json = require("json")

local function sha1(content)
	local hash = string.sha1(content)
	local newHash = ""

	-- Reverse hash
	for i = 1, #hash, 2 do
		newHash = newHash .. hash:sub(i+1, i+1) .. hash:sub(i, i)
	end

	return newHash:lower()
end

local function GenerateManifest(baseDirectory, manifestInfo, previousManifest)
	local removedFiles = {}
	if (previousManifest) then
		for k,fileInfo in pairs(previousManifest.Files or {}) do
			removedFiles[fileInfo.targetPath] = true
		end

		for k,fileName in pairs(previousManifest.RemovedFiles or {}) do
			removedFiles[fileName] = true
		end
	end

	local manifest = {}
	manifest.Directories = {}
	manifest.Files = {}

	for k,contentInfo in pairs(manifestInfo.Content) do
		local contentProtected = {}
		if (contentInfo.UserFiles) then
			for k,fileName in pairs(contentInfo.UserFiles) do
				local targetPath = contentInfo.TargetPath .. "/" .. fileName
				contentProtected[targetPath] = true
				removedFiles[targetPath] = nil
			end
		end

		local executableContent = {}
		if (contentInfo.ExecutableFiles) then
			for k,fileName in pairs(contentInfo.ExecutableFiles) do
				local targetPath = contentInfo.TargetPath .. "/" .. fileName
				executableContent[targetPath] = true
			end
		end

		local folder = baseDirectory .. "/" .. contentInfo.ContentDirectory
		for k,filePath in pairs(os.matchfiles(folder .. "/**")) do
			--print("Processing " .. filePath)
			
			local basePath = path.getrelative(folder, filePath)

			local content = io.readfile(filePath)

			local entry = {
				downloadPath = contentInfo.DownloadPath .. "/" .. basePath,
				targetPath = contentInfo.TargetPath .. "/" .. basePath,
				hash = sha1(content),
				size = #content
			}

			if (contentProtected[entry.targetPath]) then
				entry.protected = true
			end

			if (executableContent[entry.targetPath]) then
				entry.executable = true
			end

			table.insert(manifest.Files, entry)

			removedFiles[entry.targetPath] = nil
		end

		for k,v in pairs(os.matchdirs(folder .. "/**")) do
			table.insert(manifest.Directories, path.getrelative(folder, v))
		end
	end
	table.sort(manifest.Directories)
	table.sort(manifest.Files, function (a, b) return a.targetPath < b.targetPath end)

	manifest.RemovedFiles = {}
	for fileName,v in pairs(removedFiles) do
		table.insert(manifest.RemovedFiles, fileName)
		print("Removed file: " .. fileName)
	end
	table.sort(manifest.RemovedFiles)
	
	return manifest
end

newaction {
	trigger = "generatemanifest",
	shortname = "Generate the manifest file",
	description = "Generate a new manifest file based on .lua files in ../Utopia-Content directory",
	execute = function ()
		local contentDirectory = "../../Utopia-Content"
		local osname = os.target()
		local manifestFile = contentDirectory .. "/manifest." .. osname

		local previousManifestFile = io.readfile(manifestFile)
		if (not previousManifestFile) then
			print("Warning: no previous manifest found, do you really want to start a new one ? (Y/N)")
			local yn = io.stdin:read()
			if (yn:sub(1,1):lower() ~= "y") then
				print("Aborted.")
				return
			end
		end
		
		local previousManifest = previousManifestFile and json.decode(previousManifestFile) or {}

		local manifest = {}
		manifest.Version = os.date("%Y_%m_%d_%H_%M_%S")

		for k,filePath in pairs(os.matchfiles(contentDirectory .. "/*.lua")) do
			print("Loading " .. path.getname(filePath) .. "...")
			local f, err = loadfile(filePath)
			if (f) then
				ManifestInfo = {}

				f()

				manifest[ManifestInfo.Entry] = GenerateManifest(path.getdirectory(filePath), ManifestInfo, previousManifest[ManifestInfo.Entry])
			else
				print("Unable to load manifest file " .. path.getname(filePath) .. ": " .. err)
			end
		end

		io.writefile(manifestFile, json.encode(manifest))
	end
}

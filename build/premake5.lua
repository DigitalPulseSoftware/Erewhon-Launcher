Config = {}

local configLoader, err = load(io.readfile("config.lua"), "config.lua", "t", Config)
if (not configLoader) then
	if (os.isfile("config.lua")) then
		error("config.lua is missing")
	else
		error("config.lua failed to load: " .. err)
	end
end

local configLoaded, err = pcall(configLoader)
if (not configLoaded) then
	error("config.lua failed to load: " .. err)
end

local libs = {"NazaraPath", "QtPath"}
local libsDirs = {"", "/bin", "/include", "/lib"}
for k,v in pairs(libs) do
	local dir = Config[v]
	for k,v in pairs(libsDirs) do
		local checkPath = dir .. v
		if (not os.isdir(checkPath)) then
			error("\"" .. checkPath .. "\" does not exists")
		end
	end
end

ProjectSettings = {}
ProjectSettings.Defines = {}
ProjectSettings.Kind = "ConsoleApp"
ProjectSettings.Name = "ErewhonLauncher"
ProjectSettings.Libs = {}
ProjectSettings.LibsDebug = {  "Qt5Cored", "Qt5Guid", "Qt5Widgetsd", "Qt5Networkd"}
ProjectSettings.LibsRelease = {"Qt5Core",  "Qt5Gui",  "Qt5Widgets", "Qt5Network"}

dofile("generatemanifest.lua")



location(_ACTION)

workspace(ProjectSettings.Name)
	configurations { "Debug", "Release" }
	platforms("x64")

	filter "configurations:*32"
		architecture "x86"

	filter "configurations:*64"
		architecture "x86_64"

	project(ProjectSettings.Name)
		kind(ProjectSettings.Kind)
		language("C++")
		cppdialect("C++17")

		defines(ProjectSettings.Defines)
		
		flags { "MultiProcessorCompile", "NoMinimalRebuild" }

		files { "../include/**.h", "../include/**.hpp", "../include/**.inl" }
		files { "../src/**.h", "../src/**.hpp", "../src/**.inl", "../src/**.cpp" }

		includedirs { "../include/", "../src/" }

		debugdir("../bin/%{cfg.buildcfg}")
		targetdir("../bin/%{cfg.buildcfg}")

		for k,v in pairs(libs) do
			local dir = Config[v]

			filter {}
				includedirs(dir .. "/include")

			filter {"architecture:x86", "system:not Windows", "configurations:Debug"}
				libdirs(dir .. "/bin/debug")
				libdirs(dir .. "/bin/x86/debug")

			filter {"architecture:x86", "system:not Windows"}
				libdirs(dir .. "/bin")
				libdirs(dir .. "/bin/x86")

			filter {"architecture:x86_64", "system:not Windows", "configurations:Debug"}
				libdirs(dir .. "/bin/debug")
				libdirs(dir .. "/bin/x64/debug")

			filter {"architecture:x86_64", "system:not Windows"}
				libdirs(dir .. "/bin")
				libdirs(dir .. "/bin/x64")

			filter {"architecture:x86", "system:Windows", "configurations:Debug"}
				libdirs(dir .. "/lib/debug")
				libdirs(dir .. "/lib/x86/debug")

			filter {"architecture:x86", "system:Windows"}
				libdirs(dir .. "/lib")
				libdirs(dir .. "/lib/x86")

			filter {"architecture:x86_64", "system:Windows", "configurations:Debug"}
				libdirs(dir .. "/lib/debug")
				libdirs(dir .. "/lib/x64/debug")
			
			filter {"architecture:x86_64", "system:Windows"}
				libdirs(dir .. "/lib")
				libdirs(dir .. "/lib/x64")
		end
		
		filter "configurations:Debug"
			defines { "DEBUG" }
			links(ProjectSettings.LibsDebug)
			symbols "On"

		filter "configurations:Release"
			defines { "NDEBUG" }
			links(ProjectSettings.LibsRelease)
			optimize "On"

		filter "action:vs*"
			defines "_CRT_SECURE_NO_WARNINGS"

		filter {}
			links(ProjectSettings.Libs)

		if (os.ishost("windows")) then
			local commandLine = "premake5.exe " .. table.concat(_ARGV, ' ')

			prebuildcommands("cd .. && " .. commandLine)
			if (ProjectSettings.Kind == "WindowedApp" or ProjectSettings.Kind == "ConsoleApp") then
				postbuildcommands("cd .. && premake5.exe --buildarch=%{cfg.architecture} --buildmode=%{cfg.buildcfg} thirdparty_sync")
			end
		end

	newoption({
		trigger     = "buildarch",
		description = "Set the directory for the thirdparty_update",
		value       = "VALUE",
		allowed = {
			{ "x86", "/x86" },
			{ "x86_64", "/x64" }
		}
	})

	newoption({
		trigger     = "buildmode",
		description = "Set the directory for the thirdparty_update",
		value       = "VALUE",
		allowed = {
			{ "Debug", "/Debug" },
			{ "Release", "/Release" }
		}
	})

	newaction {
		trigger = "thirdparty_sync",
		description = "Update .dll files from thirdparty directory",

		execute = function()
			assert(_OPTIONS["buildarch"])
			assert(_OPTIONS["buildmode"])

			local archToDir = {
				["x86"] = "x86",
				["x86_64"] = "x64"
			}

			local archDir = archToDir[_OPTIONS["buildarch"]]
			assert(archDir)

			local binPaths = {}
			for k,v in pairs(libs) do
				table.insert(binPaths, Config[v] .. "/bin")
				table.insert(binPaths, Config[v] .. "/bin/" .. archDir)
			end

			local updatedCount = 0
			local totalCount = 0

			local libs = table.join(ProjectSettings.Libs, ProjectSettings["Libs" .. _OPTIONS["buildmode"]], ProjectSettings.AdditionalDependencies)

			for k,lib in pairs(libs) do
				lib = lib .. ".dll"
				local found = false
				local sourcePath
				for k,v in pairs(binPaths) do
					sourcePath = v .. "/" .. lib
					if (os.isfile(sourcePath)) then
						found = true
						break
					else
						sourcePath = v .. "/" .. path.getdirectory(lib) .. "/lib" .. path.getname(lib)
						if (os.isfile(sourcePath)) then
							lib = "lib" .. lib
							found = true
							break
						end
					end
				end

				if (found) then
					local fileName = path.getname(sourcePath)
					local targetPath = path.normalize(path.translate("../bin/" .. _OPTIONS["buildmode"] .. "/" .. lib), "/")

					local copy = true
					if (os.isfile(targetPath)) then
						local sourceUpdateTime = os.stat(sourcePath).mtime
						local targetUpdateTime = os.stat(targetPath).mtime

						if (targetUpdateTime > sourceUpdateTime) then
							copy = false
						end
					end

					if (copy) then
						print("Copying " .. lib .. "...")

						-- Copying using os.copyfile doesn't update modified time...
						local src = io.open(sourcePath, "rb")
						local dst = io.open(targetPath, "wb")
						if (not src or not dst) then
							error("Failed to copy " .. targetPath)
						end

						local data
						repeat
							data = src:read(10 * 1024 * 1024) -- 10 MiB
							if (not data) then
								break
							end

							dst:write(data)
						until false

						src:close()
						dst:close()

						updatedCount = updatedCount + 1
					end

					totalCount = totalCount + 1
				else
					print("Dependency not found: " .. lib)
				end
			end

			print("" .. updatedCount .. "/" .. totalCount .. " files required an update")
		end
	}

SUPPRESS_COMMAND_OUTPUT = (os.host() == "windows" and "> nul 2>&1" or "> /dev/null 2>&1")

local reinitOptionTrigger = "reinit"

newoption {
    trigger = reinitOptionTrigger,
    description = "Reinitialize all dependencies"
}

local function cloneGitDependency(dependency)
    print(string.format("Cloning Git dependency \"%s\" from \"%s\"...", dependency.Name, dependency.Git.Url))

    -- Clone the repository and cd into it https://stackoverflow.com/a/63786181
    os.executef("git clone -q --filter=blob:none -n --depth 1 %s %s %s", dependency.Git.Url, dependency.ProjectDir, SUPPRESS_COMMAND_OUTPUT)
    local cwd = os.getcwd()
    os.chdir(dependency.ProjectDir)

    if dependency['Files'] then
        -- Clone specified files and license only
        local files = "!/* /LICENSE*"
        for k, file in pairs(dependency.Files) do
            files = files .. " " .. file
        end

        os.executef("git sparse-checkout set --no-cone %s %s", files, SUPPRESS_COMMAND_OUTPUT)
    end

    -- If a specific revision is specified, point to it
    local revision = ""
    if dependency.Git.Revision then
        revision = dependency.Git.Revision
        print(string.format("\tFetching revision \"%s\"...", revision))
        os.executef("git fetch -q --all --tags --depth 1 %s", SUPPRESS_COMMAND_OUTPUT)
        os.executef("git checkout -q --no-progress %s %s", revision, SUPPRESS_COMMAND_OUTPUT)
    end

    -- Remove the .git folder as we're only using git to download the repository
    os.execute(os.translateCommands("{RMDIR} .git"))
    os.chdir(cwd)

    term.pushColor(term.green)
    print(string.format("\tDependency \"%s\" cloned succesfully!", dependency.Name))
    term.popColor()
end

local function preProcessDependencyTable(dependencies)
    for name, dependency in pairs(dependencies) do
        -- Default to Utility if no kind provided
        dependency.Kind = dependency.Kind or "Utility"

        local gitUrl = dependency.Git.Url
        name = name or path.getbasename(gitUrl)

        dependency['Name'] = name
        dependency['ProjectDir'] = Config.DependenciesDir .. name .. "/"

        if dependency['IncludeDirs'] ~= nil then
            for i, includeDir in ipairs(dependency.IncludeDirs) do
                includeDir = "%{wks.location}/" .. dependency.ProjectDir .. includeDir
                dependency.IncludeDirs[i] = includeDir
            end
        end
    end
end

local function cleanDependencyCache()
    local depsToRemove = os.matchdirs(Config.DependenciesDir .. "*")
    if #depsToRemove ~= 0 then
        print("Cleaning Dependency Cache...")
        for _, dir in pairs(depsToRemove) do
            print(string.format("\tCleaning %s...", dir))
            os.execute(os.translateCommands("{RMDIR} " .. dir))
        end
    else
        print("No Dependencies in Cache to remove.")
    end
end

local function cleanDependencyInCache(dependencyToClean)
    if (string.find(dependencyToClean, [[(\.\.)|\/|\\]], 1, true)) then
        print(string.format("Failed to clean dependency with invalid name \"\"", dependencyToClean));
        return
    end
    local depsToRemove = os.matchdirs(Config.DependenciesDir .. dependencyToClean)
    if #depsToRemove ~= 0 then
        print(string.format("Cleaning Dependency \"%s\" in Dependency Cache...", dependencyToClean))
        for _, dir in pairs(depsToRemove) do
            os.execute(os.translateCommands("{RMDIR} " .. dir))
        end
    end
end

function processDependencies(dependencies)
    preProcessDependencyTable(dependencies)
    
    local reinitOption = _OPTIONS[reinitOptionTrigger]
    if reinitOption then
        if (reinitOption ~= "") then
            cleanDependencyInCache(reinitOption)
        else
            cleanDependencyCache()
        end
    end

    for _, dependency in pairsByKeys(dependencies) do
        local dependencyExists = os.isdir(dependency.ProjectDir)
        if not dependencyExists then
            if dependency.Git then
                cloneGitDependency(dependency)
            end
        end

        -- If IncludeDirs isn't manually defined, set it to the include folder of the dependency
        if dependency['IncludeDirs'] == nil then
            local includeDirs = os.matchdirs(dependency.ProjectDir .. "/**include/")
            if #includeDirs ~= 0 then
                dependency.IncludeDirs = { "%{wks.location}/" .. includeDirs[1] }
            else
                dependency.IncludeDirs = {}
            end
        end
    end
end

function generateDependencyProjects(dependencies)
    for _, dependency in pairsByKeys(dependencies) do
        local cwd = os.getcwd()
        os.chdir(dependency.ProjectDir)

        project(dependency.Name)
            applyCommonCppSettings()
            kind(dependency.Kind)
            includedirs(dependency.IncludeDirs)
            warnings "Off"
            defines {
                -- Warning Silence Defines
                "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING", -- Silence warning in spdlog
            }
            
            filterStandalone()
            if dependency.Kind == premake.SHAREDLIB then
                kind(premake.STATICLIB)
            end
            
            filter {}
            if dependency['CustomProperties'] then
                dependency.CustomProperties()
            end
            filter {}

        os.chdir(cwd)
    end
    project "*"
end
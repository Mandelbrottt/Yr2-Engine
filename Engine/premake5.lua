include "Config.lua"

include "Scripts/Premake/Common.lua"
include "Scripts/Premake/Actions.lua"

workspace(Refly.Name)
    location "./"

    configurations {
        Config.Debug.Editor.ConfigName,
        Config.Development.Editor.ConfigName,
        Config.Release.Editor.ConfigName,

        Config.Debug.ConfigName,
        Config.Development.ConfigName,
        Config.Release.ConfigName,
    }

    platforms {
        -- "x86",
        "x64",
    }

    filter "platforms:*32 or *86"
        architecture "x86"
    filter "platforms:*64"
        architecture "x64"
    filter {}

    startproject(Refly.Name .. Refly.Entry.Name)

    group(Refly.Name)
        include("Source/" .. Refly.Core.Name)
        include("Source/" .. Refly.Entry.Name)
        include("Source/" .. Refly.Editor.Name)
    group ""

    group(Refly.ThirdParty.Name)

    group ""
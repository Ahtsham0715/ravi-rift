using UnrealBuildTool;
using System.Collections.Generic;

public class RaviCircuitTarget : TargetRules
{
	public RaviCircuitTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RaviCircuit");
	}
}

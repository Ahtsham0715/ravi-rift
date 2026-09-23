using UnrealBuildTool;
using System.Collections.Generic;

public class RaviCircuitTarget : TargetRules
{
	public RaviCircuitTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RaviCircuit");
	}
}

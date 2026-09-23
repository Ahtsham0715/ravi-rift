using UnrealBuildTool;
using System.Collections.Generic;

public class RaviCircuitEditorTarget : TargetRules
{
	public RaviCircuitEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RaviCircuit");
	}
}

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT="$ROOT/RaviCircuit.uproject"

ENGINE_ROOT="${UE_ROOT:-}"
PARTIAL_ENGINE_ROOT=""
if [[ -z "$ENGINE_ROOT" ]]; then
  for candidate in \
    "/Users/Shared/Epic Games/UE_5.8" \
    "/Users/Shared/Epic Games/UE_5.7" \
    "/Users/Shared/Epic Games/UE_5.6" \
    "/Applications/Unreal Engine/UE_5.8" \
    "/Applications/Unreal Engine/UE_5.7" \
    "/Applications/Unreal Engine/UE_5.6"; do
    if [[ -d "$candidate" && -z "$PARTIAL_ENGINE_ROOT" ]]; then
      PARTIAL_ENGINE_ROOT="$candidate"
    fi
    if [[ -x "$candidate/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" ]]; then
      ENGINE_ROOT="$candidate"
      break
    fi
  done
fi

if [[ -z "$ENGINE_ROOT" ]]; then
  if [[ -n "$PARTIAL_ENGINE_ROOT" ]]; then
    echo "Found Unreal directory at $PARTIAL_ENGINE_ROOT, but build scripts are not present yet."
    echo "Epic Games Launcher is likely still downloading or verifying the engine. Wait for the install to finish, then rerun."
    exit 1
  fi
  echo "Unreal Engine not found."
  echo "Install UE through Epic Games Launcher, or set UE_ROOT=/path/to/UE_5.x and rerun."
  exit 1
fi

echo "Using Unreal Engine at: $ENGINE_ROOT"
"$ENGINE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="$PROJECT" -game
"$ENGINE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" RaviCircuitEditor Mac Development -Project="$PROJECT" -WaitMutex

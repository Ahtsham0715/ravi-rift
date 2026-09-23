# Ravi Rift Multiplayer Plan

## Implemented Scope

- Single-player: Arcade / VS CPU and Training modes.
- Local multiplayer: Local Versus spawns a second local player controller and supports P2 keyboard controls.
- Online scaffold: the front-end can host a listen-server match or join `127.0.0.1`.
- Server-authoritative combat: replicated fighters send move and guard/movement input to the server through RPCs; health, meter, state, combo data and facing replicate back to clients.
- Online player handoff: `PostLogin` assigns the first remote challenger to P2 and points their camera at the fight camera.

## Current Limitations

- Online joining is a direct-address developer flow, not a public matchmaking/session browser.
- No rollback, input delay selection, ping display or reconnect flow yet.
- The current placeholder combat sim is server-authoritative, which is correct for safety but not final fighting-game netcode quality.
- Imported assets still depend on Unreal Editor being available.

## Production Targets

- Add OnlineSubsystem session creation/discovery for public lobbies.
- Introduce fixed-step rollback or delay-based netcode after the compiled Unreal baseline is stable.
- Replicate round HUD announcements and menu state through explicit replicated match state instead of local strings.
- Add host/client QA scripts for two local editor instances once Unreal finishes installing.

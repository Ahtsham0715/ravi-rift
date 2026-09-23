# Ravi Rift Combat Design

## Quality Bar

Ravi Rift targets Tekken-rival class modern 3D arena fighting: responsive controls, strict frame data, expressive movement, strong punishment, meaningful sidesteps, wall pressure and cinematic payoff. All IP, fighters, stages, move names, UI, audio and assets must remain original.

## Core Pillars

- Responsiveness first: inputs should buffer near recovery and wake-up windows.
- 3D spacing matters: forward/back movement and sidesteps must change ranges and tracking outcomes.
- Weighty hits: every attack needs synchronized hit stop, pushback, VFX, sound and camera response.
- Readable offense: high/mid/low/throw layers should be clear enough to learn, deep enough to mind-game.
- Walls are a system: wall splats and wall combos should create advantage without infinite loops.

## Current Fighters

### Zara Vey

Fast magnetic kickboxer. Strengths are speed, chained pokes, mid-range kicks, launcher access and stylish air follow-ups. Weaknesses should be lower damage per hit and more risk on committed specials.

### Hamza Kade

Heavy pressure grappler. Strengths are damage, throws, shoulder pressure and wall carry. Weaknesses should be slower startup, weaker sidestep and punishable whiffs.

## Needed Next Systems

- Per-move whiff recovery tuning.
- Per-move tracking arcs rather than one scalar.
- True throw break windows.
- Per-difficulty AI profiles.
- Training input history with directional notation.
- Hitbox/hurtbox debug view once Unreal compiles.
- Animation notify migration once skeletal animations exist.
- Production multiplayer layer: session browser plus rollback or tuned input-delay netcode after the Unreal compile baseline is stable.

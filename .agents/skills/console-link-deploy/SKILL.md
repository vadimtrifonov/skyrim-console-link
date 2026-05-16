---
name: console-link-deploy
description: Package and deploy Console Link to Mod Organizer 2.
---

# Console Link Deploy

## Commands

- Package: `./scripts/Package.ps1`
- Deploy: `./scripts/Deploy.ps1 -Target <MO2 mod path> [-OverwriteIni]`

## Rules

- Deploy to Mod Organizer 2, not directly to the Skyrim install folder.
- If the MO2 target is ambiguous, ask which MO2 instance to use.

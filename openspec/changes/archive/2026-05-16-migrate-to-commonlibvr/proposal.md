## Motivation

[CommonLibVR](https://github.com/alandtse/CommonLibVR) is a newer maintained fork of CommonLibSSE-NG. ConsoleLink carries local runtime-site knowledge for command submission, console-output target resolution, and plugin-qualified form lookup that CommonLibVR now maintains.

## Proposed change

### Address/site findings

1. Remove entirely: custom ExecuteCommand RVA seam
  - Current local site: src/console/ExecuteCommandSeam.cpp
  - Hardcoded local RVA: 0x00885100
  - Hardcoded prologue/hook size: 17 bytes
  - CommonLibVR now provides RE::Console::ExecuteCommand(const char*), implemented through RELOCATION_ID(50157, 51084).
  - Plan: delete ExecuteCommandSeam.*; command submission should call RE::Console::ExecuteCommand on SE/AE/VR.
  - Because the CommonLibVR API takes const char*, command submission should reject embedded NUL and pass an owned null-terminated command string.
2. Use a non-executable path for accepted input capture
  - CommonLibVR exposes the console menu FxDelegate callback table, but it does not provide a first-class accepted-input event.
  - Wrap the active console menu’s FxDelegate "ExecuteCommand" callback, call the original first, then emit [console-in].
  - Rationale: manual accepted input is a UI delegate behavior, so callback-table wrapping observes it without an executable patch.
3. Keep but shrink: console output capture
  - CommonLibVR exposes RE::ConsoleLog::VPrint internally via RELOCATION_ID(50180, 51110).
  - There is still no event/API replacement for passive output observation.
  - Plan: keep a single guarded function-entry detour for output capture, but resolve the target through CommonLibVR relocation, not legacy RE::Offset::ConsoleLog::VPrint.
  - Runtime scope is SE/AE/VR only where the target entry bytes validate at runtime; mismatch skips output capture for that runtime.
4. Remove local light-plugin form ID workaround
  - Current local code manually composes runtime form IDs in src/local_commands/support/FormKey.*.
  - CommonLibVR’s TESDataHandler::LookupFormID/LookupForm is now VRESL-aware and preserves light-plugin subindices when the VR compiled file collection is available.
  - Plan: keep form-key parsing, remove PluginLoadAddress / BuildRuntimeFormID, and use TESDataHandler::LookupForm.
  - VR light-plugin verification should confirm the SkyrimVRESL compiled file collection is available; otherwise it does not prove the VRESL-aware path.
5. Fix lock usage while touching lookup-prefix
  - LookupPrefixCommand.cpp currently constructs BSReadWriteLock instead of using a read guard.
  - Plan: use RE::BSReadLockGuard, matching CommonLibVR’s own TESForm::LookupByEditorID.

### Migration plan

1. Dependency migration
  - Remove old lib/commonlibsse-ng submodule reference.
  - Add lib/commonlibvr pointing at https://github.com/alandtse/CommonLibVR, pinned to v4.17.1 / 6705db4..., recursively.
  - Update xmake.lua path/messages to CommonLibVR.
  - Keep target/rule names commonlibsse-ng / commonlibsse-ng.plugin because CommonLibVR still exposes those.
  - Update xmake-requires.lock for CommonLibVR’s package set.
2. Command submission contraction
  - Delete ExecuteCommandSeam.*.
  - In ConsoleCommandProcessor, keep readiness based on console menu availability, reject embedded NUL, and call RE::Console::ExecuteCommand with an owned null-terminated command string.
  - Remove the artificial VR-only command-submission guard.
3. Input capture redesign
  - Rework ConsoleInputCapture to wrap the console FxDelegate "ExecuteCommand" callback.
  - Original callback runs first; only after it returns do we emit [console-in].
4. Output capture shrink
  - Resolve ConsoleLog::VPrint through CommonLibVR relocation IDs.
  - Keep strict byte validation before patching.
  - Recalculate trampoline budget from actually enabled hook families instead of reserving the old fixed 128 bytes.
5. Form-key simplification
  - Replace LookupFormByFormKey implementation with TESDataHandler::LookupForm.
  - Remove local runtime-ID composition tests; keep/adjust pure parser tests as useful.
6. Spec amendments
  - Remove retired implementation details from normative specs: local RVA, prologue size, trampoline mechanics, and VR-only ExecuteCommand seam language.
  - Keep specs focused on behavior: supported-runtime readiness, command handoff, and fail-closed input/output observation.

### Modified Capabilities

- `console-commands`: command submission no longer depends on a locally validated ExecuteCommand seam and is intended for SE/AE/VR when console readiness is active.
- `console-capture`: accepted-input observation uses console menu FxDelegate wrapping; output observation remains a byte-validated detour.

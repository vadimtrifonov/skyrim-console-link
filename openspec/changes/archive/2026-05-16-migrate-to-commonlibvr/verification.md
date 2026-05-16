## Live validation

### 2026-05-16 - Skyrim AE 1.6.1170 - Wabbajack - CSVO 2.2

1. **PASS: AE output hook installs on validated prologue**
   - ConsoleLink.log:
     - `[17:08:33.857] [26888] [I] Initializing ConsoleLink on Skyrim AE runtime 1-6-1170-0`
     - `[17:08:33.857] [26888] [I] Reserved 30 bytes of trampoline space for console output capture`
     - `[17:08:33.857] [26888] [I] Installed passive console output capture for Skyrim AE at 0x7FF662A39220 with validated AE 1.6.1170 16-byte ConsoleLog::VPrint prologue`
     - `[17:08:33.858] [26888] [I] ConsoleLink loaded`

2. **PASS: visible console output is captured**
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:08:44.695] [console-out] Registering functions started`
     - `[2026-05-16 17:08:44.698] [console-out] Registering functions finished in 0.002000 sec`

3. **PASS: manually accepted console input is captured**
   - Console input: `tgm`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:10:40.272] [console-in] tgm`
     - `[2026-05-16 17:10:40.272] [console-out] God Mode enabled.`

4. **PASS: inbox batch is queued**
   - ConsoleLink.inbox.txt append:
     - `cdbg.lookup Gold001\ncdbg.lookup-prefix Gold 3\ncdbg.lookup-form Skyrim.esm|0000000F\ncdbg.lookup-form EnchantmentArtExtender.esl|000800\ncdbg.lookup-lvli LItemAntlersLarge50 2\ncdbg.lookup-lvli-form Skyrim.esm|0010FF77 2\nplayer.getav stamina\n`
   - ConsoleLink.log:
     - `[17:42:48.655] [24640] [I] Queued 7 inbox line(s) from 'C:\Users\Vadim\Documents\My Games\Skyrim Special Edition\SKSE\ConsoleLink.inbox.txt'; pending=7`

5. **PASS: exact EditorID lookup resolves**
   - Inbox line: `cdbg.lookup Gold001`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:42:48.669] [local] cdbg.lookup Gold001`
     - `[2026-05-16 17:42:48.670] [local] lookup "Gold001" -> hit form=0x0000000F type=FORM name="Gold" source="Skyrim.esm"`

6. **PASS: prefix lookup resolves and applies limit**
   - Inbox line: `cdbg.lookup-prefix Gold 3`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:42:48.670] [local] cdbg.lookup-prefix Gold 3`
     - `[2026-05-16 17:42:48.672] [local] lookup-prefix "Gold" -> 32 match(es)`
     - `[2026-05-16 17:42:48.672] [local] lookup-prefix match editorID="Gold001" form=0x0000000F type=FORM name="Gold" source="Skyrim.esm"`
     - `[2026-05-16 17:42:48.672] [local] lookup-prefix "Gold" -> truncated to 3 of 32 match(es)`

7. **PASS: base-game form-key lookup resolves**
   - Inbox line: `cdbg.lookup-form Skyrim.esm|0000000F`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:42:48.672] [local] cdbg.lookup-form Skyrim.esm|0000000F`
     - `[2026-05-16 17:42:48.672] [local] lookup-form "Skyrim.esm|0000000F" -> hit form=0x0000000F type=FORM editorID="" name="Gold" source="Skyrim.esm"`

8. **PASS: light-plugin form-key lookup resolves**
   - Inbox line: `cdbg.lookup-form EnchantmentArtExtender.esl|000800`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:42:48.672] [local] cdbg.lookup-form EnchantmentArtExtender.esl|000800`
     - `[2026-05-16 17:42:48.672] [local] lookup-form "EnchantmentArtExtender.esl|000800" -> hit form=0xFE009800 type=FORM editorID="" source="EnchantmentArtExtender.esl"`

9. **PASS: leveled-list lookup by EditorID emits entry**
   - Inbox line: `cdbg.lookup-lvli LItemAntlersLarge50 2`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 17:42:48.672] [local] cdbg.lookup-lvli LItemAntlersLarge50 2`
     - `[2026-05-16 17:42:48.672] [local] lookup-lvli "LItemAntlersLarge50" -> hit form=0x0010FF77 entries=1 source="Skyrim.esm"`
     - `[2026-05-16 17:42:48.672] [local] lookup-lvli entry level=1 count=1 form=0x0006BC0A type=FORM name="Large Antlers" source="unofficial skyrim special edition patch.esp"`

10. **PASS: leveled-list lookup by form key emits entry**
    - Inbox line: `cdbg.lookup-lvli-form Skyrim.esm|0010FF77 2`.
    - ConsoleLink.activity.log:
      - `[2026-05-16 17:42:48.672] [local] cdbg.lookup-lvli-form Skyrim.esm|0010FF77 2`
      - `[2026-05-16 17:42:48.672] [local] lookup-lvli-form "Skyrim.esm|0010FF77" -> hit form=0x0010FF77 editorID="" entries=1 source="Skyrim.esm"`
      - `[2026-05-16 17:42:48.673] [local] lookup-lvli-form entry level=1 count=1 form=0x0006BC0A type=FORM name="Large Antlers" source="unofficial skyrim special edition patch.esp"`

11. **PASS: inbox console command is submitted and output is captured**
    - Inbox line: `player.getav stamina`.
    - ConsoleLink.activity.log:
      - `[2026-05-16 17:42:48.673] [console-submit] player.getav stamina`
      - `[2026-05-16 17:42:48.686] [console-out] GetActorValue: Stamina >> 100.00`

### 2026-05-16 - Skyrim VR 1.4.15 - Wabbajack - Sovngarde 1.3

1. **PASS: VR output hook installs on validated prologue**
   - ConsoleLink.log:
     - `[18:07:14.372] [31344] [I] Initializing ConsoleLink on Skyrim VR runtime 1-4-15-0`
     - `[18:07:14.377] [31344] [I] Reserved 30 bytes of trampoline space for console output capture`
     - `[18:07:14.377] [31344] [I] Installed passive console output capture for Skyrim VR at 0x7FF7CFC76DF0 with validated SE/VR 16-byte ConsoleLog::VPrint prologue`
     - `[18:07:14.377] [31344] [I] ConsoleLink loaded`

2. **PASS: visible console output is captured**
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:07:46.564] [console-out] Registering functions started`
     - `[2026-05-16 18:07:46.568] [console-out] Registering functions finished in 0.004000 sec`

3. **PASS: manually accepted console input is captured**
   - Console input: `tgm`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:32:24.773] [console-in] tgm`
     - `[2026-05-16 18:32:24.773] [console-out] God Mode enabled.`

4. **PASS: inbox batch is queued**
   - ConsoleLink.inbox.txt append:
     - `cdbg.lookup Gold001\ncdbg.lookup-prefix Gold 3\ncdbg.lookup-form Skyrim.esm|0000000F\ncdbg.lookup-form EnchantmentArtExtender.esl|000800\ncdbg.lookup-lvli LItemAntlersLarge50 2\ncdbg.lookup-lvli-form Skyrim.esm|0010FF77 2\nplayer.getav stamina\n`
   - ConsoleLink.log:
     - `[18:30:55.588] [6132 ] [I] Queued 7 inbox line(s) from 'C:\Users\Vadim\Documents\My Games\Skyrim VR\SKSE\ConsoleLink.inbox.txt'; pending=7`

5. **PASS: exact EditorID lookup resolves**
   - Inbox line: `cdbg.lookup Gold001`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:30:55.593] [local] cdbg.lookup Gold001`
     - `[2026-05-16 18:30:55.593] [local] lookup "Gold001" -> hit form=0x0000000F type=FORM name="Gold" source="JS Purses and Septims SE Patch.esp"`

6. **PASS: prefix lookup resolves and applies limit**
   - Inbox line: `cdbg.lookup-prefix Gold 3`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:30:55.593] [local] cdbg.lookup-prefix Gold 3`
     - `[2026-05-16 18:30:55.597] [local] lookup-prefix "Gold" -> 33 match(es)`
     - `[2026-05-16 18:30:55.597] [local] lookup-prefix match editorID="Gold001" form=0x0000000F type=FORM name="Gold" source="JS Purses and Septims SE Patch.esp"`
     - `[2026-05-16 18:30:55.597] [local] lookup-prefix "Gold" -> truncated to 3 of 33 match(es)`

7. **PASS: base-game form-key lookup resolves**
   - Inbox line: `cdbg.lookup-form Skyrim.esm|0000000F`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:30:55.597] [local] cdbg.lookup-form Skyrim.esm|0000000F`
     - `[2026-05-16 18:30:55.597] [local] lookup-form "Skyrim.esm|0000000F" -> hit form=0x0000000F type=FORM editorID="Gold001" name="Gold" source="JS Purses and Septims SE Patch.esp"`

8. **PASS: light-plugin form-key lookup resolves**
   - Inbox line: `cdbg.lookup-form EnchantmentArtExtender.esl|000800`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:30:55.597] [local] cdbg.lookup-form EnchantmentArtExtender.esl|000800`
     - `[2026-05-16 18:30:55.597] [local] lookup-form "EnchantmentArtExtender.esl|000800" -> hit form=0xFE013800 type=FORM editorID="EAE_EFS_EmptyShader" source="EnchantmentArtExtender.esl"`

9. **PASS: leveled-list lookup by EditorID emits entry**
   - Inbox line: `cdbg.lookup-lvli LItemAntlersLarge50 2`.
   - ConsoleLink.activity.log:
     - `[2026-05-16 18:30:55.597] [local] cdbg.lookup-lvli LItemAntlersLarge50 2`
     - `[2026-05-16 18:30:55.597] [local] lookup-lvli "LItemAntlersLarge50" -> hit form=0x0010FF77 entries=1 source="Skyrim.esm"`
     - `[2026-05-16 18:30:55.597] [local] lookup-lvli entry level=1 count=1 form=0x0006BC0A type=FORM editorID="MAG_AntlersLarge" name="Large Antlers" source="Apothecary.esp"`

10. **PASS: leveled-list lookup by form key emits entry**
    - Inbox line: `cdbg.lookup-lvli-form Skyrim.esm|0010FF77 2`.
    - ConsoleLink.activity.log:
      - `[2026-05-16 18:30:55.597] [local] cdbg.lookup-lvli-form Skyrim.esm|0010FF77 2`
      - `[2026-05-16 18:30:55.597] [local] lookup-lvli-form "Skyrim.esm|0010FF77" -> hit form=0x0010FF77 editorID="LItemAntlersLarge50" entries=1 source="Skyrim.esm"`
      - `[2026-05-16 18:30:55.597] [local] lookup-lvli-form entry level=1 count=1 form=0x0006BC0A type=FORM editorID="MAG_AntlersLarge" name="Large Antlers" source="Apothecary.esp"`

11. **PASS: inbox console command is submitted and output is captured**
    - Inbox line: `player.getav stamina`.
    - ConsoleLink.activity.log:
      - `[2026-05-16 18:30:55.597] [console-submit] player.getav stamina`
      - `[2026-05-16 18:30:55.615] [console-out] GetActorValue: Stamina >> 125.00`

## Reverse-engineering validation

### 2026-05-16 - Skyrim AE 1.6.1170 - Wabbajack - CSVO 2.2

1. **PASS: `ConsoleLog::VPrint` first 16 bytes are patch-safe**
   - runtime-re-validator live-process capture:
     - repo: `C:\Games\Skyrim\My Tools\runtime-re-validator`
     - interpreter: `.venv\Scripts\python.exe`
     - script: `scripts\capture_manifest.py`
     - manifest: `.tmp\consolelink_ae1170_vprint.json`
     - process/module: `SkyrimSE.exe` / `SkyrimSE.exe`
     - PID: `17196`
     - process state: `unpatched`
     - capture window: `--before 0 --after 96`
     - output: `evidence\consolelink_ae1170_vprint_live.json`
   - Observed:
     - artifact: `C:\Games\Skyrim\My Tools\runtime-re-validator\evidence\consolelink_ae1170_vprint_live.json`
     - module: `C:\Games\Skyrim\Wabbajack - CSVO 2.2\Game Root\SkyrimSE.exe`, base `0x7FF662140000`
     - target: RVA `0x8F9220`, VA `0x7FF662A39220`
     - bytes: `4C 89 44 24 18 48 89 54 24 10 53 55 56 57 41 54`
     - instructions: `mov [rsp+18h], r8`; `mov [rsp+10h], rdx`; `push rbx`; `push rbp`; `push rsi`; `push rdi`; `push r12`
   - Confirms: the patch window ends on an instruction boundary and contains no RIP-relative or branch instructions.

## Static/local checks

- 2026-05-16 `./scripts/Verify.ps1 -Mode debug -Quiet` passed.

## Runtime tests

`ConsoleLink.activity.log`:

```text
[local] lookup-form "Skyrim.esm|0003DF19" -> hit form=0x0003DF19 type=LVLI editorID="Van_LI_BanditBossCuirass" source="RoleRim - Loot.esp"
[local] lookup-prefix match editorID="ArmorIronAllOutfit" form=0x0001697C type=OTFT source="Skyrim.esm"
[local] lookup-lvli-form "Skyrim.esm|0003DF19" -> hit form=0x0003DF19 editorID="Van_LI_BanditBossCuirass" entries=4 source="RoleRim - Loot.esp"
[local] lookup-lvli-form entry level=1 count=1 form=0xFE414DFD type=LVLI editorID="rr_Sub1_Arm_Hvy_Cuirass" source="RoleRim - Loot.esp"
[local] lookup-lvli "Van_LI_BanditBossCuirass" -> hit form=0x0003DF19 entries=4 source="RoleRim - Loot.esp"
[local] lookup-lvli entry level=1 count=1 form=0xFE414DFD type=LVLI editorID="rr_Sub1_Arm_Hvy_Cuirass" source="RoleRim - Loot.esp"
[local] lookup-lvli-form "Skyrim.esm|00012E49" -> wrong-type form=0x00012E49 type=ARMO editorID="ArmorIronCuirass" source="Skyrim.esm"
```

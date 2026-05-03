#include "local_commands/lookup_lvli/LookupLvliInspect.h"

#include "local_commands/support/FormFormatting.h"
#include "pch.h"

#include <RE/RTTI.h>
#include <RE/T/TESLevItem.h>

#include <stdexcept>

namespace LocalCommands
{
	LookupLvliResult InspectLookupLvliForm(const RE::TESForm* form)
	{
		LookupLvliResult result{};
		if (form == nullptr) {
			return result;
		}

		if (form->GetFormType() != RE::FormType::LeveledItem) {
			result.kind = LookupLvliResultKind::kWrongType;
			result.wrongType.formID = form->GetFormID();
			result.wrongType.formType = std::string(DescribeFormType(form->GetFormType()));
			if (const auto* editorID = form->GetFormEditorID(); editorID != nullptr) {
				result.wrongType.editorID = editorID;
			}
			if (const auto* file = form->GetFile(); file != nullptr) {
				result.wrongType.source = std::string(file->GetFilename());
			}
			return result;
		}

		const auto* leveledItem = skyrim_cast<const RE::TESLevItem*>(form);
		if (leveledItem == nullptr) {
			throw std::runtime_error("lookup-lvli inspection resolved LVLI form that could not be cast to TESLevItem");
		}

		result.kind = LookupLvliResultKind::kHit;
		result.hit.formID = form->GetFormID();
		if (const auto* editorID = form->GetFormEditorID(); editorID != nullptr) {
			result.hit.editorID = editorID;
		}
		if (const auto* file = form->GetFile(); file != nullptr) {
			result.hit.source = std::string(file->GetFilename());
		}

		result.hit.entries.reserve(leveledItem->entries.size());
		for (const auto& object : leveledItem->entries) {
			LookupLvliEntry entry{};
			entry.level = object.level;
			entry.count = object.count;

			if (const auto* entryForm = object.form; entryForm != nullptr) {
				entry.formID = entryForm->GetFormID();
				entry.formType = std::string(DescribeFormType(entryForm->GetFormType()));
				if (const auto* entryEditorID = entryForm->GetFormEditorID(); entryEditorID != nullptr && entryEditorID[0] != '\0') {
					entry.editorID = entryEditorID;
				}
				if (const auto* entryName = entryForm->GetName(); entryName != nullptr && entryName[0] != '\0') {
					entry.name = entryName;
				}
				if (const auto* entryFile = entryForm->GetFile(); entryFile != nullptr) {
					entry.source = std::string(entryFile->GetFilename());
				}
			} else {
				entry.formType = "FORM";
			}

			result.hit.entries.push_back(std::move(entry));
		}

		return result;
	}
}

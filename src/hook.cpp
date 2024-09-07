#include "hook.h"
#include "ClibUtil/editorID.hpp"

namespace hooks
{
	bool bShouldBleedOut_Serana = false;
	bool bShouldBleedOut_Valerica = false;
	bool bElderScrollEquipped = false;

	OnMeleeHitHook& OnMeleeHitHook::GetSingleton() noexcept
	{
		static OnMeleeHitHook instance;
		return instance;
	}

	void OnMeleeHitHook::InstallHook()
	{
		// Taken from: https://github.com/doodlum/MaxsuWeaponSwingParry-ng/blob/main/src/WeaponParry_Hooks.h
		// (with permission from doodlum and Maxsu)
		SKSE::AllocTrampoline(1 << 4);

		REL::Relocation<std::uintptr_t> OnMeleeHitBase{ REL::RelocationID(37650, 38603) };
		auto&                           trampoline = SKSE::GetTrampoline();
		_OnMeleeHit = trampoline.write_call<5>(OnMeleeHitBase.address() + REL::Relocate(0x38B, 0x45A), OnMeleeHit);
	}

	void OnMeleeHitHook::Set_iFrames(RE::Actor* actor)
	{
		actor->SetGraphVariableBool("bIframeActive", true);
		actor->SetGraphVariableBool("bInIframe", true);
	}

	void OnMeleeHitHook::Reset_iFrames(RE::Actor* actor)
	{
		actor->SetGraphVariableBool("bIframeActive", false);
		actor->SetGraphVariableBool("bInIframe", false);
	}

	void OnMeleeHitHook::dispelEffect(RE::MagicItem* spellForm, RE::Actor* a_target)
	{
		const auto targetActor = a_target->AsMagicTarget();
		if (targetActor->HasMagicEffect(spellForm->effects[0]->baseEffect)) {
			auto activeEffects = targetActor->GetActiveEffectList();
			for (const auto& effect : *activeEffects) {
				if (effect->spell == spellForm) {
					effect->Dispel(true);
				}
			}
		}
	}

	void remove_item(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_item, std::uint32_t a_count, bool a_silent, RE::TESObjectREFR* a_otherContainer)
	{
		using func_t = decltype(&remove_item);
		static REL::Relocation<func_t> func{ RELOCATION_ID(56261, 56647) };
		return func(a_ref, a_item, a_count, a_silent, a_otherContainer);
	}

	void OnMeleeHitHook::UnequipAll(RE::Actor *a_actor){
		auto inv = a_actor->GetInventory();
		auto ElderScroll = RE::TESForm::LookupByEditorID<RE::TESAmmo>("DLC1ElderScrollBack");
		for (auto& [item, data] : inv) {
			const auto& [count, entry] = data;
			if (count > 0 && entry->IsWorn()) {
				if (entry->object->formID == ElderScroll->formID) {
					bElderScrollEquipped = true;
				}
				RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, item);
			}
		}
	}

	void OnMeleeHitHook::EquipfromInvent(RE::Actor* a_actor, RE::FormID a_formID)
	{
		auto inv = a_actor->GetInventory();
		for (auto& [item, data] : inv) {
			const auto& [count, entry] = data;
			if (count > 0 && entry->object->formID == a_formID) {
				RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, entry->object);
				break;
			}
			continue;
		}
	}

	void OnMeleeHitHook::VLDrain(RE::Actor* a_actor, bool remove){
		switch (a_actor->GetLevel()) {
		case 20:
		    if (remove){
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain03"));
			}else{
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain03"));
			}
			break;

		case 28:
			if (remove) {
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain04"));
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaUnarmedDamage01"));
			} else {
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain04"));
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaUnarmedDamage01"));
			}
			break;

		case 38:
			if (remove) {
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain05"));
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaUnarmedDamage02"));
			} else {
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain05"));
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaUnarmedDamage02"));
			}
			break;

		case 48:
			if (remove) {
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain06"));
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaUnarmedDamage03"));
			} else {
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain06"));
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaUnarmedDamage03"));
			}
			break;

		default:
			if (remove) {
				a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain02"));
			} else {
				a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDrain02"));
			}
			break;
		}
	}

	void OnMeleeHitHook::VLS_SendVampireLordTransformation(STATIC_ARGS, RE::Actor* a_actor)
	{
		const auto race = a_actor->GetRace();
		const auto raceEDID = race->formEditorID;
		if (raceEDID == "DLC1VampireBeastRace") {
			return;
		}
		a_actor->SetGraphVariableBool("IUBusy", true);
		logger::info("Began Transformation");
		auto data = RE::TESDataHandler::GetSingleton();
		util::playSound(a_actor, (data->LookupForm<RE::BGSSoundDescriptorForm>(0x10F564, "Skyrim.esm")));
		a_actor->NotifyAnimationGraph("IdleVampireLordTransformation");
		//a_actor->NotifyAnimationGraph("SetRace");
		Set_iFrames(a_actor);
		VLS_CompleteTransformation(a_actor);
	}

	void OnMeleeHitHook::VLS_CompleteTransformation(RE::Actor* a_actor){
		//firstpart//
		logger::info("completing Transformation");
		const auto FXchange = RE::TESForm::LookupByEditorID<RE::MagicItem>("VLSeranaChangeFX");
		const auto FXExpl = RE::TESForm::LookupByEditorID<RE::MagicItem>("VLSeranaTransformToVLExplosionSPELL");
		const auto caster = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		caster->CastSpellImmediate(FXchange, true, a_actor, 1, false, 0.0, a_actor);
		a_actor->SwitchRace(RE::TESForm::LookupByEditorID<RE::TESRace>("DLC1VampireBeastRace"), false);
		logger::info("Vampire lord form succesful");
		a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDLC1AbVampireFloatBodyFX"));
		caster->CastSpellImmediate(FXExpl, true, a_actor, 1, false, 0.0, a_actor);
		// a_actor->SetGraphVariableFloat("staggerDirection", 0.0);
		// a_actor->SetGraphVariableFloat("StaggerMagnitude", 1.0);
		// a_actor->NotifyAnimationGraph("staggerStart");
		VLDrain(a_actor);
		a_actor->SetGraphVariableBool("IUBusy", false);
	}



	void OnMeleeHitHook::VLS_RevertVampireLordform(STATIC_ARGS, RE::Actor* a_actor)
	{
		const auto race = a_actor->GetRace();
		const auto raceEDID = race->formEditorID;
		if (!(raceEDID == "DLC1VampireBeastRace")) {
			return;
		}
		a_actor->SetGraphVariableBool("IUBusy", true);
		logger::info("Reverting Form");
		auto data = RE::TESDataHandler::GetSingleton();
		util::playSound(a_actor, (data->LookupForm<RE::BGSSoundDescriptorForm>(0x10F564, "Skyrim.esm")));
		const auto FXchange = RE::TESForm::LookupByEditorID<RE::MagicItem>("VLSeranaTransformToNormal");
		const auto FXchange2 = RE::TESForm::LookupByEditorID<RE::MagicItem>("VLSeranaTransformToNormal2");
		const auto Gargoyle = RE::TESForm::LookupByEditorID<RE::MagicItem>("VLSeranaConjureGargoyle");
		const auto caster = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		caster->CastSpellImmediate(FXchange, true, a_actor, 1, false, 0.0, a_actor);
		a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("VLSeranaDLC1AbVampireFloatBodyFX"));
		auto curhealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
		if (curhealth <= 70.0f) {
			if (curhealth <= 5.0f) {
				if (a_actor->HasKeywordString("VLS_Serana_Key")) {
					bShouldBleedOut_Serana = true;
				} else {
					bShouldBleedOut_Valerica = true;
				}
			}
			a_actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, (70.f - curhealth));
		}
		auto vamp_armour = RE::TESForm::LookupByEditorID<RE::TESObjectARMO>("VLSeranaDLC1ClothesVampireLordRoyalArmor");
		RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, vamp_armour);
		remove_item(a_actor, vamp_armour, 1, true, nullptr);
		//a_actor->RemoveItem(a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kBody), 2, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		a_actor->SwitchRace(RE::TESForm::LookupByEditorID<RE::TESRace>("NordRace"), false);
		dispelEffect(FXchange, a_actor);
		caster->CastSpellImmediate(FXchange2, true, a_actor, 1, false, 0.0, a_actor);
		VLDrain(a_actor, true);
		dispelEffect(Gargoyle, a_actor);
		// a_actor->SetGraphVariableFloat("staggerDirection", 0.0);
		// a_actor->SetGraphVariableFloat("StaggerMagnitude", 1.0);
		// a_actor->NotifyAnimationGraph("staggerStart");
		a_actor->SetGraphVariableBool("IUBusy", false);
	}

	class OurEventSink : public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>, public RE::BSTEventSink<RE::TESEquipEvent>, public RE::BSTEventSink<RE::TESCombatEvent>, public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
	{
		OurEventSink() = default;
		OurEventSink(const OurEventSink&) = delete;
		OurEventSink(OurEventSink&&) = delete;
		OurEventSink& operator=(const OurEventSink&) = delete;
		OurEventSink& operator=(OurEventSink&&) = delete;

	public:
		static OurEventSink* GetSingleton()
		{
			static OurEventSink singleton;
			return &singleton;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSwitchRaceCompleteEvent* event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
		{
			auto a_actor = event->subject->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!(a_actor->HasKeywordString("VLS_Serana_Key") || a_actor->HasKeywordString("VLS_Valerica_Key"))) {
				return RE::BSEventNotifyControl::kContinue;
			}
			const auto race = a_actor->GetRace();
			const auto raceEDID = race->formEditorID;
			auto vamp_armour = RE::TESForm::LookupByEditorID<RE::TESObjectARMO>("VLSeranaDLC1ClothesVampireLordRoyalArmor");
			auto ElderScroll = RE::TESForm::LookupByEditorID<RE::TESAmmo>("DLC1ElderScrollBack");
			if (!(raceEDID == "DLC1VampireBeastRace")) {
				//Not vamp form//
				OnMeleeHitHook::Reset_iFrames(a_actor);
				if (&bElderScrollEquipped){
					bElderScrollEquipped = false;
					OnMeleeHitHook::EquipfromInvent(a_actor, ElderScroll->formID);
				}
				if (a_actor->HasKeywordString("VLS_Serana_Key")) {
					if (&bShouldBleedOut_Serana){
						bShouldBleedOut_Serana = false;
						auto curhealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
						a_actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -(curhealth + 1.0f));
					}
				} else {
					if (&bShouldBleedOut_Valerica){
						bShouldBleedOut_Valerica = false;
						auto curhealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
						a_actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -(curhealth + 1.0f));
					}
				}

			}else {//vamp form//
				OnMeleeHitHook::Reset_iFrames(a_actor);
				OnMeleeHitHook::UnequipAll(a_actor);
				a_actor->AddWornItem(vamp_armour, 1, false, 0, 0);
				RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, vamp_armour);
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*){
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!(a_actor->HasKeywordString("VLS_Serana_Key") || a_actor->HasKeywordString("VLS_Valerica_Key"))) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto race = a_actor->GetRace();
			const auto raceEDID = race->formEditorID;
			if (raceEDID == "DLC1VampireBeastRace") {
				auto ElderScroll = RE::TESForm::LookupByEditorID<RE::TESAmmo>("DLC1ElderScrollBack");
				auto item = event->originalRefr;
				if (item && item == ElderScroll->formID && event->equipped) {
					RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, ElderScroll);
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*){
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!(a_actor->HasKeywordString("VLS_Serana_Key") || a_actor->HasKeywordString("VLS_Valerica_Key"))) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto getcombatstate = event->newState.get();

			if (getcombatstate == RE::ACTOR_COMBAT_STATE::kNone) {
				const auto race = a_actor->GetRace();
				const auto raceEDID = race->formEditorID;
				if (raceEDID == "DLC1VampireBeastRace") {
					OnMeleeHitHook::VLS_RevertVampireLordform(nullptr, 0, nullptr, a_actor);
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*)
		{
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!(a_actor->HasKeywordString("VLS_Serana_Key") || a_actor->HasKeywordString("VLS_Valerica_Key"))) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto Playerhandle = RE::PlayerCharacter::GetSingleton();

			if (Playerhandle->IsSneaking() || event->newLoc && event->newLoc->HasKeyword(RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeHabitation"))){
				OnMeleeHitHook::VLS_RevertVampireLordform(nullptr, 0, nullptr, a_actor);
			}

			return RE::BSEventNotifyControl::kContinue;
		}
		
	};

	RE::BSEventNotifyControl animEventHandler::HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src)
	{
		FnProcessEvent fn = fnHash.at(*(uint64_t*)this);

		if (!a_event.holder) {
			return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
		}

		RE::Actor* actor = const_cast<RE::TESObjectREFR*>(a_event.holder)->As<RE::Actor>();
		switch (hash(a_event.tag.c_str(), a_event.tag.size())) {
		case "SoundPlay.NPCVampireLordTransformation012D"_h:
			 logger::info("Set Race Detected");
			 if (actor->HasKeywordString("VLS_Serana_Key") || actor->HasKeywordString("VLS_Valerica_Key")) {
			 	/*OnMeleeHitHook::VLS_CompleteTransformation(actor);*/
			 }
			break;
		}

		return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
	}

	std::unordered_map<uint64_t, animEventHandler::FnProcessEvent> animEventHandler::fnHash;

	void OnMeleeHitHook::install(){
		auto eventSink = OurEventSink::GetSingleton();

		// ScriptSource
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		eventSourceHolder->AddEventSink<RE::TESSwitchRaceCompleteEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESEquipEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESCombatEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESActorLocationChangeEvent>(eventSink);
		
	}

	bool OnMeleeHitHook::BindPapyrusFunctions(VM* vm)
	{
		vm->RegisterFunction("VLS_SendVampireLordTransformation", "VLS_NativeFunctions", VLS_SendVampireLordTransformation);
		vm->RegisterFunction("VLS_RevertVampireLordform", "VLS_NativeFunctions", VLS_RevertVampireLordform);
		return true;
	}

	void OnMeleeHitHook::OnMeleeHit(RE::Actor* hit_causer, RE::Actor* hit_target, std::int64_t a_int1, bool a_bool,
		void* a_unkptr)
	{
		const auto race = hit_causer->GetRace();
		const auto raceFormID = race->formEditorID;

		if (raceFormID == "WerewolfBeastRace" || raceFormID == "DLC2WerebearBeastRace") {
			auto magicTarget = hit_causer->AsMagicTarget();
			const auto magicEffect = RE::TESForm::LookupByEditorID("zxlice_cooldownEffect")->As<RE::EffectSetting>();
			magicTarget->HasMagicEffect(magicEffect);
    
			int  iState = 0;
			hit_causer->GetGraphVariableInt("iState", iState);
			switch (iState) {
			case 1:
                /* code */
                break;

			case 5:
				/* code */
				break;

			default:
                break;
			}
        }
		// Call the normal game's code
		_OnMeleeHit(hit_causer, hit_target, a_int1, a_bool, a_unkptr);
	}

}


#include "hook.h"
#include "ClibUtil/editorID.hpp"

namespace hooks
{
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

	void OnMeleeHitHook::OnMeleeHit(RE::Actor* hit_causer, RE::Actor* hit_target, std::int64_t a_int1, bool a_bool,
		void* a_unkptr)
	{
		const auto race = hit_causer->GetRace();
		const auto raceFormID = race->formEditorID;

		if (raceFormID == "WerewolfBeastRace" || raceFormID == "DLC2WerebearBeastRace") {
			auto magicTarget = hit_causer->AsMagicTarget();
			const auto magicEffect = RE::TESForm::LookupByEditorID("zxlice_cooldownEffect")->As<RE::EffectSetting>();
			magicTarget->HasMagicEffect(magicEffect);
            auto intvalue = 0;
            auto var = hit_causer->GetGraphVariableInt("XXX", intvalue);
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
			// if (iState == 10) {  // using staff
			// 	return;
			// 	//hit_causer->SetGraphVariableInt("iStep", 0);
			// }
        }

		// bool bMaxsuWeaponParry_InWeaponParry = false;

		// if ((hit_target)
		// 		->GetGraphVariableBool("bMaxsuWeaponParry_InWeaponParry", bMaxsuWeaponParry_InWeaponParry) &&
		// 	bMaxsuWeaponParry_InWeaponParry) {
		// 	return;
		// }
		// Call the normal game's code
		_OnMeleeHit(hit_causer, hit_target, a_int1, a_bool, a_unkptr);
	}

}

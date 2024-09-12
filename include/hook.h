#include "SKSE/Trampoline.h"
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#pragma warning(disable: 4100)


namespace hooks
{
	// static float& g_deltaTime = (*(float*)RELOCATION_ID(523660, 410199).address());
	using uniqueLocker = std::unique_lock<std::shared_mutex>;
	using sharedLocker = std::shared_lock<std::shared_mutex>;
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;
#define STATIC_ARGS [[maybe_unused]] VM *a_vm, [[maybe_unused]] StackID a_stackID, RE::StaticFunctionTag *
     
	void remove_item(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_item, std::uint32_t a_count, bool a_silent, RE::TESObjectREFR* a_otherContainer);

	using EventResult = RE::BSEventNotifyControl;

	class animEventHandler
	{
	private:
		template <class Ty>
		static Ty SafeWrite64Function(uintptr_t addr, Ty data)
		{
			DWORD oldProtect;
			void* _d[2];
			memcpy(_d, &data, sizeof(data));
			size_t len = sizeof(_d[0]);

			VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
			Ty olddata;
			memset(&olddata, 0, sizeof(Ty));
			memcpy(&olddata, (void*)addr, len);
			memcpy((void*)addr, &_d[0], len);
			VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
			return olddata;
		}

		typedef RE::BSEventNotifyControl (animEventHandler::*FnProcessEvent)(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* dispatcher);

		RE::BSEventNotifyControl HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src);

		static void HookSink(uintptr_t ptr)
		{
			FnProcessEvent fn = SafeWrite64Function(ptr + 0x8, &animEventHandler::HookedProcessEvent);
			fnHash.insert(std::pair<uint64_t, FnProcessEvent>(ptr, fn));
		}

	public:
		static animEventHandler* GetSingleton()
		{
			static animEventHandler singleton;
			return &singleton;
		}

		/*Hook anim event sink*/
		static void Register(bool player, bool NPC)
		{
			if (player) {
				logger::info("Sinking animation event hook for player");
				REL::Relocation<uintptr_t> pcPtr{ RE::VTABLE_PlayerCharacter[2] };
				HookSink(pcPtr.address());
			}
			if (NPC) {
				logger::info("Sinking animation event hook for NPC");
				REL::Relocation<uintptr_t> npcPtr{ RE::VTABLE_Character[2] };
				HookSink(npcPtr.address());
			}
			logger::info("Sinking complete.");
		}

		static void RegisterForPlayer()
		{
			Register(true, false);
		}

	protected:
		static std::unordered_map<uint64_t, FnProcessEvent> fnHash;
	};

	class OnMeleeHitHook
	{
	public:
		[[nodiscard]] static OnMeleeHitHook& GetSingleton() noexcept;

		static void InstallHook();
		static void install();
		// static void install_protected(){
		// 	Install_Update();
		// }
		static bool VLS_SendVampireLordTransformation(STATIC_ARGS, RE::Actor* a_actor);
		static bool VLS_RevertVampireLordform(STATIC_ARGS, RE::Actor* a_actor);
		void UnequipAll(RE::Actor* a_actor);
		void Unequip_DescendMode(RE::Actor* a_actor, RE::SpellItem* a_spell);
		std::pair<bool, RE::SpellItem*> GetAttackSpell(RE::Actor* actor, bool lefthand = false);
		void Re_EquipAll(RE::Actor *a_actor);
		void Re_EquipAll_LevitateMode(RE::Actor *a_actor);
		static bool BindPapyrusFunctions(VM* vm);
		static void Set_iFrames(RE::Actor* actor);
		static void Reset_iFrames(RE::Actor* actor);
		static void dispelEffect(RE::MagicItem *spellForm, RE::Actor *a_target);
		static void VLDrain(RE::Actor* a_actor, bool remove = false);
		static void EquipfromInvent(RE::Actor* a_actor, RE::FormID a_formID);
		static void VLS_CompleteTransformation(RE::Actor *a_actor);
		static void VLS_CompleteReversion(RE::Actor* a_actor);
		static void InterruptAttack(RE::Actor *a_actor);
		static void ResetAttack(RE::Actor *a_actor);
		static void ResetAttackMoving(RE::Actor* a_actor);
		static void ResetAttack_Melee(RE::Actor* a_actor);
		static void ResetAttackMoving_Melee(RE::Actor* a_actor);
		void PrepareForMelee(RE::Actor *a_actor);
		bool IsMoving(RE::Actor* actor);
		void Store_CStyleSettings(RE::Actor *a_actor);
		void Restore_CStyleSettings(RE::Actor *a_actor);
		// void update(RE::Actor* a_actor, float a_delta);
		// void startTiming(RE::Actor *a_actor, float a_delta);
	    // void finishTiming(RE::Actor *a_actor);

	private:
		OnMeleeHitHook() = default;
		OnMeleeHitHook(const OnMeleeHitHook&) = delete;
		OnMeleeHitHook(OnMeleeHitHook&&) = delete;
		~OnMeleeHitHook() = default;

		OnMeleeHitHook& operator=(const OnMeleeHitHook&) = delete;
		OnMeleeHitHook& operator=(OnMeleeHitHook&&) = delete;

		std::unordered_map<RE::Actor*, float> _parryTimer;
		std::unordered_map<RE::Actor*, std::pair<float, float>> _CStyledefault;
		std::unordered_map<RE::Actor*, std::vector<RE::TESBoundObject*>> _Inventory;
		std::unordered_map<RE::Actor*, std::vector<RE::SpellItem*>> _SpellList;
		std::shared_mutex mtx_Inventory;
		std::shared_mutex mtx_parryTimer;
		std::shared_mutex mtx_SpellList;
		std::shared_mutex mtx_CStyledefault;
		bool _bUpdate;

		/** Our new function */
		static void OnMeleeHit(RE::Actor* attacker, RE::Actor* target, std::int64_t a_int1, bool a_bool,
			void* a_unkptr);

		/** The original function */
		static inline REL::Relocation<decltype(OnMeleeHit)> _OnMeleeHit;

	protected:

		// struct Actor_Update
		// {
		// 	static void thunk(RE::Actor* a_actor, float a_delta)
		// 	{
		// 		func(a_actor, a_delta);
		// 		GetSingleton().update(a_actor, g_deltaTime);
		// 	}
		// 	static inline REL::Relocation<decltype(thunk)> func;
		// };

		// static void Install_Update(){
		// 	stl::write_vfunc<RE::Character, 0xAD, Actor_Update>();
		// }

	};


	class util
	{
	private:
		static int soundHelper_a(void *manager, RE::BSSoundHandle *a2, int a3, int a4) // sub_140BEEE70
		{
			using func_t = decltype(&soundHelper_a);
			REL::Relocation<func_t> func{RELOCATION_ID(66401, 67663)};
			return func(manager, a2, a3, a4);
		}

		static void soundHelper_b(RE::BSSoundHandle *a1, RE::NiAVObject *source_node) // sub_140BEDB10
		{
			using func_t = decltype(&soundHelper_b);
			REL::Relocation<func_t> func{RELOCATION_ID(66375, 67636)};
			return func(a1, source_node);
		}

		static char __fastcall soundHelper_c(RE::BSSoundHandle *a1) // sub_140BED530
		{
			using func_t = decltype(&soundHelper_c);
			REL::Relocation<func_t> func{RELOCATION_ID(66355, 67616)};
			return func(a1);
		}

		static char set_sound_position(RE::BSSoundHandle *a1, float x, float y, float z)
		{
			using func_t = decltype(&set_sound_position);
			REL::Relocation<func_t> func{RELOCATION_ID(66370, 67631)};
			return func(a1, x, y, z);
		}

		std::random_device rd;

	public:
		static void playSound(RE::Actor *a, RE::BGSSoundDescriptorForm *a_descriptor)
		{
			//logger::info("starting voicing....");

			RE::BSSoundHandle handle;
			handle.soundID = static_cast<uint32_t>(-1);
			handle.assumeSuccess = false;
			*(uint32_t *)&handle.state = 0;

			soundHelper_a(RE::BSAudioManager::GetSingleton(), &handle, a_descriptor->GetFormID(), 16);

			if (set_sound_position(&handle, a->data.location.x, a->data.location.y, a->data.location.z))
			{
				soundHelper_b(&handle, a->Get3D());
				soundHelper_c(&handle);
				//logger::info("FormID {}"sv, a_descriptor->GetFormID());
				//logger::info("voicing complete");
			}
		}

		static RE::BGSSoundDescriptor *GetSoundRecord(const char* description)
		{

			auto Ygr = RE::TESForm::LookupByEditorID<RE::BGSSoundDescriptor>(description);

			return Ygr;
		}

		static util *GetSingleton()
		{
			static util singleton;
			return &singleton;
		}

		float GenerateRandomFloat(float value_a, float value_b)
		{
			std::mt19937 generator(rd());
			std::uniform_real_distribution<float> dist(value_a, value_b);
			return dist(generator);
		}
	};
};

constexpr uint32_t hash(const char* data, size_t const size) noexcept
{
	uint32_t hash = 5381;

	for (const char* c = data; c < data + size; ++c) {
		hash = ((hash << 5) + hash) + (unsigned char)*c;
	}

	return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept
{
	return hash(str, size);
}

namespace FallLongDistance
{
	void Install();
}
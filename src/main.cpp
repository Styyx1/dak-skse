

#include "SKSEMenuFramework.h"
#include "sksemenu.h"
float g_keyCache   = 0.0;
bool g_cachedState = false;

namespace DAK
{

namespace DATA
{

inline constexpr auto TOML_P_D = "Data/SKSE/Plugins/dak.toml";
inline constexpr auto TOML_P_C = "Data/SKSE/Plugins/dak_custom.toml";
inline constexpr auto KEYS     = "Hotkeys";
inline constexpr auto DAK_KEY  = "sDakKey";
inline constexpr auto DEF_K    = "shift";

inline constexpr auto ESP_NAME = "Dynamic Activation Key.esp";

} // namespace DATA
namespace CONF
{
using namespace DATA;
inline REX::TOML::Str dak_key{KEYS, DAK_KEY, DEF_K};

void Update(bool a_save)
{
    auto t = REX::TSingleton<REX::FTomlSettingStore>::GetSingleton();
    t->Init(TOML_P_D, TOML_P_C);
    if (a_save)
        t->Save();
    else
        t->Load();
}
} // namespace CONF

namespace FORMS
{

inline RE::TESGlobal* dak_global{nullptr};
inline RE::TESGlobal* dak_keybind_global{nullptr};

void LoadGlobals()
{
    if (!StyyxUtil::MiscUtil::IsModLoaded(DATA::ESP_NAME))
    {
        REX::FAIL("{} is not loaded. Make sure to activate it!", DATA::ESP_NAME);
    }

    dak_global         = RE::TESForm::LookupByEditorID<RE::TESGlobal>("DynamicActivationKey");
    dak_keybind_global = RE::TESForm::LookupByEditorID<RE::TESGlobal>("DAK_Hotkey");

    if (!dak_global || !dak_keybind_global)
    {
        REX::FAIL("DAK globals are invalid. Try updating the esp or reinstalling it!");
    }
}
} // namespace FORMS

namespace KeyHelper
{

struct Hotkeys
{
    inline static void AdjustGlobalToKey()
    {
        FORMS::dak_keybind_global->value = clib_util::hotkeys::details::GetKeyByName(CONF::dak_key.GetValue());
    }
};
} // namespace KeyHelper

namespace GLOB
{
static void SetDAKKey(bool a_active)
{
    auto setv = a_active ? 1.0 : 0.0;

    if (g_keyCache != setv)
    {
        g_keyCache               = setv;
        FORMS::dak_global->value = setv;
    }
}

static void UpdateHUD()
{
    SKSE::GetTaskInterface()->AddUITask([]() { RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs(); });
}

} // namespace GLOB

namespace EVENT
{
using namespace clib_util::hotkeys;
using RES = RE::BSEventNotifyControl;

struct DAKInput : REX::TSingleton<DAKInput>, RE::BSTEventSink<RE::InputEvent*>
{
    static void Register()
    {
        if (const auto m = RE::BSInputDeviceManager::GetSingleton(); m)
        {
            m->AddEventSink(REX::TSingleton<DAKInput>::GetSingleton());
            REX::INFO("Registered for input");
        }
    }
    static void SetKeys()
    {

        if (!GetSingleton()->dak_key.SetPattern(CONF::dak_key.GetValue()))
        {
            REX::ERROR("Failed to set dak key");
        }
    }
    static void ProcessDAKKey(const KeyCombination* key)
    {

        ProcessDAK(key->IsTriggered() && key == &GetSingleton()->dak_key);
    }

    KeyCombination dak_key{ProcessDAKKey};

  private:
    static void ProcessDAK(bool active)
    {

        if (g_cachedState != active)
        {
            REX::INFO("Processing DAK Key");
            GLOB::SetDAKKey(active);
            g_cachedState = active;
            GLOB::UpdateHUD();
        }
    }
    static bool IsComboHeld(const KeyCombination& combo, RE::InputEvent* const* a_event)
    {
        const auto& keys = combo.GetKeys();
        if (!combo.IsValid() || keys.empty())
            return false;

        std::set<std::uint32_t> pressed;
        for (auto event = *a_event; event; event = event->next)
        {
            auto button = event->AsButtonEvent();
            if (!button || !button->HasIDCode())
                continue;

            auto key = button->GetIDCode();
            switch (button->GetDevice())
            {
                case RE::INPUT_DEVICE::kMouse:
                    key += SKSE::InputMap::kMacro_MouseButtonOffset;
                    break;
                case RE::INPUT_DEVICE::kGamepad:
                    key = SKSE::InputMap::GamepadMaskToKeycode(key);
                    break;
                default:
                    break;
            }

            if (button->IsPressed())
                pressed.insert(key);
        }

        // subset check instead of exact match
        return std::ranges::all_of(keys, [&](auto k) { return pressed.contains(k); });
    }

  protected:
    inline RES ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override
    {
        if (!a_event)
            return RES::kContinue;

        ProcessDAK(IsComboHeld(dak_key, a_event));
        return RES::kContinue;
    }
};
} // namespace EVENT

} // namespace DAK

namespace DAK::UI
{
void Reg()
{

    if (SKSEMenuFramework::GetMenuFrameworkVersion() < 3.7f)
    {
        return;
    }
    SKSEMenuFramework::SetSection("Dynamic Activation Key");
    SKSEMenuFramework::AddSectionItem("Dynamic Activation Key", &DAKMenu::Render);
}
void __stdcall DAKMenu::Render()
{
    ImGuiMCP::Text("test");
    DrawHotkey();
}
void DAKMenu::DrawHotkey()
{
    using namespace ImGuiMCP;
    Text("Select Key");
    static std::array<const char*, 12> keys{"shift",    "alt",       "ctrl",     "rshift",    "ralt",     "rctrl",
                                            "capslock", "lshoulder", "ltrigger", "rshoulder", "rtrigger", "gamepadb"};
    static int selected = 0;

    if (Combo("Hotkeys", &selected, keys.data(), static_cast<int>(keys.size())))
    {
        DAK::CONF::dak_key.SetValue(keys[selected]);
        DAK::CONF::Update(true);
        DAK::EVENT::DAKInput::SetKeys();
    }
}
} // namespace DAK::UI

void List(SKSE::MessagingInterface::Message* a_msg)
{
    switch (a_msg->type)
    {
        case SKSE::MessagingInterface::kInputLoaded:
            DAK::EVENT::DAKInput::Register();
            break;
        case SKSE::MessagingInterface::kDataLoaded:
            DAK::FORMS::LoadGlobals();
            DAK::EVENT::DAKInput::SetKeys();
            DAK::UI::Reg();
            break;
        default:
            break;
    }
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);

    DAK::CONF::Update(false);

    if (!SKSE::GetMessagingInterface()->RegisterListener(List))
    {
        return false;
    }

    return true;
}
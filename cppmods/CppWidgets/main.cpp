#include <cstdio>
#include <concepts>
#include <atomic>

#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UMG/Components/Widget.hpp>
#include <Unreal/PropertyMacros.hpp>
#include <Unreal/BPMacros.hpp>

using namespace RC;
using namespace RC::Unreal;

struct FAnchors
{
    FVector2D Minimum{};
    FVector2D Maximum{};
};

struct FLinearColor
{
    float R;
    float G;
    float B;
    float A;
};

struct FMargin
{
    float Left;
    float Top;
    float Right;
    float Bottom;
};

enum ESlateColorStylingMode
{
    UseColor_Specified = 0,
    UseColor_ColorTable = 1,
    UseColor_Foreground = 2,
    UseColor_Foreground_Subdued = 3,
    UseColor_UseStyle = 4,
    UseColor_MAX
};

struct FSlateColor
{
    ::FLinearColor SpecifiedColor;
    ESlateColorStylingMode ColorUseRule;
};

class UWidgetTree : public UObject
{
  public:
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(UWidget*, RootWidget)
};

class UUserWidget : public UWidget
{
  public:
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(UWidgetTree*, WidgetTree)

  public:
    void AddToViewport(int32 ZOrder = 0)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.UserWidget:AddToViewport")

        UE_COPY_PROPERTY(ZOrder, int32)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }
};

class UPanelSlot : public UVisual
{
};

class UCanvasPanelSlot : public UPanelSlot
{
  public:
    void SetAutoSize(bool InbAutoSize)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.CanvasPanelSlot:SetAutoSize")

        UE_COPY_PROPERTY(InbAutoSize, bool)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }

    void SetAnchors(FAnchors InAnchors)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.CanvasPanelSlot:SetAnchors")

        UE_COPY_STRUCT_PROPERTY_BEGIN(InAnchors)
        UE_COPY_STRUCT_INNER_PROPERTY(InAnchors, Minimum, FVector2D, InAnchors.Minimum)
        UE_COPY_STRUCT_INNER_PROPERTY(InAnchors, Maximum, FVector2D, InAnchors.Maximum)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }

    void SetAlignment(FVector2D InAlignment)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.CanvasPanelSlot:SetAlignment")

        UE_COPY_STRUCT_PROPERTY_BEGIN(InAlignment)
        UE_COPY_VECTOR2D(InAlignment, InAlignment)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }

    void SetPosition(FVector2D InPosition)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.CanvasPanelSlot:SetPosition")

        UE_COPY_STRUCT_PROPERTY_BEGIN(InPosition)
        UE_COPY_VECTOR2D(InPosition, InPosition)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }
};

class UPanelWidget : public UWidget
{
};

class UCanvasPanel : public UPanelWidget
{
  public:
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(UWidgetTree*, WidgetTree)

  public:
    UCanvasPanelSlot* AddChildToCanvas(UWidget* Content)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.CanvasPanel:AddChildToCanvas")

        UE_COPY_PROPERTY(Content, UWidget*)

        UE_CALL_FUNCTION()

        UE_RETURN_PROPERTY(UCanvasPanelSlot*)
    }
};

class UContentWidget : public UCanvasPanel
{
  public:
    UPanelSlot* SetContent(UWidget* Content)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.ContentWidget:SetContent")

        UE_COPY_PROPERTY(Content, UWidget*)

        UE_CALL_FUNCTION()

        UE_RETURN_PROPERTY(UPanelSlot*)
    }
};

class UBorder : public UContentWidget
{
  public:
    void SetBrushColor(::FLinearColor InBrushColor)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.Border:SetBrushColor")

        UE_COPY_STRUCT_PROPERTY_BEGIN(InBrushColor)
        UE_COPY_STRUCT_INNER_PROPERTY(InBrushColor, R, float, InBrushColor.R)
        UE_COPY_STRUCT_INNER_PROPERTY(InBrushColor, G, float, InBrushColor.G)
        UE_COPY_STRUCT_INNER_PROPERTY(InBrushColor, B, float, InBrushColor.B)
        UE_COPY_STRUCT_INNER_PROPERTY(InBrushColor, A, float, InBrushColor.A)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }

    void SetPadding(::FMargin InPadding)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.Border:SetPadding")

        UE_COPY_STRUCT_PROPERTY_BEGIN(InPadding)
        UE_COPY_STRUCT_INNER_PROPERTY(InPadding, Left, float, InPadding.Left)
        UE_COPY_STRUCT_INNER_PROPERTY(InPadding, Top, float, InPadding.Top)
        UE_COPY_STRUCT_INNER_PROPERTY(InPadding, Right, float, InPadding.Right)
        UE_COPY_STRUCT_INNER_PROPERTY(InPadding, Bottom, float, InPadding.Bottom)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }
};

class UTextLayoutWidget : public UWidget
{
};

class UTextBlock : public UTextLayoutWidget
{
  public:
    void SetText(const FText& InText)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.TextBlock:SetText")

        UE_COPY_PROPERTY(InText, FText)

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }

    void SetColorAndOpacity(const FSlateColor& InColorAndOpacity)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/UMG.TextBlock:SetColorAndOpacity")

        // TODO: Make BPMacros.hpp be able to handle nested structs.
        //UE_COPY_STRUCT_PROPERTY_BEGIN(InColorAndOpacity)
        //UE_COPY_STRUCT_INNER_PROPERTY(InColorAndOpacity, SpecifiedColor, float, InColorAndOpacity.SpecifiedColor)

        auto InColorAndOpacityProperty = Function->FindProperty(FName(L"InColorAndOpacity", FNAME_Find));
        if (!InColorAndOpacityProperty)
        {
            throw std::runtime_error{"Property not found: '""InColorAndOpacity""'"};
        }
        const auto SlateColor = static_cast<FStructProperty*>(InColorAndOpacityProperty)->GetStruct();

        // FLinearColor
        // clang-format off
        const auto SpecifiedColorProperty = SlateColor->FindProperty(FName(STR("SpecifiedColor"), FNAME_Find));
        if (!SpecifiedColorProperty)
        {
            throw std::runtime_error{"Property not found: '""SpecifiedColor""'"};
        }
        const auto SpecifiedColor = static_cast<FStructProperty*>(SpecifiedColorProperty)->GetStruct();
        const auto R = ParamData + InColorAndOpacityProperty->GetOffset_Internal() + SpecifiedColor->FindProperty(FName(STR("R"), FNAME_Find))->GetOffset_Internal();
        *std::bit_cast<float*>(R) = InColorAndOpacity.SpecifiedColor.R;
        const auto G = ParamData + InColorAndOpacityProperty->GetOffset_Internal() + SpecifiedColor->FindProperty(FName(STR("G"), FNAME_Find))->GetOffset_Internal();
        *std::bit_cast<float*>(G) = InColorAndOpacity.SpecifiedColor.G;
        const auto B = ParamData + InColorAndOpacityProperty->GetOffset_Internal() + SpecifiedColor->FindProperty(FName(STR("B"), FNAME_Find))->GetOffset_Internal();
        *std::bit_cast<float*>(B) = InColorAndOpacity.SpecifiedColor.B;
        const auto A = ParamData + InColorAndOpacityProperty->GetOffset_Internal() + SpecifiedColor->FindProperty(FName(STR("A"), FNAME_Find))->GetOffset_Internal();
        *std::bit_cast<float*>(A) = InColorAndOpacity.SpecifiedColor.A;
        // clang-format on

        // ESlateColorStylingMode
        const auto ColorUseRuleProperty = SlateColor->FindProperty(FName(STR("ColorUseRule"), FNAME_Find));
        if (!ColorUseRuleProperty)
        {
            throw std::runtime_error{"Property not found: '""SlateColorStylingMode""'"};
        }
        const auto Value = ParamData + InColorAndOpacityProperty->GetOffset_Internal() + ColorUseRuleProperty->GetOffset_Internal();
        *std::bit_cast<ESlateColorStylingMode*>(Value) = InColorAndOpacity.ColorUseRule;

        UE_CALL_FUNCTION()

        UE_END_VOID_FUNCTION()
    }
};

class UCharacterMovementComponent : public AActor
{
public:
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(float, JumpZVelocity)
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(FVector, Acceleration)
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(FVector, LastUpdateVelocity)
};

class APawn : public AActor
{
public:
    bool IsPlayerControlled()
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/Engine.Pawn:IsPlayerControlled")

        UE_CALL_FUNCTION()

        UE_RETURN_PROPERTY(bool)
    }
};

class ACharacter : public APawn
{
public:
    RC_INTERNAL_DEFINE_BP_PROPERTY_PRIVATE(UCharacterMovementComponent*, CharacterMovement)
};

bool IsReadyForWidgetConstruction(class CppWidgetsMod*);
bool CreateTextWidgetMiddleScreen(class CppWidgetsMod* mod);

CppWidgetsMod* GCppWidgetsMod{};

struct FWidgetDeleteListener : public FUObjectDeleteListener
{
    void NotifyUObjectDeleted(const UObjectBase* Object, [[maybe_unused]] int32_t Index) override;

    void OnUObjectArrayShutdown() override
    {
        UObjectArray::RemoveUObjectDeleteListener(this);
    }
};

class CppWidgetsMod : public RC::CppUserModBase
{
public:
    CppWidgetsMod() : CppUserModBase()
    {
        ModName = STR("CppWidgetsMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is my awesome mod");
        ModAuthors = STR("UE4SS Team");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        // ModIntendedSDKVersion = STR("2.6");

        Output::send(STR("CppWidgetsMod says hello!!!\n"));
        GCppWidgetsMod = this;
    }

    auto on_unreal_init() -> void override
    {
        UObjectArray::AddUObjectDeleteListener(&WidgetDeleteListener);
        Hook::RegisterEngineTickPreCallback([this](Hook::TCallbackIterationData<void>&, UEngine* Context, float Delta, bool) -> void {
            if (bPendingWidgetCreation.exchange(false))
            {
                Output::send(STR("Creating widgets!\n"));
                if (!IsReadyForWidgetConstruction(this) || !CreateTextWidgetMiddleScreen(this))
                {
                    // Signal we want to try again next frame.
                    bPendingWidgetCreation.store(true);
                    return;
                }
            }
            if (Text)
            {
                std::vector<UObject*> Characters{};
                UObjectGlobals::FindAllOf(STR("Character"), Characters);
                ACharacter* Character{};
                for (const auto& PotentialCharacter : Characters)
                {
                    if (PotentialCharacter && static_cast<ACharacter*>(PotentialCharacter)->IsPlayerControlled())
                    {
                        Character = static_cast<ACharacter*>(PotentialCharacter);
                        break;
                    }
                }
                if (!Character)
                {
                    //Output::send(STR("Player controlled character not found\n"));
                    return;
                }
                //Output::send(STR("Character: '{}'\n"), Character->GetFullName());
                const auto CharacterMovementComponent = Character->RC_INTERNAL_BP_GetCharacterMovement();
                if (!CharacterMovementComponent)
                {
                    Output::send(STR("Player controlled character has no movement component\n"));
                    return;
                }
                const auto LastUpdateVelocity = CharacterMovementComponent->RC_INTERNAL_BP_GetLastUpdateVelocity();
                Text->SetText(FText(fmt::format(STR("{:07.2f}\n{:07.2f}\n{:07.2f}"), LastUpdateVelocity.X(), LastUpdateVelocity.Y(), LastUpdateVelocity.Z())));
            }
        }, { false, false, STR("CppWidgets"), STR("WidgetCreationHook") });
        bPendingWidgetCreation.store(true);
    }

    auto on_update() -> void override
    {
    }

public:
    std::atomic_bool bPendingWidgetCreation{};
    UClass* UserWidgetClass{};
    UClass* WidgetTreeClass{};
    UClass* CanvasPanelClass{};
    UClass* BorderClass{};
    UClass* TextBlockClass{};
    UClass* CanvasPanelSlotClass{};
    UTextBlock* Text{};
    UUserWidget* HUD{};
    FWidgetDeleteListener WidgetDeleteListener{};
};

bool IsReadyForWidgetConstruction(CppWidgetsMod* mod)
{
    // Note: Executes every frame until it succeeds, and then it executes after the widget has been destructed, and it's signaled to be constructed again.

    mod->UserWidgetClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/UMG.UserWidget"));
    mod->WidgetTreeClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/UMG.WidgetTree"));
    mod->CanvasPanelClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/UMG.CanvasPanel"));
    mod->BorderClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/UMG.Border"));
    mod->TextBlockClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/UMG.TextBlock"));
    mod->CanvasPanelSlotClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/UMG.CanvasPanelSlot"));

    if (!mod->UserWidgetClass ||
        !mod->WidgetTreeClass ||
        !mod->CanvasPanelClass ||
        !mod->BorderClass ||
        !mod->TextBlockClass ||
        !mod->CanvasPanelSlotClass)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool CreateTextWidgetMiddleScreen(CppWidgetsMod* mod)
{
    const auto GameInstance = UObjectGlobals::FindFirstOf(STR("GameInstance"));

    // Hierarchy: UserWidget -> WidgetTree -> CanvasPanel -> Border -> TextBlock

    FStaticConstructObjectParameters HUDParams{mod->UserWidgetClass, GameInstance};
    HUDParams.Name = FName(STR("TestHUD"));
    const auto HUD = static_cast<UUserWidget*>(UObjectGlobals::StaticConstructObject(HUDParams));
    if (!HUD)
    {
        Output::send<LogLevel::Error>(STR("[CppWidgets] Unable to construct HUD\n"));
        return false;
    }

    FStaticConstructObjectParameters TreeParams{mod->WidgetTreeClass, HUD};
    TreeParams.Name = FName(STR("TestTree"));
    const auto Tree = UObjectGlobals::StaticConstructObject(TreeParams);
    if (!Tree)
    {
        Output::send<LogLevel::Error>(STR("[CppWidgets] Unable to construct Tree\n"));
        return false;
    }
    if (const auto value_ptr = HUD->GetValuePtrByPropertyNameInChain<UObject*>(STR("WidgetTree")); value_ptr)
    {
        *value_ptr = Tree;
    }

    FStaticConstructObjectParameters CanvasParams{mod->CanvasPanelClass, Tree};
    CanvasParams.Name = FName(STR("TestCanvas"));
    const auto Canvas = static_cast<UCanvasPanel*>(UObjectGlobals::StaticConstructObject(CanvasParams));
    if (!Canvas)
    {
        Output::send<LogLevel::Error>(STR("[CppWidgets] Unable to construct Canvas\n"));
        return false;
    }
    if (const auto value_ptr = Tree->GetValuePtrByPropertyNameInChain<UObject*>(STR("RootWidget")); value_ptr)
    {
        *value_ptr = Canvas;
    }

    FStaticConstructObjectParameters BorderParams{mod->BorderClass, Tree};
    BorderParams.Name = FName(STR("TestBorder"));
    const auto Border = static_cast<UBorder*>(UObjectGlobals::StaticConstructObject(BorderParams));
    if (!Border)
    {
        Output::send<LogLevel::Error>(STR("[CppWidgets] Unable to construct Border\n"));
        return false;
    }
    Border->SetBrushColor({0.f, 0.f, 0.f, 0.5f});
    Border->SetPadding({12.f, 6.f, 12.f, 6.f});

    FStaticConstructObjectParameters TextParams{mod->TextBlockClass, Tree};
    TextParams.Name = FName(STR("TestText"));
    const auto Text = static_cast<UTextBlock*>(UObjectGlobals::StaticConstructObject(TextParams));
    if (!Text)
    {
        Output::send<LogLevel::Error>(STR("[CppWidgets] Unable to construct TextBlock\n"));
        return false;
    }
    Text->SetText(FText(STR("Test")));
    Text->SetColorAndOpacity({1.f, 1.0f, 1.0f, 1.0f});
    Border->SetContent(Text);

    const auto Slot = Canvas->AddChildToCanvas(Border);
    if (!Slot)
    {
        Output::send<LogLevel::Error>(STR("[CppWidgets] Unable to add '{}' to canvas: '{}'\n"), Border->GetFullName(), Canvas->GetFullName());
        return false;
    }

    Slot->SetAutoSize(true);
    Slot->SetAnchors({{0.5f, 0.5f}, {0.5f, 0.5f}});
    Slot->SetAlignment({1.f, 0.f});
    Slot->SetPosition({-10.f, 10.f});

    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    HUD->AddToViewport(99);
    mod->Text = Text;
    mod->HUD = HUD;

    return true;
}
void FWidgetDeleteListener::NotifyUObjectDeleted(const UObjectBase* Object, int32_t Index)
{
    if (Object == GCppWidgetsMod->HUD)
    {
        Output::send(STR("Widget deleted\n"));
        GCppWidgetsMod->Text = nullptr;
        GCppWidgetsMod->bPendingWidgetCreation.store(true);
    }
}

#define MY_AWESOME_MOD_API __declspec(dllexport)
extern "C"
{
    MY_AWESOME_MOD_API RC::CppUserModBase* start_mod()
    {
        return new CppWidgetsMod();
    }

    MY_AWESOME_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}

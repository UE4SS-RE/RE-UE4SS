#include <Mod/CppUserModBase.hpp>
#include "sigscan.h"
#include "arcsys.h"
#include "math_util.h"
#include <array>
#include <iostream>
#include <Unreal/World.hpp>
#include <UnrealDef.hpp>
#include <DynamicOutput/Output.hpp>
#include <MinHook.h>

#define WIN32_LEAN_AND_MEAN
#include <imgui.h>
#include <LuaType/LuaFString.hpp>

#include "Windows.h"

#define STRIVEHITBOXES_API __declspec(dllexport)

struct FLinearColor {
    float R, G, B, A;

    FLinearColor() : R(0), G(0), B(0), A(0) {}
    FLinearColor(float R, float G, float B, float A) : R(R), G(G), B(B), A(A) {}
};

struct FCanvasUVTri {
	FVector2D V0_Pos;
	FVector2D V0_UV;
	FLinearColor V0_Color;
	FVector2D V1_Pos;
	FVector2D V1_UV;
	FLinearColor V1_Color;
	FVector2D V2_Pos;
	FVector2D V2_UV;
	FLinearColor V2_Color;
};

class FCanvas {
public:
	enum ECanvasDrawMode {
		CDM_DeferDrawing,
		CDM_ImmediateDrawing
	};

	FIELD(0xA0, ECanvasDrawMode, DrawMode);

	void Flush_GameThread(bool bForce = false);
};

class UTexture;

class UCanvas : public RC::Unreal::UObject {
public:
	void K2_DrawLine(FVector2D ScreenPositionA, FVector2D ScreenPositionB, float Thickness, FLinearColor RenderColor);
	RC::Unreal::FVector K2_Project(const RC::Unreal::FVector WorldPosition);

	void K2_DrawTriangle(UTexture* RenderTexture, RC::Unreal::TArray<FCanvasUVTri>* Triangles);

	FIELD(0x260, FCanvas*, Canvas);
};

using UCanvas_K2_DrawLine_t = void(*)(UCanvas*, FVector2D, FVector2D, float, const FLinearColor&);
const auto UCanvas_K2_DrawLine = (UCanvas_K2_DrawLine_t)(
	sigscan::get().scan("\x0F\x2F\xC8\x0F\x86\x94", "xxxxxx") - 0x51);

using UCanvas_K2_Project_t = void(*)(const UCanvas*, RC::Unreal::FVector*, const RC::Unreal::FVector&);
const auto UCanvas_K2_Project = (UCanvas_K2_Project_t)(
	sigscan::get().scan("\x48\x8B\x89\x68\x02\x00\x00\x48\x8B\xDA", "xxxxxxxxxx") - 0x12);

using UCanvas_K2_DrawTriangle_t = void(*)(UCanvas*, UTexture*, RC::Unreal::TArray<FCanvasUVTri>*);
const auto UCanvas_K2_DrawTriangle = (UCanvas_K2_DrawTriangle_t)(
	sigscan::get().scan("\x48\x81\xEC\x90\x00\x00\x00\x41\x83\x78\x08\x00", "xxxxxxxxxxxx") - 6);

void UCanvas::K2_DrawLine(FVector2D ScreenPositionA, FVector2D ScreenPositionB, float Thickness, FLinearColor RenderColor)
{
	UCanvas_K2_DrawLine(this, ScreenPositionA, ScreenPositionB, Thickness, RenderColor);
}

RC::Unreal::FVector UCanvas::K2_Project(const RC::Unreal::FVector WorldPosition)
{
	RC::Unreal::FVector out;
	UCanvas_K2_Project(this, &out, WorldPosition);
	return out;
}

void UCanvas::K2_DrawTriangle(UTexture* RenderTexture, RC::Unreal::TArray<FCanvasUVTri>* Triangles)
{
	UCanvas_K2_DrawTriangle(this, RenderTexture, Triangles);
}

class AHUD : public RC::Unreal::UObject {
public:
	FIELD(0x278, UCanvas*, Canvas);
};

constexpr auto AHUD_PostRender_index = 214;

// Actually AREDHUD_Battle
const auto** AHUD_vtable = (const void**)get_rip_relative(sigscan::get().scan(
	"\x48\x8D\x05\x00\x00\x00\x00\xC6\x83\x18\x03", "xxx????xxxx") + 3);

using AHUD_PostRender_t = void(*)(AHUD*);
AHUD_PostRender_t orig_AHUD_PostRender;

class FTexture** GWhiteTexture = (FTexture**)get_rip_relative((uintptr_t)UCanvas_K2_DrawTriangle + 0x3A);;

enum ESimpleElementBlendMode
{
	SE_BLEND_Opaque = 0,
	SE_BLEND_Masked,
	SE_BLEND_Translucent,
	SE_BLEND_Additive,
	SE_BLEND_Modulate,
	SE_BLEND_MaskedDistanceField,
	SE_BLEND_MaskedDistanceFieldShadowed,
	SE_BLEND_TranslucentDistanceField,
	SE_BLEND_TranslucentDistanceFieldShadowed,
	SE_BLEND_AlphaComposite,
	SE_BLEND_AlphaHoldout,
	// Like SE_BLEND_Translucent, but modifies destination alpha
	SE_BLEND_AlphaBlend,
	// Like SE_BLEND_Translucent, but reads from an alpha-only texture
	SE_BLEND_TranslucentAlphaOnly,
	SE_BLEND_TranslucentAlphaOnlyWriteAlpha,

	SE_BLEND_RGBA_MASK_START,
	SE_BLEND_RGBA_MASK_END = SE_BLEND_RGBA_MASK_START + 31, //Using 5bit bit-field for red, green, blue, alpha and desaturation

	SE_BLEND_MAX
};

class FCanvasItem {
protected:
	FCanvasItem() = default;

public:

	// Virtual function wrapper
	void Draw(class FCanvas* InCanvas)
	{
		using Draw_t = void(*)(FCanvasItem*, class FCanvas*);
		((Draw_t)(*(void***)this)[3])(this, InCanvas);
	}

	FIELD(0x14, ESimpleElementBlendMode, BlendMode);
};

class FCanvasTriangleItem : public FCanvasItem {
public:
	FCanvasTriangleItem(
		const FVector2D& InPointA,
		const FVector2D& InPointB,
		const FVector2D& InPointC,
		const FTexture* InTexture);

	~FCanvasTriangleItem()
	{
		TriangleList.~TArray<FCanvasUVTri>();
	}

	FIELD(0x50, RC::Unreal::TArray<FCanvasUVTri>, TriangleList);

private:
	char pad[0x60];
};

static_assert(sizeof(FCanvasTriangleItem) == 0x60);

using FCanvasTriangleItem_ctor_t = void(*)(
	FCanvasTriangleItem*,
	const FVector2D&,
	const FVector2D&,
	const FVector2D&,
	const FTexture*);
const auto FCanvasTriangleItem_ctor = (FCanvasTriangleItem_ctor_t)(
	sigscan::get().scan("\x48\x89\x43\x38\x48\x89\x4B\x50", "xxxxxxxx") - 0x5D);

FCanvasTriangleItem::FCanvasTriangleItem(
	const FVector2D& InPointA,
	const FVector2D& InPointB,
	const FVector2D& InPointC,
	const FTexture* InTexture)
{
	FCanvasTriangleItem_ctor(this, InPointA, InPointB, InPointC, InTexture);
}

struct drawn_hitbox {
	hitbox::box_type type;

	// Unclipped corners of filled box
	std::array<FVector2D, 4> corners;

	// Boxes to fill, clipped against other boxes
	std::vector<std::array<FVector2D, 4>> fill;

	// Outlines
	std::vector<std::array<FVector2D, 2>> lines;

	drawn_hitbox(const hitbox& box) :
		type(box.type),
		corners{
			FVector2D(box.x, box.y),
			FVector2D(box.x + box.w, box.y),
			FVector2D(box.x + box.w, box.y + box.h),
			FVector2D(box.x, box.y + box.h) }
	{
		for (auto i = 0; i < 4; i++)
			lines.push_back(std::array{ corners[i], corners[(i + 1) % 4] });

		fill.push_back(corners);
	}

	// Clip outlines against another hitbox
	void clip_lines(const drawn_hitbox& other)
	{
		auto old_lines = std::move(lines);
		lines.clear();

		for (auto& line : old_lines) {
			float entry_fraction, exit_fraction;
			auto intersected = line_box_intersection(
				other.corners[0], other.corners[2],
				line[0], line[1],
				&entry_fraction, &exit_fraction);

			if (!intersected) {
				lines.push_back(line);
				continue;
			}

			const auto delta = line[1] - line[0];

			if (entry_fraction != 0.f)
				lines.push_back(std::array{ line[0], line[0] + delta * entry_fraction });

			if (exit_fraction != 1.f)
				lines.push_back(std::array{ line[0] + delta * exit_fraction, line[1] });
		}
	}

	// Clip filled rectangle against another hitbox
	void clip_fill(const drawn_hitbox& other)
	{
		auto old_fill = std::move(fill);
		fill.clear();

		for (const auto& box : old_fill) {
			const auto& box_min = box[0];
			const auto& box_max = box[2];

			const auto clip_min = FVector2D(
				max_(box_min.X, other.corners[0].X),
				max_(box_min.Y, other.corners[0].Y));

			const auto clip_max = FVector2D(
				min_(box_max.X, other.corners[2].X),
				min_(box_max.Y, other.corners[2].Y));

			if (clip_min.X > clip_max.X || clip_min.Y > clip_max.Y) {
				// No intersection
				fill.push_back(box);
				continue;
			}

			if (clip_min.X > box_min.X) {
				// Left box
				fill.push_back(std::array{
					FVector2D(box_min.X, box_min.Y),
					FVector2D(clip_min.X, box_min.Y),
					FVector2D(clip_min.X, box_max.Y),
					FVector2D(box_min.X, box_max.Y) });
			}

			if (clip_max.X < box_max.X) {
				// Right box
				fill.push_back(std::array{
					FVector2D(clip_max.X, box_min.Y),
					FVector2D(box_max.X, box_min.Y),
					FVector2D(box_max.X, box_max.Y),
					FVector2D(clip_max.X, box_max.Y) });
			}

			if (clip_min.Y > box_min.Y) {
				// Top box
				fill.push_back(std::array{
					FVector2D(clip_min.X, box_min.Y),
					FVector2D(clip_max.X, box_min.Y),
					FVector2D(clip_max.X, clip_min.Y),
					FVector2D(clip_min.X, clip_min.Y) });
			}

			if (clip_max.Y < box_max.Y) {
				// Bottom box
				fill.push_back(std::array{
					FVector2D(clip_min.X, clip_max.Y),
					FVector2D(clip_max.X, clip_max.Y),
					FVector2D(clip_max.X, box_max.Y),
					FVector2D(clip_min.X, box_max.Y) });
			}
		}
	}
};

void asw_coords_to_screen(UCanvas* canvas, FVector2D* pos)
{
	pos->X *= asw_engine::COORD_SCALE / 1000.F;
	pos->Y *= asw_engine::COORD_SCALE / 1000.F;

	RC::Unreal::FVector pos3d(pos->X, 0.f, pos->Y);
	asw_scene::get()->camera_transform(&pos3d, nullptr);

	const auto proj = canvas->K2_Project(pos3d);
	*pos = FVector2D(static_cast<float>(proj.X()), static_cast<float>(proj.Y()));
}

// Corners must be in CW or CCW order
void fill_rect(
	UCanvas* canvas,
	const std::array<FVector2D, 4>& corners,
	const FLinearColor& color)
{
	FCanvasUVTri triangles[2];
	triangles[0].V0_Color = triangles[0].V1_Color = triangles[0].V2_Color = color;
	triangles[1].V0_Color = triangles[1].V1_Color = triangles[1].V2_Color = color;

	triangles[0].V0_Pos = corners[0];
	triangles[0].V1_Pos = corners[1];
	triangles[0].V2_Pos = corners[2];

	triangles[1].V0_Pos = corners[2];
	triangles[1].V1_Pos = corners[3];
	triangles[1].V2_Pos = corners[0];

	FCanvasTriangleItem item(
		FVector2D(0.f, 0.f),
		FVector2D(0.f, 0.f),
		FVector2D(0.f, 0.f),
		*GWhiteTexture);

	RC::Unreal::TArray<FCanvasUVTri> List;
	for (auto Triangle : triangles)
		List.Add(Triangle);

	item.TriangleList = List;
	item.BlendMode = SE_BLEND_Translucent;
	item.Draw(canvas->Canvas);
}

// Corners must be in CW or CCW order
void draw_rect(
	UCanvas* canvas,
	const std::array<FVector2D, 4>& corners,
	const FLinearColor& color)
{
	fill_rect(canvas, corners, color);

	for (auto i = 0; i < 4; i++)
		canvas->K2_DrawLine(corners[i], corners[(i + 1) % 4], 2.F, color);
}

// Transform entity local space to screen space
void transform_hitbox_point(UCanvas* canvas, const asw_entity* entity, FVector2D* pos, bool is_throw)
{
	if (!is_throw) {
		pos->X *= -entity->scale_x;
		pos->Y *= -entity->scale_y;

		*pos = pos->Rotate((float)entity->angle_x * (float)M_PI / 180000.f);

		if (entity->facing == direction::left)
			pos->X *= -1.f;
	}
	else if (entity->opponent != nullptr) {
		// Throws hit on either side, so show it directed towards opponent
		if (entity->get_pos_x() > entity->opponent->get_pos_x())
			pos->X *= -1.f;
	}

	pos->X += entity->get_pos_x();
	pos->Y += entity->get_pos_y();

	asw_coords_to_screen(canvas, pos);
}

void draw_hitbox(UCanvas* canvas, const asw_entity* entity, const drawn_hitbox& box)
{
	FLinearColor color;
	if (box.type == hitbox::box_type::hit)
		color = FLinearColor(1.f, 0.f, 0.f, .25f);
	else if (box.type == hitbox::box_type::grab)
		color = FLinearColor(1.f, 0.f, 1.f, .25f);
	else if (entity->counterhit)
		color = FLinearColor(0.f, 1.f, 1.f, .25f);
	else
		color = FLinearColor(0.f, 1.f, 0.f, .25f);

	const auto is_throw = box.type == hitbox::box_type::grab;

	for (auto fill : box.fill) {
		for (auto& pos : fill)
			transform_hitbox_point(canvas, entity, &pos, is_throw);

		fill_rect(canvas, fill, color);
	}

	for (const auto& line : box.lines) {
		auto start = line[0];
		auto end = line[1];
		transform_hitbox_point(canvas, entity, &start, is_throw);
		transform_hitbox_point(canvas, entity, &end, is_throw);
		canvas->K2_DrawLine(start, end, 2.F, color);
	}
}

hitbox calc_throw_box(const asw_entity* entity)
{
	// Create a fake hitbox for throws to be displayed
	hitbox box;
	box.type = hitbox::box_type::grab;

	const auto pushbox_front = entity->pushbox_width() / 2 + entity->pushbox_front_offset;
	box.x = 0.f;
	box.w = (float)(pushbox_front + entity->throw_range);

	if (entity->throw_box_top <= entity->throw_box_bottom) {
		// No throw height, use pushbox height for display
		box.y = 0.f;
		box.h = (float)entity->pushbox_height();
		return box;
	}

	box.y = (float)entity->throw_box_bottom;
	box.h = (float)(entity->throw_box_top - entity->throw_box_bottom);
	return box;
}

void draw_hitboxes(UCanvas* canvas, const asw_entity* entity, bool active)
{
	const auto count = entity->hitbox_count + entity->hurtbox_count;

	std::vector<drawn_hitbox> hitboxes;

	// Collect hitbox info
	for (auto i = 0; i < count; i++) {
		const auto& box = entity->hitboxes[i];

		// Don't show inactive hitboxes
		if (box.type == hitbox::box_type::hit && !active)
			continue;
		else if (box.type == hitbox::box_type::hurt && entity->is_strike_invuln())
			continue;

		hitboxes.push_back(drawn_hitbox(box));
	}

	// Add throw hitbox if in use
	if (entity->throw_range >= 0 && active)
		hitboxes.push_back(calc_throw_box(entity));

	for (auto i = 0; i < hitboxes.size(); i++) {
		// Clip outlines
		for (auto j = 0; j < hitboxes.size(); j++) {
			if (i != j && hitboxes[i].type == hitboxes[j].type)
				hitboxes[i].clip_lines(hitboxes[j]);
		}

		// Clip fill against every hitbox after, since two boxes
		// shouldn't both be clipped against each other
		for (auto j = i + 1; j < hitboxes.size(); j++) {
			if (hitboxes[i].type == hitboxes[j].type)
				hitboxes[i].clip_fill(hitboxes[j]);
		}

		draw_hitbox(canvas, entity, hitboxes[i]);
	}
}

void draw_pushbox(UCanvas* canvas, const asw_entity* entity)
{
	int left, top, right, bottom;
	entity->get_pushbox(&left, &top, &right, &bottom);

	std::array corners = {
		FVector2D(left, top),
		FVector2D(right, top),
		FVector2D(right, bottom),
		FVector2D(left, bottom)
	};

	for (auto& pos : corners)
		asw_coords_to_screen(canvas, &pos);

	// Show hollow pushbox when throw invuln
	FLinearColor color;
	if (entity->is_throw_invuln())
		color = FLinearColor(1.f, 1.f, 0.f, 0.f);
	else
		color = FLinearColor(1.f, 1.f, 0.f, .2f);

	draw_rect(canvas, corners, color);
}

class UREDGameCommon : public RC::Unreal::UObject {};
UREDGameCommon* GameCommon;

typedef int(*GetGameMode_Func)(UREDGameCommon*);
GetGameMode_Func GetGameMode;

enum GAME_MODE : int32_t
{
    GAME_MODE_DEBUG_BATTLE = 0x0,
    GAME_MODE_ADVERTISE = 0x1,
    GAME_MODE_MAINTENANCEVS = 0x2,
    GAME_MODE_ARCADE = 0x3,
    GAME_MODE_MOM = 0x4,
    GAME_MODE_SPARRING = 0x5,
    GAME_MODE_VERSUS = 0x6,
    GAME_MODE_VERSUS_PREINSTALL = 0x7,
    GAME_MODE_TRAINING = 0x8,
    GAME_MODE_TOURNAMENT = 0x9,
    GAME_MODE_RANNYU_VERSUS = 0xA,
    GAME_MODE_EVENT = 0xB,
    GAME_MODE_SURVIVAL = 0xC,
    GAME_MODE_STORY = 0xD,
    GAME_MODE_MAINMENU = 0xE,
    GAME_MODE_TUTORIAL = 0xF,
    GAME_MODE_LOBBYTUTORIAL = 0x10,
    GAME_MODE_CHALLENGE = 0x11,
    GAME_MODE_KENTEI = 0x12,
    GAME_MODE_MISSION = 0x13,
    GAME_MODE_GALLERY = 0x14,
    GAME_MODE_LIBRARY = 0x15,
    GAME_MODE_NETWORK = 0x16,
    GAME_MODE_REPLAY = 0x17,
    GAME_MODE_LOBBYSUB = 0x18,
    GAME_MODE_MAINMENU_QUICK_BATTLE = 0x19,
    GAME_MODE_UNDECIDED = 0x1A,
    GAME_MODE_INVALID = 0x1B,
};

void draw_display(UCanvas* canvas)
{
	const auto* engine = asw_engine::get();
	if (engine == nullptr)
		return;
    if (!GameCommon)
    {
        GameCommon = static_cast<UREDGameCommon*>(RC::UObjectGlobals::FindFirstOf(L"REDGameCommon"));
        return;
    }
    if (GetGameMode(GameCommon) != GAME_MODE_TRAINING) return;

	if (canvas->Canvas == nullptr)
		return;

	// Loop through entities backwards because the player that most
	// recently landed a hit is at index 0
	for (auto entidx = engine->entity_count - 1; entidx >= 0; entidx--) {
		const auto* entity = engine->entities[entidx];

		if (entity->is_pushbox_active())
			draw_pushbox(canvas, entity);

		const auto active = entity->is_active();
		draw_hitboxes(canvas, entity, active);

		const auto* attached = entity->attached;
		while (attached != nullptr) {
			draw_hitboxes(canvas, attached, active);
			attached = attached->attached;
		}
	}
}

struct DrawHudParams
{
    DrawHudParams()
    {
        Text = nullptr;
        TextColor = FLinearColor();
        ScreenX = 0;
        ScreenY = 0;
        Font = nullptr;
        Scale = 1;
        bScalePosition = false;
    }
    RC::FString* Text;
    FLinearColor TextColor;
    float ScreenX;
    float ScreenY;
    char * Font;
    float Scale;
    bool bScalePosition;
};

void hook_AHUD_PostRender(AHUD* hud)
{
	draw_display(hud->Canvas);
	orig_AHUD_PostRender(hud);
}

const void* vtable_hook(const void** vtable, const int index, const void* hook)
{
	DWORD old_protect;
	VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &old_protect);
	const auto* orig = vtable[index];
	vtable[index] = hook;
	VirtualProtect(&vtable[index], sizeof(void*), old_protect, &old_protect);
	return orig;
}

void install_hooks()
{
	// AHUD::PostRender
	orig_AHUD_PostRender = (AHUD_PostRender_t)
		vtable_hook(AHUD_vtable, AHUD_PostRender_index, hook_AHUD_PostRender);
}

void uninstall_hooks()
{
    // AHUD::PostRender
    vtable_hook(AHUD_vtable, AHUD_PostRender_index, orig_AHUD_PostRender);
}

bool ShouldUpdateBattle = true;
bool ShouldAdvanceBattle = false;

typedef void(*UpdateBattle_Func)(AREDGameState_Battle*, float);
UpdateBattle_Func UpdateBattle;

struct UpdateAdvantage
{
    UpdateAdvantage()
    {
        Text = RC::FString(L"");
    }
    
    RC::FString Text;
};

std::vector<RC::UObject*> mod_actors{};
RC::UFunction* Function;

bool MatchStartFlag = false;

void(*MatchStart_Orig)(AREDGameState_Battle*);
void MatchStart_New(AREDGameState_Battle* GameState)
{
    MatchStartFlag = true;
    ShouldAdvanceBattle = false;
    ShouldUpdateBattle = true;
    MatchStart_Orig(GameState);
}

int p1_advantage = 0;
int p2_advantage = 0;
int p1_hitstop_prev = 0;
int p2_hitstop_prev = 0;
int p1_act = 0;
int p2_act = 0;

bool was_f3_pressed;

void(*UpdateBattle_Orig)(AREDGameState_Battle*, float);
void UpdateBattle_New(AREDGameState_Battle* GameState, float DeltaTime) {
    if (GetAsyncKeyState(VK_F2) & 0x8001)
    {
        ShouldUpdateBattle = !ShouldUpdateBattle;
    }
    if (GetAsyncKeyState(VK_F3) & 0x8001)
    {
        if (!was_f3_pressed)
        {
            was_f3_pressed = true;
            ShouldAdvanceBattle = true;
        }
    }
    else
    {
        was_f3_pressed = false;
    }
    if (ShouldUpdateBattle || GetGameMode(GameCommon) != static_cast<int>(GAME_MODE_TRAINING) || ShouldAdvanceBattle)
	{
		UpdateBattle_Orig(GameState, DeltaTime);
		ShouldAdvanceBattle = false;

	    const auto engine = asw_engine::get();
	    if (!engine) return;
	    if ((engine->players[0].entity->action_time == 1 && engine->players[0].entity->hitstop != p1_hitstop_prev - 1)
	        || (engine->players[1].entity->action_time == 1 && engine->players[1].entity->hitstop != p2_hitstop_prev - 1))
	    {
	        if (!engine->players[0].entity->can_act() || !engine->players[1].entity->can_act())
	        {
	            if (!engine->players[1].entity->is_knockdown() || engine->players[1].entity->is_down_bound() || engine->players[1].entity->is_quick_down_1())
	            {
	                p1_advantage = engine->players[0].entity->calc_advantage() + p1_act;
	            }
	            if (!engine->players[0].entity->is_knockdown() || engine->players[0].entity->is_down_bound() || engine->players[0].entity->is_quick_down_1())
	            {
	                p2_advantage = engine->players[1].entity->calc_advantage() + p2_act;
	            }
	        }
	        else
	        {
	            p1_advantage = p1_act - p2_act;
	            p2_advantage = p2_act - p1_act;
	        }
	    }

	    if (engine->players[0].entity->is_stunned() && !engine->players[1].entity->is_stunned())
	        p1_advantage = -p2_advantage;
	    else if (engine->players[1].entity->is_stunned() && !engine->players[0].entity->is_stunned())
	        p2_advantage = -p1_advantage;
	    
	    p1_hitstop_prev = engine->players[0].entity->hitstop;
	    p2_hitstop_prev = engine->players[1].entity->hitstop;
	    if (engine->players[0].entity->can_act() && !engine->players[1].entity->can_act())
	        p1_act++;
	    else p1_act = 0;
	    if (engine->players[1].entity->can_act() && !engine->players[0].entity->can_act())
	        p2_act++;
	    else p2_act = 0;

        if (MatchStartFlag)
        {
            static auto battle_trainingdamage_name = RC::FName(STR("Battle_TrainingDamage_C"), RC::Unreal::FNAME_Add);

            RC::UObjectGlobals::FindAllOf(battle_trainingdamage_name, mod_actors);

            if (mod_actors.size() < 1) return;
            
            Function = mod_actors[0]->GetFunctionByNameInChain(STR("UpdateAdvantage"));
            MatchStartFlag = false;
        }
	    
	    UpdateAdvantage params = UpdateAdvantage();
	    auto p1_string = std::to_wstring(p1_advantage).append(L"0");
	    if (p1_advantage > 0)
	    {
	        p1_string.insert(0, L"+");
	    }
	    if (p1_advantage > 9000 || p1_advantage < -9000)
	    {
	        p1_string = L"???";
	    }
	    params.Text = RC::FString(p1_string.c_str());
	    mod_actors[mod_actors.size() - 1]->ProcessEvent(Function, &params);

	    auto p2_string = std::to_wstring(p2_advantage).append(L"0");
	    if (p2_advantage > 0)
	    {
	        p2_string.insert(0, L"+");
	    }
	    if (p2_advantage > 9000 || p2_advantage < -9000)
	    {
	        p2_string = L"???";
	    }
	    UpdateAdvantage params2 = UpdateAdvantage();
	    params2.Text = RC::FString(p2_string.c_str());
	    mod_actors[mod_actors.size() - 2]->ProcessEvent(Function, &params2);
	}
}

/*BPFUNCTION(ToggleUpdateBattle)
{
	ShouldUpdateBattle = !ShouldUpdateBattle;
}

BPFUNCTION(AdvanceBattle)
{
	ShouldAdvanceBattle = true;
}*/

class StriveHitboxes : public RC::CppUserModBase
{
public:
    StriveHitboxes() : CppUserModBase()
    {
        GameCommon = static_cast<UREDGameCommon*>(RC::UObjectGlobals::FindFirstOf(L"REDGameCommon"));
        
        if (MH_Initialize() != MH_OK)
            RC::Output::send(L"ERROR: Failed to initialize MinHook!");
        
        uint64_t UpdateBattle_Addr = sigscan::get().scan("\x48\x8B\x97\x88\x0B\x00\x00", "xxxxxxx") - 0x2B;
        if (MH_CreateHook(reinterpret_cast<LPVOID>(UpdateBattle_Addr), &UpdateBattle_New, reinterpret_cast<LPVOID*>(&UpdateBattle_Orig))  != MH_OK)
            RC::Output::send(L"ERROR: Failed to hook UpdateBattle!");
        if (MH_EnableHook((LPVOID)UpdateBattle_Addr) != MH_OK)
            RC::Output::send(L"ERROR: Failed to enable hook on UpdateBattle!");
        
        uint64_t MatchStart_Addr = sigscan::get().scan("\x48\x89\x85\x18\x04\x00\x00\x33\xC0", "xxxxxxxxx") - 0x31;
        if (MH_CreateHook(reinterpret_cast<LPVOID>(MatchStart_Addr), &MatchStart_New, reinterpret_cast<LPVOID*>(&MatchStart_Orig))  != MH_OK)
            RC::Output::send(L"ERROR: Failed to hook MatchStart!");
        if (MH_EnableHook((LPVOID)MatchStart_Addr) != MH_OK)
            RC::Output::send(L"ERROR: Failed to enable hook on MatchStart!");
        
        uintptr_t GetGameMode_Addr = sigscan::get().scan("\x0F\xB6\x81\xF0\x02\x00\x00\xC3", "xxxxxxxx");
        GetGameMode = (GetGameMode_Func)GetGameMode_Addr;
        
        install_hooks();
    }

    ~StriveHitboxes()
    {
        uninstall_hooks();
    }

    auto update() -> void override
    {
    }
};

extern "C"
{
    STRIVEHITBOXES_API RC::CppUserModBase* start_mod()
    {
        return new StriveHitboxes();
    }

    STRIVEHITBOXES_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
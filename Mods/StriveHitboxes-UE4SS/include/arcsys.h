#pragma once

#include <Unreal/AActor.hpp>
#include "struct_util.h"
#include "bbscript.h"

class AGameState : public RC::Unreal::AActor {};

class UWorld : public RC::Unreal::UObject {
public:
    FIELD(0x130, AGameState*, GameState);
};

class AREDGameState_Battle : public AGameState {
public:
    FIELD(0xBA0, class asw_engine*, Engine);
	FIELD(0xBA8, class asw_scene*, Scene);
};

class player_block {
	char pad[0x160];
public:
	FIELD(0x8, class asw_player*, entity);
};

static_assert(sizeof(player_block) == 0x160);

// Used by the shared GG/BB/DBFZ engine code
class asw_engine {
public:
	static constexpr auto COORD_SCALE = .458f;

	static asw_engine *get();

	ARRAY_FIELD(0x0, player_block[2], players);
	FIELD(0x8A0, int, entity_count);
	ARRAY_FIELD(0xC10, class asw_entity* [107], entities);
	ARRAY_FIELD(0x1498, RC::Unreal::AActor* [7], pawns);
};

class asw_scene {
public:
	static asw_scene *get();

	// "delta" is the difference between input and output position
	// position gets written in place
	// position/angle can be null
	void camera_transform(RC::Unreal::FVector *delta, RC::Unreal::FVector *position, RC::Unreal::FVector *angle) const;
	void camera_transform(RC::Unreal::FVector *position, RC::Unreal::FVector *angle) const;
};

class hitbox {
public:
	enum class box_type : int {
		hurt = 0,
		hit = 1,
		grab = 2 // Not used by the game
	};

	box_type type;
	float x, y;
	float w, h;
};

enum class direction : int {
	right = 0,
	left = 1
};

class event_handler {
	char pad[0x58];

public:
	FIELD(0x0, char*, script);
    FIELD(0x8, char*, action_name);
	FIELD(0x28, int, trigger_value);
	FIELD(0x2C, int, trigger_value_2);
    FIELD(0x30, char*, label);
    FIELD(0x50, unsigned int, int_flag);
};

static_assert(sizeof(event_handler) == 0x58);

class atk_param {
    char pad[0x3F8];

public:
    FIELD(0x0, int, atk_type);
    FIELD(0x4, int, atk_level);
    FIELD(0x8, int, atk_level_clash);
    FIELD(0xC, int, damage);
    FIELD(0x24, int*, hitstop_enemy_addition);
    FIELD(0x30, int, hitstop);
    FIELD(0x34, int, grab_wait_time);
    FIELD(0x38, int, guard_time);
    FIELD(0x12C, int, pushback);
    FIELD(0x130, int, fd_min_pushback);
    FIELD(0x134, int, guard_break_pushback);
    FIELD(0x138, int, hit_pushback);
    FIELD(0x13C, int, wall_pushback);
    FIELD(0x14C, int, stagger_time);
    FIELD(0x254, int, atk_level_guard);
    FIELD(0x258, int, risc);
    FIELD(0x25C, int, proration);
    FIELD(0x260, int, proration_rate_first);
    FIELD(0x264, int, proration_rate_once);
    FIELD(0x268, int, proration_first);
    FIELD(0x26C, int, proration_once);
    FIELD(0x270, int, chip_damage);
    FIELD(0x274, int, chip_damage_rate);
    FIELD(0x278, int, unburst_time);
    FIELD(0x304, int, guard_crush_time);
};

static_assert(sizeof(atk_param) == 0x3F8);

class atk_param_ex {
    char pad[0xB8];

public:
    FIELD(0x0, int, air_pushback_x);
    FIELD(0x4, int, air_pushback_y);
    FIELD(0x8, int, atk_gravity);
    FIELD(0x14, int, atk_hitstun);
    FIELD(0x18, int, atk_untech);
    FIELD(0x24, int, atk_knockdown_time);
    FIELD(0x30, int, atk_wallstick_time);
    FIELD(0x38, int, atk_roll_time);
    FIELD(0x3C, int, atk_slide_time);
    FIELD(0x48, int, atk_soft_knockdown);
};

static_assert(sizeof(atk_param_ex) == 0xB8);

class asw_entity {

public:
    FIELD(0x18, bool, is_player);
	FIELD(0x44, unsigned char, player_index);
	FIELD(0x70, hitbox*, hitboxes);
	FIELD(0x104, int, hurtbox_count);
	FIELD(0x108, int, hitbox_count);
	//   _____    ____    _    _   _   _   _______   ______   _____  
	//  / ____|  / __ \  | |  | | | \ | | |__   __| |  ____| |  __ \ 
	// | |      | |  | | | |  | | |  \| |    | |    | |__    | |__) |
	// | |      | |  | | | |  | | | . ` |    | |    |  __|   |  _  / 
	// | |____  | |__| | | |__| | | |\  |    | |    | |____  | | \ \ 
	//  \_____|  \____/   \____/  |_| \_|    |_|    |______| |_|  \_\ 
	BIT_FIELD(0x1A0, 0x4000000, cinematic_counter);
	FIELD(0x1BC, int, action_time);
	FIELD(0x1D8, int, act_reg_0);
    FIELD(0x278, int, hitstop);
    FIELD(0x2A8, asw_entity*, parent_ply);
    FIELD(0x2B0, asw_entity*, parent_obj);
	FIELD(0x2B8, asw_player*, opponent);
	FIELD(0x310, asw_entity*, attached);
    BIT_FIELD(0x388, 1, airborne);
	BIT_FIELD(0x388, 256, counterhit);
	BIT_FIELD(0x38C, 16, strike_invuln);
	BIT_FIELD(0x38C, 32, throw_invuln);
	BIT_FIELD(0x38C, 64, wakeup);
	FIELD(0x39C, direction, facing);
	FIELD(0x3A0, int, pos_x);
	FIELD(0x3A4, int, pos_y);
	FIELD(0x3A8, int, pos_z);
	FIELD(0x3AC, int, angle_x);
	FIELD(0x3B0, int, angle_y);
	FIELD(0x3B4, int, angle_z);
	FIELD(0x3BC, int, scale_x);
	FIELD(0x3C0, int, scale_y);
	FIELD(0x3C4, int, scale_z);
	FIELD(0x4C0, int, vel_x);
    FIELD(0x4C4, int, vel_y);
    FIELD(0x4C8, int, gravity);
    FIELD(0x4F4, int, pushbox_front_offset);
    FIELD(0x6F0, atk_param, atk_param_hit);
    FIELD(0x734, int, throw_box_top);
    FIELD(0x73C, int, throw_box_bottom);
    FIELD(0x740, int, throw_range);
    FIELD(0xAE8, atk_param_ex, atk_param_ex_normal);
    FIELD(0xBA0, atk_param_ex, atk_param_ex_counter);
    FIELD(0xC78, atk_param, atk_param_defend);
    FIELD(0x1070, atk_param_ex, atk_param_ex_defend);
    FIELD(0x1144, int, backdash_invuln);
    // bbscript
    FIELD(0x11A8, bbscript::event_bitmask, event_handler_bitmask);
    FIELD(0x11F8, char*, bbs_file);
    FIELD(0x1200, char*, script_base);
    FIELD(0x1208, char*, next_script_cmd);
    FIELD(0x1210, char*, first_script_cmd);
    FIELD(0x1218, char*, sprite_name);
    FIELD(0x1238, int, sprite_duration);
    FIELD(0x1240, int, sprite_total_duration);
    FIELD(0x1324, int, sprite_changes);
    ARRAY_FIELD(0x1330, event_handler[(size_t)bbscript::event_type::MAX], event_handlers);
    ARRAY_FIELD(0x3718, char[20], state_name);

    bool is_active() const;
    bool is_pushbox_active() const;
    bool is_strike_invuln() const;
    bool is_throw_invuln() const;
    int get_pos_x() const;
    int get_pos_y() const;
    int pushbox_width() const;
    int pushbox_height() const;
    int pushbox_bottom() const;
    void get_pushbox(int* left, int* top, int* right, int* bottom) const;
};

enum PLAYER_ENABLE_FLAG : uint32_t
{
    ENABLE_STANDING = 0x1,
    ENABLE_CROUCHING = 0x2,
    ENABLE_FORWARDWALK = 0x4,
    ENABLE_FORWARDDASH = 0x8,
    ENABLE_FORWARDCROUCHWALK = 0x10,
    ENABLE_BACKWALK = 0x20,
    ENABLE_BACKDASH = 0x40,
    ENABLE_BACKCROUCHWALK = 0x80,
    ENABLE_JUMP = 0x100,
    ENABLE_BARRIER_CANCEL = 0x200,
    ENABLE_AIRJUMP = 0x400,
    ENABLE_AIRFORWARDDASH = 0x800,
    ENABLE_NORMALATTACK = 0x1000,
    ENABLE_SPECIALATTACK = 0x2000,
    ENABLE_STANDTURN = 0x4000,
    ENABLE_DEAD = 0x8000,
    ENABLE_GUARD = 0x10000,
    ENABLE_AIRBACKDASH = 0x40000,
    ENABLE_CROUCHTURN = 0x80000,
    ENABLE_AIRTURN = 0x200000,
    ENABLE_ROMANCANCEL = 0x800000,
    ENABLE_NAMA_FAULT = 0x1000000,
    ENABLE_BARRIER = 0x2000000,
    ENABLE_LOCKREJECT = 0x4000000,
    ENABLE_AUTOLOCKREJECT = 0x8000000,
    ENABLE_DEMO = 0x10000000,
    ENABLE_PRE_GUARD = 0x20000000,
    ENABLE_AUTO_GUARD = 0x40000000,
    ENABLE_BURST = 0x80000000,
};

enum ID_CMNACT : uint32_t
{
    ID_CmnActStand = 0x0,
    ID_CmnActStandTurn = 0x1,
    ID_CmnActStand2Crouch = 0x2,
    ID_CmnActCrouch = 0x3,
    ID_CmnActCrouchTurn = 0x4,
    ID_CmnActCrouch2Stand = 0x5,
    ID_CmnActJumpPre = 0x6,
    ID_CmnActJump = 0x7,
    ID_CmnActJumpLanding = 0x8,
    ID_CmnActLandingStiff = 0x9,
    ID_CmnActFWalk = 0xA,
    ID_CmnActBWalk = 0xB,
    ID_CmnActFDash = 0xC,
    ID_CmnActFDashStop = 0xD,
    ID_CmnActBDash = 0xE,
    ID_CmnActBDashStop = 0xF,
    ID_CmnActAirFDash = 0x10,
    ID_CmnActAirBDash = 0x11,
    ID_CmnActNokezoriHighLv1 = 0x12,
    ID_CmnActNokezoriHighLv2 = 0x13,
    ID_CmnActNokezoriHighLv3 = 0x14,
    ID_CmnActNokezoriHighLv4 = 0x15,
    ID_CmnActNokezoriHighLv5 = 0x16,
    ID_CmnActNokezoriLowLv1 = 0x17,
    ID_CmnActNokezoriLowLv2 = 0x18,
    ID_CmnActNokezoriLowLv3 = 0x19,
    ID_CmnActNokezoriLowLv4 = 0x1A,
    ID_CmnActNokezoriLowLv5 = 0x1B,
    ID_CmnActNokezoriCrouchLv1 = 0x1C,
    ID_CmnActNokezoriCrouchLv2 = 0x1D,
    ID_CmnActNokezoriCrouchLv3 = 0x1E,
    ID_CmnActNokezoriCrouchLv4 = 0x1F,
    ID_CmnActNokezoriCrouchLv5 = 0x20,
    ID_CmnActBDownUpper = 0x21,
    ID_CmnActBDownUpperEnd = 0x22,
    ID_CmnActBDownDown = 0x23,
    ID_CmnActBDownBound = 0x24,
    ID_CmnActBDownLoop = 0x25,
    ID_CmnActBDown2Stand = 0x26,
    ID_CmnActFDownUpper = 0x27,
    ID_CmnActFDownUpperEnd = 0x28,
    ID_CmnActFDownDown = 0x29,
    ID_CmnActFDownBound = 0x2A,
    ID_CmnActFDownLoop = 0x2B,
    ID_CmnActFDown2Stand = 0x2C,
    ID_CmnActVDownUpper = 0x2D,
    ID_CmnActVDownUpperEnd = 0x2E,
    ID_CmnActVDownDown = 0x2F,
    ID_CmnActVDownBound = 0x30,
    ID_CmnActVDownLoop = 0x31,
    ID_CmnActBlowoff = 0x32,
    ID_CmnActBlowoffUpper90 = 0x33,
    ID_CmnActBlowoffUpper60 = 0x34,
    ID_CmnActBlowoffUpper30 = 0x35,
    ID_CmnActBlowoffDown30 = 0x36,
    ID_CmnActBlowoffDown60 = 0x37,
    ID_CmnActBlowoffDown90 = 0x38,
    ID_CmnActKirimomiUpper = 0x39,
    ID_CmnActWallBound = 0x3A,
    ID_CmnActWallBoundDown = 0x3B,
    ID_CmnActWallHaritsuki = 0x3C,
    ID_CmnActWallHaritsukiLand = 0x3D,
    ID_CmnActWallHaritsukiGetUp = 0x3E,
    ID_CmnActJitabataLoop = 0x3F,
    ID_CmnActKizetsu = 0x40,
    ID_CmnActHizakuzure = 0x41,
    ID_CmnActKorogari = 0x42,
    ID_CmnActZSpin = 0x43,
    ID_CmnActFuttobiFinish = 0x44,
    ID_CmnActFuttobiBGTrans = 0x45,
    ID_CmnActUkemi = 0x46,
    ID_CmnActLandUkemi = 0x47,
    ID_CmnActVUkemi = 0x48,
    ID_CmnActFUkemi = 0x49,
    ID_CmnActBUkemi = 0x4A,
    ID_CmnActKirimomiLand = 0x4B,
    ID_CmnActKirimomiLandEnd = 0x4C,
    ID_CmnActSlideDown = 0x4D,
    ID_CmnActRushFinishDown = 0x4E,
    ID_CmnActKirimomiVert = 0x4F,
    ID_CmnActKirimomiVertEnd = 0x50,
    ID_CmnActKirimomiSide = 0x51,
    ID_CmnActMidGuardPre = 0x52,
    ID_CmnActMidGuardLoop = 0x53,
    ID_CmnActMidGuardEnd = 0x54,
    ID_CmnActHighGuardPre = 0x55,
    ID_CmnActHighGuardLoop = 0x56,
    ID_CmnActHighGuardEnd = 0x57,
    ID_CmnActCrouchGuardPre = 0x58,
    ID_CmnActCrouchGuardLoop = 0x59,
    ID_CmnActCrouchGuardEnd = 0x5A,
    ID_CmnActAirGuardPre = 0x5B,
    ID_CmnActAirGuardLoop = 0x5C,
    ID_CmnActAirGuardEnd = 0x5D,
    ID_CmnActHajikareStand = 0x5E,
    ID_CmnActHajikareCrouch = 0x5F,
    ID_CmnActHajikareAir = 0x60,
    ID_CmnActAirTurn = 0x61,
    ID_CmnActLockWait = 0x62,
    ID_CmnActLockReject = 0x63,
    ID_CmnActAirLockWait = 0x64,
    ID_CmnActAirLockReject = 0x65,
    ID_CmnActItemUse = 0x66,
    ID_CmnActBurst = 0x67,
    ID_CmnActRomanCancel = 0x68,
    ID_CmnActEntry = 0x69,
    ID_CmnActRoundWin = 0x6A,
    ID_CmnActMatchWin = 0x6B,
    ID_CmnActLose = 0x6C,
    ID_CmnActResultWin = 0x6D,
    ID_CmnActResultLose = 0x6E,
    ID_CmnActEntryWait = 0x6F,
    ID_CmnActSubEntry = 0x70,
    ID_CmnActSpecialFinishWait = 0x71,
    ID_CmnActExDamage = 0x72,
    ID_CmnActExDamageLand = 0x73,
    ID_CmnActHide = 0x74,
    ID_CmnActChangeEnter = 0x75,
    ID_CmnActChangeEnterCutscene = 0x76,
    ID_CmnActChangeEnterCutsceneRecv = 0x77,
    ID_CmnActChangeEnterAttack = 0x78,
    ID_CmnActChangeEnterStiff = 0x79,
    ID_CmnActChangeLeave = 0x7A,
    ID_CmnActEnterAfterDestruction = 0x7B,
    ID_CmnActEnterAfterBGTransLeftIn = 0x7C,
    ID_CmnActEnterAfterBGTransRightIn = 0x7D,
    ID_CmnActRushStart = 0x7E,
    ID_CmnActRushRush = 0x7F,
    ID_CmnActRushFinish = 0x80,
    ID_CmnActRushFinishChaseLand = 0x81,
    ID_CmnActRushFinishChaseAir = 0x82,
    ID_CmnActRushFinishChaseEnd = 0x83,
    ID_CmnActRushFinishChange = 0x84,
    ID_CmnActRushSousai = 0x85,
    ID_CmnActRushSousaiPrimary = 0x86,
    ID_CmnActHomingDash = 0x87,
    ID_CmnActHomingDashCurve = 0x88,
    ID_CmnActHomingDashBrake = 0x89,
    ID_CmnActMikiwameMove = 0x8A,
    ID_CmnActShotHajiki = 0x8B,
    ID_CmnActGuardCancelAttack = 0x8C,
    ID_CmnActLimitBurst = 0x8D,
    ID_CmnActSparkingBurst = 0x8E,
    ID_CmnActRushRejectWait = 0x8F,
    ID_CmnActRushFinishDamage = 0x90,
    ID_CmnActQuickDown = 0x91,
    ID_CmnActQuickDown2Stand = 0x92,
    ID_CmnActNokezoriBottomLv1 = 0x93,
    ID_CmnActNokezoriBottomLv2 = 0x94,
    ID_CmnActNokezoriBottomLv3 = 0x95,
    ID_CmnActNokezoriBottomLv4 = 0x96,
    ID_CmnActNokezoriBottomLv5 = 0x97,
    ID_CmnActFloatDamage = 0x98,
    ID_CmnActEntryToStand1P = 0x99,
    ID_CmnActEntryToStand2P = 0x9A,
    ID_CmnActGuardLanding = 0x9B,
    ID_CmnAct_NUM = 0x9C,
    ID_CmnAct_NULL = 0xFFFFFFFF,
};

class asw_player : public asw_entity {

public:
    FIELD(0x6060, int, enable_flag);
    FIELD(0x6080, int, blockstun);
    FIELD(0x9844, int, hitstun);
    FIELD(0xC248, ID_CMNACT, cur_cmn_action_id);

    int calc_advantage();
    bool is_in_hitstun();
    bool is_in_blockstun();
    bool can_act();
    bool is_down_bound();
    bool is_quick_down_1();
    bool is_quick_down_2();
    bool is_down_loop();
    bool is_down_2_stand();
    bool is_knockdown();
    bool is_roll();
    bool is_stagger();
    bool is_guard_crush();
    bool is_stunned();
};

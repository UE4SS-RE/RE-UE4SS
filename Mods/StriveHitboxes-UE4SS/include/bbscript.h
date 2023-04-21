#pragma once

#include "bitmask.h"

namespace bbscript {

	inline int BeginEndPairs[17][2] =
	{
	  { 0, 1 },
	  { 4, 5 },
	  { 7, 5 },
	  { 9, 10 },
	  { 15, 16 },
	  { 21, 22 },
	  { 6, 5 },
	  { 8, 5 },
	  { 39, 5 },
	  { 30, 5 },
	  { 68, 5 },
	  { 69, 5 },
	  { 2498, 5 },
	  { 2499, 5 },
	  { 2500, 5 },
	  { 74, 5 },
	  { 2513, 5 }
	};

	enum class opcode : int {
		begin_state = 0,
		end_state = 1,
		set_sprite = 2,
		end_sprite = 3,
		goto_label = 12,
		goto_if_operation = 13,
		call_subroutine = 17,
		exit_state = 18,
		upon = 21,
		end_upon = 22,
		remove_upon = 23,
		goto_if = 24,
		set_sprite_time = 26,
		set_event_trigger = 36,
		call_subroutine_args = 75,
		modify_run_momentum = 143,
		set_speed_mult = 207,
		grab_or_release = 233,
		hit = 234,
		sprite_time_add = 2315,
		MAX = 2596,
	};

	enum class variable_type : int {
		abs_distance_x = 0xE,
		abs_distance_y = 0xF,
		pushbox_distance = 0x6A
	};

	enum class event_type : int {
		immediate = 0,
		before_exit = 1,
		landing = 2,
		frame_step = 3,
		fall_speed = 4, // requires trigger value
	    landing_infinity = 5,
		hit_or_guard = 9,
		hit = 10,
		timer = 13, // requires trigger value
		guard = 74,
		MAX = 0x68
	};

	using event_bitmask = bitmask<(size_t)event_type::MAX>;

	enum class value_type : int {
		constant = 0,
		variable = 2
	};

	enum class operation : int {
		add = 0,
		sub = 1,
		mul = 2,
		div = 3,
		mod = 4,
		bool_and = 5,
		bool_or = 6,
		bit_and = 7,
		bit_or = 8,
		eq = 9,
		lt = 10,
		gt = 11,
		ge = 12,
		le = 13,
		not_and = 14, // ~a & b
		ne = 15,
		mod_0 = 16, // b % a == 0
		mod_1 = 17, // b % a == 1
		mod_2 = 18, // b % a == 2
		mul_direction_offset = 19, // a * direction + b
		select_a = 20, // a
		mul_direction = 21, // a * direction
		unk22 = 22
	};

	struct value {
		value_type type;
		union {
			int constant;
			variable_type variable;
		};
	};
	
	class code_pointer {
	public:
        code_pointer()
        {
            owner = 0;
            ptr = 0;
            base_ptr = 0;
            state_remaining_time = 0;
            last_sprite_time = 0;
            nandemo = false;
        }

	    char* owner;
		char* ptr;
		char* base_ptr;
	    int state_remaining_time;
	    int last_sprite_time;
	    bool nandemo;

		opcode next_op() const
		{
			return *(opcode*)ptr;
		}
	    
		void read_script();
		void read_subroutine(char* name);
		void execute_instruction(int code);
	    void get_skip_begin_end_addr();
		static char* get_func_addr_base(char* bbs_file, char* func_name);
	    static char* get_action_addr_base(char* obj, char* action_name, int* out_index);
	};

} // bbscript

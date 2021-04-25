#pragma once

// - Returns a value from 'EScratchOpcode' or 0
int ScratchOpcode_FromString(const char* Str, int Len = -1);
const char* ScratchOpcode_ToString(int Opcode);

enum EScratchOpcode
{
	ScratchOpcode_unknown,

	event_whentouchingobject,
	event_touchingobjectmenu,
	event_whenflagclicked,
	event_whenthisspriteclicked,
	event_whenstageclicked,
	event_whenbroadcastreceived,
	event_whenbackdropswitchesto,
	event_whengreaterthan,
	event_broadcast_menu,
	event_broadcast,
	event_broadcastandwait,
	event_whenkeypressed,

	data_variable,
	data_setvariableto,
	data_changevariableby,
	data_showvariable,
	data_hidevariable,
	data_listcontents,
	data_listindexall,
	data_listindexrandom,
	data_addtolist,
	data_deleteoflist,
	data_deletealloflist,
	data_insertatlist,
	data_replaceitemoflist,
	data_itemoflist,
	data_itemnumoflist,
	data_lengthoflist,
	data_listcontainsitem,
	data_showlist,
	data_hidelist,

	control_forever,
	control_repeat,
	control_if,
	control_if_else,
	control_stop,
	control_wait,
	control_wait_until,
	control_repeat_until,
	control_while, // Obsolete
	control_for_each, // Obsolete 
	control_start_as_clone,
	control_create_clone_of_menu,
	control_create_clone_of,
	control_delete_this_clone,
	control_get_counter, // Obsolete 
	control_incr_counter, // Obsolete
	control_clear_counter,
	control_all_at_once, // Obsolete

	looks_sayforsecs,
	looks_say,
	looks_thinkforsecs,
	looks_think,
	looks_show,
	looks_hide,
	looks_hideallsprites,
	looks_changeeffectby,
	looks_seteffectto,
	looks_cleargraphiceffects,
	looks_changesizeby,
	looks_setsizeto,
	looks_size,
	looks_changestretchby,
	looks_setstretchto,
	looks_costume,
	looks_switchcostumeto,
	looks_nextcostume,
	looks_switchbackdropto,
	looks_backdrops,
	looks_gotofrontback,
	looks_goforwardbackwardlayers,
	looks_backdropnumbername,
	looks_costumenumbername,
	looks_switchbackdroptoandwait,
	looks_nextbackdrop,

	motion_movesteps,
	motion_turnright,
	motion_turnleft,
	motion_pointindirection,
	motion_pointtowards_menu,
	motion_pointtowards,
	motion_goto_menu,
	motion_gotoxy,
	motion_goto,
	motion_glidesecstoxy,
	motion_glideto_menu,
	motion_glideto,
	motion_changexby,
	motion_setx,
	motion_changeyby,
	motion_sety,
	motion_ifonedgebounce,
	motion_setrotationstyle,
	motion_xposition,
	motion_yposition,
	motion_direction,
	motion_scroll_right,
	motion_scroll_up,
	motion_align_scene,
	motion_xscroll,
	motion_yscroll,

	operator_add,
	operator_subtract,
	operator_multiply,
	operator_divide,
	operator_random,
	operator_lt,
	operator_equals,
	operator_gt,
	operator_and,
	operator_or,
	operator_join,
	operator_letter_of,
	operator_length,
	operator_contains,
	operator_mod,
	operator_round,
	operator_mathop,

	procedures_definition,
	procedures_call,
	procedures_prototype,
	procedures_declaration,
	argument_reporter_string_number,
	argument_reporter_boolean,

	sensing_touchingobject,
	sensing_touchingobjectmenu,
	sensing_touchingcolor,
	sensing_coloristouchingcolor,
	sensing_distanceto,
	sensing_distancetomenu,
	sensing_askandwait,
	sensing_answer,
	sensing_keypressed,
	sensing_keyoptions,
	sensing_mousedown,
	sensing_mousex,
	sensing_mousey,
	sensing_setdragmode,
	sensing_loudness,
	sensing_loud,
	sensing_timer,
	sensing_resettimer,
	sensing_of_object_menu,
	sensing_of,
	sensing_current,
	sensing_dayssince2000,
	sensing_username,
	sensing_userid, // Obsolete, does nothing as of 3.0

	sound_sounds_menu,
	sound_play,
	sound_playuntildone,
	sound_stopallsounds,
	sound_seteffectto,
	sound_changeeffectby,
	sound_cleareffects,
	sound_changevolumeby,
	sound_setvolumeto,
	sound_volume,
};

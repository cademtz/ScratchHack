#include "scratchopcodes.h"
#include <string.h>

static const struct {
	const char* name; int opcode;
} _ScratchOpcode_map[] =
{
	{ "ScratchOpcode_unknown",		   ScratchOpcode_unknown },

	{ "event_whentouchingobject",	   event_whentouchingobject },
	{ "event_touchingobjectmenu",	   event_touchingobjectmenu },
	{ "event_whenflagclicked",		   event_whenflagclicked },
	{ "event_whenthisspriteclicked",   event_whenthisspriteclicked },
	{ "event_whenstageclicked",		   event_whenstageclicked },
	{ "event_whenbroadcastreceived",   event_whenbroadcastreceived },
	{ "event_whenbackdropswitchesto",  event_whenbackdropswitchesto },
	{ "event_whengreaterthan",		   event_whengreaterthan },
	{ "event_broadcast_menu",		   event_broadcast_menu },
	{ "event_broadcast",			   event_broadcast },
	{ "event_broadcastandwait",		   event_broadcastandwait },
	{ "event_whenkeypressed",		   event_whenkeypressed },

	{ "data_variable",				   data_variable },
	{ "data_setvariableto",			   data_setvariableto },
	{ "data_changevariableby",		   data_changevariableby },
	{ "data_showvariable",			   data_showvariable },
	{ "data_hidevariable",			   data_hidevariable },
	{ "data_listcontents",			   data_listcontents },
	{ "data_listindexall",			   data_listindexall },
	{ "data_listindexrandom",		   data_listindexrandom },
	{ "data_addtolist",				   data_addtolist },
	{ "data_deleteoflist",			   data_deleteoflist },
	{ "data_deletealloflist",		   data_deletealloflist },
	{ "data_insertatlist",			   data_insertatlist },
	{ "data_replaceitemoflist",		   data_replaceitemoflist },
	{ "data_itemoflist",			   data_itemoflist },
	{ "data_itemnumoflist",			   data_itemnumoflist },
	{ "data_lengthoflist",			   data_lengthoflist },
	{ "data_listcontainsitem",		   data_listcontainsitem },
	{ "data_showlist",				   data_showlist },
	{ "data_hidelist",				   data_hidelist },

	{ "control_forever",			   control_forever },
	{ "control_repeat",				   control_repeat },
	{ "control_if",					   control_if },
	{ "control_if_else",			   control_if_else },
	{ "control_stop",				   control_stop },
	{ "control_wait",				   control_wait },
	{ "control_wait_until",			   control_wait_until },
	{ "control_repeat_until",		   control_repeat_until },
	{ "control_while",				   control_while },
	{ "control_for_each",			   control_for_each },
	{ "control_start_as_clone",		   control_start_as_clone },
	{ "control_create_clone_of_menu",  control_create_clone_of_menu },
	{ "control_create_clone_of",	   control_create_clone_of },
	{ "control_delete_this_clone",	   control_delete_this_clone },
	{ "control_get_counter",		   control_get_counter },
	{ "control_incr_counter",		   control_incr_counter },
	{ "control_clear_counter",		   control_clear_counter },
	{ "control_all_at_once",		   control_all_at_once },

	{ "looks_sayforsecs",			   looks_sayforsecs },
	{ "looks_say",					   looks_say },
	{ "looks_thinkforsecs",			   looks_thinkforsecs },
	{ "looks_think",				   looks_think },
	{ "looks_show",					   looks_show },
	{ "looks_hide",					   looks_hide },
	{ "looks_hideallsprites",		   looks_hideallsprites },
	{ "looks_changeeffectby",		   looks_changeeffectby },
	{ "looks_seteffectto",			   looks_seteffectto },
	{ "looks_cleargraphiceffects",	   looks_cleargraphiceffects },
	{ "looks_changesizeby",			   looks_changesizeby },
	{ "looks_setsizeto",			   looks_setsizeto },
	{ "looks_size",					   looks_size },
	{ "looks_changestretchby",		   looks_changestretchby },
	{ "looks_setstretchto",			   looks_setstretchto },
	{ "looks_costume",				   looks_costume },
	{ "looks_switchcostumeto",		   looks_switchcostumeto },
	{ "looks_nextcostume",			   looks_nextcostume },
	{ "looks_switchbackdropto",		   looks_switchbackdropto },
	{ "looks_backdrops",			   looks_backdrops },
	{ "looks_gotofrontback",		   looks_gotofrontback },
	{ "looks_goforwardbackwardlayers", looks_goforwardbackwardlayers },
	{ "looks_backdropnumbername",	   looks_backdropnumbername },
	{ "looks_costumenumbername",	   looks_costumenumbername },
	{ "looks_switchbackdroptoandwait", looks_switchbackdroptoandwait },
	{ "looks_nextbackdrop",			   looks_nextbackdrop },

	{ "motion_movesteps",			   motion_movesteps },
	{ "motion_turnright",			   motion_turnright },
	{ "motion_turnleft",			   motion_turnleft },
	{ "motion_pointindirection",	   motion_pointindirection },
	{ "motion_pointtowards_menu",	   motion_pointtowards_menu },
	{ "motion_pointtowards",		   motion_pointtowards },
	{ "motion_goto_menu",			   motion_goto_menu },
	{ "motion_gotoxy",				   motion_gotoxy },
	{ "motion_goto",				   motion_goto },
	{ "motion_glidesecstoxy",		   motion_glidesecstoxy },
	{ "motion_glideto_menu",		   motion_glideto_menu },
	{ "motion_glideto",				   motion_glideto },
	{ "motion_changexby",			   motion_changexby },
	{ "motion_setx",				   motion_setx },
	{ "motion_changeyby",			   motion_changeyby },
	{ "motion_sety",				   motion_sety },
	{ "motion_ifonedgebounce",		   motion_ifonedgebounce },
	{ "motion_setrotationstyle",	   motion_setrotationstyle },
	{ "motion_xposition",			   motion_xposition },
	{ "motion_yposition",			   motion_yposition },
	{ "motion_direction",			   motion_direction },
	{ "motion_scroll_right",		   motion_scroll_right },
	{ "motion_scroll_up",			   motion_scroll_up },
	{ "motion_align_scene",			   motion_align_scene },
	{ "motion_xscroll",				   motion_xscroll },
	{ "motion_yscroll",				   motion_yscroll },

	{ "operator_add",				   operator_add },
	{ "operator_subtract",			   operator_subtract },
	{ "operator_multiply",			   operator_multiply },
	{ "operator_divide",			   operator_divide },
	{ "operator_random",			   operator_random },
	{ "operator_lt",				   operator_lt },
	{ "operator_equals",			   operator_equals },
	{ "operator_gt",				   operator_gt },
	{ "operator_and",				   operator_and },
	{ "operator_or",				   operator_or },
	{ "operator_join",				   operator_join },
	{ "operator_letter_of",			   operator_letter_of },
	{ "operator_length",			   operator_length },
	{ "operator_contains",			   operator_contains },
	{ "operator_mod",				   operator_mod },
	{ "operator_round",				   operator_round },
	{ "operator_mathop",			   operator_mathop },

	{ "procedures_definition",		   procedures_definition },
	{ "procedures_call",			   procedures_call },
	{ "procedures_prototype",		   procedures_prototype },
	{ "procedures_declaration",		   procedures_declaration },

	{ "sensing_touchingobject",		   sensing_touchingobject },
	{ "sensing_touchingobjectmenu",	   sensing_touchingobjectmenu },
	{ "sensing_touchingcolor",		   sensing_touchingcolor },
	{ "sensing_coloristouchingcolor",  sensing_coloristouchingcolor },
	{ "sensing_distanceto",			   sensing_distanceto },
	{ "sensing_distancetomenu",		   sensing_distancetomenu },
	{ "sensing_askandwait",			   sensing_askandwait },
	{ "sensing_answer",				   sensing_answer },
	{ "sensing_keypressed",			   sensing_keypressed },
	{ "sensing_keyoptions",			   sensing_keyoptions },
	{ "sensing_mousedown",			   sensing_mousedown },
	{ "sensing_mousex",				   sensing_mousex },
	{ "sensing_mousey",				   sensing_mousey },
	{ "sensing_setdragmode",		   sensing_setdragmode },
	{ "sensing_loudness",			   sensing_loudness },
	{ "sensing_loud",				   sensing_loud },
	{ "sensing_timer",				   sensing_timer },
	{ "sensing_resettimer",			   sensing_resettimer },
	{ "sensing_of_object_menu",		   sensing_of_object_menu },
	{ "sensing_of",					   sensing_of },
	{ "sensing_current",			   sensing_current },
	{ "sensing_dayssince2000",		   sensing_dayssince2000 },
	{ "sensing_username",			   sensing_username },
	{ "sensing_userid",				   sensing_userid },

	{ "sound_sounds_menu",			   sound_sounds_menu },
	{ "sound_play",					   sound_play },
	{ "sound_playuntildone",		   sound_playuntildone },
	{ "sound_stopallsounds",		   sound_stopallsounds },
	{ "sound_seteffectto",			   sound_seteffectto },
	{ "sound_changeeffectby",		   sound_changeeffectby },
	{ "sound_cleareffects",			   sound_cleareffects },
	{ "sound_changevolumeby",		   sound_changevolumeby },
	{ "sound_setvolumeto",			   sound_setvolumeto },
	{ "sound_volume",				   sound_volume },
};

int ScratchOpcode_FromString(const char* Str, int Len)
{
	size_t len = Len < 0 ? strlen(Str) : Len;

	for (auto& op : _ScratchOpcode_map)
	{
		if (!strncmp(op.name, Str, len) && !op.name[len])
			return op.opcode;
	}
	return 0;
}

const char* ScratchOpcode_ToString(int Opcode)
{
	for (auto& op : _ScratchOpcode_map)
	{
		if (op.opcode == Opcode)
			return op.name;
	}
	return 0;
}

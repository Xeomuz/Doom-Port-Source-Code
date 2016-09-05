/*
#include "actor.h"
#include "info.h"
#include "m_random.h"
#include "s_sound.h"
#include "p_local.h"
#include "p_enemy.h"
#include "gstrings.h"
#include "a_action.h"
#include "thingdef/thingdef.h"
*/

static FRandom pr_troopattack ("TroopAttack");

//
// A_TroopAttack
//
DEFINE_ACTION_FUNCTION(AActor, A_TroopAttack)
{
	PARAM_ACTION_PROLOGUE;

	if (!self->target)
		return 0;
				
	A_FaceTarget (self);
	if (self->CheckMeleeRange ())
	{
		int damage = (pr_troopattack()%8+1)*3;
		S_Sound (self, CHAN_WEAPON, "imp/melee", 1, ATTN_NORM);
		int newdam = P_DamageMobj (self->target, self, self, damage, NAME_Melee);
		P_TraceBleed (newdam > 0 ? newdam : damage, self->target, self);
		return 0;
	}
	
	// launch a missile
	P_SpawnMissile (self, self->target, PClass::FindActor("DoomImpBall"));
	return 0;
}

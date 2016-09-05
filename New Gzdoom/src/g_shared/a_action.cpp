#include "actor.h"
#include "thingdef/thingdef.h"
#include "p_conversation.h"
#include "p_lnspec.h"
#include "a_action.h"
#include "m_random.h"
#include "s_sound.h"
#include "d_player.h"
#include "p_local.h"
#include "p_terrain.h"
#include "p_enemy.h"
#include "statnums.h"
#include "templates.h"
#include "farchive.h"
#include "r_data/r_translate.h"

static FRandom pr_freezedeath ("FreezeDeath");
static FRandom pr_icesettics ("IceSetTics");
static FRandom pr_freeze ("FreezeDeathChunks");


// SwitchableDecoration: Activate and Deactivate change state ---------------

class ASwitchableDecoration : public AActor
{
	DECLARE_CLASS (ASwitchableDecoration, AActor)
public:
	void Activate (AActor *activator);
	void Deactivate (AActor *activator);
};

IMPLEMENT_CLASS (ASwitchableDecoration)

void ASwitchableDecoration::Activate (AActor *activator)
{
	SetState (FindState(NAME_Active));
}

void ASwitchableDecoration::Deactivate (AActor *activator)
{
	SetState (FindState(NAME_Inactive));
}

// SwitchingDecoration: Only Activate changes state -------------------------

class ASwitchingDecoration : public ASwitchableDecoration
{
	DECLARE_CLASS (ASwitchingDecoration, ASwitchableDecoration)
public:
	void Deactivate (AActor *activator) {}
};

IMPLEMENT_CLASS (ASwitchingDecoration)

//----------------------------------------------------------------------------
//
// PROC A_NoBlocking
//
//----------------------------------------------------------------------------

void A_Unblock(AActor *self, bool drop)
{
	// [RH] Andy Baker's stealth monsters
	if (self->flags & MF_STEALTH)
	{
		self->Alpha = 1.;
		self->visdir = 0;
	}

	self->flags &= ~MF_SOLID;

	// If the actor has a conversation that sets an item to drop, drop that.
	if (self->Conversation != NULL && self->Conversation->DropType != NULL)
	{
		P_DropItem (self, self->Conversation->DropType, -1, 256);
		self->Conversation = NULL;
		return;
	}

	self->Conversation = NULL;

	// If the actor has attached metadata for items to drop, drop those.
	if (drop && !self->IsKindOf (RUNTIME_CLASS (APlayerPawn)))	// [GRB]
	{
		DDropItem *di = self->GetDropItems();

		if (di != NULL)
		{
			while (di != NULL)
			{
				if (di->Name != NAME_None)
				{
					PClassActor *ti = PClass::FindActor(di->Name);
					if (ti != NULL)
					{
						P_DropItem (self, ti, di->Amount, di->Probability);
					}
				}
				di = di->Next;
			}
		}
	}
}

DEFINE_ACTION_FUNCTION(AActor, A_NoBlocking)
{
	PARAM_ACTION_PROLOGUE;
	A_Unblock(self, true);
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_Fall)
{
	PARAM_ACTION_PROLOGUE;
	A_Unblock(self, true);
	return 0;
}

//==========================================================================
//
// A_SetFloorClip
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_SetFloorClip)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 |= MF2_FLOORCLIP;
	self->AdjustFloorClip ();
	return 0;
}

//==========================================================================
//
// A_UnSetFloorClip
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_UnSetFloorClip)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 &= ~MF2_FLOORCLIP;
	self->Floorclip = 0;
	return 0;
}

//==========================================================================
//
// A_HideThing
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_HideThing)
{
	PARAM_ACTION_PROLOGUE;

	self->renderflags |= RF_INVISIBLE;
	return 0;
}

//==========================================================================
//
// A_UnHideThing
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_UnHideThing)
{
	PARAM_ACTION_PROLOGUE;

	self->renderflags &= ~RF_INVISIBLE;
	return 0;
}

//============================================================================
//
// A_FreezeDeath
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_FreezeDeath)
{
	PARAM_ACTION_PROLOGUE;

	int t = pr_freezedeath();
	self->tics = 75+t+pr_freezedeath();
	self->flags |= MF_SOLID|MF_SHOOTABLE|MF_NOBLOOD|MF_ICECORPSE;
	self->flags2 |= MF2_PUSHABLE|MF2_TELESTOMP|MF2_PASSMOBJ|MF2_SLIDE;
	self->flags3 |= MF3_CRASHED;
	self->Height = self->GetDefault()->Height;
	// Remove fuzz effects from frozen actors.
	if (self->RenderStyle.BlendOp >= STYLEOP_Fuzz && self->RenderStyle.BlendOp <= STYLEOP_FuzzOrRevSub)
	{
		self->RenderStyle = STYLE_Normal;
	}

	S_Sound (self, CHAN_BODY, "misc/freeze", 1, ATTN_NORM);

	// [RH] Andy Baker's stealth monsters
	if (self->flags & MF_STEALTH)
	{
		self->Alpha = 1;
		self->visdir = 0;
	}

	if (self->player)
	{
		self->player->damagecount = 0;
		self->player->poisoncount = 0;
		self->player->bonuscount = 0;
	}
	else if (self->flags3 & MF3_ISMONSTER && self->special)
	{ // Initiate monster death actions
		P_ExecuteSpecial(self->special, NULL, self, false, self->args[0],
			self->args[1], self->args[2], self->args[3], self->args[4]);
		self->special = 0;
	}
	return 0;
}

//==========================================================================
//
// A_GenericFreezeDeath
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_GenericFreezeDeath)
{
	PARAM_ACTION_PROLOGUE;

	self->Translation = TRANSLATION(TRANSLATION_Standard, 7);
	CALL_ACTION(A_FreezeDeath, self);
	return 0;
}

//============================================================================
//
// A_IceSetTics
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_IceSetTics)
{
	PARAM_ACTION_PROLOGUE;

	int floor;

	self->tics = 70+(pr_icesettics()&63);
	floor = P_GetThingFloorType (self);
	if (Terrains[floor].DamageMOD == NAME_Fire)
	{
		self->tics >>= 2;
	}
	else if (Terrains[floor].DamageMOD == NAME_Ice)
	{
		self->tics <<= 1;
	}
	return 0;
}

//============================================================================
//
// A_FreezeDeathChunks
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_FreezeDeathChunks)
{
	PARAM_ACTION_PROLOGUE;

	int i;
	int numChunks;
	AActor *mo;
	
	if (!self->Vel.isZero() && !(self->flags6 & MF6_SHATTERING))
	{
		self->tics = 3*TICRATE;
		return 0;
	}
	self->Vel.Zero();
	S_Sound (self, CHAN_BODY, "misc/icebreak", 1, ATTN_NORM);

	// [RH] In Hexen, this creates a random number of shards (range [24,56])
	// with no relation to the size of the self shattering. I think it should
	// base the number of shards on the size of the dead thing, so bigger
	// things break up into more shards than smaller things.
	// An actor with radius 20 and height 64 creates ~40 chunks.
	numChunks = MAX<int>(4, int(self->radius * self->Height)/32);
	i = (pr_freeze.Random2()) % (numChunks/4);
	for (i = MAX (24, numChunks + i); i >= 0; i--)
	{
		double xo = (pr_freeze() - 128)*self->radius / 128;
		double yo = (pr_freeze() - 128)*self->radius / 128;
		double zo = (pr_freeze()*self->Height / 255);

		mo = Spawn("IceChunk", self->Vec3Offset(xo, yo, zo), ALLOW_REPLACE);
		if (mo)
		{
			mo->SetState (mo->SpawnState + (pr_freeze()%3));
			mo->Vel.X = pr_freeze.Random2() / 128.;
			mo->Vel.Y = pr_freeze.Random2() / 128.;
			mo->Vel.Z = (mo->Z() - self->Z()) / self->Height * 4;
			CALL_ACTION(A_IceSetTics, mo); // set a random tic wait
			mo->RenderStyle = self->RenderStyle;
			mo->Alpha = self->Alpha;
		}
	}
	if (self->player)
	{ // attach the player's view to a chunk of ice
		AActor *head = Spawn("IceChunkHead", self->PosPlusZ(self->player->mo->ViewHeight), ALLOW_REPLACE);
		if (head != NULL)
		{
			head->Vel.X = pr_freeze.Random2() / 128.;
			head->Vel.Y = pr_freeze.Random2() / 128.;
			head->Vel.Z = (mo->Z() - self->Z()) / self->Height * 4;

			head->health = self->health;
			head->Angles.Yaw = self->Angles.Yaw;
			if (head->IsKindOf(RUNTIME_CLASS(APlayerPawn)))
			{
				head->player = self->player;
				head->player->mo = static_cast<APlayerPawn*>(head);
				self->player = NULL;
				head->ObtainInventory (self);
			}
			head->Angles.Pitch = 0.;
			head->RenderStyle = self->RenderStyle;
			head->Alpha = self->Alpha;
			if (head->player->camera == self)
			{
				head->player->camera = head;
			}
		}
	}

	// [RH] Do some stuff to make this more useful outside Hexen
	if (self->flags4 & MF4_BOSSDEATH)
	{
		A_BossDeath(self);
	}
	A_Unblock(self, true);

	self->SetState(self->FindState(NAME_Null));
	return 0;
}

//----------------------------------------------------------------------------
//
// CorpseQueue Routines (used by Hexen)
//
//----------------------------------------------------------------------------

// Corpse queue for monsters - this should be saved out

class DCorpsePointer : public DThinker
{
	DECLARE_CLASS (DCorpsePointer, DThinker)
	HAS_OBJECT_POINTERS
public:
	DCorpsePointer (AActor *ptr);
	void Destroy ();
	void Serialize (FArchive &arc);
	TObjPtr<AActor> Corpse;
	DWORD Count;	// Only the first corpse pointer's count is valid.
private:
	DCorpsePointer () {}
};

IMPLEMENT_POINTY_CLASS(DCorpsePointer)
 DECLARE_POINTER(Corpse)
END_POINTERS

CUSTOM_CVAR(Int, sv_corpsequeuesize, 64, CVAR_ARCHIVE|CVAR_SERVERINFO)
{
	if (self > 0)
	{
		TThinkerIterator<DCorpsePointer> iterator (STAT_CORPSEPOINTER);
		DCorpsePointer *first = iterator.Next ();
		while (first != NULL && first->Count > (DWORD)self)
		{
			DCorpsePointer *next = iterator.Next ();
			first->Destroy ();
			first = next;
		}
	}
}


DCorpsePointer::DCorpsePointer (AActor *ptr)
: DThinker (STAT_CORPSEPOINTER), Corpse (ptr)
{
	Count = 0;

	// Thinkers are added to the end of their respective lists, so
	// the first thinker in the list is the oldest one.
	TThinkerIterator<DCorpsePointer> iterator (STAT_CORPSEPOINTER);
	DCorpsePointer *first = iterator.Next ();

	if (first != this)
	{
		if (first->Count >= (DWORD)sv_corpsequeuesize)
		{
			DCorpsePointer *next = iterator.Next ();
			first->Destroy ();
			first = next;
		}
	}
	++first->Count;
}

void DCorpsePointer::Destroy ()
{
	// Store the count of corpses in the first thinker in the list
	TThinkerIterator<DCorpsePointer> iterator (STAT_CORPSEPOINTER);
	DCorpsePointer *first = iterator.Next ();

	int prevCount = first->Count;

	if (first == this)
	{
		first = iterator.Next ();
	}

	if (first != NULL)
	{
		first->Count = prevCount - 1;
	}

	if (Corpse != NULL)
	{
		Corpse->Destroy ();
	}
	Super::Destroy ();
}

void DCorpsePointer::Serialize (FArchive &arc)
{
	Super::Serialize(arc);
	arc << Corpse << Count;
}


// throw another corpse on the queue
DEFINE_ACTION_FUNCTION(AActor, A_QueueCorpse)
{
	PARAM_ACTION_PROLOGUE;

	if (sv_corpsequeuesize > 0)
	{
		new DCorpsePointer (self);
	}
	return 0;
}

// Remove an self from the queue (for resurrection)
DEFINE_ACTION_FUNCTION(AActor, A_DeQueueCorpse)
{
	PARAM_ACTION_PROLOGUE;

	TThinkerIterator<DCorpsePointer> iterator (STAT_CORPSEPOINTER);
	DCorpsePointer *corpsePtr;

	while ((corpsePtr = iterator.Next()) != NULL)
	{
		if (corpsePtr->Corpse == self)
		{
			corpsePtr->Corpse = NULL;
			corpsePtr->Destroy ();
			return 0;
		}
	}
	return 0;
}

//============================================================================
//
// A_SetInvulnerable
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_SetInvulnerable)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 |= MF2_INVULNERABLE;
	return 0;
}

//============================================================================
//
// A_UnSetInvulnerable
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_UnSetInvulnerable)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 &= ~MF2_INVULNERABLE;
	return 0;
}

//============================================================================
//
// A_SetReflective
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_SetReflective)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 |= MF2_REFLECTIVE;
	return 0;
}

//============================================================================
//
// A_UnSetReflective
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_UnSetReflective)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 &= ~MF2_REFLECTIVE;
	return 0;
}

//============================================================================
//
// A_SetReflectiveInvulnerable
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_SetReflectiveInvulnerable)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 |= MF2_REFLECTIVE|MF2_INVULNERABLE;
	return 0;
}

//============================================================================
//
// A_UnSetReflectiveInvulnerable
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_UnSetReflectiveInvulnerable)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 &= ~(MF2_REFLECTIVE|MF2_INVULNERABLE);
	return 0;
}

//==========================================================================
//
// A_SetShootable
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_SetShootable)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 &= ~MF2_NONSHOOTABLE;
	self->flags |= MF_SHOOTABLE;
	return 0;
}

//==========================================================================
//
// A_UnSetShootable
//
//==========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_UnSetShootable)
{
	PARAM_ACTION_PROLOGUE;

	self->flags2 |= MF2_NONSHOOTABLE;
	self->flags &= ~MF_SHOOTABLE;
	return 0;
}

//===========================================================================
//
// A_NoGravity
//
//===========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_NoGravity)
{
	PARAM_ACTION_PROLOGUE;

	self->flags |= MF_NOGRAVITY;
	return 0;
}

//===========================================================================
//
// A_Gravity
//
//===========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_Gravity)
{
	PARAM_ACTION_PROLOGUE;

	self->flags &= ~MF_NOGRAVITY;
	self->Gravity = 1;
	return 0;
}

//===========================================================================
//
// A_LowGravity
//
//===========================================================================

DEFINE_ACTION_FUNCTION(AActor, A_LowGravity)
{
	PARAM_ACTION_PROLOGUE;

	self->flags &= ~MF_NOGRAVITY;
	self->Gravity = 1. / 8;;
	return 0;
}

//===========================================================================
//
// FaceMovementDirection
//
//===========================================================================

void FaceMovementDirection(AActor *actor)
{
	switch (actor->movedir)
	{
	case DI_EAST:
		actor->Angles.Yaw = 0.;
		break;
	case DI_NORTHEAST:
		actor->Angles.Yaw = 45.;
		break;
	case DI_NORTH:
		actor->Angles.Yaw = 90.;
		break;
	case DI_NORTHWEST:
		actor->Angles.Yaw = 135.;
		break;
	case DI_WEST:
		actor->Angles.Yaw = 180.;
		break;
	case DI_SOUTHWEST:
		actor->Angles.Yaw = 225.;
		break;
	case DI_SOUTH:
		actor->Angles.Yaw = 270.;
		break;
	case DI_SOUTHEAST:
		actor->Angles.Yaw = 315.;
		break;
	}
}
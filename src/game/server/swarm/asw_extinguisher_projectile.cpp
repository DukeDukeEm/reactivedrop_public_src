#include "cbase.h"
#include "asw_extinguisher_projectile.h"
#include "Sprite.h"
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "IEffects.h"
#include "EntityFlame.h"
#include "asw_fire.h"
#include "asw_marine.h"
#include "asw_weapon_flamer_shared.h"
#include "asw_sentry_top_icer.h"
#include "asw_sentry_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar sk_plr_dmg_asw_f;
extern ConVar sk_npc_dmg_asw_f;
extern ConVar asw_flamer_debug;
ConVar rd_extinguisher_freeze_amount( "rd_extinguisher_freeze_amount", "0.0", FCVAR_REPLICATED | FCVAR_CHEAT, "The amount of freezing to apply to the extinguisher" );
ConVar rd_extinguisher_dmg_amount( "rd_extinguisher_dmg_amount", "0.0", FCVAR_REPLICATED | FCVAR_CHEAT, "The amount of damage the extinguisher does to entities" );

#define PELLET_MODEL "models/swarm/Shotgun/ShotgunPellet.mdl"

LINK_ENTITY_TO_CLASS( asw_extinguisher_projectile, CASW_Extinguisher_Projectile );

IMPLEMENT_SERVERCLASS_ST( CASW_Extinguisher_Projectile, DT_ASW_Extinguisher_Projectile )
	SendPropDataTable( SENDINFO_DT( m_ProjectileData ), &REFERENCE_SEND_TABLE( DT_RD_ProjectileData ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CASW_Extinguisher_Projectile )
	DEFINE_FUNCTION( ProjectileTouch ),
	DEFINE_FIELD( m_flDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_inSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hFirer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hFirerWeapon, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flFreezeAmount, FIELD_FLOAT ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CASW_Extinguisher_Projectile::~CASW_Extinguisher_Projectile( void )
{
}

void CASW_Extinguisher_Projectile::Spawn( void )
{
	Precache( );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );

	m_flDamage		= sk_plr_dmg_asw_f.GetFloat();
	m_takedamage	= DAMAGE_NO;
	m_flFreezeAmount = rd_extinguisher_freeze_amount.GetFloat();

	SetSize( -Vector(4,4,4), Vector(4,4,4) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.05f );
	SetCollisionGroup( ASW_COLLISION_GROUP_EXTINGUISHER_PELLETS );

	SetTouch( &CASW_Extinguisher_Projectile::ProjectileTouch );

	// flamer projectile only lasts 1 second
	SetThink( &CASW_Extinguisher_Projectile::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}

bool CASW_Extinguisher_Projectile::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );
	return true;
}

unsigned int CASW_Extinguisher_Projectile::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}

void CASW_Extinguisher_Projectile::ProjectileTouch( CBaseEntity *pOther )
{
	if ( !g_pGameRules || !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;	

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		CASW_Marine *pMarine = CASW_Marine::AsMarine( pOther );
		if (pMarine && pMarine->m_bOnFire)
		{
			pMarine->Extinguish( m_hFirer, this );
		}
		else
		{		
			CBaseAnimating* pAnim = pOther->GetBaseAnimating();
			if (pAnim && pAnim->IsOnFire())
			{						
				CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>( pAnim->GetEffectEntity() );
				if ( pFireChild )
				{
					pAnim->SetEffectEntity( NULL );
					UTIL_Remove( pFireChild );
				}

				pAnim->Extinguish();
			}
		}

		if ( rd_extinguisher_dmg_amount.GetFloat() > 0.0 && !pMarine )
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), rd_extinguisher_dmg_amount.GetFloat(), DMG_GENERIC );
			pOther->TakeDamage( dmgInfo );
		}

		CAI_BaseNPC * RESTRICT pNPC = pOther->MyNPCPointer();
		if ( pNPC )
		{
			if ( m_flFreezeAmount > 0 && ( !m_hFirer || !m_hFirer->MyCombatCharacterPointer() || m_hFirer->MyCombatCharacterPointer()->IRelationType( pNPC ) != D_LI ) )
			{
				bool bWasFrozen = pNPC->m_bWasEverFrozen;
				pNPC->Freeze( m_flFreezeAmount, m_hFirer ? m_hFirer : this );
				if ( !bWasFrozen && pNPC->IsFrozen() )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "entity_frozen" );
					if ( event )
					{
						event->SetInt( "entindex", pNPC->entindex() );
						event->SetInt( "attacker", m_hFirer ? m_hFirer->entindex() : -1 );
						event->SetInt( "weapon", m_hFirerWeapon ? m_hFirerWeapon->entindex() : -1 );

						gameeventmanager->FireEvent( event );
					}
				}

				if ( pNPC->GetFrozenAmount() >= 0.9f )
					return;
			}
		}
		SetAbsVelocity( Vector( 0, 0, 0 ) );

		SetTouch( NULL );
		SetThink( NULL );

		UTIL_Remove( this );
	}
	else
	{
#if 0
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			Vector vel = GetAbsVelocity();
			if ( tr.startsolid )
			{
				if ( !m_inSolid )
				{
					// UNDONE: Do a better contact solution that uses relative velocity?
					vel *= -1.0f; // bounce backwards
					SetAbsVelocity(vel);
				}
				m_inSolid = true;
				return;
			}
			m_inSolid = false;
			if ( tr.DidHit() )
			{
				Vector dir = vel;
				VectorNormalize(dir);

				// reflect velocity around normal
				vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;
				
				// absorb 80% in impact
				//vel *= GRENADE_COEFFICIENT_OF_RESTITUTION;
				SetAbsVelocity( vel );
			}
			return;
		}
		else
#endif
		{
			UTIL_Remove( this );
		}
	}
}

CASW_Extinguisher_Projectile* CASW_Extinguisher_Projectile::Extinguisher_Projectile_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	CASW_Extinguisher_Projectile *pPellet = (CASW_Extinguisher_Projectile *)CreateEntityByName( "asw_extinguisher_projectile" );	
	pPellet->SetAbsAngles( angles );
	pPellet->Spawn();
	pPellet->SetOwnerEntity( pOwner );
	pPellet->m_hFirer = pOwner;
	pPellet->m_hFirerWeapon = pWeapon;
	pPellet->m_ProjectileData.GetForModify().SetFromWeapon( pWeapon );
	UTIL_SetOrigin( pPellet, position );
	pPellet->SetAbsVelocity( velocity );


	if (asw_flamer_debug.GetBool())
		pPellet->m_debugOverlays |= OVERLAY_BBOX_BIT;


	return pPellet;
}

#define ASW_EXTINGUISHER_PROJECTILE_ACCN 250.0f
void CASW_Extinguisher_Projectile::PhysicsSimulate()
{
	// Make sure not to simulate this guy twice per frame
	if (m_nSimulationTick == gpGlobals->tickcount)
		return;

	// slow down the projectile's velocity	
	/*
	Vector dir = GetAbsVelocity();
	VectorNormalize(dir);		
	SetAbsVelocity(GetAbsVelocity() - (dir * gpGlobals->frametime * ASW_EXTINGUISHER_PROJECTILE_ACCN));
	dir = GetAbsVelocity();
	*/
	SetAbsVelocity( GetAbsVelocity() * 
		( 1 - gpGlobals->frametime * ASW_EXTINGUISHER_PROJECTILE_ACCN / CASW_Weapon_Flamer::EXTINGUISHER_PROJECTILE_AIR_VELOCITY )  );

	if (asw_flamer_debug.GetBool())
	{
		NDebugOverlay::Box( GetAbsOrigin(), -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, 255, 0.2f );

		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() ,
			255, 255, 0, true, 0.3f );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * gpGlobals->interval_per_tick,
			128, 255, 0, true, 0.3f );
	}
	
	BaseClass::PhysicsSimulate();
}

// need to force send as it has no model
int CASW_Extinguisher_Projectile::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

void CASW_Extinguisher_Projectile::TouchedEnvFire()
{
	SetThink( &CASW_Extinguisher_Projectile::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

CBaseEntity* CASW_Extinguisher_Projectile::GetFirer()
{
	return m_hFirer.Get();
}	
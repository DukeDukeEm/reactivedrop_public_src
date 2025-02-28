#ifndef _INLCUDED_ASW_BUZZER_H
#define _INLCUDED_ASW_BUZZER_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc_physicsflyer.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "player_pickup.h"
#include "iasw_spawnable_npc.h"
#include "asw_lag_compensation.h"
#include "asw_shareddefs.h"

// Start with the engine off and folded up.
#define SF_ASW_BUZZER_NO_DAMAGE_EFFECTS	(1 << 17)
#define SF_ASW_BUZZER_USE_AIR_NODES		(1 << 18)
#define SF_ASW_BUZZER_NO_DANGER_SOUNDS		(1 << 20)

//-----------------------------------------------------------------------------
// Attachment points.
//-----------------------------------------------------------------------------
#define	ASW_BUZZER_GIB_HEALTH				30
#define	ASW_BUZZER_INACTIVE_HEALTH			25
#define	ASW_BUZZER_MAX_SPEED				300

//-----------------------------------------------------------------------------
// Movement parameters.
//-----------------------------------------------------------------------------
#define ASW_BUZZER_WAYPOINT_DISTANCE		25	// Distance from waypoint that counts as arrival.

class CSprite;
class CSoundPatch;
class CASW_AI_Senses;
struct envelopePoint_t;

class CASW_Buzzer : public CAI_BasePhysicsFlyingBot
{
DECLARE_CLASS( CASW_Buzzer, CAI_BasePhysicsFlyingBot );
DECLARE_SERVERCLASS();

public:
	CASW_Buzzer();
	virtual ~CASW_Buzzer();

	Class_T		Classify( void ) { return (Class_T) CLASS_ASW_BUZZER; }

	bool			CorpseGib( const CTakeDamageInfo &info );
	void			Event_Dying(void);
	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int				OnTakeDamage_Dying( const CTakeDamageInfo &info );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr);
	void			ASWTraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	void			TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition );
	float			GetDefaultNavGoalTolerance();

	void			OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	virtual bool	CreateVPhysics( void );

	virtual void	DeathSound( const CTakeDamageInfo &info );
	virtual void	PainSound( const CTakeDamageInfo &info );
	virtual bool	ShouldGib( const CTakeDamageInfo &info );

	Activity		NPC_TranslateActivity( Activity baseAct );
	virtual int		TranslateSchedule( int scheduleType );
	int				MeleeAttack1Conditions ( float flDot, float flDist );

	bool			OverrideMove(float flInterval);
	void			MoveToTarget(float flInterval, const Vector &MoveTarget);
	virtual void	TurnHeadToTarget( float flInterval, const Vector &moveTarget );
	void			MoveExecute_Alive(float flInterval);
	void			MoveExecute_Dead(float flInterval);
	int				MoveCollisionMask(void);

	void			TurnHeadRandomly( float flInterval );


	void			StartEngine( bool fStartSound );

	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true ) { return WorldSpaceCenter(); }

	virtual float	GetHeadTurnRate( void ) { return 45.0f; } // Degrees per second

	void			CheckCollisions(float flInterval);
	virtual void	GatherEnemyConditions( CBaseEntity *pEnemy );
	void			PlayFlySound(void);
	virtual void	StopLoopingSounds(void);

	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	void			Spawn(void);
	virtual void	OnRestore();
	void			Activate();
	void			StartTask( const Task_t *pTask );

	void			BladesInit();
	void			SoundInit( void );
	void			StartEye( void );
	
	bool			HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt);

	void			PostNPCInit( void );

	void			GatherConditions();
	void			PrescheduleThink( void );

	void			FlapWings(float flInterval);

	void			Slice( CBaseEntity *pHitEntity, float flInterval, trace_t &tr );
	void			Bump( CBaseEntity *pHitEntity, float flInterval, trace_t &tr );
	void			Splash( const Vector &vecSplashPos );
	void			Freeze( float flFreezeAmount, CBaseEntity *pFreezer, Ray_t *pFreezeRay ) override;
	void			Unfreeze() override;
	float			IceStatueChance() override { return 0.0f; }
	float			StatueShatterDelay() override { return 0.25f; }

	float			ManhackMaxSpeed( void );
	virtual void	VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
	void			VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void			HitPhysicsObject( CBaseEntity *pOther );
	virtual void	ClampMotorForces( Vector &linear, AngularImpulse &angular );

	void			InputDisableSwarm( inputdata_t &inputdata );	

	float GetMaxEnginePower();

	virtual void	InputPowerdown( inputdata_t &inputdata )
	{
		m_iHealth = 0;
	}

	enum
	{
		SCHED_ASW_BUZZER_ATTACK_HOVER = BaseClass::NEXT_SCHEDULE,
		SCHED_ASW_BUZZER_REGROUP,
		SCHED_ASW_BUZZER_SWARM_IDLE,
		SCHED_ASW_BUZZER_SWARM,
		SCHED_ASW_BUZZER_SWARM_FAILURE,
		SCHED_ASW_BUZZER_ORDER_MOVE,
		NEXT_SCHEDULE,
	};

	enum
	{
		TASK_ASW_BUZZER_HOVER = BaseClass::NEXT_TASK,
		TASK_ASW_BUZZER_FIND_SQUAD_CENTER,
		TASK_ASW_BUZZER_FIND_SQUAD_MEMBER,
		TASK_ASW_BUZZER_MOVEAT_SAVEPOSITION,
		TASK_ASW_BUZZER_BUILD_PATH_TO_ORDER,
		NEXT_TASK,
	};

	enum
	{
		COND_ASW_BUZZER_START_ATTACK = BaseClass::NEXT_CONDITION,	// We are able to do a bombing run on the current enemy.
		NEXT_CONDITION,
	};

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();

	// IASW_Spawnable_NPC implementation
	virtual int SelectAlienOrdersSchedule();
	virtual void OnMovementComplete();
	virtual int SelectSchedule();
	virtual void IgnoreMarines(bool bIgnoreMarines);

	bool m_bIgnoreMarines;
	bool m_bFailedMoveTo;

	virtual int GetBaseHealth() override;

	virtual bool IsAlien(void) const { return true; }

	virtual void MoveAside() { }
	virtual void ASW_Ignite( float flFlameLifetime, float flSize, CBaseEntity *pAttacker, CBaseEntity *pDamagingWeapon = NULL );
	virtual void Extinguish();
	virtual void OnSwarmSensed(int iDistance);

	void NPCInit();
	CAI_Senses* CreateSenses();
	CASW_AI_Senses* GetASWSenses();
	void SetDistSwarmSense( float flDistSense );

	void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );

	virtual void NPCThink();

	//stuff related to sleep state
	bool MarineCanSee(int padding, float interval);// can a marine see us? //copy from asw_alien
	float m_fLastMarineCanSeeTime;
	bool m_bLastMarineCanSee;
	bool m_bRegisteredAsAwake;
	float m_fLastSleepCheckTime;
	bool m_bVisibleWhenAsleep;
	virtual void UpdateSleepState(bool bInPVS);
	void UpdateEfficiency(bool bInPVS);
	virtual void UpdateOnRemove();
private:

	bool IsInEffectiveTargetZone( CBaseEntity *pTarget );
	void MaintainGroundHeight( void );

	void StartBurst( const Vector &vecDirection );
	void StopBurst( bool bInterruptSchedule = false );

	void ShowHostile( bool hostile = true );

	bool IsFlyingActivity( Activity baseAct );

	// Computes the slice bounce velocity
	void ComputeSliceBounceVelocity( CBaseEntity *pHitEntity, trace_t &tr );

	// Take damage from a vehicle: 
	void TakeDamageFromVehicle( int index, gamevcollisionevent_t *pEvent );

	// Take damage from physics impacts
	void TakeDamageFromPhysicsImpact( int index, gamevcollisionevent_t *pEvent );

	void StartLoitering( const Vector &vecLoiterPosition );
	void StopLoitering() { m_vecLoiterPosition = vec3_invalid; m_fTimeNextLoiterPulse = gpGlobals->curtime; }
	bool IsLoitering() { return m_vecLoiterPosition != vec3_invalid; }
	void Loiter();

	//
	// Movement variables.
	//

	Vector			m_vForceVelocity;		// Someone forced me to move

	Vector			m_vTargetBanking;

	Vector			m_vForceMoveTarget;		// Will fly here
	float			m_fForceMoveTime;		// If time is less than this
	Vector			m_vSwarmMoveTarget;		// Will fly here
	float			m_fSwarmMoveTime;		// If time is less than this
	float			m_fEnginePowerScale;	// scale all thrust by this amount (usually 1.0!)

	float			m_flNextEngineSoundTime;
	float			m_flEngineStallTime;

	float			m_flNextBurstTime;
	float			m_flBurstDuration;
	Vector			m_vecBurstDirection;

	float			m_flWaterSuspendTime;
	int				m_nLastSpinSound;

	float			m_flLastMarineDamageTime;

	// physics influence
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;	

	bool			m_bDirtyPitch; // indicates whether we want the sound pitch updated.(sjb)
	bool			m_bShowingHostile;

	bool			m_bBladesActive;
	float			m_flWingFlapSpeed;

	int				m_iPanel1;
	int				m_iPanel2;
	int				m_iPanel3;
	int				m_iPanel4;

	int				m_nLastWaterLevel;
	bool			m_bDoSwarmBehavior;
	bool			m_bGib;

	bool			m_bHeld;
	Vector			m_vecLoiterPosition;
	float			m_fTimeNextLoiterPulse;
	float			m_fSuicideTime;

	float	m_fNextPainSound;
	CSoundPatch	*m_pMoanSound;
	float m_flMoanPitch;
	float m_flNextMoanSound;
	bool m_bHoldoutAlien;

	CNetworkVar( int,	m_nEnginePitch1 );
	CNetworkVar( float,	m_flEnginePitch1Time );
};

#endif	//_INLCUDED_ASW_BUZZER_H

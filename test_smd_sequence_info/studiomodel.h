/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

#pragma once

#include "steamtypes.h"
#include "mathlib.h"
#include "studio.h"

class StudioModel
{
public:
	StudioModel();

	void					Init( const char *modelname );
	void					Free();

	const studiohdr_t* GetStudioHdr() const { return m_pstudiohdr; }
	const mstudiobone_t*			GetBone(int iBone) const;
	void					GetBoneValues(int iBone, vec3_t position, vec3_t rotation) const;

	const mstudioseqdesc_t*		GetSequence( int iSequence ) const;
	const mstudioanim_t			*GetAnim( const mstudioseqdesc_t *pseqdesc ) const;

	void					CalcSequenceBoneAngle( int frame, float s, const mstudiobone_t *pbone, const mstudioanim_t *panim, vec3_t angle1, vec3_t angle2 ) const;
	void					CalcSequenceBonePosition( int frame, float s, const mstudiobone_t *pbone, const mstudioanim_t *panim, vec3_t pos ) const;

private:

	// internal data
	studiohdr_t				*m_pstudiohdr = nullptr;
	studiohdr_t				*m_ptexturehdr = nullptr;
	studioseqhdr_t			*m_panimhdr[32] {};

	studiohdr_t				*LoadModel( const char *modelname );
	studioseqhdr_t			*LoadDemandSequences( const char *modelname );
};


/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

#include <cstdio>
#include <cstring>
#include <Windows.h>

#include "studiomodel.h"


StudioModel::StudioModel() :
	m_pstudiohdr(nullptr), 
	m_ptexturehdr(nullptr),
	m_panimhdr{}
{
}

studiohdr_t *StudioModel::LoadModel( const char *modelname )
{
	FILE *fp = nullptr;
	long size = 0;
	void *buffer = nullptr;

	// load the model
	if (fopen_s(&fp, modelname, "rb") != 0)
	{
		printf("unable to open %s\n", modelname );
		throw;
	}

	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	buffer = malloc( size );
	fread( buffer, size, 1, fp );

	fclose( fp );

	return (studiohdr_t *)buffer;
}


studioseqhdr_t *StudioModel::LoadDemandSequences( const char *modelname )
{
	FILE *fp = nullptr;
	long size = 0;
	void *buffer = nullptr;

	// load the model
	if(fopen_s(&fp, modelname, "rb") != 0)
	{
		printf("unable to open %s\n", modelname );
		throw;
	}

	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	buffer = malloc( size );
	fread( buffer, size, 1, fp );

	fclose( fp );

	return (studioseqhdr_t *)buffer;
}


void StudioModel::Init( const char *modelname )
{ 
	m_pstudiohdr = LoadModel( modelname );

	// preload textures
	if (m_pstudiohdr->numtextures == 0)
	{
		char texturename[256] {};

		strcpy( texturename, modelname );
		strcpy( &texturename[strlen(texturename) - 4], "T.mdl" );

		m_ptexturehdr = LoadModel( texturename );
	}
	else
	{
		m_ptexturehdr = m_pstudiohdr;
	}

	// preload animations
	if (m_pstudiohdr->numseqgroups > 1)
	{
		for (int i = 1; i < m_pstudiohdr->numseqgroups; i++)
		{
			char seqgroupname[256]{};

			strcpy( seqgroupname, modelname );
			sprintf( &seqgroupname[strlen(seqgroupname) - 4], "%02d.mdl", i );

			m_panimhdr[i] = LoadDemandSequences( seqgroupname );
		}
	}
}

void StudioModel::Free()
{
	for (int i = 0; i < ARRAYSIZE(m_panimhdr); ++i)
	{
		if (m_panimhdr[i])
		{
			free(m_panimhdr[i]);
			m_panimhdr[i] = nullptr;
		}
	}

	if (m_ptexturehdr && m_ptexturehdr != m_pstudiohdr)
	{
		free(m_ptexturehdr);
		m_ptexturehdr = nullptr;
	}

	if (m_pstudiohdr)
	{
		free(m_pstudiohdr);
		m_pstudiohdr = nullptr;
	}
}


////////////////////////////////////////////////////////////////////////


const mstudiobone_t* StudioModel::GetBone(int iBone) const
{
	mstudiobone_t* pbone = (mstudiobone_t*)((byte*)m_pstudiohdr + m_pstudiohdr->boneindex) + iBone;
	return pbone;
}

void StudioModel::GetBoneValues(int iBone, vec3_t position, vec3_t rotation) const
{
	const mstudiobone_t* pbone = GetBone(iBone);
	memcpy(position, &pbone->value[0], sizeof(position));
	memcpy(rotation, &pbone->value[3], sizeof(rotation));
}

const mstudioseqdesc_t* StudioModel::GetSequence( int iSequence ) const
{
	mstudioseqdesc_t* pseqdesc = (mstudioseqdesc_t*)((byte*)m_pstudiohdr + m_pstudiohdr->seqindex) + (int)iSequence;
	return pseqdesc;
}

const mstudioanim_t * StudioModel::GetAnim(const mstudioseqdesc_t *pseqdesc ) const
{
	mstudioseqgroup_t	*pseqgroup = nullptr;
	pseqgroup = (mstudioseqgroup_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
	{
		return (mstudioanim_t *)((byte *)m_pstudiohdr + pseqgroup->unused2 /* was pseqgroup->data, will be almost always be 0 */ + pseqdesc->animindex);
	}

	return (mstudioanim_t *)((byte *)m_panimhdr[pseqdesc->seqgroup] + pseqdesc->animindex);
}

//============================================================
// From studio_render.cpp
//============================================================

void StudioModel::CalcSequenceBoneAngle( int frame, float s, const mstudiobone_t *pbone, const mstudioanim_t *panim, vec3_t angle1, vec3_t angle2 ) const
{
	int					j = 0, k = 0;
	mstudioanimvalue_t	*panimvalue = nullptr;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j+3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			k = frame;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * /*(double)*/pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * /*(double)*/pbone->scale[j+3];
		}
	}
}

void StudioModel::CalcSequenceBonePosition( int frame, float s, const mstudiobone_t *pbone, const mstudioanim_t *panim, vec3_t pos ) const
{
	int					j = 0, k = 0;
	mstudioanimvalue_t	*panimvalue = nullptr;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			
			k = frame;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0 - s) + s * panimvalue[k+2].value) * /*(double)*/pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * /*(double)*/pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * /*(double)*/pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * /*(double)*/pbone->scale[j];
				}
			}
		}
	}
}




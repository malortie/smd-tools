// test_smd_fix_uv.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdio>
#include <cstring>
#include <list>

#include "studiomdl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

void Error(const char* error, ...)
{
	va_list argptr;

	printf("\n************ ERROR ************\n");

	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);
	printf("\n");

	exit(1);
}

static char line[1024]{};
static int	linecount = 0;
static char texturenames[256][MAXSTUDIOSKINS];
static int numtextures = 0;

int lookup_texture(char* texturename)
{
	int i;

	for (i = 0; i < numtextures; i++) {
		if (stricmp(texturenames[i], texturename) == 0) {
			return i;
		}
	}

	strncpy(texturenames[i], texturename, sizeof(texturenames[i]));

	numtextures++;
	return i;
}

void Grab_Triangles(FILE* input, const char* filepath, s_model_s* pmodel)
{
	int		i, j;
	int		tcount = 0;
	int		ncount = 0;
	vec3_t	vmin;

	pmodel->trimesh = new s_triangle_s[MAXSTUDIOTRIANGLES];

	//
	// load the base triangles
	//
	while (1)
	{
		if (fgets(line, sizeof(line), input) != NULL)
		{
			char texturename[64]{};
			s_trianglevert_t* ptriv = nullptr;
			int bone;

			vec3_t vert[3]{};
			vec3_t norm[3]{};

			linecount++;

			// check for end
			if (strcmp("end\n", line) == 0)
				return;

			// strip off trailing smag
			strcpy(texturename, line);
			for (i = strlen(texturename) - 1; i >= 0 && !isgraph(texturename[i]); i--);
			texturename[i + 1] = '\0';

			if (texturename[0] == '\0')
			{
				// weird model problem, skip them
				fgets(line, sizeof(line), input);
				fgets(line, sizeof(line), input);
				fgets(line, sizeof(line), input);
				linecount += 3;
				continue;
			}
			


			s_triangle_s* ptriangle = &pmodel->trimesh[tcount];
			ptriangle->texture_id = lookup_texture(texturename);

			for (j = 0; j < 3; j++)
			{
				ptriv = &ptriangle->triverts[j];

				if (fgets(line, sizeof(line), input) != NULL)
				{
					s_vertex_t& p = ptriv->pos;
					s_normal_t& normal = ptriv->normal;

					linecount++;
					if (sscanf(line, "%d %f %f %f %f %f %f %f %f",
						&bone,
						&p.org[0], &p.org[1], &p.org[2],
						&normal.org[0], &normal.org[1], &normal.org[2],
						&ptriv->u, &ptriv->v) == 9)
					{
						if (bone < 0 || bone >= pmodel->numbones)
						{
							fprintf(stderr, "bogus bone index\n");
							fprintf(stderr, "%d %s :\n%s", linecount, filepath, line);
							exit(1);
						}

						/*
						if (ptriv->u > 2.0)
						{
							printf("%d %f\n", linecount, ptriv->u );
						}
						*/

						VectorCopy(p.org, vert[j]);
						VectorCopy(normal.org, norm[j]);

						p.bone = bone;
						normal.bone = bone;
						//normal.skinref = ptriangle->skinref;
					}
					else
					{
						Error("%s: error on line %d: %s", filepath, linecount, line);
					}
				}
			}

			//pmodel->trimap[tcount] = pmesh->numtris++;
			tcount++;
			pmodel->numtriangles = tcount;
		}
		else {
			break;
		}
	}

	if (ncount) printf("%d triangles with misdirected normals\n", ncount);

	if (vmin[2] != 0.0)
	{
		printf("lowest vector at %f\n", vmin[2]);
	}
}

void Grab_Skeleton(FILE* input, s_node_t* pnodes, s_bone_t* pbones)
{
	float x, y, z, xr, yr, zr;
	char cmd[1024];
	int index;

	while (fgets(line, sizeof(line), input) != NULL)
	{
		linecount++;
		if (sscanf(line, "%d %f %f %f %f %f %f", &index, &x, &y, &z, &xr, &yr, &zr) == 7)
		{
			pbones[index].pos[0] = x;
			pbones[index].pos[1] = y;
			pbones[index].pos[2] = z;
			pbones[index].rot[0] = xr;
			pbones[index].rot[1] = yr;
			pbones[index].rot[2] = zr;
		}
		else if (sscanf(line, "%s %d", cmd, &index))
		{
			if (strcmp(cmd, "time") == 0)
			{
				// pbones = pnode->bones[index] = kalloc(1, sizeof( s_bones_t ));
			}
			else if (strcmp(cmd, "end") == 0)
			{
				return;
			}
		}
	}
}



int Grab_Nodes(FILE* input, s_model_s* pmodel)
{
	int index;
	char name[1024]{};
	int parent;
	int numbones = 0;

	while (fgets(line, sizeof(line), input) != NULL)
	{
		linecount++;
		if (sscanf(line, "%d \"%[^\"]\" %d", &index, name, &parent) == 3)
		{
			// check for duplicated bones
			/*
			if (strlen(pnodes[index].name) != 0)
			{
				Error( "bone \"%s\" exists more than once\n", name );
			}
			*/

			strncpy(pmodel->node[index].name, name, sizeof(pmodel->node[index].name));
			pmodel->node[index].parent = parent;
			numbones = index;
		}
		else
		{
			return numbones + 1;
		}
	}
	Error("Unexpected EOF at line %d\n", linecount);
	return 0;
}

void Grab_Studio(FILE* input, const char* filepath, s_model_s* pmodel)
{
	char	cmd[1024] {};
	int		option;
	
	strncpy(pmodel->name, filepath, sizeof(pmodel->name));

	linecount = 0;

	while (fgets(line, sizeof(line), input) != NULL) {
		sscanf(line, "%s %d", cmd, &option);
		if (strcmp(cmd, "version") == 0) {
			if (option != 1) {
				throw;
			}
		}
		else if (strcmp(cmd, "nodes") == 0) {
			pmodel->numbones = Grab_Nodes(input, pmodel);
		}
		else if (strcmp(cmd, "skeleton") == 0) {
			Grab_Skeleton(input, pmodel->node, pmodel->skeleton);
		}
		else if (strcmp(cmd, "triangles") == 0) {
			Grab_Triangles(input, filepath, pmodel);
		}
		else
		{
			printf("unknown studio command\n");
		}
	}
	fclose(input);
}

void Open_Studio(const char* filepath, s_model_s* pmodel)
{
	FILE* input = nullptr;

	printf("grabbing %s\n", filepath);

	if ((input = fopen(filepath, "r")) == 0) {
		fprintf(stderr, "reader: could not open file '%s'\n", filepath);
	}
	linecount = 0;
	memset(line, 0, sizeof(line));

	Grab_Studio(input, filepath, pmodel);

	if (input)
	{
		fclose(input);
		input = nullptr;
	}
}

void Free_Studio(s_model_t* pmodel)
{
	if (pmodel->trimesh)
	{
		delete [] pmodel->trimesh;
		pmodel->trimesh = nullptr;
	}
}

s_bone_t* lookup_bone(const char* name, s_model_t* pmodel)
{
	for (int i = 0; i < pmodel->numbones; ++i)
	{
		if (0 == strnicmp(name, pmodel->node[i].name, sizeof(pmodel->node[i].name)))
		{
			return &pmodel->skeleton[i];
		}
	}

	return nullptr;
}

void Fix_Studio(s_model_t* modified, s_model_t* original)
{
#if 0
	if (modified->numbones != original->numbones)
		Error("modified has different bone count from original (modified: %d original: %d)\n", 
			modified->numbones, original->numbones);
#endif

	for (int i = 0; i < modified->numbones; ++i)
	{
		s_bone_t* original_bone = lookup_bone(modified->node[i].name, original);
		if (original_bone)
		{
			VectorCopy(original_bone->pos, modified->skeleton[i].pos);
			VectorCopy(original_bone->rot, modified->skeleton[i].rot);
		}
		else
		{
			printf("No match for bone %s in modified skeleton. Skipping...\n", modified->node[i].name);
		}
	}
}

void Write_Studio(const char* patched_file, s_model_t* modified)
{
	FILE* f = fopen(patched_file,"w");
	if (!f)
		throw;

	fprintf(f, "version %d\n", 1);

	fputs("nodes\n", f);
	for (int i = 0; i < modified->numbones; ++i)
	{
		fprintf(f, "%d \"%s\" %d\n",
			i,
			modified->node[i].name,
			modified->node[i].parent
		);
	}
	fputs("end\n", f);

	fputs("skeleton\n", f);
	fprintf(f, "time %d\n", 0); // Always time 0

	for (int j = 0; j < modified->numbones; ++j)
	{
		vec3_t& pos = modified->skeleton[j].pos;
		vec3_t& rot = modified->skeleton[j].rot;

		fprintf(f, "%d  %f %f %f  %f %f %f\n",
			j,
			pos[0], pos[1], pos[2],
			rot[0], rot[1], rot[2]
		);
	}
	fputs("end\n", f);

	fputs("triangles\n", f);

	for (int i = 0; i < modified->numtriangles; ++i)
	{
		s_triangle_s* ptriangle = &modified->trimesh[i];
		fprintf(f, "%s\n", texturenames[ptriangle->texture_id]);

		for (int j = 0; j < 3; ++j)
		{
			s_trianglevert_t* ptrivert = &ptriangle->triverts[j];

			fprintf(f, "%d  %f %f %f  %f %f %f  %f %f\n",
				ptrivert->pos.bone,
				ptrivert->pos.org[0], ptrivert->pos.org[1], ptrivert->pos.org[2],
				ptrivert->normal.org[0], ptrivert->normal.org[1], ptrivert->normal.org[2],
				ptrivert->u, ptrivert->v
			);
		}
	}

	fputs("end\n", f);

	if (f)
	{
		fclose(f);
		f = nullptr;
	}
}



struct FilePatch
{
	const char* original_reference;
	const char* modified_reference;
	const char* patched_reference;
};

#if 0

#pragma region LD_MODELS

#define ORIGINAL_HGRUNT_DIRECTORY "C:/Users/marc-/Bureau/massn_and_grunt/original/"
#define ORIGINAL_HGRUNT_MEDIC_DIRECTORY ORIGINAL_HGRUNT_DIRECTORY"hgrunt_medic/"
#define ORIGINAL_HGRUNT_OPFOR_DIRECTORY ORIGINAL_HGRUNT_DIRECTORY"hgrunt_opfor/"
#define ORIGINAL_HGRUNT_TORCH_DIRECTORY ORIGINAL_HGRUNT_DIRECTORY"hgrunt_torch/"

#define BASE_MODELSRC_DIRECTORY "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/"
#define BASE_MEDIC_DIRECTORY BASE_MODELSRC_DIRECTORY"hgrunt_medic/meshes/ld/"
#define BASE_OPFOR_DIRECTORY BASE_MODELSRC_DIRECTORY"hgrunt_opfor/meshes/ld/"
#define BASE_TORCH_DIRECTORY BASE_MODELSRC_DIRECTORY"hgrunt_torch/meshes/ld/"
#define BASE_GRUNT_SHARED_DIRECTORY "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/hgrunt_opfor/ld/"

#if 0

std::list<FilePatch> file_patches = {

	// HGRUNT_OPFOR
#if 1
	{	
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_commander_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_blk_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_commander_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_major_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_major_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_major_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_mask_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_mask_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_mask_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_MP_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_MP_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_MP_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_saw_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_blk_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_saw_wht_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_wht_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_wht_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_shotgun_reference_patched.smd"
	},
#endif
#if 0
	// torso
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_noback_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_noback_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_noback_torso_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_reg_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_reg_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_reg_torso_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_saw_gunner_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_saw_gunner_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_saw_gunner_torso_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_shotgun_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_shotgun_torso_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_shotgun_torso_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"MP5_reference.smd",
		BASE_OPFOR_DIRECTORY"MP5_reference.smd",
		BASE_OPFOR_DIRECTORY"MP5_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"SAW_reference.smd",
		BASE_OPFOR_DIRECTORY"SAW_reference.smd",
		BASE_OPFOR_DIRECTORY"SAW_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"shotgun_reference_patched.smd"
	},
#endif

	// HGRUNT_MEDIC
#if 0
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_gunholster_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_gunholster_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_gunholster_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"glock_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_reference_patched.smd"
	},
#endif
#if 1
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"grunt_medic_head_wht_reference.smd",
		BASE_MEDIC_DIRECTORY"grunt_medic_head_reference.smd",
		BASE_MEDIC_DIRECTORY"grunt_medic_head_reference_patched.smd"
	},
#endif
#if 0 //----
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"grunt_medic_torso_reference.smd",
		BASE_MEDIC_DIRECTORY"grunt_medic_torso_reference.smd",
		BASE_MEDIC_DIRECTORY"grunt_medic_torso_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"hypodermic_reference.smd",
		BASE_MEDIC_DIRECTORY"hypodermic_reference.smd",
		BASE_MEDIC_DIRECTORY"hypodermic_reference_patched.smd"
	},
#endif

	// HGRUNT_TORCH
#if 1
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_head_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_head_reference_patched.smd"
	},
#endif
#if 0 //----
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torso_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torso_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_torch_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torch_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torch_reference_patched.smd"
	},
#endif

#if 0
	// SHARED MESHES
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_fatigues_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_fatigues_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_fatigues_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_gunholster_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_gunholster_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_gunholster_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"Desert_Eagle_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"Desert_Eagle_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"Desert_Eagle_reference_patched.smd"
	}
#endif
};
#endif

#pragma endregion

#endif

#pragma region HD_MODELS

#define ORIGINAL_HGRUNT_DIRECTORY "C:/Users/marc-/Bureau/massn_and_grunt/original_hd/"
#define ORIGINAL_HGRUNT_MEDIC_DIRECTORY ORIGINAL_HGRUNT_DIRECTORY"hgrunt_medic/"
#define ORIGINAL_HGRUNT_OPFOR_DIRECTORY ORIGINAL_HGRUNT_DIRECTORY"hgrunt_opfor/"
#define ORIGINAL_HGRUNT_TORCH_DIRECTORY ORIGINAL_HGRUNT_DIRECTORY"hgrunt_torch/"

#define BASE_MODELSRC_DIRECTORY "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/"
#define BASE_MEDIC_DIRECTORY BASE_MODELSRC_DIRECTORY"hgrunt_medic/meshes/hd/"
#define BASE_OPFOR_DIRECTORY BASE_MODELSRC_DIRECTORY"hgrunt_opfor/meshes/hd/"
#define BASE_TORCH_DIRECTORY BASE_MODELSRC_DIRECTORY"hgrunt_torch/meshes/hd/"
#define BASE_GRUNT_SHARED_DIRECTORY "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/hgrunt_opfor/hd/"

#if 1

std::list<FilePatch> file_patches = {

	// HGRUNT_OPFOR
#if 0
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_commander_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_blk_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_commander_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_commander_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_major_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_major_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_major_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_mask_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_mask_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_mask_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_MP_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_MP_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_MP_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_saw_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_blk_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_blk_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_saw_wht_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_wht_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_saw_wht_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"grunt_head_shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"grunt_head_shotgun_reference_patched.smd"
	},
#endif
#if 0
	// torso
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"torso_noback_reference.smd",
		BASE_OPFOR_DIRECTORY"torso_noback_reference.smd",
		BASE_OPFOR_DIRECTORY"torso_noback_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"torso_regular_reference.smd",
		BASE_OPFOR_DIRECTORY"torso_regular_reference.smd",
		BASE_OPFOR_DIRECTORY"torso_regular_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"torso_saw_reference.smd",
		BASE_OPFOR_DIRECTORY"torso_saw_reference.smd",
		BASE_OPFOR_DIRECTORY"torso_saw_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"MP5_reference.smd",
		BASE_OPFOR_DIRECTORY"MP5_reference.smd",
		BASE_OPFOR_DIRECTORY"MP5_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"SAW_reference.smd",
		BASE_OPFOR_DIRECTORY"SAW_reference.smd",
		BASE_OPFOR_DIRECTORY"SAW_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"shotgun_reference.smd",
		BASE_OPFOR_DIRECTORY"shotgun_reference_patched.smd"
	},
#endif

	// HGRUNT_MEDIC
#if 0
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_gunholster_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_gunholster_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_gunholster_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"glock_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_reference.smd",
		BASE_MEDIC_DIRECTORY"glock_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"grunt_medic_head_wht_reference.smd",
		BASE_MEDIC_DIRECTORY"grunt_medic_head_reference.smd",
		BASE_MEDIC_DIRECTORY"grunt_medic_head_reference_patched.smd"
	},
#endif
#if 0 //----
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"torso_medic_reference.smd",
		BASE_MEDIC_DIRECTORY"torso_medic_reference.smd",
		BASE_MEDIC_DIRECTORY"torso_medic_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_MEDIC_DIRECTORY"hypodermic_reference.smd",
		BASE_MEDIC_DIRECTORY"hypodermic_reference.smd",
		BASE_MEDIC_DIRECTORY"hypodermic_reference_patched.smd"
	},
#endif

	// HGRUNT_TORCH
#if 0
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_head_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_head_reference_patched.smd"
	},
#endif
#if 0 //----
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torso_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torso_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_torch_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torch_reference.smd",
		BASE_TORCH_DIRECTORY"engineer_torch_reference_patched.smd"
	},
#endif

#if 1
	// SHARED MESHES
	{
		ORIGINAL_HGRUNT_OPFOR_DIRECTORY"fatigues_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_fatigues_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_fatigues_reference_patched.smd"
	},
#endif
#if 0
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"engineer_gunholster_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_gunholster_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"grunt_gunholster_reference_patched.smd"
	},
	{
		ORIGINAL_HGRUNT_TORCH_DIRECTORY"Desert_Eagle_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"Desert_Eagle_reference.smd",
		BASE_GRUNT_SHARED_DIRECTORY"Desert_Eagle_reference_patched.smd"
	}
#endif

};
#endif

#pragma endregion


int main()
{
	for (auto it = file_patches.begin(); it != file_patches.end(); ++it)
	{
		s_model_t pmodel_modified{}, pmodel_original{};

		try
		{
			Open_Studio(it->modified_reference, &pmodel_modified);
			Open_Studio(it->original_reference, &pmodel_original);

			Fix_Studio(&pmodel_modified, &pmodel_original);

			Write_Studio(it->patched_reference, &pmodel_modified);

			Free_Studio(&pmodel_modified);
			Free_Studio(&pmodel_original);

			int a = 2;
			a++;
		}
		catch (...)
		{
			int a = 2;
			a++;
		}
	}

	return 0;
}

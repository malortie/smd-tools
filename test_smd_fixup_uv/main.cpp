// test_smd_fix_uv.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdio>
#include <cstring>
#include <list>

enum CopyFlags
{
	BONE_ID = (1 << 0),

	POSITION_X = (1 << 1),
	POSITION_Y = (1 << 2),
	POSITION_Z = (1 << 3),

	ROTATION_X = (1 << 4),
	ROTATION_Y = (1 << 5),
	ROTATION_Z = (1 << 6),

	UV_U = (1 << 7),
	UV_V = (1 << 8),

	TEXTURE_NAME = (1 << 9),

	ALL_POSITIONS = (POSITION_X | POSITION_Y | POSITION_Z),
	ALL_ROTATIONS = (ROTATION_X | ROTATION_Y | ROTATION_Z),
	ALL_UVS = (UV_U | UV_V),
};

struct LineRange
{
	int line_start;
	int line_end;

	CopyFlags copy_flags;
};


#if 1
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/meshes/BARNEY-X_Template_Biped(Gun)1 - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/meshes/BARNEY-X_Template_Biped(Gun)1_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/meshes/BARNEY-X_Template_Biped(Gun)1_patched.smd"

std::list<LineRange> file_patches = {
	{ 0, 376, CopyFlags(BONE_ID | TEXTURE_NAME | ALL_POSITIONS | ALL_ROTATIONS | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/meshes/BARNEY-X_Template_Biped(Gun-Hlstr)1 - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/meshes/BARNEY-X_Template_Biped(Gun-Hlstr)1_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/meshes/BARNEY-X_Template_Biped(Gun-Hlstr)1_patched.smd"

std::list<LineRange> file_patches = {
	{ 0, 312, CopyFlags(BONE_ID | TEXTURE_NAME | ALL_POSITIONS | ALL_ROTATIONS | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/otis/meshes/otis_reference_wgun_original.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/otis/meshes/otis_reference_wgun_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/otis/meshes/otis_reference_wgun_patched.smd"


std::list<LineRange> file_patches = {
	{ 0, 406, CopyFlags(BONE_ID | TEXTURE_NAME | ALL_POSITIONS | ALL_ROTATIONS | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/osprey/meshes/V2_osprey_reference_modified - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/osprey/meshes/V2_osprey_reference_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/osprey/meshes/V2_osprey_reference_patched.smd"

std::list<LineRange> file_patches = {
	{ 44, 2896, CopyFlags(BONE_ID | TEXTURE_NAME | ALL_POSITIONS | ALL_ROTATIONS | ALL_UVS) },
};
#if 0
std::list<LineRange> file_patches = {
	{ 44, 2864, CopyFlags(BONE_ID | TEXTURE_NAME | ALL_POSITIONS | ALL_ROTATIONS | ALL_UVS) },
	//{ 44, 2872, CopyFlags(TEXTURE_NAME | ALL_UVS) },
};
#endif
#endif
#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/osprey/meshes/V2_osprey_reference - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/osprey/meshes/V2_osprey_reference_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/osprey/meshes/V2_osprey_reference_patched.smd"

std::list<LineRange> file_patches = {
	{ 44, 2876, CopyFlags(TEXTURE_NAME | ALL_UVS) },
};
#if 0
std::list<LineRange> file_patches = {
	{ 44, 2876, CopyFlags(TEXTURE_NAME | ALL_POSITIONS | ALL_ROTATIONS) },
};
#endif
#endif
#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/meshes/hd/dc_gman_reference - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/meshes/hd/dc_gman_reference_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/meshes/hd/dc_gman_reference_patched.smd"

std::list<LineRange> file_patches = {
	{ 4204, 5344, CopyFlags(ALL_ROTATIONS) },
};
#endif
#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/animations/hd/push_button - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/animations/hd/push_button_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/animations/hd/push_button_patched.smd"

std::list<LineRange> file_patches = {
	{ 96, 96 +3, CopyFlags(ALL_ROTATIONS) },
	{ 152, 152 +3, CopyFlags(ALL_ROTATIONS) },
	{ 208, 208 +3, CopyFlags(ALL_ROTATIONS) },
	{ 264, 264 +3, CopyFlags(ALL_ROTATIONS) },
	{ 320, 320 +3, CopyFlags(ALL_ROTATIONS) },
	{ 376, 376 +3, CopyFlags(ALL_ROTATIONS) },
	{ 432, 432 +3, CopyFlags(ALL_ROTATIONS) },
	{ 488, 488 +3, CopyFlags(ALL_ROTATIONS) },
	{ 544, 544 +3, CopyFlags(ALL_ROTATIONS) },
	{ 600, 600 +3, CopyFlags(ALL_ROTATIONS) },
	{ 656, 656 +3, CopyFlags(ALL_ROTATIONS) },
	{ 712, 712 +3, CopyFlags(ALL_ROTATIONS) },
	{ 768, 768 +3, CopyFlags(ALL_ROTATIONS) },
	{ 824, 824 +3, CopyFlags(ALL_ROTATIONS) },
	{ 880, 880 +3, CopyFlags(ALL_ROTATIONS) },
	{ 936, 936 +3, CopyFlags(ALL_ROTATIONS) },
	{ 992, 992 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1048, 1048 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1104, 1104 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1160, 1160 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1216, 1216 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1272, 1272 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1328, 1328 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1384, 1384 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1440, 1440 +3, CopyFlags(ALL_ROTATIONS) },
	{ 1496, 1496 +3, CopyFlags(ALL_ROTATIONS) },
};
#endif
#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/meshes/ld/gman_reference - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/meshes/ld/gman_reference_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/meshes/ld/gman_reference_patched.smd"

std::list<LineRange> file_patches = {
	{ 2340, 2372, CopyFlags(ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/SCI3_Template_Biped1(Headless_Body) - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/SCI3_Template_Biped1(Headless_Body)_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/SCI3_Template_Biped1(Headless_Body)_patched.smd"

std::list<LineRange> file_patches = {
	{ 924, 1000, CopyFlags(ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/ld/bshift/SCI2_Template_Biped1(EinstienHead) - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/ld/bshift/SCI2_Template_Biped1(EinstienHead)_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/ld/bshift/SCI2_Template_Biped1(EinstienHead)_patched.smd"

std::list<LineRange> file_patches = {
	{ 568, 680, CopyFlags(TEXTURE_NAME | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/bshift/SCI2_Template_Biped1(EinstienHead) - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/bshift/SCI2_Template_Biped1(EinstienHead)_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/bshift/SCI2_Template_Biped1(EinstienHead)_patched.smd"

std::list<LineRange> file_patches = {
	{ 852, 964, CopyFlags(TEXTURE_NAME | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/ld/SCI2_Template_Biped1(EinstienHead) - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/ld/SCI2_Template_Biped1(EinstienHead)_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/ld/SCI2_Template_Biped1(EinstienHead)_patched.smd"

std::list<LineRange> file_patches = {
	{ 560, 672, CopyFlags(TEXTURE_NAME | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/SCI2_Template_Biped1(EinstienHead) - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/SCI2_Template_Biped1(EinstienHead)_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/shared/meshes/scientists/ld/SCI2_Template_Biped1(EinstienHead)_patched.smd"

std::list<LineRange> file_patches = {
	{ 816, 928, CopyFlags(TEXTURE_NAME | ALL_UVS) },
};
#endif

#if 0
#define ORIGINAL_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/hd/dc_sci_(head_LUTHER) - Copy.smd"
#define MODIFIED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/hd/dc_sci_(head_LUTHER)_modified.smd"
#define PATCHED_FILE	"C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/cleansuit_scientist/meshes/hd/dc_sci_(head_LUTHER)_patched.smd"

std::list<LineRange> file_patches = {
	{ 56, 1244, CopyFlags(ALL_POSITIONS) },
};
#endif


//constexpr int LINE_RANGE_START = 180;
//constexpr int LINE_RANGE_END = 608;

int main()
{
	FILE* fp_orig = nullptr;
	FILE* fp_modified = nullptr;
	FILE* fp_patched = nullptr;

	try
	{
		if (0 != fopen_s(&fp_orig, ORIGINAL_FILE, "r"))
			throw;
		if (0 != fopen_s(&fp_modified, MODIFIED_FILE, "r"))
			throw;

		fseek(fp_orig, 0, SEEK_SET);
		fseek(fp_modified, 0, SEEK_SET);

		long orig_line_count = 1;
		long modified_line_count = 1;

		char szTemp[1024]{};
		while (fgets(szTemp, sizeof(szTemp), fp_orig)) {
			//puts(szTemp);
			++orig_line_count;
		}

		while (fgets(szTemp, sizeof(szTemp), fp_modified)) {
			//puts(szTemp);
			++modified_line_count;
		}

		if (orig_line_count != modified_line_count)
		{
			printf("original file and modified file have different line count (%d) vs (%d)\n",
				orig_line_count, modified_line_count);
			throw;
		}

		// Reset file pointers to start of file
		fseek(fp_orig, 0, SEEK_SET);
		fseek(fp_modified, 0, SEEK_SET);

		// Open the patch file
		if (0 != fopen_s(&fp_patched, PATCHED_FILE, "w"))
			throw;

		long line = 1;

		char szOrigLine[1024]{};
		char szModifiedLine[1024]{};

		for (auto it_patch = file_patches.begin(); it_patch != file_patches.end(); ++it_patch)
		{
			const int LINE_RANGE_START = (*it_patch).line_start;
			const int LINE_RANGE_END = (*it_patch).line_end;

			// Reach the line we want to start from.
			while (line < LINE_RANGE_START)
			{
				if (fgets(szOrigLine, sizeof(szOrigLine), fp_orig) == NULL)
					throw; // An error occured. Exit
				if (fgets(szModifiedLine, sizeof(szModifiedLine), fp_modified) == NULL)
					throw; // An error occured. Exit
				++line;

				// Write the original file lines to the patched file.
				fputs(szOrigLine, fp_patched);

				puts(szOrigLine);
			}

			int orig_bone;
			float orig_pos[3];
			float orig_rot[3];
			float orig_uv[2];

			int modified_bone;
			float modified_pos[3];
			float modified_rot[3];
			float modified_uv[2];

			int output_bone;
			float output_pos[3];
			float output_rot[3];
			float output_uv[2];

			// Write the patched lines
			while (line < LINE_RANGE_END)
			{
				if (fgets(szOrigLine, sizeof(szOrigLine), fp_orig) == NULL)
					throw; // An error occured. Exit
				if (fgets(szModifiedLine, sizeof(szModifiedLine), fp_modified) == NULL)
					throw; // An error occured. Exit

				++line;

				// If it's a mesh triangles
				if (
					sscanf_s(szOrigLine, "%d %f %f %f %f %f %f %f %f",
						&orig_bone,
						&orig_pos[0], &orig_pos[1], &orig_pos[2],
						&orig_rot[0], &orig_rot[1], &orig_rot[2],
						&orig_uv[0], &orig_uv[1]) == 9
					&&
					sscanf_s(szModifiedLine, "%d %f %f %f %f %f %f %f %f",
						&modified_bone,
						&modified_pos[0], &modified_pos[1], &modified_pos[2],
						&modified_rot[0], &modified_rot[1], &modified_rot[2],
						&modified_uv[0], &modified_uv[1]) == 9
					)
				{
					output_bone = (0 != ((*it_patch).copy_flags & CopyFlags::BONE_ID)) ? modified_bone : orig_bone;

					output_pos[0] = (0 != ((*it_patch).copy_flags & CopyFlags::POSITION_X)) ? modified_pos[0] : orig_pos[0];
					output_pos[1] = (0 != ((*it_patch).copy_flags & CopyFlags::POSITION_Y)) ? modified_pos[1] : orig_pos[1];
					output_pos[2] = (0 != ((*it_patch).copy_flags & CopyFlags::POSITION_Z)) ? modified_pos[2] : orig_pos[2];

					output_rot[0] = (0 != ((*it_patch).copy_flags & CopyFlags::ROTATION_X)) ? modified_rot[0] : orig_rot[0];
					output_rot[1] = (0 != ((*it_patch).copy_flags & CopyFlags::ROTATION_Y)) ? modified_rot[1] : orig_rot[1];
					output_rot[2] = (0 != ((*it_patch).copy_flags & CopyFlags::ROTATION_Z)) ? modified_rot[2] : orig_rot[2];

					output_uv[0] = (0 != ((*it_patch).copy_flags & CopyFlags::UV_U)) ? modified_uv[0] : orig_uv[0];
					output_uv[1] = (0 != ((*it_patch).copy_flags & CopyFlags::UV_V)) ? modified_uv[1] : orig_uv[1];

					fprintf(fp_patched, "%3d %f %f %f %f %f %f %f %f\n",
						output_bone,
						output_pos[0], output_pos[1], output_pos[2],
						output_rot[0], output_rot[1], output_rot[2],
						output_uv[0], output_uv[1]
					);
					printf("%d %f %f %f %f %f %f %f %f\n",
						output_bone,
						output_pos[0], output_pos[1], output_pos[2],
						output_rot[0], output_rot[1], output_rot[2],
						output_uv[0], output_uv[1]);
				}
				// If it's an animation (No UV)
				else if (
					sscanf_s(szOrigLine, "%d %f %f %f %f %f %f",
						&orig_bone,
						&orig_pos[0], &orig_pos[1], &orig_pos[2],
						&orig_rot[0], &orig_rot[1], &orig_rot[2],
						&orig_uv[0], &orig_uv[1]) == 7
					&&
					sscanf_s(szModifiedLine, "%d %f %f %f %f %f %f",
						&modified_bone,
						&modified_pos[0], &modified_pos[1], &modified_pos[2],
						&modified_rot[0], &modified_rot[1], &modified_rot[2],
						&modified_uv[0], &modified_uv[1]) == 7
					)
				{
					output_bone = (0 != ((*it_patch).copy_flags & CopyFlags::BONE_ID)) ? modified_bone : orig_bone;

					output_pos[0] = (0 != ((*it_patch).copy_flags & CopyFlags::POSITION_X)) ? modified_pos[0] : orig_pos[0];
					output_pos[1] = (0 != ((*it_patch).copy_flags & CopyFlags::POSITION_Y)) ? modified_pos[1] : orig_pos[1];
					output_pos[2] = (0 != ((*it_patch).copy_flags & CopyFlags::POSITION_Z)) ? modified_pos[2] : orig_pos[2];

					output_rot[0] = (0 != ((*it_patch).copy_flags & CopyFlags::ROTATION_X)) ? modified_rot[0] : orig_rot[0];
					output_rot[1] = (0 != ((*it_patch).copy_flags & CopyFlags::ROTATION_Y)) ? modified_rot[1] : orig_rot[1];
					output_rot[2] = (0 != ((*it_patch).copy_flags & CopyFlags::ROTATION_Z)) ? modified_rot[2] : orig_rot[2];

					fprintf(fp_patched, "%3d %f %f %f %f %f %f\n",
						output_bone,
						output_pos[0], output_pos[1], output_pos[2],
						output_rot[0], output_rot[1], output_rot[2]
					);
					printf("%d %f %f %f %f %f %f\n",
						output_bone,
						output_pos[0], output_pos[1], output_pos[2],
						output_rot[0], output_rot[1], output_rot[2]
					);
				}
				else
				{
					// It's a texture name.
					if (0 != ((*it_patch).copy_flags & CopyFlags::TEXTURE_NAME))
					{
						fputs(szModifiedLine, fp_patched);
						puts(szModifiedLine);
					}
					else
					{
						fputs(szOrigLine, fp_patched);
						puts(szOrigLine);
					}
				}
			}

			int a = 2;
			a++;
		}

		// Write the rest of the original lines to the patch file.
		while (fgets(szOrigLine, sizeof(szOrigLine), fp_orig) != NULL)
		{
			++line;

			fputs(szOrigLine, fp_patched);
			puts(szOrigLine);
		}

		int a = 2;
		a++;
	}
	catch (...)
	{
		int a = 0;
		a++;
	}

	if (fp_orig)
	{
		fclose(fp_orig);
		fp_orig = nullptr;
	}
	if (fp_modified)
	{
		fclose(fp_modified);
		fp_modified = nullptr;
	}
	if (fp_patched)
	{
		fclose(fp_patched);
		fp_patched = nullptr;
	}

	return 0;
}


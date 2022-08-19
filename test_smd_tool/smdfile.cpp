
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <sys/types.h>
#include <sys/stat.h>
#include <Windows.h>
#include <filesystem>

#include "smdfile.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtx/handed_coordinate_space.hpp"
#include "glm/gtx/matrix_cross_product.hpp"
#include "glm/gtx/matrix_operation.hpp"
#include "glm/gtx/rotate_normalized_axis.hpp"
#include "glm/gtx/projection.hpp"
#include "glm/gtx/intersect.hpp"

#define DEBUG_MESSAGES 0

void Error (const char *error, ...)
{
	va_list argptr;

	printf ("\n************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

int	FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

char	filename[1024]{};
FILE* input = nullptr;
char	line[1024]{};
int		linecount = 0;

void clip_rotations( glm::vec3& rot )
{
	int j;
	// clip everything to : -Q_PI <= x < Q_PI

	for (j = 0; j < 3; j++) {
		while (rot[j] > M_PI)
			rot[j] -= M_PI*2;
		while (rot[j] < -M_PI)
			rot[j] += M_PI*2;
	}
}

void make_zero_positive(glm::vec3 &v)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (abs(v[i]) == 0.0f)
			v[i] = abs(v[i]);
	}
}

inline void extract_euler_angles_from_matrix(const glm::mat4& m, glm::vec3& anglesPitchYawRoll)
{
	glm::extractEulerAngleZYX(m, anglesPitchYawRoll[2], anglesPitchYawRoll[1], anglesPitchYawRoll[0]);
}

inline void extract_position_from_matrix(const glm::mat4& m, glm::vec3& pos)
{
	pos = m[3];
}


inline glm::mat4 create_rotation_matrix(const glm::vec3& anglesPitchYawRoll)
{
	glm::mat4 xRot = glm::rotate(glm::identity<glm::mat4>(), anglesPitchYawRoll[0], glm::vec3(1,0,0));
	glm::mat4 yRot = glm::rotate(glm::identity<glm::mat4>(), anglesPitchYawRoll[1], glm::vec3(0,1,0));
	glm::mat4 zRot = glm::rotate(glm::identity<glm::mat4>(), anglesPitchYawRoll[2], glm::vec3(0,0,1));

	return zRot * yRot * xRot;
}


void Grab_Nodes( std::vector<s_node_t>& nodes )
{
	int index;
	char name[1024];
	int parent;
	int i;

	while (fgets( line, sizeof( line ), input ) != NULL) 
	{
		linecount++;
		if (sscanf( line, "%d \"%[^\"]\" %d", &index, name, &parent ) == 3)
		{
			nodes.push_back({});

			nodes[index].index = index;
			nodes[index].name = name;
			nodes[index].parent = parent;
			if (parent != -1)
			{
				nodes[parent].children.push_back(index);
			}
		}
		else 
		{
			return;
		}
	}
	Error( "Unexpected EOF at line %d\n", linecount );
	return;
}

void Grab_Animation( s_animation_t & anim )
{
	glm::vec3 pos;
	glm::vec3 rot;
	char cmd[1024];
	int index = 0;
	int	t = -99999999;
	int start = 99999;
	int end = 0;

	while (fgets( line, sizeof( line ), input ) != NULL) 
	{
		linecount++;
		if (sscanf( line, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &rot[0], &rot[1], &rot[2] ) == 7)
		{
			clip_rotations(rot);

			/*
					 x         y        z
					roll      pitch     yaw
-0.221744 -0.803308 36.328743 0.000000 -0.026135 -1.570888
*/
//local_transform.m = create_rotation_matrix(glm::vec3(rot[1], rot[2], rot[0]));


			auto& local_transform = anim.frames.back().entries[index].local_transform;
			local_transform = create_rotation_matrix(rot);
			local_transform[3] = glm::vec4(pos, 1.0);
		}
		else if (sscanf( line, "%s %d", cmd, &index ))
		{
			if (strcmp( cmd, "time" ) == 0) 
			{
				t = index;
				anim.frames.push_back({});
				anim.frames.back().entries.resize(anim.nodes.size());
			}
			else if (strcmp( cmd, "end") == 0) 
			{
				// Build bone world transform.
				SMDHelper::BuildAnimationWorldTransform(anim);

				return;
			}
			else
			{
				Error( "Error(%d) : %s", linecount, line );
			}
		}
		else
		{
			Error( "Error(%d) : %s", linecount, line );
		}
	}
	Error( "unexpected EOF: %s\n", anim.name.c_str() );
}

void Option_Animation ( const char *file_path, s_animation_t& anim )
{
	int		time1;
	char	cmd[1024]{};
	int		option;

	char name[MAX_PATH]{};
	strcpy(name, std::filesystem::path(file_path).filename().string().c_str());

	anim.name = name;

	sprintf (filename, "%s", file_path);

	time1 = FileTime (filename);
	if (time1 == -1)
		Error ("%s doesn't exist", filename);

	printf ("grabbing %s\n", filename);

	if ((input = fopen(filename, "r")) == 0) {
		fprintf(stderr,"reader: could not open file '%s'\n", filename);
		Error(0);
	}
	linecount = 0;

	while (fgets( line, sizeof( line ), input ) != NULL) {
		linecount++;
		sscanf( line, "%s %d", cmd, &option );
		if (strcmp( cmd, "version" ) == 0) {
			if (option != 1) {
				Error("bad version\n");
			}
		}
		else if (strcmp( cmd, "nodes" ) == 0) {
			Grab_Nodes( anim.nodes );
		}
		else if (strcmp( cmd, "skeleton" ) == 0) {
			Grab_Animation( anim );
		}
		else 
		{
			printf("unknown studio command : %s\n", cmd );
			while (fgets( line, sizeof( line ), input ) != NULL) {
				linecount++;
				if (strncmp(line,"end",3)==0)
					break;
			}
		}
	}
	fclose( input );
}

void SMDFileLoader::LoadAnimation(const char* file_path, s_animation_t& anim) const
{
	Option_Animation(file_path, anim);
}

void SMDSerializer::WriteAnimation(const s_animation_t& anim, const char* output_path) const
{
	FILE* fp = nullptr;
    if (fopen_s(&fp, output_path, "w") != 0)
        throw;

    fprintf(fp, "version %d\n", SMD_VERSION);
    fputs("nodes\n", fp);

	for (int i = 0; i < anim.nodes.size(); ++i)
	{
		fprintf(fp, "%3d \"%s\" %d\n",
			i,
			anim.nodes[i].name.c_str(),
			anim.nodes[i].parent
		);
	}

    fputs("end\n", fp);
    fputs("skeleton\n", fp);

	for (int t = 0; t < anim.frames.size(); ++t)
    {
		fprintf(fp, "time %d\n", t);

        for (int i = 0; i < anim.frames[t].entries.size(); ++i)
        {
			const auto& local_m = anim.frames[t].entries[i].local_transform;

			glm::vec3 rot, pos;
			extract_euler_angles_from_matrix(local_m, rot);
			extract_position_from_matrix(local_m, pos);
			make_zero_positive(pos);
			make_zero_positive(rot);

			fprintf(fp, "%3d   %f %f %f %f %f %f\n",
				i,
				pos[0], pos[1], pos[2],
				rot[0], rot[1], rot[2]
			);
        }
    }

    fputs("end\n", fp);
    fclose(fp);
    fp = nullptr;
}

void SMDSerializer::WriteOBJ(const s_animation_t& anim, const char* output_path) const
{
	FILE* fp = nullptr;
	if (fopen_s(&fp, output_path, "w") != 0)
		throw;

	const auto& frame = anim.frames[0];

	int num_vertices = 0;
	int skeleton_vertex_base = 1;

	// Skeleton with lines
	fprintf(fp, "o objSkeleton\n\n");

	for (int i = 0; i < frame.entries.size(); ++i)
	{
		const auto& world_m = frame.entries[i].world_transform;

		glm::vec3 rot, pos;
		extract_euler_angles_from_matrix(world_m, rot);
		extract_position_from_matrix(world_m, pos);

		fprintf(fp, "v %f %f %f\n", pos[0], pos[1], pos[2]);
		++num_vertices;
	}

	fprintf(fp, "\n");

	for (int i = 0; i < anim.nodes.size(); ++i)
	{
		const auto& n = anim.nodes[i];

		if (n.parent != -1)
		{
			fprintf(fp, "l %d %d\n", skeleton_vertex_base + n.parent, skeleton_vertex_base + n.index);
		}
	}

	fprintf(fp, "\n");

	// matrices axes

	fprintf(fp, "o objMatrices\n\n");

	const char* axis_group_name[3] = { 
		"Forward", "Right", "Up"
	};

	for (int v = 0; v < 3; ++v)
	{
		fprintf(fp, "g grp%s\n\n", axis_group_name[v]);

		int axis_vertex_base = num_vertices + 1;

		for (int i = 0; i < frame.entries.size(); ++i)
		{
			const auto& world_m = frame.entries[i].world_transform;
			glm::vec3 pos;
			glm::vec3 dir = glm::normalize(glm::vec3(world_m[v]));
			extract_position_from_matrix(world_m, pos);

			glm::vec3 axis_end = pos + dir * 1.0f;

			fprintf(fp, "v %f %f %f\n", axis_end[0], axis_end[1], axis_end[2]);
			++num_vertices;
		}

		fprintf(fp, "\n");

		for (int i = 0; i < frame.entries.size(); ++i)
		{
			const auto& n = anim.nodes[i];

			fprintf(fp, "l %d %d\n", skeleton_vertex_base + n.index, axis_vertex_base + n.index);
		}
	}

	fprintf(fp, "\n");


	fclose(fp);
	fp = nullptr;
}


int SMDHelper::FindNodeByName(const std::vector<s_node_t>& nodes, const char* name)
{
	for (int i = 0; i < nodes.size(); ++i)
	{
		if (!_stricmp(nodes[i].name.c_str(), name))
		{
			return i;
		}
	}

	return -1;
}

void SMDHelper::BuildAnimationWorldTransform(s_animation_t& anim)
{
	for (int t = 0; t < anim.frames.size(); ++t)
	{
		for (int i = 0; i < anim.nodes.size(); ++i)
		{
			auto& node = anim.frames[t].entries[i];

			if (anim.nodes[i].parent == -1)
			{
				node.world_transform = node.local_transform;
			}
			else
			{
				const auto& parent_node = anim.frames[t].entries[anim.nodes[i].parent];
				node.world_transform = parent_node.world_transform * node.local_transform;
			}
		}
	}
}


void SMDHelper::UpdateBoneHierarchyLocalTransformFromWorldTransform(s_animation_t& anim, int bone, int frame)
{
	//printf("Computing bone %d local transform\n", bone);

	auto& node = anim.frames[frame].entries[bone];

	if (anim.nodes[bone].parent == -1)
	{
		node.local_transform = node.world_transform;
	}
	else
	{
		const auto& parent_node = anim.frames[frame].entries[anim.nodes[bone].parent];
		node.local_transform = glm::inverse(parent_node.world_transform) * node.world_transform;
	}

	for(auto& child : anim.nodes[bone].children)
		UpdateBoneHierarchyLocalTransformFromWorldTransform(anim, child, frame);
}

void SMDHelper::UpdateBoneHierarchyLocalTransformFromWorldTransform(s_animation_t& anim, int bone)
{
	for (int t = 0; t < anim.frames.size(); ++t)
		UpdateBoneHierarchyLocalTransformFromWorldTransform(anim, bone, t);
}

void SMDHelper::UpdateBoneHierarchyWorldTransformFromLocalTransform(s_animation_t& anim, int bone, int frame)
{
	//printf("Computing bone %d local transform\n", bone);

	auto& node = anim.frames[frame].entries[bone];

	if (anim.nodes[bone].parent == -1)
	{
		node.world_transform = node.local_transform;
	}
	else
	{
		const auto& parent_node = anim.frames[frame].entries[anim.nodes[bone].parent];
		node.world_transform = parent_node.world_transform * node.local_transform;
	}

	for (auto& child : anim.nodes[bone].children)
		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, child, frame);
}

void SMDHelper::UpdateBoneHierarchyWorldTransformFromLocalTransform(s_animation_t& anim, int bone)
{
	for (int t = 0; t < anim.frames.size(); ++t)
		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, bone, t);
}

void SMDHelper::RotateBoneInWorldSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& angles)
{
	auto& node = anim.nodes[bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entry = anim.frames[t].entries[bone];

		glm::mat4 local_matrix;

		if (node.parent == -1)
		{
			local_matrix = frame_entry.world_transform;
		}
		else
		{
			const auto& frame_parent_node = anim.frames[t].entries[node.parent];
			local_matrix = glm::inverse(frame_parent_node.world_transform) * frame_entry.world_transform;
		}

		glm::mat4 r_x = glm::rotate(glm::identity<glm::mat4>(), angles[0], glm::normalize(glm::vec3(local_matrix[0])));
		glm::mat4 r_y = glm::rotate(glm::identity<glm::mat4>(), angles[1], glm::normalize(glm::vec3(local_matrix[1])));
		glm::mat4 r_z = glm::rotate(glm::identity<glm::mat4>(), angles[2], glm::normalize(glm::vec3(local_matrix[2])));
		glm::mat4 rotated_local_matrix = r_z * r_y * r_x * local_matrix;
		rotated_local_matrix[3] = glm::vec4(glm::vec3(local_matrix[3]), 1.0f);

		if (node.parent == -1)
		{
			frame_entry.world_transform = rotated_local_matrix;
		}
		else
		{
			// Set new worldspace bone matrix.
			const auto& frame_parent_node = anim.frames[t].entries[node.parent];
			frame_entry.world_transform = frame_parent_node.world_transform * rotated_local_matrix;
		}

		UpdateBoneHierarchyLocalTransformFromWorldTransform(anim, bone, t);
	}
}

void SMDHelper::RotateBoneInLocalSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& angles)
{
	auto& node = anim.nodes[bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entry = anim.frames[t].entries[bone];

		const glm::mat4& local_matrix = frame_entry.local_transform;

		glm::mat4 r_x = glm::rotate(glm::identity<glm::mat4>(), angles[0], glm::normalize(glm::vec3(local_matrix[0])));
		glm::mat4 r_y = glm::rotate(glm::identity<glm::mat4>(), angles[1], glm::normalize(glm::vec3(local_matrix[1])));
		glm::mat4 r_z = glm::rotate(glm::identity<glm::mat4>(), angles[2], glm::normalize(glm::vec3(local_matrix[2])));
		glm::mat4 rotated_local_matrix = r_z * r_y * r_x * local_matrix;
		rotated_local_matrix[3] = glm::vec4(glm::vec3(local_matrix[3]), 1.0f);

		frame_entry.local_transform = rotated_local_matrix;

		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, bone, t);
	}
}

void SMDHelper::TranslateBoneInWorldSpace(s_animation_t& anim, int bone, const glm::vec3& translation)
{
	auto& node = anim.nodes[bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entry = anim.frames[t].entries[bone];

		frame_entry.world_transform[3] += glm::vec4(translation, 0.0f);

		UpdateBoneHierarchyLocalTransformFromWorldTransform(anim, bone, t);
	}
}

void SMDHelper::TranslateBoneInWorldSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& translation)
{
	auto& node = anim.nodes[bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entry = anim.frames[t].entries[bone];

		for (int v = 0; v < 3; ++v)
			frame_entry.world_transform[3] += frame_entry.world_transform[v] * translation[v];

		UpdateBoneHierarchyLocalTransformFromWorldTransform(anim, bone, t);
	}
}

void SMDHelper::TranslateBoneInLocalSpace(s_animation_t& anim, int bone, const glm::vec3& translation)
{
	auto& node = anim.nodes[bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entry = anim.frames[t].entries[bone];

		frame_entry.local_transform[3] += glm::vec4(translation, 0.0f);

		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, bone, t);
	}
}

void SMDHelper::TranslateBoneInLocalSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& translation)
{
	auto& node = anim.nodes[bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entry = anim.frames[t].entries[bone];

		for (int v = 0; v < 3; ++v)
			frame_entry.local_transform[3] += frame_entry.local_transform[v] * translation[v];

		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, bone, t);
	}
}

glm::vec3 SMDHelper::GetBonePositionInLocalSpace(const s_animation_t& anim, const int bone, const int frame)
{
	return anim.frames[frame].entries[bone].local_transform[3];
}

glm::vec3 SMDHelper::GetBonePositionInWorldSpace(const s_animation_t& anim, const int bone, const int frame)
{
	return anim.frames[frame].entries[bone].world_transform[3];
}

glm::vec3 SMDHelper::GetBoneAnglesInLocalSpace(const s_animation_t& anim, const int bone, const int frame)
{
	glm::vec3 angles;
	extract_euler_angles_from_matrix(anim.frames[frame].entries[bone].local_transform, angles);
	return angles;
}

glm::vec3 SMDHelper::GetBoneAnglesInWorldSpace(const s_animation_t& anim, const int bone, const int frame)
{
	glm::vec3 angles;
	extract_euler_angles_from_matrix(anim.frames[frame].entries[bone].world_transform, angles);
	return angles;
}

void SMDHelper::RenameBone(s_animation_t& anim, int bone, const char* new_name)
{
	for (const auto& node : anim.nodes)
	{
		if (!_stricmp(node.name.c_str(), new_name))
			throw;
	}

	anim.nodes[bone].name = new_name;
}

static void get_node_children_full_hierarchy(const std::vector<s_node_t>& nodes, int bone, std::list<int>& children)
{
	children.push_back(bone);
	for (const auto& child : nodes[bone].children)
		get_node_children_full_hierarchy(nodes, child, children);
}

/*

ReplaceBoneParent

Because some bones may have multiple children that need to be remapped, the easiest way
is to rebuild the entire hierarchy and ensure all new bones parent and children are mapped
to the correct index.

1. Build a list of all bones names, in the order that will be the new hierarchy.
2. Create a new node list and map each original parent or *new_parent* with
   respect to the new index
3. Build a list that maps each new bone index to the original bone index
4. For each frame, update the transform with respect to to the new index
5. Recalculate all bone transformations from root.

*/

void SMDHelper::ReplaceBoneParent(s_animation_t& anim, int bone, int new_parent)
{
	std::list<int> replacee_with_children;
	get_node_children_full_hierarchy(anim.nodes, bone, replacee_with_children);

	std::list<std::string> new_hierarchy;
	for (int i = anim.nodes.size() - 1; i >= 0; --i)
	{
		if (std::find(replacee_with_children.begin(), replacee_with_children.end(), i) != replacee_with_children.end())
			continue; // Do not add bones that will be moved.

		new_hierarchy.push_front(anim.nodes[i].name);
	}

	// Find the new bone parent position in the new hierarchy.
	auto insert_pos = std::find(new_hierarchy.begin(), new_hierarchy.end(), anim.nodes[new_parent].name);

	// Advance, so we can append after the parent.
	insert_pos++;

	// Append the replacee bone and children.
	for (const auto& child : replacee_with_children)
		new_hierarchy.emplace(insert_pos, anim.nodes[child].name);

	// Rebuild the entire hierarchy.
	std::vector<s_node_t> new_nodes(anim.nodes.size());

	int new_index = 0;
	for (const auto& name : new_hierarchy)
	{
		new_nodes[new_index].index = new_index;
		new_nodes[new_index].name = name;
		new_nodes[new_index].parent = -1;
		++new_index;
	}

	// Build a map of old -> new bones and new -> old bones.

	std::vector<int> new_to_old_hierarchy, old_to_new_hierarchy;

	old_to_new_hierarchy.reserve(anim.nodes.size());
	for (int i = 0; i < anim.nodes.size(); ++i)
		old_to_new_hierarchy.push_back(FindNodeByName(new_nodes, anim.nodes[i].name.c_str()));

	new_to_old_hierarchy.reserve(new_nodes.size());
	for (const auto& name : new_hierarchy)
		new_to_old_hierarchy.push_back(FindNodeByName(anim.nodes, name.c_str()));

	// Update parents and children.
	for (int i = 0; i < new_nodes.size(); ++i)
	{
		auto& new_node = new_nodes[i];
		const auto& anim_node = anim.nodes[new_to_old_hierarchy[i]];

		// Check if this is the bone we want to replace the parent.
		if (anim_node.index == bone)
		{
			// Map to new parent.
			new_node.parent = old_to_new_hierarchy[new_parent];
		}
		else if (anim_node.parent != -1)
		{
			// Map to original parent.
			new_node.parent = old_to_new_hierarchy[anim_node.parent];
		}

		if (new_node.parent != -1)
		{
			// Add node to parent children list.
			new_nodes[new_node.parent].children.push_back(new_node.index);
		}
	}

	// Copy transform of each frame with respect to the new hierarchy.
	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame = anim.frames[t];

		s_animation_frame_t new_frame{};
		new_frame.entries.resize(frame.entries.size());

		for (int i = 0; i < frame.entries.size(); ++i)
		{
			new_frame.entries[old_to_new_hierarchy[i]] = frame.entries[i];
		}

		frame.entries = new_frame.entries;
	}

	// Set new node hierarchy.
	anim.nodes = new_nodes;

	// Update all frames transform from the root.
	UpdateBoneHierarchyLocalTransformFromWorldTransform(anim, 0);
}

void SMDHelper::RemoveBone(s_animation_t& anim, int bone)
{
	// Set no parent for this bones' children.
	for (auto& child : anim.nodes[bone].children)
		anim.nodes[child].parent = -1;

	// Remove this bone from all children list.
	for (int i = 0; i < anim.nodes.size(); ++i)
	{
		if (i == bone)
			continue; // Skip the input bone since it will be deleted.

		auto& node = anim.nodes[i];

		// Find and remove the bone from children list if it exists.
		auto it = std::find(node.children.begin(), node.children.end(), bone);
		if (it != node.children.end())
			node.children.erase(it);
	}

	std::vector<s_node_t> new_nodes;
	new_nodes.reserve(anim.nodes.size() - 1);

	int new_node_index = 0;

	// Make a copy of all actual nodes.
	for (int i = 0; i < anim.nodes.size(); ++i)
	{
		if (i == bone)
			continue; // Skip the input bone since it will be deleted.

		const auto& real_node = anim.nodes[i];

		new_nodes.push_back({});

		auto& new_node = new_nodes.back();
		new_node.index = new_node_index++;
		new_node.name = real_node.name;
		new_node.parent = -1;
	}

	new_node_index = 0;

	// Resolve bone children and parents.
	for (int i = 0; i < anim.nodes.size(); ++i)
	{
		if (i == bone)
			continue; // Ignore current bone since it will be deleted.

		const auto& real_node = anim.nodes[i];

		auto& new_node = new_nodes[new_node_index++];

		if (real_node.parent == -1)
			new_node.parent = -1;
		else
			new_node.parent = FindNodeByName(new_nodes, anim.nodes[real_node.parent].name.c_str());

		new_node.children.reserve(real_node.children.size());

		// Bone was removed from all children list previously. It is safe to append.
		// Append each children by name.
		for (const auto& real_child : real_node.children)
			new_node.children.push_back(FindNodeByName(new_nodes, anim.nodes[real_child].name.c_str()));
	}

	// Update the current hierarchy with the new nodes.
	anim.nodes = new_nodes;

	for (auto& frame : anim.frames)
	{
		// Remove the frame entry where input bone was.
		frame.entries.erase(frame.entries.begin() + bone);
	}
}

void SMDHelper::AddBone(s_animation_t& anim,
	const char* name,
	const glm::vec3& local_space_position,
	const glm::vec3& local_space_angles,
	const int parent /*= -1*/
)
{
	int new_bone_index;
	if (parent == -1)
	{
		new_bone_index = 1; // Insert after first root
	}
	else
	{
		new_bone_index = parent + 1;
	}

	// Update higher nodes index and children.
	for (int i = 0; i < anim.nodes.size(); ++i)
	{
		auto& node = anim.nodes[i];

		// If this bone index is higher than the insertion position, increase it's index
		// since we shift 1 index higher.
		if (node.index >= new_bone_index)
			++node.index;

		for (auto& child : node.children)
		{
			// If the child bone index is higher than the insertion position, increase it's index
			// since we shift 1 index higher.
			if (child >= new_bone_index)
				++child;
		}

		// If the parent index is higher than the insertion position, increase it's index
		// since we shift 1 index higher.
		if (node.parent >= new_bone_index)
			++node.parent;
	}

	// Insert the new node at the position.
	s_node_t& new_bone = *anim.nodes.emplace(anim.nodes.begin() + new_bone_index);
	new_bone.index = new_bone_index;
	new_bone.name = name;
	new_bone.parent = parent;

	if (new_bone.parent != -1)
	{
		// Add bone to parent children list.
		anim.nodes[new_bone.parent].children.push_back(new_bone.index);
	}

	// Add new bone local transform for each frame.
	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_entries = anim.frames[t].entries;
		// Insert a new frame entry at the new index position.
		auto& new_frame_entry = *frame_entries.emplace(frame_entries.begin() + new_bone_index);

		new_frame_entry.local_transform = create_rotation_matrix(local_space_angles);
		new_frame_entry.local_transform[3] = glm::vec4(local_space_position, 1.0f);

		// Update new bone world transform.
		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, new_bone.index, t);
	}
}

void SMDHelper::FixupBonesLengths(s_animation_t& anim, const s_animation_t& input_reference)
{
	std::vector<int> reference_to_anim_bones;
	GetReferenceBonesMappedToTargetBones(input_reference, anim, {}, reference_to_anim_bones);

	std::vector<float> ref_bone_lengths(input_reference.nodes.size(), 0);

	// Calculate bone length
	for (int i = 0; i < input_reference.nodes.size(); ++i)
	{
		if (input_reference.nodes[i].parent == -1)
		{
			// No length
		}
		else
		{
			const auto& node = input_reference.frames[0].entries[i];
			const auto& parent_node = input_reference.frames[0].entries[input_reference.nodes[i].parent];

			ref_bone_lengths[i] = glm::length(glm::vec3(node.world_transform[3]) - glm::vec3(parent_node.world_transform[3]));
		}
	}

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		for (int i = 0; i < input_reference.nodes.size(); ++i)
		{
			if (reference_to_anim_bones[i] == -1)
			{
				// No bone mapping exists
			}
			else if (anim.nodes[reference_to_anim_bones[i]].parent == -1)
			{
				// Ignore root
			}
			else
			{
				auto& anim_node = anim.frames[t].entries[reference_to_anim_bones[i]];
				glm::vec3 pos = anim_node.local_transform[3];

				// Check that the bone position is non null before normalizing.
				// If length is 0, it means the bone has the same position as the parent bone in worldspace.
				if (glm::length(pos) > 0.0f)
					pos = glm::normalize(pos) * ref_bone_lengths[i];
				anim_node.local_transform[3] = glm::vec4(pos, 1.0f);

				UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, reference_to_anim_bones[i], t);
			}
		}
	}
}

//
// This assumes the input animation is identical to the original animation.
//
void SMDHelper::SolveFoots(
	s_animation_t& anim,
	const char* anim_left_foot_name,
	const char* anim_right_foot_name,
	const char* anim_pelvis_name,
	const s_animation_t& original_animation,
	const char* original_anim_left_foot_name,
	const char* original_anim_right_foot_name)
{
	int anim_left_foot_index = FindNodeByName(anim.nodes, anim_left_foot_name);
	int anim_right_foot_index = FindNodeByName(anim.nodes, anim_right_foot_name);
	int anim_pelvis_index = FindNodeByName(anim.nodes, anim_pelvis_name);

	int original_anim_left_foot_index = FindNodeByName(original_animation.nodes, original_anim_left_foot_name);
	int original_anim_right_foot_index = FindNodeByName(original_animation.nodes, original_anim_right_foot_name);

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		const auto& original_anim_left_foot = original_animation.frames[t].entries[original_anim_left_foot_index];
		const auto& original_anim_right_foot = original_animation.frames[t].entries[original_anim_right_foot_index];

		const auto& anim_left_foot = anim.frames[t].entries[anim_left_foot_index];
		const auto& anim_right_foot = anim.frames[t].entries[anim_right_foot_index];
		auto& anim_pelvis = anim.frames[t].entries[anim_pelvis_index];

		// All in worldspace.
		const glm::vec3& orig_anim_left_foot_pos = original_anim_left_foot.world_transform[3];
		const glm::vec3& orig_anim_right_foot_pos = original_anim_right_foot.world_transform[3];

		const glm::vec3& anim_left_foot_pos = anim_left_foot.world_transform[3];
		const glm::vec3& anim_right_foot_pos = anim_right_foot.world_transform[3];
		glm::vec3 anim_pelvis_pos = anim_pelvis.world_transform[3];

		anim_pelvis_pos += orig_anim_left_foot_pos - anim_left_foot_pos;

		auto set_pelvis_world_position = [&](const glm::vec3& new_pos) {
			anim_pelvis.world_transform[3] = glm::vec4(new_pos, 1.0f);

			if (anim.nodes[anim_pelvis_index].parent == -1)
			{
				anim_pelvis.local_transform = anim_pelvis.world_transform;
			}
			else
			{
				auto& anim_pelvis_parent = anim.frames[t].entries[anim.nodes[anim_pelvis_index].parent];
				anim_pelvis.local_transform = glm::inverse(anim_pelvis_parent.world_transform) * anim_pelvis.world_transform;
			}

			UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, anim_pelvis_index, t);
		};

		set_pelvis_world_position(anim_pelvis_pos);

		glm::vec3 right_delta = (original_anim_right_foot.world_transform[3] - anim_right_foot.world_transform[3]) * 0.5f;
		anim_pelvis_pos = glm::vec3(anim_pelvis.world_transform[3]) + right_delta;

		set_pelvis_world_position(anim_pelvis_pos);
	}

	int a = 2;
	a++;

}

// This assumes anim and target anim have the same frame count and are alike.
void SMDHelper::TranslateToBoneInWorldSpace(
	s_animation_t& anim,
	int bone,
	const s_animation_t& target_animation,
	int target_bone
)
{
	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_node = anim.frames[t].entries[bone];

		glm::vec3 target_bone_pos = glm::vec3(target_animation.frames[t].entries[target_bone].world_transform[3]);

		frame_node.world_transform[3] = glm::vec4(target_bone_pos, 1.0f);

		if (anim.nodes[bone].parent == -1)
		{
			frame_node.local_transform = frame_node.world_transform;
		}
		else
		{
			const auto& frame_node_parent = anim.frames[t].entries[anim.nodes[bone].parent];
			frame_node.local_transform = glm::inverse(frame_node_parent.world_transform) * frame_node.world_transform;
		}

		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, bone, t);
	}
}

#if 1
// This assumes anim and target anim have the same frame count and are alike.
void SMDHelper::CopyBoneTransformation(
	s_animation_t& anim,
	int bone,
	const s_animation_t& target_animation,
	int target_bone,
	int target_bone_frame
)
{
	if (bone == 38 || target_bone == 38)
	{
		int a = 2;
		a++;
	}

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_node = anim.frames[t].entries[bone];
		auto& target_node_frame = target_animation.frames[target_bone_frame].entries[target_bone];

		frame_node.local_transform = target_node_frame.local_transform;

		if (anim.nodes[bone].parent == -1)
		{
			frame_node.world_transform = frame_node.local_transform;
		}
		else
		{
			// Set new worldspace bone matrix.
			const auto& frame_parent_node = anim.frames[t].entries[anim.nodes[bone].parent];
			frame_node.world_transform = frame_parent_node.world_transform * frame_node.local_transform;
		}

		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, bone, t);
	}
}
#endif

#if 0
// This assumes anim and target anim have the same frame count and are alike.
void SMDHelper::CopyBoneLocalSpaceTransformToBone(
	s_animation_t& dest_anim,
	int dest_bone,
	const s_animation_t& src_anim,
	int src_bone
)
{
	for (int t = 0; t < dest_anim.frames.size(); ++t)
	{
		const auto& src_frame_node = src_anim.frames[t].entries[src_bone];
		auto& dest_frame_node = dest_anim.frames[t].entries[dest_bone];
		dest_frame_node.local_transform = src_frame_node.local_transform;

		UpdateBoneHierarchyWorldTransformFromLocalTransform(dest_anim, dest_bone, t);
	}
}
#endif

void SMDHelper::CopyReferenceBoneLocalSpaceToAnimationBone(
	s_animation_t& anim,
	const int anim_bone,
	const s_animation_t& reference,
	const int reference_bone)
{
	const auto& reference_node = reference.frames[0].entries[reference_bone];

	for (int t = 0; t < anim.frames.size(); ++t)
	{
		auto& frame_node = anim.frames[t].entries[anim_bone];
		frame_node.local_transform = reference_node.local_transform;

		UpdateBoneHierarchyWorldTransformFromLocalTransform(anim, anim_bone, t);
	}
}

void SMDHelper::GetReferenceBonesMappedToTargetBones(
	const s_animation_t& reference, 
	const s_animation_t& target, 
	const std::list<BoneMappingEntry>& bone_mapping_list,
	std::vector<int>& bones_mapped)
{
	bones_mapped.resize(reference.nodes.size(), -1);

	// Find bones in target that have the same name as reference ones.
	for (auto& reference_node : reference.nodes)
	{
		int target_index = FindNodeByName(target.nodes, reference_node.name.c_str());
		if (target_index == -1)
		{
#if DEBUG_MESSAGES
			printf("Target has no bone %s. The reference pose will be used.\n", reference_node.name.c_str());
#endif
		}
		else
		{
#if DEBUG_MESSAGES
			printf("Found bone %s in target. The target pose will be used.\n", reference_node.name.c_str());
#endif
			bones_mapped[reference_node.index] = target_index;
		}
	}

	// See if it is possible to map specific reference bones to target bones.
	for (auto& bm_entry : bone_mapping_list)
	{
		// first = input
		// second = target

		int reference_index = FindNodeByName(reference.nodes, bm_entry.first.c_str());
		if (reference_index == -1)
		{
#if DEBUG_MESSAGES
			printf("Reference has no bone %s. The reference pose will be used.\n", bm_entry.first.c_str());
#endif
			continue;
		}

		int target_index = FindNodeByName(target.nodes, bm_entry.second.c_str());
		if (target_index != -1)
		{
#if DEBUG_MESSAGES
			printf("Found bone mapping %s => %s\n",
				reference.nodes[reference_index].name.c_str(),
				target.nodes[target_index].name.c_str()
			);
#endif
			bones_mapped[reference_index] = target_index;
		}
		else
		{
#if DEBUG_MESSAGES
			printf("Target has no bone %s. The reference pose will be used.\n", bm_entry.second.c_str());
#endif
		}
	}
}

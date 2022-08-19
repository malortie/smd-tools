#pragma once

#include "studio.h"
#include <vector>
#include <list>
#include <functional>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

#define SMD_VERSION 1

typedef struct
{
	std::string name;
	int index;
	int	parent;
	std::vector<int> children;
} s_node_t;

class s_nodetransform_t
{
	glm::mat4 m;

public:
	s_nodetransform_t() : m(glm::identity<glm::mat4>())
	{
	}

	s_nodetransform_t(const glm::mat4& p_m) : m(p_m)
	{
	}

	s_nodetransform_t(const glm::vec3& angles, const glm::vec3& translation)
	{
		InitFromEulerAngles(angles);
		SetTranslation(translation);
	}

	inline s_nodetransform_t operator*(const s_nodetransform_t& other) const {
		return s_nodetransform_t(m * other.m);
	}

	inline s_nodetransform_t& operator*=(const s_nodetransform_t& other) {
		m = m * other.m;
		return *this;
	}

	inline s_nodetransform_t& operator=(const s_nodetransform_t& other) {
		m = other.m;
		return *this;
	}

	inline void InitFromEulerAngles(const glm::vec3& angles) {
		m = glm::eulerAngleZYX(angles[2], angles[1], angles[0]);
	}

	inline glm::vec3 GetTranslation() const { return m[3]; }
	inline void SetTranslation(const glm::vec3& t) { m[3] = glm::vec4(t, m[3][3]); }

	inline glm::vec3 GetEulerAngles() const {
		glm::vec3 angles;
		glm::extractEulerAngleZYX(m, angles[2], angles[1], angles[0]);
		return angles;
	}

	inline glm::mat4 GetInverse() const { return glm::inverse(m); }
};

struct s_animation_frame_entry_t
{
	glm::mat4 local_transform;
	glm::mat4 world_transform;
};

struct s_animation_frame_t
{
	std::vector<s_animation_frame_entry_t> entries;
};

struct s_animation_t
{
	std::string name;
	std::vector<s_node_t> nodes;
	std::vector<s_animation_frame_t> frames;
};


class SMDFileLoader
{
public:
	void LoadAnimation( const char* file_path, s_animation_t& anim ) const;
};

class SMDSerializer
{
public:
	void WriteAnimation(const s_animation_t& anim, const char* output_path) const;
	void WriteOBJ(const s_animation_t& anim, const char* output_path) const;
};

using BoneMappingEntry = std::pair<std::string, std::string>;

class SMDHelper
{
public:
	static glm::vec3 GetBonePositionInLocalSpace(const s_animation_t& anim, const int bone, const int frame);
	static glm::vec3 GetBonePositionInWorldSpace(const s_animation_t& anim, const int bone, const int frame);
	static glm::vec3 GetBoneAnglesInLocalSpace(const s_animation_t& anim, const int bone, const int frame);
	static glm::vec3 GetBoneAnglesInWorldSpace(const s_animation_t& anim, const int bone, const int frame);

	static void ReplaceBoneParent(s_animation_t& anim, int bone, int new_parent); 
	static void RemoveBone(s_animation_t& anim, int bone);
	static void AddBone(s_animation_t& anim,
		const char* name,
		const glm::vec3& local_space_position,
		const glm::vec3& local_space_angles,
		const int parent = -1
	);
	static void RotateBoneInWorldSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& angles);
	static void RotateBoneInLocalSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& angles);
	static void TranslateBoneInWorldSpace(s_animation_t& anim, int bone, const glm::vec3& translation);
	static void TranslateBoneInLocalSpace(s_animation_t& anim, int bone, const glm::vec3& translation);
	static void TranslateBoneInWorldSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& translation);
	static void TranslateBoneInLocalSpaceRelative(s_animation_t& anim, int bone, const glm::vec3& translation);
	static void RenameBone(s_animation_t& anim, int bone, const char* new_name);
	// Make all bones in the animation match the length of the input reference.
	static void FixupBonesLengths(s_animation_t& anim, const s_animation_t& input_reference);
	// Move pelvis position so that animation foots are between the ones in the original animation.
	static void SolveFoots(
		s_animation_t& anim, 
		const char* anim_left_foot_name,
		const char* anim_right_foot_name,
		const char* anim_pelvis_name,
		const s_animation_t& original_animation,
		const char* original_anim_left_foot_name,
		const char* original_anim_right_foot_name
	);
	static void TranslateToBoneInWorldSpace(
		s_animation_t& anim,
		int bone,
		const s_animation_t& target_animation,
		int target_bone
	);

#if 1
	// Copy a bone transformation from target_
	static void CopyBoneTransformation(
		s_animation_t& anim,
		int bone,
		const s_animation_t& target_animation,
		int target_bone,
		int target_bone_frame
	);
#endif

#if 0
	// Copy src bone local transform to dest bone local transform.
	static void CopyBoneLocalSpaceTransformToBone(
		s_animation_t& dest_anim,
		int dest_bone,
		const s_animation_t& src_anim,
		int src_bone
	);
#endif

	static void CopyReferenceBoneLocalSpaceToAnimationBone(
		s_animation_t& anim,
		const int anim_bone,
		const s_animation_t& reference,
		const int reference_bone
	);

	static int FindNodeByName(const std::vector<s_node_t>& nodes, const char* name);


	// Private

	static void BuildAnimationWorldTransform(s_animation_t& anim);
	static void UpdateBoneHierarchyLocalTransformFromWorldTransform(s_animation_t& anim, int bone, int frame);
	static void UpdateBoneHierarchyLocalTransformFromWorldTransform(s_animation_t& anim, int bone);
	static void UpdateBoneHierarchyWorldTransformFromLocalTransform(s_animation_t& anim, int bone, int frame);
	static void UpdateBoneHierarchyWorldTransformFromLocalTransform(s_animation_t& anim, int bone);

	static void GetReferenceBonesMappedToTargetBones(const s_animation_t& reference,
		const s_animation_t& target,
		const std::list<BoneMappingEntry>& bone_mapping_list,
		std::vector<int>& bones_mapped);

};

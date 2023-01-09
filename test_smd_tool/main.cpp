#include <iostream>
#include <filesystem>
#include <map>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

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

enum class VariableType
{
    INVALID = -1,
    REFERENCE = 0,
    ANIMATION = 1,
    VECTOR3D = 2,
};

struct Variable
{
    Variable() : type(VariableType::INVALID), name("")
    {
    }

    Variable(const VariableType p_type, const char* p_name) : type(p_type), name(p_name)
    {
    }

    VariableType type;
    std::string name;
};

class OperationContext
{
public:

    s_animation_t* GetAnimation(const char* name) {

        for (const auto& entry : _animations) {
            if (!_stricmp(name, entry.first.c_str()))
                return entry.second;
        }

        return nullptr;
    }

    void SetAnimation(const char* name, s_animation_t* animation) {

        for (auto& entry : _animations) {
            if (!_stricmp(name, entry.first.c_str())) {
                entry.second = animation;
                return;
            }
        }
        _animations.push_back(std::make_pair(name, animation));
    }

    s_animation_t* GetReference(const char* name) {

        for (const auto& entry : _references) {
            if (!_stricmp(name, entry.first.c_str()))
                return entry.second;
        }

        return nullptr;
    }

    void SetReference(const char* name, s_animation_t* reference) {

        for (auto& entry : _references) {
            if (!_stricmp(name, entry.first.c_str())) {
                entry.second = reference;
                return;
            }
        }
        _references.push_back(std::make_pair(name, reference));
    }

    glm::vec3 GetVector3D(const char* name) {

        for (const auto& entry : _vec3ds) {
            if (!_stricmp(name, entry.first.c_str()))
                return entry.second;
        }

        throw;
    }

    void SetVector3D(const char* name, const glm::vec3& v) {

        for (auto& entry : _vec3ds) {
            if (!_stricmp(name, entry.first.c_str())) {
                entry.second = v;
                return;
            }
        }

        _vec3ds.push_back(std::make_pair(name, v));
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _vec3ds;
    std::list<std::pair<std::string, s_animation_t*>> _references;
    std::list<std::pair<std::string, s_animation_t*>> _animations;
};

class OperationList;

class Operation
{
public:
    virtual void Invoke(OperationContext* const context) = 0;
    virtual const char* GetDescription() const = 0;
};

class OperationList : public Operation
{
public:
    OperationList(const char* description) : _desc(description)
    {
    }

    const char* GetDescription() const override { return _desc.c_str(); }

    void AddOperation(Operation* operation)
    {
        _operations.push_back(operation);
    }

    void Invoke(OperationContext* const context) override
    {
        for (auto& o : _operations)
        {
            try
            {
                printf("%s\n", o->GetDescription());
                o->Invoke(context);
            }
            catch (...)
            {
                int a = 2;
                a++;
            }
        }
    }

    std::list<Operation*> _operations;
    std::string _desc;
};


//========================================================================================
// Concrete Operations
//========================================================================================

class ReplaceBoneParentOperation : public Operation
{
public:
    ReplaceBoneParentOperation(const char* bone, const char* new_parent)
    {
        _replacements.push_back(std::make_pair(bone, new_parent));
    }
    ReplaceBoneParentOperation(const std::list<std::pair<const char*, const char*>>& replacements)
    {
        for (auto replacement : replacements)
            _replacements.push_back(replacement);
    }

    const char* GetDescription() const override { return "ReplaceBoneParentOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& replacement : _replacements) {
            SMDHelper::ReplaceBoneParent(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, replacement.first.c_str()),
                SMDHelper::FindNodeByName(animation.nodes, replacement.second.c_str())
            );
        }
    }

private:
    std::list<std::pair<std::string, std::string>> _replacements;
};

class RemoveBoneOperation : public Operation
{
public:
    RemoveBoneOperation(const char* bone)
    {
        _bones_to_remove.push_back(bone);
    }
    RemoveBoneOperation(const std::list<const char*>& bones_to_remove)
    {
        for (auto bone : bones_to_remove)
            _bones_to_remove.push_back(bone);
    }

    const char* GetDescription() const override { return "RemoveBoneOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone : _bones_to_remove) {
            SMDHelper::RemoveBone(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone.c_str())
            );
        }
    }

private:
    std::list<std::string> _bones_to_remove;
};

class AddBoneOperation : public Operation
{
public:
    AddBoneOperation(
        const char* name,
        const glm::vec3& local_bone_position,
        const glm::vec3& local_bone_angles,
        const char* parent = nullptr) : 
        _name(name), 
        _local_bone_position(local_bone_position),
        _local_bone_angles(local_bone_angles),
        _var_position(""),
        _var_angles(""),
        _parent(parent ? parent : "")
    {
    }

    AddBoneOperation(
        const char* name,
        const char* position_variable,
        const char* angles_variable,
        const char* parent = nullptr) :
        _name(name),
        _local_bone_position(),
        _local_bone_angles(),
        _var_position(position_variable),
        _var_angles(angles_variable),
        _parent(parent ? parent : "")
    {
    }

    const char* GetDescription() const override { return "AddBoneOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");

        if (_var_position != "")
            _local_bone_position = context->GetVector3D(_var_position.c_str());
        if (_var_angles != "")
            _local_bone_angles = context->GetVector3D(_var_angles.c_str());

        SMDHelper::AddBone(
            animation,
            _name.c_str(),
            _local_bone_position,
            _local_bone_angles,
            SMDHelper::FindNodeByName(animation.nodes, _parent.c_str())
        );
    }

private:
    std::string _name;
    std::string _parent;
    glm::vec3 _local_bone_position;
    glm::vec3 _local_bone_angles;

    std::string _var_position;
    std::string _var_angles;
};

class RenameBoneOperation : public Operation
{
public:
    RenameBoneOperation(
        const char* bone,
        const char* new_name)
    {
        _bones_to_rename.push_back(std::make_pair(bone, new_name));
    }

    RenameBoneOperation(
        const std::list<std::pair<const char*, const char*>>& bones_to_rename)
    {
        for (auto renaming : bones_to_rename)
            _bones_to_rename.push_back(renaming);
    }

    const char* GetDescription() const override { return "RenameBoneOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& renaming : _bones_to_rename) {
            SMDHelper::RenameBone(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, renaming.first.c_str()),
                renaming.second.c_str()
            );
        }
    }

private:
    std::list<std::pair<std::string, std::string>> _bones_to_rename;
};

class RotateBoneInWorldSpaceRelativeOperation : public Operation
{
public:
    RotateBoneInWorldSpaceRelativeOperation(const char* bone, const glm::vec3& angles)
    {
        _rotations.push_back(std::make_pair(bone, angles));
    }

    RotateBoneInWorldSpaceRelativeOperation(const std::list<std::pair<const char*, glm::vec3>>& rotations)
    {
        for (const auto& bone_and_rotation : rotations)
            _rotations.push_back(std::make_pair(bone_and_rotation.first, bone_and_rotation.second));
    }

    const char* GetDescription() const override { return "RotateBoneInWorldSpaceRelativeOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone_and_rotation : _rotations)
        {
            SMDHelper::RotateBoneInWorldSpaceRelative(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_and_rotation.first.c_str()),
                bone_and_rotation.second
            );
        }
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _rotations;
};

class RotateBoneInLocalSpaceRelativeOperation : public Operation
{
public:
    RotateBoneInLocalSpaceRelativeOperation(const char* bone, const glm::vec3& angles)
    {
        _rotations.push_back(std::make_pair(bone, angles));
    }

    RotateBoneInLocalSpaceRelativeOperation(const std::list<std::pair<const char*, glm::vec3>>& rotations)
    {
        for (const auto& bone_and_rotation : rotations)
            _rotations.push_back(std::make_pair(bone_and_rotation.first, bone_and_rotation.second));
    }

    const char* GetDescription() const override { return "RotateBoneInLocalSpaceRelativeOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone_and_rotation : _rotations)
        {
            SMDHelper::RotateBoneInLocalSpaceRelative(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_and_rotation.first.c_str()),
                bone_and_rotation.second
            );
        }
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _rotations;
};

class TranslateBoneInWorldSpaceRelativeOperation : public Operation
{
public:
    TranslateBoneInWorldSpaceRelativeOperation(const char* bone, const glm::vec3& translation)
    {
        _translations.push_back(std::make_pair(bone, translation));
    }

    TranslateBoneInWorldSpaceRelativeOperation(const std::list<std::pair<const char*, glm::vec3>>& translations)
    {
        for (const auto& bone_and_translation : translations)
            _translations.push_back(bone_and_translation);
    }

    const char* GetDescription() const override { return "TranslateBoneInWorldSpaceRelativeOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone_and_translation : _translations)
        {
            SMDHelper::TranslateBoneInWorldSpaceRelative(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_and_translation.first.c_str()),
                bone_and_translation.second
            );
        }
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _translations;
};

class TranslateBoneInLocalSpaceRelativeOperation : public Operation
{
public:
    TranslateBoneInLocalSpaceRelativeOperation(const char* bone, const glm::vec3& translation)
    {
        _translations.push_back(std::make_pair(bone, translation));
    }

    TranslateBoneInLocalSpaceRelativeOperation(const std::list<std::pair<const char*, glm::vec3>>& translations)
    {
        for (const auto& bone_and_translation : translations)
            _translations.push_back(bone_and_translation);
    }

    const char* GetDescription() const override { return "TranslateBoneInLocalSpaceRelativeOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone_and_translation : _translations)
        {
            SMDHelper::TranslateBoneInLocalSpaceRelative(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_and_translation.first.c_str()),
                bone_and_translation.second
            );
        }
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _translations;
};

class TranslateBoneInWorldSpaceOperation : public Operation
{
public:
    TranslateBoneInWorldSpaceOperation(const char* bone, const glm::vec3& translation)
    {
        _translations.push_back(std::make_pair(bone, translation));
    }
    TranslateBoneInWorldSpaceOperation(const std::list<std::pair<const char*, glm::vec3>>& translations)
    {
        for (const auto& translation : translations)
            _translations.push_back(translation);
    }

    const char* GetDescription() const override { return "TranslateBoneInWorldSpaceOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone_and_translation : _translations)
        {
            SMDHelper::TranslateBoneInWorldSpace(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_and_translation.first.c_str()),
                bone_and_translation.second
            );
        }
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _translations;
};

class TranslateBoneInLocalSpaceOperation : public Operation
{
public:
    TranslateBoneInLocalSpaceOperation(const char* bone, const glm::vec3& translation)
    {
        _translations.push_back(std::make_pair(bone, translation));
    }
    TranslateBoneInLocalSpaceOperation(const std::list<std::pair<const char*, glm::vec3>>& translations)
    {
        for (const auto& translation : translations)
            _translations.push_back(translation);
    }

    const char* GetDescription() const override { return "TranslateBoneInLocalSpaceOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        for (const auto& bone_and_translation : _translations)
        {
            SMDHelper::TranslateBoneInLocalSpace(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_and_translation.first.c_str()),
                bone_and_translation.second
            );
        }
    }

private:
    std::list<std::pair<std::string, glm::vec3>> _translations;
};


class FixupBonesLengthsOperation : public Operation
{
public:
    const char* GetDescription() const override { return "FixupBonesLengthsOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        auto& input_reference = *context->GetReference("input_reference");
        SMDHelper::FixupBonesLengths(
            animation,
            input_reference
        );
    }
};


class SolveFootsOperation : public Operation
{
public:

    SolveFootsOperation(
        const char* anim_left_foot_name,
        const char* anim_right_foot_name,
        const char* anim_pelvis_name, 
        const char* original_anim_left_foot_name,
        const char* original_anim_right_foot_name) : 
        _anim_left_foot(anim_left_foot_name),
        _anim_right_foot(anim_right_foot_name),
        _anim_pelvis(anim_pelvis_name),
        _original_anim_left_foot(original_anim_left_foot_name),
        _original_anim_right_foot(original_anim_right_foot_name)
    {
    }

    const char* GetDescription() const override { return "SolveFootsOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        const auto& original_animation = *context->GetAnimation("original_animation");
        SMDHelper::SolveFoots(
            animation,
            _anim_left_foot.c_str(),
            _anim_right_foot.c_str(),
            _anim_pelvis.c_str(),
            original_animation,
            _original_anim_left_foot.c_str(),
            _original_anim_right_foot.c_str()
        );
    }

private:

    std::string _anim_left_foot;
    std::string _anim_right_foot;
    std::string _anim_pelvis;
    std::string _original_anim_left_foot;
    std::string _original_anim_right_foot;
};

class TranslateToBoneInWorldSpaceOperation : public Operation
{
public:

    TranslateToBoneInWorldSpaceOperation(const char* bone, const char* target_bone, const char* target_bone_animation_variable) :
        _target_bone_animation_variable(target_bone_animation_variable)
    {
        _translations.push_back(std::make_pair(bone, target_bone));
    }
    TranslateToBoneInWorldSpaceOperation(
        const char* target_bone_animation_variable,
        const std::list<std::pair<const char*, const char*>>& translations) :
        _target_bone_animation_variable(target_bone_animation_variable)
    {
        for (const auto& translation : translations)
            _translations.push_back(translation);
    }

    const char* GetDescription() const override { return "TranslateToBoneInWorldSpaceOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        const auto& target_animation = *context->GetAnimation(_target_bone_animation_variable.c_str());
        for (const auto& translation : _translations)
        {
            SMDHelper::TranslateToBoneInWorldSpace(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, translation.first.c_str()),
                target_animation,
                SMDHelper::FindNodeByName(target_animation.nodes, translation.second.c_str())
            );
        }
    }

private:

    std::list<std::pair<std::string, std::string>> _translations;
    std::string _target_bone_animation_variable;
};

#if 1
class CopyBoneTransformationOperation : public Operation
{
public:
    CopyBoneTransformationOperation(const char* bone, const char* target_bone, const char* target_bone_animation_variable,
        int target_bone_frame) :
        _target_bone_frame(target_bone_frame),
        _target_bone_animation_variable(target_bone_animation_variable)
    {
        _bones.push_back(std::make_pair(bone, target_bone));
    }
    CopyBoneTransformationOperation(
        const char* target_bone_animation_variable,
        const int target_bone_frame,
        const std::list<std::pair<const char*, const char*>>& bones) :
        _target_bone_frame(target_bone_frame),
        _target_bone_animation_variable(target_bone_animation_variable)
    {
        for (const auto& bone : bones)
            _bones.push_back(bone);
    }

    const char* GetDescription() const override { return "CopyBoneTransformationOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        const auto& target_animation = *context->GetAnimation(_target_bone_animation_variable.c_str());
        for (const auto& bones : _bones)
        {
            SMDHelper::CopyBoneTransformation(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bones.first.c_str()),
                target_animation,
                SMDHelper::FindNodeByName(target_animation.nodes, bones.second.c_str()),
                _target_bone_frame
            );
        }
    }

private:
    std::list<std::pair<std::string, std::string>> _bones;
    std::string _target_bone_animation_variable;
    int _target_bone_frame;
};

#endif

class WriteOBJOperation : public Operation
{
public:
    WriteOBJOperation(const char* output_dir, const SMDSerializer& serializer) :
        _output_dir(output_dir),
        _serializer(serializer)
    {
    }

    const char* GetDescription() const override { return "WriteOBJOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");

        char filepath[_MAX_PATH]{};
        snprintf(filepath, sizeof(filepath), "%s/%s.obj", _output_dir.c_str(), animation.name.c_str());
        _serializer.WriteOBJ(animation, filepath);
    }

private:

    const SMDSerializer& _serializer;
    std::string _output_dir;
};

class WriteAnimationOperation : public Operation
{
public:
    WriteAnimationOperation(const char* output_dir, const SMDSerializer& serializer) :
        _output_dir(output_dir),
        _serializer(serializer)
    {
    }

    const char* GetDescription() const override { return "WriteAnimationOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");

        char filepath[_MAX_PATH]{};
        snprintf(filepath, sizeof(filepath), "%s/%s", _output_dir.c_str(), animation.name.c_str());
        _serializer.WriteAnimation(animation, filepath);
    }

private:

    const SMDSerializer& _serializer;
    std::string _output_dir;
};



class GetBonePositionInLocalSpaceOperation : public Operation
{
public:

    GetBonePositionInLocalSpaceOperation(
        const char* dest_variable,
        const Variable& src_variable,
        const char* src_bone,
        int frame = 0) :
        _dest_var(dest_variable),
        _src_var(src_variable),
        _src_bone(src_bone),
        _frame(frame)
    {
    }

    const char* GetDescription() const override { return "GetBonePositionInLocalSpaceOperation"; }

    void Invoke(OperationContext* const context) override
    {
        s_animation_t* src = nullptr;
        switch (_src_var.type)
        {
        case  VariableType::REFERENCE:
            src = context->GetReference(_src_var.name.c_str()); 
            break;
        case  VariableType::ANIMATION:
            src = context->GetAnimation(_src_var.name.c_str());
            break;
        default:
            throw;
        }

        glm::vec3 pos = SMDHelper::GetBonePositionInLocalSpace(*src,
            SMDHelper::FindNodeByName(src->nodes, _src_bone.c_str()),
            _frame);

        context->SetVector3D(_dest_var.c_str(), pos);
    }

private:

    std::string _dest_var;
    Variable _src_var;
    std::string _src_bone;
    int _frame;
};

class GetBoneAnglesInLocalSpaceOperation : public Operation
{
public:

    GetBoneAnglesInLocalSpaceOperation(
        const char* dest_variable,
        const Variable& src_variable,
        const char* src_bone,
        int frame = 0) :
        _dest_var(dest_variable),
        _src_var(src_variable),
        _src_bone(src_bone),
        _frame(frame)
    {
    }

    const char* GetDescription() const override { return "GetBoneAnglesInLocalSpaceOperation"; }

    void Invoke(OperationContext* const context) override
    {
        s_animation_t* src = nullptr;
        switch (_src_var.type)
        {
        case  VariableType::REFERENCE:
            src = context->GetReference(_src_var.name.c_str());
            break;
        case  VariableType::ANIMATION:
            src = context->GetAnimation(_src_var.name.c_str());
            break;
        default:
            throw;
        }

        glm::vec3 angles = SMDHelper::GetBoneAnglesInLocalSpace(*src,
            SMDHelper::FindNodeByName(src->nodes, _src_bone.c_str()),
            _frame);

        context->SetVector3D(_dest_var.c_str(), angles);
    }

private:

    std::string _dest_var;
    Variable _src_var;
    std::string _src_bone;
    int _frame;
};


class ScaleVector3DOperation : public Operation
{
public:

    ScaleVector3DOperation(
        const char* vector3d_variable,
        const float scale) :
        _variable_name(vector3d_variable),
        _scale(scale)
    {
    }

    const char* GetDescription() const override { return "ScaleVector3DOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto v = context->GetVector3D(_variable_name.c_str());
        v *= _scale;
        context->SetVector3D(_variable_name.c_str(), v);
    }

private:

    std::string _variable_name;
    float _scale;
};

#if 0
class CopyBoneLocalSpaceTransformToBoneOperation : public Operation
{
public:
    CopyBoneLocalSpaceTransformToBoneOperation(
        const Variable& dest_variable,
        const char* dest_bone,
        const Variable& src_variable,
        const char* src_bone) : 
        _dest_var(dest_variable),
        _src_var(src_variable)
    {
        _bones_src_dest.push_back(std::make_pair(src_bone, dest_bone));
    }
    CopyBoneLocalSpaceTransformToBoneOperation(
        const Variable& dest_variable,
        const Variable& src_variable,
        const std::list<std::pair<const char*, const char*>>& bones_src_dest) :
        _dest_var(dest_variable),
        _src_var(src_variable)
    {
        for (auto bone_src_dest : bones_src_dest)
            _bones_src_dest.push_back(bone_src_dest);
    }

    const char* GetDescription() const override { return "CopyBoneLocalSpaceTransformToBoneOperation"; }

    void Invoke(OperationContext* const context) override
    {
        s_animation_t* src = nullptr;
        s_animation_t* dest = nullptr;
        switch (_src_var.type)
        {
        case  VariableType::REFERENCE:
            src = context->GetReference(_src_var.name.c_str());
            break;
        case  VariableType::ANIMATION:
            src = context->GetAnimation(_src_var.name.c_str());
            break;
        default:
            throw;
        }
        switch (_dest_var.type)
        {
        case  VariableType::REFERENCE:
            dest = context->GetReference(_dest_var.name.c_str());
            break;
        case  VariableType::ANIMATION:
            dest = context->GetAnimation(_dest_var.name.c_str());
            break;
        default:
            throw;
        }

        for (const auto& bone_src_dest : _bones_src_dest) {
            SMDHelper::CopyBoneLocalSpaceTransformToBone(
                *dest,
                SMDHelper::FindNodeByName(dest->nodes, bone_src_dest.second.c_str()),
                *src,
                SMDHelper::FindNodeByName(src->nodes, bone_src_dest.first.c_str())
            );
        }
    }

private:
    Variable _src_var;
    Variable _dest_var;

    // src -> dest
    std::list<std::pair<std::string, std::string>> _bones_src_dest;
};
#endif

class CopyReferenceBoneLocalSpaceToAnimationBoneOperation : public Operation
{
public:
    CopyReferenceBoneLocalSpaceToAnimationBoneOperation(
        const char* anim_bone,
        const char* reference_bone)
    {
        _bones_anim_ref.push_back(std::make_pair(anim_bone, reference_bone));
    }
    CopyReferenceBoneLocalSpaceToAnimationBoneOperation(
        const std::list<std::pair<const char*, const char*>>& bones_anim_ref)
    {
        for (auto bone_anim_ref : bones_anim_ref)
            _bones_anim_ref.push_back(bone_anim_ref);
    }

    const char* GetDescription() const override { return "CopyReferenceBoneLocalSpaceToAnimationBoneOperation"; }

    void Invoke(OperationContext* const context) override
    {
        auto& animation = *context->GetAnimation("animation");
        auto& input_reference = *context->GetReference("input_reference");
        for (const auto& bone_anim_ref : _bones_anim_ref)
        {
            SMDHelper::CopyReferenceBoneLocalSpaceToAnimationBone(
                animation,
                SMDHelper::FindNodeByName(animation.nodes, bone_anim_ref.first.c_str()),
                input_reference,
                SMDHelper::FindNodeByName(input_reference.nodes, bone_anim_ref.second.c_str())
            );
        }
    }

private:
    // anim <- ref
    std::list<std::pair<std::string, std::string>> _bones_anim_ref;
};

//========================================================================================
// Pipeline
//========================================================================================

struct AnimationPipelineEntry
{
    AnimationPipelineEntry(const char* p_name, const char* p_output_dir, 
        const char* p_original_directory) :
        name(p_name), directory(p_output_dir), 
        original_directory(p_original_directory)
    {
    }
    AnimationPipelineEntry(const char* p_name, const char* p_output_dir,
        const char* p_original_directory, Operation* operation) :
        AnimationPipelineEntry(p_name, p_output_dir, p_original_directory)
    {
        operations.push_back(operation);
    }

    const char* name = nullptr;
    const char* directory = nullptr;
    const char* original_directory = nullptr;
    std::list<Operation*> operations;
};

class AnimationPipeline
{
public:
    AnimationPipeline(const SMDFileLoader& smdloader) : _smdloader(smdloader)
    {
    }

    void SetContextAnimation(const char* name, s_animation_t& animation, Variable* output_var = nullptr)
    {
        _context.SetAnimation(name, &animation);
        if (output_var) {
            output_var->type = VariableType::ANIMATION;
            output_var->name = name;
        }
    }

    void SetContextReference(const char* name, s_animation_t& reference, Variable* output_var = nullptr)
    {
        _context.SetReference(name, &reference);
        if (output_var) {
            output_var->type = VariableType::REFERENCE;
            output_var->name = name;
        }
    }

    void RegisterFiles(const std::list<const char*>& filenames,
        const char* input_directory,
        const char* original_directory)
    {
        for (const auto name : filenames)
            _entries.push_back(AnimationPipelineEntry(name, input_directory, original_directory));
    }

    void RegisterFilesWithOperation(Operation* operation, const std::list<const char*>& filenames,
        const char* input_directory,
        const char* original_directory)
    {
        for (const auto name : filenames)
            _entries.push_back(AnimationPipelineEntry(name, input_directory, original_directory, operation));
    }

    void AddOperationToAllFiles(Operation* operation)
    {
        for (auto& entry : _entries)
            entry.operations.push_back(operation);
    }

    void AddOperationToFiles(Operation* operation, const std::list<const char*>& animations)
    {
        for (auto& entry : _entries) {
            for (const auto animation_name : animations) {
                if (!_stricmp(entry.name, animation_name))
                {
                    entry.operations.push_back(operation);
                    break;
                }
            }
        }
    }

    void Invoke()
    {
        for (auto& entry : _entries)
        {
            try
            {
                char file_path[_MAX_PATH]{};
                snprintf(file_path, sizeof(file_path), "%s/%s.smd", entry.directory, entry.name);

                char original_file_path[_MAX_PATH]{};
                snprintf(original_file_path, sizeof(original_file_path), "%s/%s.smd", entry.original_directory, entry.name);

                s_animation_t anim{}, original_anim{};

                _smdloader.LoadAnimation(file_path, anim);
                _smdloader.LoadAnimation(original_file_path, original_anim);

                _context.SetAnimation("animation", &anim);
                _context.SetAnimation("original_animation", &original_anim);

                for (auto o : entry.operations) {
                    printf("%s\n", o->GetDescription());
                    o->Invoke(&_context);
                }
            }
            catch (...)
            {
                int a = 2;
                a++;
            }
        }
    }

private:
    const SMDFileLoader& _smdloader;
    std::list<AnimationPipelineEntry> _entries;
    OperationContext _context;
};


#define INPUT_DIRECTORY_BASE "C:/Users/marc-/Documents/GitHub/decompiled_hl_models"
#define TARGET_DIRECTORY_BASE "C:/Users/marc-/Documents/GitHub/halflife-unified-sdk-assets/modelsrc/models"

class Convert_LD_BlueShift_animations_to_LD_HL1
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_SCIENTIST = INPUT_DIRECTORY_BASE "/""bshift/ld/scientist";
        constexpr const char* INPUT_DIRECTORY_CIV_SCIENTIST = INPUT_DIRECTORY_BASE "/""bshift/ld/civ_scientist";
        constexpr const char* INPUT_DIRECTORY_CIV_PAPER_SCIENTIST = INPUT_DIRECTORY_BASE "/""bshift/ld/civ_paper_scientist";
        constexpr const char* INPUT_DIRECTORY_CONSOLE_CIV_SCIENTIST = INPUT_DIRECTORY_BASE "/""bshift/ld/console_civ_scientist";
        constexpr const char* INPUT_DIRECTORY_SCIENTIST_COWER = INPUT_DIRECTORY_BASE "/""bshift/ld/scientist_cower";
        constexpr const char* INPUT_DIRECTORY_GORDON_SCIENTIST = INPUT_DIRECTORY_BASE "/""bshift/ld/gordon_scientist";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE "/""scientist/animations/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE "/""scientist/meshes/ld/SCI3_Template_Biped1(Headless_Body).smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        p.SetContextReference("input_reference", target_reference);

        //=================================================================================
        // Process to apply to all Blue Shift animations.
        //=================================================================================

        const std::list<const char*> bshift_sequences = {
            "diesimple_rose",
            "cower_chat_die",
            "dead_in_truck",
            "cower_chat",
            "boost",
            "boost_idle",
            "rose_glance",
            "elev_dead1",
            "elev_dead2",
            "tap_idle",
            "tap_down",
            "railing_chat1",
            "railing_chat2",
            "telework1",
            "telework1_stand",
            "telework2",
            "telework3",
            "wallwork1",
            "wallwork2",
            "wallwork3",
            "wallwork4",
            "wallwork5",
            "rose_console1",
            "rose_console2",
            "rose_console3",
            "tele_jump1",
            "tele_jump2",
            "handscan",
            "eating",
            "working_console",
            "ponder_console",
            "cower_vents",
            "ponder_teleporter",
            "power_pipes",
            "in_theory",
            "you_go",
            "sim_ack",
            "sim_lever",
            "hambone",
            "rose_signal",
            "chum_walk",
            "chum_idle",
            "crowbar_swing",
            "open_gate",
            "work_jeep",
            "drive_jeep1",
            "drive_jeep2",
            "rose_outro",
        };

        const std::list<const char*> civ_scientist_sequences = {
           "sit1",
           "sit2",
           "playgame1",
        };

        const std::list<const char*> civ_paper_scientist_sequences = {
           "paper_idle1",
           "paper_idle2",
           "paper_idle3",
        };

        const std::list<const char*> console_civ_scientist_sequences = {
           "console_idle1",
           "console_work",
           "console_shocked",
           "console_sneeze",
        };

        const std::list<const char*> scientist_cower_sequences = {
            "cower_die",
        };

        const std::list<const char*> gordon_scientist_sequences = {
           "tram_glance",
        };

        p.RegisterFiles(
            bshift_sequences,
            INPUT_DIRECTORY_SCIENTIST,
            INPUT_DIRECTORY_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            civ_scientist_sequences,
            INPUT_DIRECTORY_CIV_SCIENTIST,
            INPUT_DIRECTORY_CIV_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            civ_paper_scientist_sequences,
            INPUT_DIRECTORY_CIV_PAPER_SCIENTIST,
            INPUT_DIRECTORY_CIV_PAPER_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            console_civ_scientist_sequences,
            INPUT_DIRECTORY_CONSOLE_CIV_SCIENTIST,
            INPUT_DIRECTORY_CONSOLE_CIV_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            scientist_cower_sequences,
            INPUT_DIRECTORY_SCIENTIST_COWER,
            INPUT_DIRECTORY_SCIENTIST_COWER); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            gordon_scientist_sequences,
            INPUT_DIRECTORY_GORDON_SCIENTIST,
            INPUT_DIRECTORY_GORDON_SCIENTIST); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Gordon scientist - Add Dummy05 and Mouth bones so it's compatible with the upcoming operations.
        //                    They will be removed at the end.
        //=================================================================================
        OperationList add_missing_bones_to_gordon_sequences("Add missing bones to gordon_scientist sequences");

        // Just give a non zero vector position to this bone.
        AddBoneOperation op_gordon_add_dummy_bone("Dummy05", glm::vec3(0,0,1), glm::vec3(0,0,0), "Bip01 Head"
        ); add_missing_bones_to_gordon_sequences.AddOperation(&op_gordon_add_dummy_bone);

        // Just give a non zero vector position to this bone.
        AddBoneOperation op_gordon_add_mouth_bone("Mouth", glm::vec3(0,0,1), glm::vec3(0,0,0), "Dummy05"
        ); add_missing_bones_to_gordon_sequences.AddOperation(&op_gordon_add_mouth_bone);

        p.AddOperationToFiles(&add_missing_bones_to_gordon_sequences,
            gordon_scientist_sequences);

        //=================================================================================
        // Rename bones LD Blue Shift -> LD HL1 bones
        //=================================================================================
        OperationList rename_bs_to_hl1_ld_bones("Rename LD Blue Shift bones to LD HL1 bones");

        RenameBoneOperation op_rb({
            { "Bip01", "Bip02" },
            { "Bip01 Pelvis", "Bip02 Pelvis" },
            { "Bip01 Spine", "Bip02 Spine" },
            { "Bip01 Spine1", "Bip02 Spine1" },
            { "Bip01 Spine2", "Bip02 Spine2" },
            { "Bip01 Spine3", "Bip02 Spine3" },

            { "Bip01 L Clavicle", "Bip02 L Arm" },
            { "Bip01 L UpperArm", "Bip02 L Arm1" },
            { "Bip01 L Forearm", "Bip02 L Arm2" },
            { "Bip01 L Hand", "Bip02 L Hand" },
            { "Bip01 L Finger0", "Bip02 L Finger0" },
            { "Bip01 L Finger01", "Bip02 L Finger01" },
            { "Bip01 L Finger1", "Bip02 L Finger1" },
            { "Bip01 L Finger3", "Bip02 L Finger3" },
            { "Bip01 L Finger4", "Bip02 L Finger4" },

            { "Bip01 L Thigh", "Bip02 L Leg" },
            { "Bip01 L Calf", "Bip02 L Leg1" },
            { "Bip01 L Foot", "Bip02 L Foot" },
            { "Bip01 L Toe0", "Bip02 L Toe0" },

            { "Bip01 R Clavicle", "Bip02 R Arm" },
            { "Bip01 R UpperArm", "Bip02 R Arm1" },
            { "Bip01 R Forearm", "Bip02 R Arm2" },
            { "Bip01 R Hand", "Bip02 R Hand" },
            { "Bip01 R Finger0", "Bip02 R Finger0" },
            { "Bip01 R Finger01", "Bip02 R Finger01" },
            { "Bip01 R Finger1", "Bip02 R Finger1" },
            { "Bip01 R Finger3", "Bip02 R Finger3" },
            { "Bip01 R Finger4", "Bip02 R Finger4" },

            { "Bip01 R Thigh", "Bip02 R Leg" },
            { "Bip01 R Calf", "Bip02 R Leg1" },
            { "Bip01 R Foot", "Bip02 R Foot" },
            { "Bip01 R Toe0", "Bip02 R Toe0" },

            { "Bip01 Neck", "Bip02 Neck" },
            { "Bip01 Head", "Bip02 Head" },

            { "Mouth", "Bone01" },
        }); rename_bs_to_hl1_ld_bones.AddOperation(&op_rb);

        p.AddOperationToAllFiles(&rename_bs_to_hl1_ld_bones);

        //=================================================================================
        // Common transformations
        //=================================================================================

        OperationList common_transformations("Common transformations");

        ReplaceBoneParentOperation op_rp({
            { "Bone01", "Bip02 Head" },
            { "Bip02 L Leg", "Bip02 Pelvis" },
            { "Bip02 R Leg", "Bip02 Pelvis" },
            { "Bip02 Spine1", "Bip02 Pelvis" },
        }); common_transformations.AddOperation(&op_rp);

        RemoveBoneOperation op_br({
            "Dummy05",
            "Bip02 L Finger0",
            "Bip02 L Finger01",
            "Bip02 L Finger1",
            "Bip02 L Finger3",
            "Bip02 L Finger4",
            "Bip02 L Toe0",

            "Bip02 R Finger0",
            "Bip02 R Finger01",
            "Bip02 R Finger1",
            "Bip02 R Finger3",
            "Bip02 R Finger4",
            "Bip02 R Toe0",

            "Bip02 Spine",
        }); common_transformations.AddOperation(&op_br);

        // Rename spine bones.
        RenameBoneOperation op_rn_spine_bones({
            { "Bip02 Spine1", "Bip02 Spine" },
            { "Bip02 Spine2", "Bip02 Spine1" },
            { "Bip02 Spine3", "Bip02 Spine2" },
        }); common_transformations.AddOperation(&op_rn_spine_bones);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local({
            // Rotate legs backward
            { "Bip02 L Leg", glm::vec3(0, 0, glm::radians(-1.0f)) },
            { "Bip02 R Leg", glm::vec3(0, 0, glm::radians(-1.0f)) },

            // Rotate spines backward
            { "Bip02 Spine", glm::vec3(0, 0, glm::radians(-2.0f)) },
            { "Bip02 Spine1", glm::vec3(0, 0, glm::radians(-2.0f)) },
            { "Bip02 Spine2", glm::vec3(0, 0, glm::radians(-2.0f)) },

            // Rotate neck forward
            { "Bip02 Neck", glm::vec3(0, 0, glm::radians(20.0f)) },

            // Rotate clavicles
            { "Bip02 L Arm", glm::vec3(glm::radians(-20.0f), 0, glm::radians(-5.0f)) },
            { "Bip02 R Arm", glm::vec3(glm::radians(20.0f), 0, glm::radians(-5.0f)) },

            // Rotate upper arms
            { "Bip02 L Arm1", glm::vec3(0, glm::radians(-3.0f), glm::radians(6.0f)) },
            { "Bip02 R Arm1", glm::vec3(0, glm::radians(3.0f), glm::radians(6.0f)) },

            // Rotate head
            { "Bip02 Head", glm::vec3(0, 0, glm::radians(-22.0f)) },

            // Rotate mouth axis
            { "Bone01", glm::vec3(0, glm::radians(180.0f), glm::radians(-8.0f)) },

        }); common_transformations.AddOperation(&op_rotations_local);

        // Fix head axis directly in worldspace
        RotateBoneInWorldSpaceRelativeOperation op_rotations_world(
            "Bip02 Head", glm::vec3(0, 0, glm::radians(18.0f))
        ); common_transformations.AddOperation(&op_rotations_world);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip02 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Make foots stand where the foots are in the original animations.
        //=================================================================================

        SolveFootsOperation op_solve_foot(
            "Bip02 L Foot", "Bip02 R Foot", "Bip02 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );

        p.AddOperationToFiles(&op_solve_foot, {
            "diesimple_rose",
            "boost",
            "boost_idle",
            "rose_glance",
            "tap_idle",
            "tap_down",
            "railing_chat1",
            "railing_chat2",
            "telework1",
            "telework1_stand",
            "telework2",
            "telework3",
            "wallwork1",
            "wallwork2",
            "wallwork3",
            "wallwork4",
            "wallwork5",
            "rose_console1",
            "rose_console2",
            "rose_console3",
            "tele_jump1",
            "tele_jump2",
            "handscan",
            "working_console",
            "ponder_console",
            "ponder_teleporter",
            "power_pipes",
            "in_theory",
            "you_go",
            "sim_ack",
            "sim_lever",
            "hambone",
            "rose_signal",
            "chum_walk",
            "chum_idle",
            "crowbar_swing",
            "open_gate",
            "work_jeep",
            "rose_outro",

            // civ_scientist sequences
            "playgame1",

            // gordon_scientist sequences
            "tram_glance",
        });

        //=================================================================================
        // Move left foot up and right foot away from left foot in cower_vents.
        //=================================================================================

        TranslateBoneInWorldSpaceOperation op_translations({
            { "Bip02 L Foot", glm::vec3(0, 0, 2) },
            { "Bip02 R Foot", glm::vec3(-2, 0, 0) }
            });

        p.AddOperationToFiles(&op_translations, {
            "cower_vents",
        });

        //=================================================================================
        // Move right foot away from left foot in cower_chat and cower_chat_die sequences.
        //=================================================================================

        TranslateBoneInWorldSpaceOperation op_translations2(
            "Bip02 R Foot", glm::vec3(-2, 0, 0)
        );

        p.AddOperationToFiles(&op_translations2, {
            "cower_chat_die",
            "cower_chat",

            // scientist_cower
            "cower_die",
        });

        //=================================================================================
        // Move left foot up in eating.
        //=================================================================================

        TranslateBoneInWorldSpaceOperation op_translations3(
            "Bip02 L Foot", glm::vec3(0, 0, 1)
        );
        p.AddOperationToFiles(&op_translations3, {
            "eating",
        });

        //=================================================================================
        // Move calf and foots up in civ_scientist sitting sequences.
        //=================================================================================

        TranslateBoneInWorldSpaceOperation op_translations_world({
            { "Bip02 L Leg1", glm::vec3(0, 0, 1) },
            { "Bip02 L Foot", glm::vec3(0, 0, 1) },

            { "Bip02 R Leg1", glm::vec3(0, 0, 1) },
            { "Bip02 R Foot", glm::vec3(0, 0, 1) },
        });
        p.AddOperationToFiles(&op_translations_world, {
            "sit1",
            "sit2",
        });

        //=================================================================================
        // Apply fixes to civ_paper_scientist sequences
        //=================================================================================
        OperationList fix_paper_sequences("Fix civ_paper_scientist sequences");

        // Move up calf and foots up a bit in civ_paper_scientist idle sequences.
        TranslateBoneInWorldSpaceOperation op_translations_world_2({
            { "Bip02 L Foot", glm::vec3(0, 0, 1) },
            { "Bip02 R Foot", glm::vec3(0, 0, 1) },
        }); fix_paper_sequences.AddOperation(&op_translations_world_2);

        // Rotate Upper arms in civ_paper_scientist sequences.
        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_2({
            { "Bip02 L Arm1", glm::vec3(0, glm::radians(10.0f), 0) },
            { "Bip02 R Arm1", glm::vec3(0, glm::radians(-10.0f), 0) },
        }); fix_paper_sequences.AddOperation(&op_rotations_local_2);

        p.AddOperationToFiles(&fix_paper_sequences, {
           "paper_idle1",
           "paper_idle2",
           "paper_idle3",
        });

        //=================================================================================
        // Apply fixes to console_civ_scientist sequences
        //=================================================================================
        OperationList fix_console_sequences("Fix console_civ_scientist sequences");

        // Move up right calf and foot vertically a bit.
        TranslateBoneInWorldSpaceOperation op_translations_world_3({
            { "Bip02 R Leg1", glm::vec3(0, 0, 0.5) },
            { "Bip02 R Foot", glm::vec3(0, 0, 0.5) },
        }); fix_console_sequences.AddOperation(&op_translations_world_3);

        // Move left calf and foot to the same location as in the original anim.
        TranslateToBoneInWorldSpaceOperation op_console_leg_foot_translation(
            "original_animation", {
            { "Bip02 L Leg1", "Bip01 L Calf" },
            { "Bip02 L Foot", "Bip01 L Foot" },
        }); fix_console_sequences.AddOperation(&op_console_leg_foot_translation);

        p.AddOperationToFiles(&fix_console_sequences, {
            "console_idle1",
            "console_shocked",
            "console_sneeze",
            "console_work",
        });

        //=================================================================================
       // Gordon scientist - Remove Mouth bone. They were unused in the original animation.
       //=================================================================================
        RemoveBoneOperation op_rb_gordon({
            "Bone01",
            // No need to delete Dummy05. It was deleted previously in the process.
        });

        p.AddOperationToFiles(&op_rb_gordon,
            gordon_scientist_sequences);
        
        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_HL1_and_LD_Op4_animations_to_LD_BlueShift
{
public:

    void Invoke()
    {
        //=================================================================================
        // Default operations to apply to all HL1 and Op4 sequences
        //=================================================================================

        constexpr const char* INPUT_DIRECTORY_HL1 = INPUT_DIRECTORY_BASE"/""hl1/ld/scientist";
        constexpr const char* INPUT_DIRECTORY_OP4 = INPUT_DIRECTORY_BASE"/""gearbox/ld/scientist";
        constexpr const char* INPUT_DIRECTORY_OP4_CS = INPUT_DIRECTORY_BASE"/""gearbox/ld/cleansuit_scientist";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""scientist/animations/ld/bshift";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""bshift/ld/scientist/sci3_template_biped1(headless_body).smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);


        //=================================================================================
        // Base animations
        //=================================================================================
        const std::list<const char*> hl1_ld_animations = {
            "dryhands",
            "lean",
            "pause",
            "talkleft",
            "talkright",
            "startle",
            "cprscientist",
            "cprscientistrevive",
            "sstruggleidle",
            "sstruggle",
            "headcrabbed",
            "ceiling_dangle",
            "ventpull1",
            "ventpull2",
            "ventpullidle1",
            "ventpullidle2",
            "keypad",
            "lookwindow",
            "fallingloop",
            "crawlwindow",
            "divewindow",
            "locked_door",
            "unlock_door",
            "handrailidle",
            "fall",
            "scientist_get_pulled",
            "hanging_idle2",
            "fall_elevator",
            "scientist_idlewall",
            "ickyjump_sci",
            "tentacle_grab",
            "helicack",
            "windive",
            "scicrashidle",
            "scicrash",
            "onguard",
            "seeya",
            "kneel",
        };

        const std::list<const char*> op4_ld_animations = {
            "germandeath",
            "of1_a1_cpr1",
            "of1_a1_cpr2",
            "of1a1_sci1_stretcher",
            "of1a1_sci2_stretcher",
            "of4a1_hologram_clipboard1",
            "of4a1_hologram_clipboard2",
            "of1a1_prod_headcrab",
            "cage_float",
            "table_twitch",
            "interogation_idle",
            "interogation",
            "cell_idle",
            "kneel_idle",
            "cpr_gesture1",
            "cpr_gesture2",
            "thrown_out_window",

            //"eye_wipe", // eye_wipe no longer used.
        };

        const std::list<const char*> op4_cs_ld_animations = {
            "scientist_throwna",
            "scientist_thrownb",
            "scientist_beatwindow",
            "scientist_deadpose1",
            "scientist_zombiefear",
            "dead_against_wall",
            "teleport_fidget",
        };

        p.RegisterFiles(
            hl1_ld_animations,
            INPUT_DIRECTORY_HL1,
            INPUT_DIRECTORY_HL1); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_animations,
            INPUT_DIRECTORY_OP4,
            INPUT_DIRECTORY_OP4); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_cs_ld_animations,
            INPUT_DIRECTORY_OP4_CS,
            INPUT_DIRECTORY_OP4_CS); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Rename bones HL1 -> BS bones
        //=================================================================================
        OperationList rename_hl1_to_bs_bones("Rename LD HL1 bones to LD Blue Shift bones");

        RenameBoneOperation op_rb2({
            { "Bip02", "Bip01" },
            { "Bip02 Pelvis", "Bip01 Pelvis" },
            { "Bip02 Spine", "Bip01 Spine" },
            { "Bip02 Spine1", "Bip01 Spine1" },
            { "Bip02 Spine2", "Bip01 Spine2" },

            { "Bip02 L Arm", "Bip01 L Clavicle" },
            { "Bip02 L Arm1", "Bip01 L UpperArm" },
            { "Bip02 L Arm2", "Bip01 L Forearm" },
            { "Bip02 L Hand", "Bip01 L Hand" },
            { "Bip02 L Leg", "Bip01 L Thigh" },
            { "Bip02 L Leg1", "Bip01 L Calf" },
            { "Bip02 L Foot", "Bip01 L Foot" },

            { "Bip02 R Arm", "Bip01 R Clavicle" },
            { "Bip02 R Arm1", "Bip01 R UpperArm" },
            { "Bip02 R Arm2", "Bip01 R Forearm" },
            { "Bip02 R Hand", "Bip01 R Hand" },
            { "Bip02 R Leg", "Bip01 R Thigh" },
            { "Bip02 R Leg1", "Bip01 R Calf" },
            { "Bip02 R Foot", "Bip01 R Foot" },

            { "Bip02 Neck", "Bip01 Neck" },
            { "Bip02 Head", "Bip01 Head" },

            { "Bone01", "Mouth" },
        }); rename_hl1_to_bs_bones.AddOperation(&op_rb2);

        p.AddOperationToAllFiles(&rename_hl1_to_bs_bones);

        //=================================================================================
        // Remove and rename specific CS bones.
        //=================================================================================
        OperationList remove_cs_bones("Remove CS bones and rename");

        RemoveBoneOperation op_rm_cs_bone({
            "Bip02 L Finger0",
            "Bip02 L Finger01",
            "Bip02 L Finger02",
            "Bip02 L Toe01",

            "Bip02 R Finger0",
            "Bip02 R Finger01",
            "Bip02 R Finger02",
            "Bip02 R Toe01",
        }); remove_cs_bones.AddOperation(&op_rm_cs_bone);

        // Additional bones renaming for CS scientists.
        RenameBoneOperation op_rb_cs({
            { "Bip02 L Toe0", "Bip01 L Toe0" },
            { "Bip02 R Toe0", "Bip01 R Toe0" },
        }); remove_cs_bones.AddOperation(&op_rb_cs);

        p.AddOperationToFiles(&remove_cs_bones,
            op4_cs_ld_animations);

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        ReplaceBoneParentOperation op_rp({
            { "Bip01 L Thigh", "Bip01 Spine" },
            { "Bip01 R Thigh", "Bip01 Spine" },
        }); common_transformations.AddOperation(&op_rp);

        // Copy Spine1 data
        const Variable var_anim(VariableType::ANIMATION, "animation");
        GetBonePositionInLocalSpaceOperation op_bpls("new_bone_pos", var_anim, "Bip01 Spine1"); common_transformations.AddOperation(&op_bpls);
        GetBoneAnglesInLocalSpaceOperation op_bals("new_bone_angles", var_target_reference, "Bip01 Spine1"); common_transformations.AddOperation(&op_bals);
        ScaleVector3DOperation op_sv3("new_bone_pos", 0.5f); common_transformations.AddOperation(&op_sv3);

        AddBoneOperation op_ab("NewBone", "new_bone_pos", "new_bone_angles", "Bip01 Spine"
        ); common_transformations.AddOperation(&op_ab);

        ReplaceBoneParentOperation op_rp2(
            "Bip01 Spine1", "NewBone"
        ); common_transformations.AddOperation(&op_rp2);

        RenameBoneOperation op_rb({
            { "Bip01 Spine2", "Bip01 Spine3" },
            { "Bip01 Spine1", "Bip01 Spine2" },
            { "NewBone", "Bip01 Spine1" },
        }); common_transformations.AddOperation(&op_rb);

        // Dummy bone (Links head and mouth in Blue Shift)
        GetBonePositionInLocalSpaceOperation op_bpls_2("new_bone_pos", var_target_reference, "Dummy05"); common_transformations.AddOperation(&op_bpls_2);
        GetBoneAnglesInLocalSpaceOperation op_bals_2("new_bone_angles", var_target_reference, "Dummy05"); common_transformations.AddOperation(&op_bals_2);
        common_transformations.AddOperation(&op_sv3); // Re-add bone 0.5 scaling

        AddBoneOperation op_ab2("Dummy05", "new_bone_pos", "new_bone_angles", "Bip01 Head"
        ); common_transformations.AddOperation(&op_ab2);

        ReplaceBoneParentOperation op_rp3(
            "Mouth", "Dummy05"
        ); common_transformations.AddOperation(&op_rp3);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_1(
            "Bip01 L Thigh", glm::vec3(0, 0, glm::radians(2.0f))
        ); common_transformations.AddOperation(&op_rotations_local_1);
        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_1(
            "Bip01 L Thigh", glm::vec3(0, -0.4, 0)
        ); common_transformations.AddOperation(&op_translations_local_1);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_2(
            "Bip01 R Thigh", glm::vec3(0, 0, glm::radians(2.0f))
        ); common_transformations.AddOperation(&op_rotations_local_2);
        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_2(
            "Bip01 R Thigh", glm::vec3(0, -0.4, 0)
        ); common_transformations.AddOperation(&op_translations_local_2);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_3({
            { "Bip01 L Clavicle", glm::vec3(glm::radians(20.0f), 0, glm::radians(4.0f)) },
            { "Bip01 R Clavicle", glm::vec3(glm::radians(-20.0f), 0, glm::radians(4.0f)) },
            { "Bip01 L UpperArm", glm::vec3(glm::radians(-1.0f), glm::radians(3.0f), glm::radians(-5.0f)) },
            { "Bip01 R UpperArm", glm::vec3(glm::radians(1.0f), glm::radians(-1.0f), glm::radians(-5.0f)) },
            { "Bip01 L Forearm", glm::vec3(0, 0, glm::radians(1.0f)) },
            { "Bip01 R Forearm", glm::vec3(0, 0, glm::radians(1.0f)) },
            { "Bip01 Spine", glm::vec3(0, 0, glm::radians(1.0f)) },
            { "Bip01 Spine2", glm::vec3(0, 0, glm::radians(2.5f)) },
            { "Bip01 Spine3", glm::vec3(0, 0, glm::radians(1.5f)) },
            { "Bip01 Neck", glm::vec3(0, 0, glm::radians(-20.0f)) },
            { "Bip01 Head", glm::vec3(0, 0, glm::radians(5.0f)) },
            { "Dummy05", glm::vec3(0, 0, glm::radians(10.0f)) },
            }); common_transformations.AddOperation(&op_rotations_local_3);

        RotateBoneInWorldSpaceRelativeOperation op_rotations_world(
            "Dummy05", glm::vec3(0, 0, glm::radians(-10.0f))
        ); common_transformations.AddOperation(&op_rotations_world);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_4(
            "Mouth", glm::vec3(0, glm::radians(180.0f), 0)
        ); common_transformations.AddOperation(&op_rotations_local_4);

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_3(
            "Mouth", glm::vec3(0, 0, 0.4f)
        ); common_transformations.AddOperation(&op_translations_local_3);

        // Blue Shift spine and Thigh are offset in animations.

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_4({
            { "Bip01 Spine1", glm::vec3(-2, 0, 0) },
            { "Bip01 Spine2", glm::vec3(-1, 0, 0) },
            { "Bip01 Spine3", glm::vec3(-1, 0, 0) },
            { "Bip01 Neck", glm::vec3(-0.5, 0, 0) },
            }); common_transformations.AddOperation(&op_translations_local_4);

        float spineTranslation = 4.0f;

        // First translate the legs forward relatively so the rest of the bones follow.
        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_5({
            { "Bip01 Pelvis", glm::vec3(spineTranslation, 0, 0) },
            { "Bip01 L Thigh", glm::vec3(spineTranslation, 0, 0) },
            { "Bip01 R Thigh", glm::vec3(spineTranslation, 0, 0) },
        }); common_transformations.AddOperation(&op_translations_local_5);

        // Translate the legs back in world space to put the legs to their previous position,
        // while keeping all child bones at their current place.

        TranslateBoneInWorldSpaceRelativeOperation op_translations_world_1({
            { "Bip01 L Thigh", glm::vec3(-spineTranslation, 0, 0) },
            { "Bip01 R Thigh", glm::vec3(-spineTranslation, 0, 0) },
        }); common_transformations.AddOperation(&op_translations_world_1);


        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip02 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);


        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Move headcrab_stick bone downward in of1a1_prod_headcrab.
        //=================================================================================

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_6(
            "headcrab_stick", glm::vec3(0.5, 0, -1)
        );
        p.AddOperationToFiles(&op_translations_local_6, {
            "of1a1_prod_headcrab",
        });

        //=================================================================================
        // In HL1 and Op4 animations, make foots stand where they are in the original animations.
        //=================================================================================
        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip02 L Foot", "Bip02 R Foot"
        );
        p.AddOperationToFiles(&op_solve_foot, {
            // HL1 animations
            "helicack",
            "windive",

            // Op4 animations
            "germandeath",
            "of1_a1_cpr1",
            "of1a1_sci1_stretcher",
            "of1a1_sci2_stretcher",
            "of1a1_prod_headcrab",
            "interogation_idle",
            "interogation",
            "kneel_idle",

            //"eye_wipe", // eye_wipe no longer used.

            // Cleansuit scientist Op4 animations
            "scientist_throwna",
            "scientist_beatwindow",
            "scientist_zombiefear",
            "teleport_fidget",
        });


        //=================================================================================
        // In dead_against_wall, rotate the upper arms left right to fix hand clipping through ground and leg.
        //=================================================================================
        OperationList fix_dead_against_wall_arms("Fix dead_against_wall arms");

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_5({
            { "Bip01 L UpperArm", glm::vec3(0, glm::radians(-10.0f), 0) },
            { "Bip01 R UpperArm", glm::vec3(0, glm::radians(10.0f), 0) },
        }); fix_dead_against_wall_arms.AddOperation(&op_rotations_local_5);

        // Translate upper arms back a bit locally so the rest of the bones follow.
        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_7({
            { "Bip01 L UpperArm", glm::vec3(-1, 0, 0) },
            { "Bip01 R UpperArm", glm::vec3(-1, 0, 0) },
        }); fix_dead_against_wall_arms.AddOperation(&op_translations_local_7);

        // Translate upper arms back in world space so the rest of the bones keep their offset.
        TranslateBoneInWorldSpaceRelativeOperation op_translations_world_2({
            { "Bip01 L UpperArm", glm::vec3(1, 0, 0) },
            { "Bip01 R UpperArm", glm::vec3(1, 0, 0) },
        }); fix_dead_against_wall_arms.AddOperation(&op_translations_world_2);

        p.AddOperationToFiles(&fix_dead_against_wall_arms, {
            "dead_against_wall",
        });

        /* eye_wipe no longer used.
        //=================================================================================
        // Rotate the arms away a bit in eye_wipe (op4)
        //=================================================================================
        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_6({
            { "Bip01 L UpperArm", glm::vec3(0, glm::radians(-5.0f), 0) },
            { "Bip01 R UpperArm", glm::vec3(0, glm::radians(2.0f), 0) },
        });
        p.AddOperationToFiles(&op_rotations_local_6, {
            "eye_wipe",
        });
        */

        //=================================================================================
        // Rename bones BS -> HL1 bones
        //=================================================================================
        OperationList rename_bs_bones_to_hl1("Rename LD Blue Shift bones to LD HL1 bones");

        // Rename bones

        RenameBoneOperation op_rb3({
            { "Bip01", "Bip02" },
            { "Bip01 Pelvis", "Bip02 Pelvis" },
            { "Bip01 Spine", "Bip02 Spine" },
            { "Bip01 Spine1", "Bip02 Spine1" },
            { "Bip01 Spine2", "Bip02 Spine2" },
            { "Bip01 Spine3", "Bip02 Spine3" },

            { "Bip01 L Clavicle", "Bip02 L Arm" },
            { "Bip01 L UpperArm", "Bip02 L Arm1" },
            { "Bip01 L Forearm", "Bip02 L Arm2" },
            { "Bip01 L Hand", "Bip02 L Hand" },
            { "Bip01 L Thigh", "Bip02 L Leg" },
            { "Bip01 L Calf", "Bip02 L Leg1" },
            { "Bip01 L Foot", "Bip02 L Foot" },

            { "Bip01 R Clavicle", "Bip02 R Arm" },
            { "Bip01 R UpperArm", "Bip02 R Arm1" },
            { "Bip01 R Forearm", "Bip02 R Arm2" },
            { "Bip01 R Hand", "Bip02 R Hand" },
            { "Bip01 R Thigh", "Bip02 R Leg" },
            { "Bip01 R Calf", "Bip02 R Leg1" },
            { "Bip01 R Foot", "Bip02 R Foot" },

            { "Bip01 Neck", "Bip02 Neck" },
            { "Bip01 Head", "Bip02 Head" },

            { "Mouth", "Bone01" },
            }); rename_bs_bones_to_hl1.AddOperation(&op_rb3);

        p.AddOperationToAllFiles(&rename_bs_bones_to_hl1);

        // Additional bones renaming for CS scientists.
        RenameBoneOperation op_rb_cs_2({
            { "Bip01 L Toe0", "Bip02 L Toe0" },
            { "Bip01 R Toe0", "Bip02 R Toe0" },
        });

        p.AddOperationToFiles(&op_rb_cs_2,
            op4_cs_ld_animations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_HL1_and_LD_Op4_animations_to_HD_Op4
{
public:

    void Invoke()
    {
        //=================================================================================
        // Default operations to apply to all HL1 and Op4 sequences
        //=================================================================================

        constexpr const char* INPUT_DIRECTORY_HL1 = INPUT_DIRECTORY_BASE"/""hl1/ld/scientist";
        constexpr const char* INPUT_DIRECTORY_OP4 = INPUT_DIRECTORY_BASE"/""gearbox/ld/scientist";
        constexpr const char* INPUT_DIRECTORY_OP4_CS = INPUT_DIRECTORY_BASE"/""gearbox/ld/cleansuit_scientist";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""scientist/animations/hd";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""gearbox/hd/scientist/dc_sci_(headless_body).smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        //=================================================================================
        // Base animations
        //=================================================================================

        const std::list<const char*> hl1_ld_animations = {
            "scicrashidle",
            "scicrash",
        };

        const std::list<const char*> op4_ld_animations = {
            "of1_a1_cpr2",
            "cpr_gesture1",
            "cpr_gesture2"
        };

        const std::list<const char*> op4_cs_ld_animations = {
            "scientist_throwna",
            "scientist_thrownb",
            "scientist_beatwindow",
            "scientist_deadpose1",
            "scientist_zombiefear",
            "dead_against_wall",
            "teleport_fidget",
        };

        p.RegisterFiles(
            hl1_ld_animations,
            INPUT_DIRECTORY_HL1,
            INPUT_DIRECTORY_HL1); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_animations,
            INPUT_DIRECTORY_OP4,
            INPUT_DIRECTORY_OP4); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_cs_ld_animations,
            INPUT_DIRECTORY_OP4_CS,
            INPUT_DIRECTORY_OP4_CS); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Rename bones LD HL1 -> HD Op4 bones
        //=================================================================================
        OperationList rename_hl1_to_op4_hd_bones("Rename LD HL1 bones to HD Op4 bones");

        RenameBoneOperation op_rb2({
            { "Bip02", "Bip01" },
            { "Bip02 Pelvis", "Bip01 Pelvis" },
            { "Bip02 Spine", "Bip01 Spine" },
            { "Bip02 Spine1", "Bip01 Spine1" },
            { "Bip02 Spine2", "Bip01 Spine2" },
//            { "Bip02 Spine3", "Bip01 Spine3" },

            { "Bip02 L Arm", "Bip01 L Clavicle" },
            { "Bip02 L Arm1", "Bip01 L UpperArm" },
            { "Bip02 L Arm2", "Bip01 L Forearm" },
            { "Bip02 L Hand", "Bip01 L Hand" },
            { "Bip02 L Leg", "Bip01 L Thigh" },
            { "Bip02 L Leg1", "Bip01 L Calf" },
            { "Bip02 L Foot", "Bip01 L Foot" },

            { "Bip02 R Arm", "Bip01 R Clavicle" },
            { "Bip02 R Arm1", "Bip01 R UpperArm" },
            { "Bip02 R Arm2", "Bip01 R Forearm" },
            { "Bip02 R Hand", "Bip01 R Hand" },
            { "Bip02 R Leg", "Bip01 R Thigh" },
            { "Bip02 R Leg1", "Bip01 R Calf" },
            { "Bip02 R Foot", "Bip01 R Foot" },

            { "Bip02 Neck", "Bip01 Neck" },
            { "Bip02 Head", "Bip01 Head" },

            { "Bone01", "Mouth" },
        }); rename_hl1_to_op4_hd_bones.AddOperation(&op_rb2);

        p.AddOperationToAllFiles(&rename_hl1_to_op4_hd_bones);

        //=================================================================================
        // Remove and rename specific CS bones.
        //=================================================================================
        OperationList remove_cs_bones("Remove CS bones and rename");

        RemoveBoneOperation op_rm_cs_bone({
            "Bip02 L Finger0",
            "Bip02 L Finger01",
            "Bip02 L Finger02",
            "Bip02 L Toe01",

            "Bip02 R Finger0",
            "Bip02 R Finger01",
            "Bip02 R Finger02",
            "Bip02 R Toe01",
        }); remove_cs_bones.AddOperation(&op_rm_cs_bone);

        // Additional bones renaming for CS scientists.
        RenameBoneOperation op_rb_cs({
            { "Bip02 L Toe0", "Bip01 L Toe0" },
            { "Bip02 R Toe0", "Bip01 R Toe0" },
        }); remove_cs_bones.AddOperation(&op_rb_cs);

        p.AddOperationToFiles(&remove_cs_bones,
            op4_cs_ld_animations);

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        ReplaceBoneParentOperation op_rp({
           { "Bip01 L Thigh", "Bip01 Spine" },
           { "Bip01 R Thigh", "Bip01 Spine" },
        }); common_transformations.AddOperation(&op_rp);

        // Copy Spine1 data
        const Variable var_anim(VariableType::ANIMATION, "animation");
        GetBonePositionInLocalSpaceOperation op_bpls("new_bone_pos", var_anim, "Bip01 Spine1"); common_transformations.AddOperation(&op_bpls);
        GetBoneAnglesInLocalSpaceOperation op_bals("new_bone_angles", var_target_reference, "Bip01 Spine1"); common_transformations.AddOperation(&op_bals);
        ScaleVector3DOperation op_sv3("new_bone_pos", 0.5f); common_transformations.AddOperation(&op_sv3);

        AddBoneOperation op_ab("NewBone", "new_bone_pos", "new_bone_angles", "Bip01 Spine"
        ); common_transformations.AddOperation(&op_ab);

        ReplaceBoneParentOperation op_rp2(
            "Bip01 Spine1", "NewBone"
        ); common_transformations.AddOperation(&op_rp2);

        RenameBoneOperation op_rb({
            { "Bip01 Spine2", "Bip01 Spine3" },
            { "Bip01 Spine1", "Bip01 Spine2" },
            { "NewBone", "Bip01 Spine1" },
        }); common_transformations.AddOperation(&op_rb);

        // Dummy bone (Links head and mouth in HL1 HD)

        GetBonePositionInLocalSpaceOperation op_bpls_2("new_bone_pos", var_target_reference, "Dummy05"); common_transformations.AddOperation(&op_bpls_2);
        GetBoneAnglesInLocalSpaceOperation op_bals_2("new_bone_angles", var_target_reference, "Dummy05"); common_transformations.AddOperation(&op_bals_2);
        common_transformations.AddOperation(&op_sv3); // Re-add bone 0.5 scaling

        AddBoneOperation op_ab2("Dummy05", "new_bone_pos", "new_bone_angles", "Bip01 Head"
        ); common_transformations.AddOperation(&op_ab2);

        ReplaceBoneParentOperation op_rp3(
            "Mouth", "Dummy05"
        ); common_transformations.AddOperation(&op_rp3);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_1({
            // Rotate foots
            { "Bip01 L Foot", glm::vec3(glm::radians(-8.0f), 0, 0) },
            { "Bip01 R Foot", glm::vec3(glm::radians(8.0f), 0, 0) },

            // Rotate spine
            { "Bip01 Spine", glm::vec3(0, 0, glm::radians(1.0f)) },
            { "Bip01 Spine2", glm::vec3(0, 0, glm::radians(3.0f)) },

            // Rotate neck and head
            { "Bip01 Neck", glm::vec3(0, 0, glm::radians(-18.0f)) },
            { "Bip01 Head", glm::vec3(0, 0, glm::radians(4.0f)) },

            { "Dummy05", glm::vec3(0, 0, glm::radians(10.0f)) },
        }); common_transformations.AddOperation(&op_rotations_local_1);

        // Rotate back in worldspace to realign the axis, while keeping the offset.
        // Fix head axis directly in worldspace
        RotateBoneInWorldSpaceRelativeOperation op_rotations_world(
            "Dummy05", glm::vec3(0, 0, glm::radians(-10.0f))
        ); common_transformations.AddOperation(&op_rotations_world);

        // Rotate mouth
        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_2(
            "Mouth", glm::vec3(0, glm::radians(180.0f), 0)
        ); common_transformations.AddOperation(&op_rotations_local_2);

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_1(
            "Mouth", glm::vec3(0, 0, 0.3)
        ); common_transformations.AddOperation(&op_translations_local_1);

        // Rotate arms and hands
        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_3({
            { "Bip01 L Clavicle", glm::vec3(glm::radians(20.0f), glm::radians(-1.0f), glm::radians(4.0f)) },
            { "Bip01 R Clavicle", glm::vec3(glm::radians(-20.0f), glm::radians(1.0f), glm::radians(4.0f)) },
            { "Bip01 L UpperArm", glm::vec3(0, glm::radians(3.0f), glm::radians(-7.0f)) },
            { "Bip01 R UpperArm", glm::vec3(0, glm::radians(-2.0f), glm::radians(-7.0f)) },
            { "Bip01 L Forearm", glm::vec3(0, 0, glm::radians(1.0f)) },
            { "Bip01 R Forearm", glm::vec3(0, 0, glm::radians(1.0f)) },
            { "Bip01 L Hand", glm::vec3(0, 0, glm::radians(8.0f)) },
            { "Bip01 R Hand", glm::vec3(0, 0, glm::radians(5.0f)) },
        }); common_transformations.AddOperation(&op_rotations_local_3);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip02 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // In HL1 and Op4 animations, make foots stand where they are in the original animations.
        //=================================================================================
        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip02 L Foot", "Bip02 R Foot"
        );
        p.AddOperationToFiles(&op_solve_foot, {
            // HL1 animations
            "scicrashidle",
            "scicrash",

            // Op4 animations
            "of1_a1_cpr2",
            "cpr_gesture1",
            "cpr_gesture2",

            // Cleansuit scientist Op4 animations
            "scientist_throwna",
            "scientist_beatwindow",
            "scientist_zombiefear",
            "teleport_fidget",
        });

        //=================================================================================
        // In dead_against_wall, rotate the upper arms left right to fix hand clipping through ground and leg.
        //=================================================================================
        OperationList fix_dead_against_wall_arms("Fix dead_against_wall arms");

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_5({
            { "Bip01 L UpperArm", glm::vec3(0, glm::radians(-10.0f), 0) },
            { "Bip01 R UpperArm", glm::vec3(0, glm::radians(10.0f), 0) },
        }); fix_dead_against_wall_arms.AddOperation(&op_rotations_local_5);

        // Translate upper arms back a bit locally so the rest of the bones follow.
        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_7({
            { "Bip01 L UpperArm", glm::vec3(-2, 0, 0) },
            { "Bip01 R UpperArm", glm::vec3(-2, 0, 0) },
        }); fix_dead_against_wall_arms.AddOperation(&op_translations_local_7);

        // Translate upper arms back in world space so the rest of the bones keep their offset.
        TranslateBoneInWorldSpaceRelativeOperation op_translations_world_2({
            { "Bip01 L UpperArm", glm::vec3(2, 0, 0) },
            { "Bip01 R UpperArm", glm::vec3(2, 0, 0) },
        }); fix_dead_against_wall_arms.AddOperation(&op_translations_world_2);

        p.AddOperationToFiles(&fix_dead_against_wall_arms, {
            "dead_against_wall",
        });

        //=================================================================================
        // Rename bones HD Op4 -> LD HL1 bones
        //=================================================================================
        OperationList rename_hd_op4_bones_to_hl1("Rename HD Op4 bones to LD HL1 bones");

        RenameBoneOperation op_rb3({
            { "Bip01", "Bip02" },
            { "Bip01 Pelvis", "Bip02 Pelvis" },
            { "Bip01 Spine", "Bip02 Spine" },
            { "Bip01 Spine1", "Bip02 Spine1" },
            { "Bip01 Spine2", "Bip02 Spine2" },
            { "Bip01 Spine3", "Bip02 Spine3" },

            { "Bip01 L Clavicle", "Bip02 L Arm" },
            { "Bip01 L UpperArm", "Bip02 L Arm1" },
            { "Bip01 L Forearm", "Bip02 L Arm2" },
            { "Bip01 L Hand", "Bip02 L Hand" },
            { "Bip01 L Thigh", "Bip02 L Leg" },
            { "Bip01 L Calf", "Bip02 L Leg1" },
            { "Bip01 L Foot", "Bip02 L Foot" },

            { "Bip01 R Clavicle", "Bip02 R Arm" },
            { "Bip01 R UpperArm", "Bip02 R Arm1" },
            { "Bip01 R Forearm", "Bip02 R Arm2" },
            { "Bip01 R Hand", "Bip02 R Hand" },
            { "Bip01 R Thigh", "Bip02 R Leg" },
            { "Bip01 R Calf", "Bip02 R Leg1" },
            { "Bip01 R Foot", "Bip02 R Foot" },

            { "Bip01 Neck", "Bip02 Neck" },
            { "Bip01 Head", "Bip02 Head" },

            { "Mouth", "Bone01" },
        }); rename_hd_op4_bones_to_hl1.AddOperation(&op_rb3);

        p.AddOperationToAllFiles(&rename_hd_op4_bones_to_hl1);

        // Additional bones renaming for CS scientists.
        RenameBoneOperation op_rb_cs_2({
            { "Bip01 L Toe0", "Bip02 L Toe0" },
            { "Bip01 R Toe0", "Bip02 R Toe0" },
        });

        p.AddOperationToFiles(&op_rb_cs_2,
            op4_cs_ld_animations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }

};

class Convert_HD_Civilian_Blue_Shift_to_HD_Scientist
{
public:

    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_CIV_SCIENTIST = INPUT_DIRECTORY_BASE"/""bshift/hd/civ_scientist";
        constexpr const char* INPUT_DIRECTORY_CIV_COAT_SCIENTIST = INPUT_DIRECTORY_BASE"/""bshift/hd/civ_coat_scientist";
        constexpr const char* INPUT_DIRECTORY_CIV_PAPER_SCIENTIST = INPUT_DIRECTORY_BASE"/""bshift/hd/civ_paper_scientist";
        constexpr const char* INPUT_DIRECTORY_CONSOLE_CIV_SCIENTIST = INPUT_DIRECTORY_BASE"/""bshift/hd/console_civ_scientist";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/civ_scientists/hd";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""hl1/hd/scientist/dc_sci_(headless_body).smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> civ_scientist_hd_animations = {
            //"DELETE_ME_HD_HL1",
            //"DELETE_ME_HD_CIV",
            "sit1",
            "sit2",
            "playgame1",
        };

        const std::list<const char*> civ_coat_scientist_hd_animations = {
           "coat_idle1",
           "coat_walk",
        };

        const std::list<const char*> civ_paper_scientist_hd_animations = {
            "paper_idle1",
            "paper_idle2",
            "paper_idle3",
        };

        const std::list<const char*> console_civ_scientist_hd_animations = {
            "console_idle1",
            "console_work",
            "console_shocked",
            "console_sneeze",
        };

        p.RegisterFiles(
            civ_scientist_hd_animations,
            INPUT_DIRECTORY_CIV_SCIENTIST,
            INPUT_DIRECTORY_CIV_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            civ_coat_scientist_hd_animations,
            INPUT_DIRECTORY_CIV_COAT_SCIENTIST,
            INPUT_DIRECTORY_CIV_COAT_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            civ_paper_scientist_hd_animations,
            INPUT_DIRECTORY_CIV_PAPER_SCIENTIST,
            INPUT_DIRECTORY_CIV_PAPER_SCIENTIST); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            console_civ_scientist_hd_animations,
            INPUT_DIRECTORY_CONSOLE_CIV_SCIENTIST,
            INPUT_DIRECTORY_CONSOLE_CIV_SCIENTIST); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local({

            // Rotate spine backward
            { "Bip01 Spine", glm::vec3(0, 0, glm::radians(-2.0f)) },

            // Rotate legs and foots
            { "Bip01 L Thigh", glm::vec3(0, 0, glm::radians(-2.0f)) },
            { "Bip01 R Thigh", glm::vec3(0, 0, glm::radians(-2.0f)) },

            { "Bip01 L Foot", glm::vec3(0, 0, glm::radians(3.0f)) },
            { "Bip01 R Foot", glm::vec3(0, 0, glm::radians(3.0f)) },

            { "Bip01 L Toe0", glm::vec3(0, 0, glm::radians(-3.0f)) },
            { "Bip01 R Toe0", glm::vec3(0, 0, glm::radians(-3.0f)) },

            // Rotate upper spine bones
            { "Bip01 Head", glm::vec3(0, 0, glm::radians(2.0f)) },

            { "Bip01 L Clavicle", glm::vec3(0, 0, glm::radians(-5.0f)) },
            { "Bip01 R Clavicle", glm::vec3(0, 0, glm::radians(-5.0f)) },

            { "Bip01 L UpperArm", glm::vec3(glm::radians(-4.0f), glm::radians(2.0f), glm::radians(2.0f)) },
            { "Bip01 R UpperArm", glm::vec3(glm::radians(4.0f), 0, glm::radians(2.0f)) },

        }); common_transformations.AddOperation(&op_rotations_local);

        // Rotate back in worldspace to realign the axis, while keeping the offset.
        // Fix head axis directly in worldspace
        RotateBoneInWorldSpaceRelativeOperation op_rotations_local_2({
            { "Bip01 L Foot", glm::vec3(0, 0, glm::radians(-3.0f)) },
            { "Bip01 R Foot", glm::vec3(0, 0, glm::radians(-3.0f)) },
        }); common_transformations.AddOperation(&op_rotations_local_2);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
       // In Make foots stand where they are in the original animations.
       //=================================================================================
        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );
        p.AddOperationToFiles(&op_solve_foot, {
            //"DELETE_ME_HD_CIV",

            // civ_scientist
            "playgame1",

            // civ_coat_scientist
            "coat_walk",
            "coat_idle1",
        });

        //=================================================================================
        // Move calf and foots up in civ_scientist sitting sequences.
        //=================================================================================
        OperationList fix_civ_scientist_sequences("Fix civ_scientist sequences");

        TranslateBoneInWorldSpaceOperation op_translations_world({
            { "Bip01 L Calf", glm::vec3(0, 0, 0.5) },
            { "Bip01 L Foot", glm::vec3(0, 0, 0.5) },
            { "Bip01 L Toe0", glm::vec3(0, 0, 0.5) },

            //{ "Bip01 R Calf", glm::vec3(0, 0, 0.5) },
            //{ "Bip01 R Foot", glm::vec3(0, 0, 0.5) },
            //{ "Bip01 R Toe0", glm::vec3(0, 0, 0.5) },
        }); fix_civ_scientist_sequences.AddOperation(&op_translations_world);

        p.AddOperationToFiles(&fix_civ_scientist_sequences, {
            "sit1",
            "sit2",
        });

        //=================================================================================
        // Move calf and foots up in civ_scientist sitting sequences.
        //=================================================================================
        OperationList fix_paper_sequences("Fix civ_paper_scientist sequences");

        TranslateBoneInWorldSpaceOperation op_translations_world_2({
            { "Bip01 L Calf", glm::vec3(0, 0, 0.5) },
            { "Bip01 L Foot", glm::vec3(0, 0, 0.5) },
            { "Bip01 L Toe0", glm::vec3(0, 0, 0.5) },
        }); fix_paper_sequences.AddOperation(&op_translations_world_2);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_3({

            // Rotate Upper arms in civ_paper_scientist sequences.
            { "Bip01 L UpperArm", glm::vec3(0, glm::radians(2.0f), 0) },
            { "Bip01 R UpperArm", glm::vec3(0, glm::radians(-2.0f), 0) },

            // Slightly rotate fingers.
            { "Bip01 L Finger2", glm::vec3(0, 0, glm::radians(45.0f)) },
            { "Bip01 R Finger2", glm::vec3(0, 0, glm::radians(45.0f)) },

            // Adjust newspaper angles.
             { "newspaper", glm::vec3(0, glm::radians(4.0f), glm::radians(3.0f)) },

        }); fix_paper_sequences.AddOperation(&op_rotations_local_3);

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_3(
           // Move the newspaper away a bit.
           "newspaper", glm::vec3(0, 0, -0.5)
        ); fix_paper_sequences.AddOperation(&op_translations_local_3);

        p.AddOperationToFiles(&fix_paper_sequences, {
            "paper_idle1",
            "paper_idle2",
            "paper_idle3",
        });

        //=================================================================================
        // Apply fixes to console_civ_scientist sequences
        //=================================================================================
        OperationList fix_console_sequences("Fix console_civ_scientist sequences");

        // Move left calf; foot; toe to the same location as in the original anim.
        TranslateToBoneInWorldSpaceOperation op_console_leg_foot_translation(
            "original_animation", {
            { "Bip01 L Calf", "Bip01 L Calf" },
            { "Bip01 L Foot", "Bip01 L Foot" },
            { "Bip01 L Toe0", "Bip01 L Toe0" },
        }); fix_console_sequences.AddOperation(&op_console_leg_foot_translation);

        p.AddOperationToFiles(&fix_console_sequences, {
            "console_idle1",
            "console_shocked",
            "console_sneeze",
            "console_work",
        });

        //=================================================================================
        // Rename bones HD Op4 -> LD HL1 bones
        //=================================================================================
        OperationList rename_hd_bones("Rename HD bones to match with HL1 LD bones");

        RenameBoneOperation op_rb({
            { "Bip01", "Bip02" },
            { "Bip01 Pelvis", "Bip02 Pelvis" },
            { "Bip01 Spine", "Bip02 Spine" },
            { "Bip01 Spine1", "Bip02 Spine1" },
            { "Bip01 Spine2", "Bip02 Spine2" },
            { "Bip01 Spine3", "Bip02 Spine3" },

            { "Bip01 L Clavicle", "Bip02 L Arm" },
            { "Bip01 L UpperArm", "Bip02 L Arm1" },
            { "Bip01 L Forearm", "Bip02 L Arm2" },
            { "Bip01 L Hand", "Bip02 L Hand" },
            { "Bip01 L Finger0", "Bip02 L Finger0" },
            { "Bip01 L Finger01", "Bip02 L Finger01" },
            { "Bip01 L Finger02", "Bip02 L Finger02" },
            { "Bip01 L Finger2", "Bip02 L Finger2" },
            { "Bip01 L Finger21", "Bip02 L Finger21" },
            { "Bip01 L Finger22", "Bip02 L Finger22" },

            { "Bip01 L Thigh", "Bip02 L Leg" },
            { "Bip01 L Calf", "Bip02 L Leg1" },
            { "Bip01 L Foot", "Bip02 L Foot" },
            { "Bip01 L Toe0", "Bip02 L Toe0" },

            { "Bip01 R Clavicle", "Bip02 R Arm" },
            { "Bip01 R UpperArm", "Bip02 R Arm1" },
            { "Bip01 R Forearm", "Bip02 R Arm2" },
            { "Bip01 R Hand", "Bip02 R Hand" },
            { "Bip01 R Finger0", "Bip02 R Finger0" },
            { "Bip01 R Finger01", "Bip02 R Finger01" },
            { "Bip01 R Finger02", "Bip02 R Finger02" },
            { "Bip01 R Finger2", "Bip02 R Finger2" },
            { "Bip01 R Finger21", "Bip02 R Finger21" },
            { "Bip01 R Finger22", "Bip02 R Finger22" },

            { "Bip01 R Thigh", "Bip02 R Leg" },
            { "Bip01 R Calf", "Bip02 R Leg1" },
            { "Bip01 R Foot", "Bip02 R Foot" },
            { "Bip01 R Toe0", "Bip02 R Toe0" },

            { "Bip01 Neck", "Bip02 Neck" },
            { "Bip01 Head", "Bip02 Head" },

            { "Mouth", "Bone01" },
        }); rename_hd_bones.AddOperation(&op_rb);

        p.AddOperationToAllFiles(&rename_hd_bones);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fixup_Gman_LD_Briefcase
{
public:

    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_GMAN = INPUT_DIRECTORY_BASE"/""temp_gman_briefcase/ld";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""gman/animations/ld";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""temp_gman_briefcase/ld/gman_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> gman_ld_animations = {
            "bigno",
            "bigyes",
            "cell_away",
            "cell_idle",
            "gman_cell",
            "gman_reference",
            "idle01",
            "idle02",
            "idlebrush",
            "idlelook",
            "listen",
            "lookdown",
            "lookdown2",
            "no",
            "open",
            "push_button",
            "stand",
            "walk",
            "working_nuke",
            "yes",
        };

        p.RegisterFiles(
            gman_ld_animations,
            INPUT_DIRECTORY_GMAN,
            INPUT_DIRECTORY_GMAN); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        RotateBoneInWorldSpaceRelativeOperation op_rotations_world({
            { "Bone01", glm::vec3(0, 0, glm::radians(-30.75f)) },
            { "Bone02", glm::vec3(0, 0, glm::radians(-30.75f)) },
            { "Bone03", glm::vec3(0, 0, glm::radians(-30.75f)) }
        }); common_transformations.AddOperation(&op_rotations_world);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_BlueShift_Otis_Sequences_To_LD_Op4_Otis
{
public:

    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_BSHIFT_OTIS = INPUT_DIRECTORY_BASE"/""bshift/ld/otis";
        constexpr const char* INPUT_DIRECTORY_BSHIFT_INTRO_OTIS = INPUT_DIRECTORY_BASE"/""bshift/ld/intro_otis";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""otis/animations";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""gearbox/ld/otis/otis_body_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> bshift_otis_ld_animations = {
            "otis_dumped",
            "otis_dumped_idle",
            "range_fire",
            "range_struggle",
        };

        const std::list<const char*> bshift_intro_otis_ld_animations = {
            "intro_glance",
        };

        p.RegisterFiles(
            bshift_otis_ld_animations,
            INPUT_DIRECTORY_BSHIFT_OTIS,
            INPUT_DIRECTORY_BSHIFT_OTIS); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            bshift_intro_otis_ld_animations,
            INPUT_DIRECTORY_BSHIFT_INTRO_OTIS,
            INPUT_DIRECTORY_BSHIFT_INTRO_OTIS); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        ReplaceBoneParentOperation op_rp(
          "Torus01", "Bip01 R Finger0"
        ); common_transformations.AddOperation(&op_rp);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Otis_Sequences_To_HD_Barney
{
public:

    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_OP4_OTIS = INPUT_DIRECTORY_BASE"/""gearbox/ld/otis";
        constexpr const char* INPUT_DIRECTORY_BSHIFT_OTIS = INPUT_DIRECTORY_BASE"/""bshift/ld/otis";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/otis/hd";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""hl1/hd/barney/dc_barney_Reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

#if 0
        const std::list<const char*> temp_animations = {
            "otis_body_reference_DELETE_ME",
            //"dc_barney_Reference_DELETE_ME",
        };

        p.RegisterFiles(
            temp_animations,
            INPUT_DIRECTORY_OP4_OTIS,
            INPUT_DIRECTORY_OP4_OTIS);

#else
        const std::list<const char*> op4_otis_ld_animations = {
            "fence",
            "wave",
            "dead_sitting",
            "cowering",
        };

        const std::list<const char*> bshift_otis_ld_animations = {
            "range_fire",
            "otis_dumped",
            "otis_dumped_idle",
        };

        p.RegisterFiles(
            op4_otis_ld_animations,
            INPUT_DIRECTORY_OP4_OTIS,
            INPUT_DIRECTORY_OP4_OTIS); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            bshift_otis_ld_animations,
            INPUT_DIRECTORY_BSHIFT_OTIS,
            INPUT_DIRECTORY_BSHIFT_OTIS); // First time, the original animation directory is the same as input directory.

#endif

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        ReplaceBoneParentOperation op_rp({
           { "Bip01 Spine2", "Bip01 Spine" },
           { "Bip01 Neck", "Bip01 Spine2" },
            }); common_transformations.AddOperation(&op_rp);

        RemoveBoneOperation op_br({
            "Bip01 Spine1",
            "Bip01 Spine3",
            }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({
            { "Bip01 Spine2", "Bip01 Spine1" },

            { "Bip01 L Arm", "Bip01 L Clavicle" },
            { "Bip01 L Arm1", "Bip01 L UpperArm" },
            { "Bip01 L Arm2", "Bip01 L Forearm" },

            { "Bip01 L Leg", "Bip01 L Thigh" },
            { "Bip01 L Leg1", "Bip01 L Calf" },

            { "Bip01 R Arm", "Bip01 R Clavicle" },
            { "Bip01 R Arm1", "Bip01 R UpperArm" },
            { "Bip01 R Arm2", "Bip01 R Forearm" },

            { "Bip01 R Leg", "Bip01 R Thigh" },
            { "Bip01 R Leg1", "Bip01 R Calf" },

            }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

#if 0 // INCORRECT
        RotateBoneInLocalSpaceRelativeOperation op_rotations_local({
            { "Bip01", glm::vec3(0, 0, glm::radians(7.3f)) },
            //{ "Bip01 Pelvis", glm::vec3(glm::radians(5.0f), 0, 0) },

            { "Bip01 R Thigh", glm::vec3(glm::radians(-7.0f), glm::radians(0.35f), glm::radians(-12.5f)) },
            { "Bip01 R Calf", glm::vec3(0, 0, glm::radians(-1.5f)) },
            { "Bip01 R Foot", glm::vec3(0, 0, glm::radians(14.0f)) },

            { "Bip01 L Thigh", glm::vec3(glm::radians(9.0f), glm::radians(0.35f), glm::radians(3.5f)) },
            { "Bip01 L Calf", glm::vec3(0, 0, glm::radians(10.0f)) },
            { "Bip01 L Foot", glm::vec3(0, 0, glm::radians(-13.0f)) },

            { "Bip01 Spine", glm::vec3(glm::radians(-7.5f), glm::radians(0.5f), 0) },
            { "Bip01 Spine1", glm::vec3(0, glm::radians(0.5f), glm::radians(2.0f)) },

            { "Bip01 Neck", glm::vec3(glm::radians(1.0f), glm::radians(-2.5f), glm::radians(9.0f)) },
            { "Bip01 Head", glm::vec3(0, 0, glm::radians(-13.0f)) },

            { "Bip01 L Clavicle", glm::vec3(glm::radians(-8.0f), glm::radians(4.0f), glm::radians(-12.0f)) },
            { "Bip01 R Clavicle", glm::vec3(glm::radians(8.0f), glm::radians(-7.0f), glm::radians(-10.5f)) },

            { "Bip01 R UpperArm", glm::vec3(0, glm::radians(-54.5f), glm::radians(20.0f)) },
            { "Bip01 L UpperArm", glm::vec3(glm::radians(-19.0f), glm::radians(-7.5f), glm::radians(-9.0f)) },

            { "Bip01 L Forearm", glm::vec3(0, glm::radians(-0.5f), glm::radians(4.0f)) },
            { "Bip01 R Forearm", glm::vec3(glm::radians(-1.0f), 0, glm::radians(-12.0f)) },

            { "Bip01 L Hand", glm::vec3(glm::radians(-4.0f), glm::radians(-8.0f), glm::radians(-4.0f)) },
            { "Bip01 R Hand", glm::vec3(0, glm::radians(10.0f), glm::radians(10.0f)) },

            }); common_transformations.AddOperation(&op_rotations_local);

        TranslateBoneInLocalSpaceRelativeOperation op_translation_local({
             { "Bone05", glm::vec3(-0.08, 0.21, -0.28) },

             { "Bip01 L Clavicle", glm::vec3(0.45, -0.33, 0.6) },
             { "Bip01 R Clavicle", glm::vec3(0.35, -0.26, -0.5) },

            }); common_transformations.AddOperation(&op_translation_local);
#endif

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );

        p.AddOperationToFiles(&op_solve_foot, {
            "fence",
            "wave",
            "range_fire",
        });

        //=================================================================================
        // Move Bip01 downward in cowering
        //=================================================================================

        TranslateBoneInLocalSpaceOperation op_cowering_translations(
          "Bip01", glm::vec3(0, 0, -2)
        );

        p.AddOperationToFiles(&op_cowering_translations, {
            "cowering",
        });

        //=================================================================================
        // Rename bones HD Op4 -> LD HL1 bones
        //=================================================================================
        OperationList rename_hd_bones("Rename HD bones to match with HL1 LD bones");

        RenameBoneOperation op_rb_2({

            { "Bip01 L Clavicle", "Bip01 L Arm" },
            { "Bip01 L UpperArm", "Bip01 L Arm1" },
            { "Bip01 L Forearm", "Bip01 L Arm2" },

            { "Bip01 L Thigh", "Bip01 L Leg" },
            { "Bip01 L Calf", "Bip01 L Leg1" },

            { "Bip01 R Clavicle", "Bip01 R Arm" },
            { "Bip01 R UpperArm", "Bip01 R Arm1" },
            { "Bip01 R Forearm", "Bip01 R Arm2" },

            { "Bip01 R Thigh", "Bip01 R Leg" },
            { "Bip01 R Calf", "Bip01 R Leg1" },

            }); rename_hd_bones.AddOperation(&op_rb_2);

        p.AddOperationToAllFiles(&rename_hd_bones);

        //=================================================================================
        // Remove unnecessary bones.
        //=================================================================================
        RemoveBoneOperation op_rem_b_2({
            "Bone06", // Extra bone mouth
            "Torus01", // Donut
            "p_desert_eagle",
            "Bip01 R Finger0",
            });

        p.AddOperationToAllFiles(&op_rem_b_2);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fixup_HGrunt_LD_BlueShift_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_BSHIFT_HGRUNT = INPUT_DIRECTORY_BASE"/""bshift/ld/hgrunt";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""hgrunt/animations/ld";
        constexpr const char* TARGET_REFERENCE = INPUT_DIRECTORY_BASE"/""hl1/ld/hgrunt/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> bshift_hgrunt_ld_animations = {
            "soldier_dump",
            "soldier_dump_idle",
            "soldier_kick",
            "soldier_shoot_ahead",
        };

        p.RegisterFiles(
            bshift_hgrunt_ld_animations,
            INPUT_DIRECTORY_BSHIFT_HGRUNT,
            INPUT_DIRECTORY_BSHIFT_HGRUNT); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);


        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );

        p.AddOperationToAllFiles(&op_solve_foot);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_HL1_HGrunt_To_HD_HL1_HGrunt_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_HGRUNT = INPUT_DIRECTORY_BASE"/""temp_hgrunt/ld";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""hgrunt/animations/hd";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/hd/DC_soldier_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> hl1_hgrunt_ld_animations = {
            "cliffdie",
        };

        p.RegisterFiles(
            hl1_hgrunt_ld_animations,
            INPUT_DIRECTORY_HGRUNT,
            INPUT_DIRECTORY_HGRUNT); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Remove unnecessary bones.
        //=================================================================================
        RemoveBoneOperation op_rem_b_2({
            "Line01",
            "Shotgun",
            "Bip01 L Toe01",
            "Bip01 L Toe02",
            "Bip01 R Toe01",
            "Bip01 R Toe02",
        });

        p.AddOperationToAllFiles(&op_rem_b_2);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fix_Op4_LD_Skeleton_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_SKELETON = INPUT_DIRECTORY_BASE"/""gearbox/ld/skeleton";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""skeleton/animations";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""skeleton/meshes/SKELETON_Template_Biped1.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_skeleton_ld_animations = {
            "dead_against_wall",
            "dead_stomach",
            "s_onback",
            "s_sitting",
        };

        p.RegisterFiles(
            op4_skeleton_ld_animations,
            INPUT_DIRECTORY_SKELETON,
            INPUT_DIRECTORY_SKELETON); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Fixup hands dead_against_wall sequence.
        //=================================================================================
#if 0
        RotateBoneInLocalSpaceRelativeOperation op_rotations_local({
            { "Bip01 L Hand", glm::vec3(0, 0, glm::radians(20.0f)) },
            { "Bip01 R Hand", glm::vec3(/*glm::radians(-10.0f)*/ 0, 0, glm::radians(-10.0f))},
        });

        p.AddOperationToFiles(&op_rotations_local, {
            "dead_against_wall"
        });
#endif

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

#if 0
class Convert_Egon_LD_to_HD_Egon_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_EGON = INPUT_DIRECTORY_BASE"/""hl1/ld/v_egon";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""v_egon/animations/hd";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""v_egon/meshes/hd/v_egon_reference.smd";

        constexpr const char* SAMPLE_HD_ANIMATION = TARGET_DIRECTORY_BASE"/""v_egon/animations/hd/idle1.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        s_animation_t sample_hd_animation;
        smd_loader.LoadAnimation(SAMPLE_HD_ANIMATION, sample_hd_animation);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        Variable var_sample_hd_animation;
        p.SetContextAnimation("sample_hd_animation", sample_hd_animation, &var_sample_hd_animation);

        const std::list<const char*> hl1_ld_egon_animations = {
            "altfirecycle",
            "altfireoff",
            "altfireon",
        };

        p.RegisterFiles(
            hl1_ld_egon_animations,
            INPUT_DIRECTORY_EGON,
            INPUT_DIRECTORY_EGON); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        // Additional bones renaming.
        RenameBoneOperation op_rb({
            { "Hands biped", "Bip01" },

            { "Bip01 L Arm", "Bip01 L Clavicle" },
            { "Bip01 L Arm1", "Bip01 L UpperArm" },
            { "Bip01 L Arm2", "Bip01 L Forearm" },

            { "Bip01 R Arm", "Bip01 R Clavicle" },
            { "Bip01 R Arm1", "Bip01 R UpperArm" },
            { "Bip01 R Arm2", "Bip01 R Forearm" },

            { "Gauss", "egon" },
            { "Bone01", "handle" },

        }); common_transformations.AddOperation(&op_rb);

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_1(
            "egon", glm::vec3(5, 0, 0)
        ); common_transformations.AddOperation(&op_translations_local_1);

        // Rotate egon and handle bones
        RotateBoneInWorldSpaceRelativeOperation op_rotations_world({
            { "egon", glm::vec3(0, glm::radians(-90.0f), 0) },
            { "handle", glm::vec3(0, glm::radians(-90.0f), 0) },
        }); common_transformations.AddOperation(&op_rotations_world);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

#if 1
        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01", "Bip01", "sample_hd_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);
#endif

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};
#endif


class Convert_LD_HL1_HGrunt_To_LD_Op4_Massn_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_HGRUNT = TARGET_DIRECTORY_BASE"/""hgrunt/animations/ld";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""massn/animations/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""massn/meshes/ld/Massn_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> hl1_hgrunt_ld_animations = {
            "leftlegsmflinch",
            "rightlegsmflinch",
            "rightarmflinch",
            "leftarmflinch",
            "180L",
            "180R",
            "dragholeidle",
            "draghole",
            "bustwall",
            "hoprail",
            "converse1",
            "converse2",
            "startleleft",
            "startleright",
            "divecover",
            "defuse",
            "corner1",
            "corner2",
            "stone_toss",
            "cliffdie",
            "diveaside_idle",
            "diveaside",
            "kneeldive_idle",
            "kneeldive",
            "WM_button",
            "WM_moatjump",
            "bustwindow",
            "dragleft",
            "dragright",
            "trackwave",
            "trackdive",
            "flyback",
            "impaled",
            "jumptracks",
            "pipetoss",
            "plunger",
            "soldier_dump",
            "soldier_dump_idle",
            "soldier_shoot_ahead",
            "soldier_kick",
        };

        p.RegisterFiles(
            hl1_hgrunt_ld_animations,
            INPUT_DIRECTORY_HGRUNT,
            INPUT_DIRECTORY_HGRUNT); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Make foots stand where the foots are in the original animations.
        //=================================================================================

        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );

        p.AddOperationToFiles(&op_solve_foot, {
            "leftlegsmflinch",
            "rightlegsmflinch",
            "rightarmflinch",
            "leftarmflinch",
            "180L",
            "180R",
            "dragholeidle",
            "draghole",
            //"bustwall",
            "hoprail",
            "converse1",
            "converse2",
            "startleleft",
            "startleright",
            "divecover",
            "defuse",
            "corner1",
            "corner2",
            "stone_toss",
            "cliffdie",
            "diveaside_idle",
            "diveaside",
            "kneeldive_idle",
            "kneeldive",
            "WM_button",
            "WM_moatjump",
            "bustwindow",
            "dragleft",
            "dragright",
            "trackwave",
            "trackdive",
            "flyback",
            //"impaled",
            "jumptracks",
            "pipetoss",
            "plunger",
            "soldier_dump",
            "soldier_dump_idle",
            "soldier_shoot_ahead",
            "soldier_kick",
        });

        //=================================================================================
        // Remove unnecessary bones.
        //=================================================================================
        RemoveBoneOperation op_rem_b_2({
            "Line01",
            "Shotgun",
            "Bip01 L Toe0",
            "Bip01 L Toe01",
            "Bip01 L Toe02",
            "Bip01 R Toe0",
            "Bip01 R Toe01",
            "Bip01 R Toe02",
        });

        p.AddOperationToAllFiles(&op_rem_b_2);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fix_Op4_LD_Medic_Grunts_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_MEDIC = INPUT_DIRECTORY_BASE"/""gearbox/ld/hgrunt_medic";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/hgrunt_medic/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt_medic/meshes/ld/grunt_medic_head_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_medic_ld_animations = {
            "run",
        };

        p.RegisterFiles(
            op4_medic_ld_animations,
            INPUT_DIRECTORY_MEDIC,
            INPUT_DIRECTORY_MEDIC); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        // No need to fix bone lengths. They are already the same.
        //FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Translate the root bone forward so the sequence is more centered.
        //=================================================================================
        TranslateBoneInLocalSpaceOperation op_translations({
            {  "Bip01", glm::vec3(0, -10, 0) },
        });

        p.AddOperationToFiles(&op_translations, {
            "run",
        });

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fix_Op4_LD_Engineer_Grunts_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_TORCH = INPUT_DIRECTORY_BASE"/""gearbox/ld/hgrunt_torch";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""hgrunt_torch/animations/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt_torch/meshes/ld/engineer_head_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_torch_ld_animations = {
            "walk1",
        };

        p.RegisterFiles(
            op4_torch_ld_animations,
            INPUT_DIRECTORY_TORCH,
            INPUT_DIRECTORY_TORCH); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        // No need to fix bone lengths. They are already the same.
        //FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Translate the root bone backward so the sequence is more centered.
        //=================================================================================
        TranslateBoneInLocalSpaceOperation op_translations({
            {  "Bip01", glm::vec3(0, 16, 0) },
        });

        p.AddOperationToFiles(&op_translations, {
            "walk1",
        });

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fix_Op4_LD_Opfor_Grunts_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_OPFOR = INPUT_DIRECTORY_BASE"/""gearbox/ld/hgrunt_opfor";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/hgrunt_opfor/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_opfor_ld_animations = {
            "combatidle",
            "cpr",
            "crawling",
            "crouching_idle",
            "crouching_saw_blend1",
            "crouching_saw_blend2",
            "crouching_wait",
            "dead_canyon",
            "dead_headcrabed",
            "dead_on_back",
            "hgrunt_dead_stomach",
            "hgrunt_on_stretcher",
            "holding_on",
            "idle2",
            "interogation",
            "interogation_idle",
            "mic_idle1",
            "mic_idle2",
            "mic_idle3",
            "mic_idle4",
            "mic_stand",
            "of1a6_get_on",
            "of2a2_death_speech",
            "of2a2_speech_idle",
            "react",
            "reload_saw",
            "shoot_shotgun",
            "sitting",
            "sitting2",
            "sitting3",
            "standing_saw_blend1",
            "standing_saw_blend2",
        };

        p.RegisterFiles(
            op4_opfor_ld_animations,
            INPUT_DIRECTORY_OPFOR,
            INPUT_DIRECTORY_OPFOR); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

#if 0
        ReplaceBoneParentOperation op_rp({
            { "MP5_model", "Bip01 R Hand" },
        }); common_transformations.AddOperation(&op_rp);
#endif

        RemoveBoneOperation op_br({
            "MP5_model",
            "Bip01 R Finger0",
            "Shotgun",
        }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

//          { "MP5_model", "Line01" },
        }); common_transformations.AddOperation(&op_rb);

        // Used to fix Shotgun bone since they have different positions.
        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        p.AddOperationToAllFiles(&common_transformations);

        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );
        p.AddOperationToFiles(&op_solve_foot, {
            "combatidle",
            //"cpr",
            "crawling",
            "crouching_idle",
            "crouching_saw_blend1",
            "crouching_saw_blend2",
            "crouching_wait",
            //"dead_canyon",
            //"dead_headcrabed",
            //"dead_on_back",
            //"hgrunt_dead_stomach",
            "hgrunt_on_stretcher",
            "holding_on",
            "idle2",
            "interogation",
            "interogation_idle",
            "mic_idle1",
            "mic_idle2",
            "mic_idle3",
            "mic_idle4",
            "mic_stand",
            "of1a6_get_on",
            "of2a2_death_speech",
            "of2a2_speech_idle",
            "react",
            "reload_saw",
            "shoot_shotgun",
            //"sitting",
            //"sitting2",
            //"sitting3",
            "standing_saw_blend1",
            "standing_saw_blend2",
        });

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fix_Op4_LD_Opfor_Grunts_Shared_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_OPFOR = INPUT_DIRECTORY_BASE"/""gearbox/ld/hgrunt_opfor";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/hgrunt_opfor/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_opfor_ld_shared_animations = {
            "dieback1",
            "diebackwards",
            "dieforward",
            "diegutshot",
            "dieheadshot",
            "diesimple",
            "open_floor_grate",
        };

        p.RegisterFiles(
            op4_opfor_ld_shared_animations,
            INPUT_DIRECTORY_OPFOR,
            INPUT_DIRECTORY_OPFOR); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

#if 0
        ReplaceBoneParentOperation op_rp({
            { "MP5_model", "Bip01 R Hand" },
        }); common_transformations.AddOperation(&op_rp);
#endif

        RemoveBoneOperation op_br({
            "MP5_model",
            "Bip01 R Finger0",
            "Shotgun",
        }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

 //         { "MP5_model", "Line01" },
        }); common_transformations.AddOperation(&op_rb);

        // Used to fix Shotgun bone since they have different positions.
        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        p.AddOperationToAllFiles(&common_transformations);

        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );
        p.AddOperationToFiles(&op_solve_foot, {
            "dieback1",
            "diebackwards",
            "dieforward",
            "diegutshot",
            "dieheadshot",
            "diesimple",
            "open_floor_grate",
        });

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Op4_Intro_Regular_To_LD_HL1_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/ld/intro_regular";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/intro_regular/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_intro_regular_ld_shared_animations = {
            "sitting1",
            "sitting2",
            "sitting3",
            "sitting4",
            "sitting5",
        };

        p.RegisterFiles(
            op4_intro_regular_ld_shared_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        RemoveBoneOperation op_br({
            "MP5_model",
            "Bip01 R Finger0",
        }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

#if 0
        SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
        );
        p.AddOperationToFiles(&op_solve_foot, {
            "dieback1",
            "diebackwards",
            "dieforward",
            "diegutshot",
            "dieheadshot",
            "diesimple",
            "open_floor_grate",
        });
#endif

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Op4_Intro_SAW_To_LD_HL1_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/ld/intro_saw";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/intro_saw/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_intro_saw_ld_shared_animations = {
            "cower",
            "sit_React",
            "stiff",
        };

        p.RegisterFiles(
            op4_intro_saw_ld_shared_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        RemoveBoneOperation op_br({
            "MP5_model",
            "Bip01 R Finger0",
        }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};


class Convert_LD_Op4_Intro_Torch_To_LD_HL1_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/ld/intro_torch";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/intro_torch/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_intro_torch_ld_shared_animations = {
            "cower",
        };

        p.RegisterFiles(
            op4_intro_torch_ld_shared_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        RemoveBoneOperation op_br({
            "Bip01 L Finger0",
        }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Op4_Intro_Medic_To_LD_HL1_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/ld/intro_medic";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/intro_medic/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_intro_medic_ld_shared_animations = {
            "sitting1",
            "sitting2",
            "sitting3",
            "sitting4",
            "sitting5",
            "sitting6",
            "sitting7",
        };

        p.RegisterFiles(
            op4_intro_medic_ld_shared_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_HD_Op4_Intro_SAW_To_HD_HL1_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/hd/intro_saw";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/intro_saw/hd";
        //constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/hd/DC_soldier_reference.smd";
        
        /*
           TODO: This should be DC_soldier_reference.smd, but due to the forearm length difference between
           HD HL1 grunts and HD Op4 grunts, the hand position will be inconsistent. For now, target the
           HD Op4 grunt so the forearm length stays the same across LD and HD. 
        */
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt_opfor/meshes/hd/grunt_head_mask_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_intro_saw_hd_shared_animations = {
            "cower",
            "sit_React",
            "sitting1",
            "sitting2",
            "sitting3",
            "sitting4",
            "sitting5",
            "stiff",
        };

        p.RegisterFiles(
            op4_intro_saw_hd_shared_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");


        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        RotateBoneInLocalSpaceRelativeOperation op_rotations_local_2({
            { "m4", glm::vec3(0, 0, glm::radians(-3.0f)) }, // was -5.0
        }); common_transformations.AddOperation(&op_rotations_local_2);

        TranslateBoneInLocalSpaceRelativeOperation op_translations_local_1(
            "m4", glm::vec3(-0.25, 2, 7)
        ); common_transformations.AddOperation(&op_translations_local_1);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Op4_Grunt_To_LD_Op4_Massn_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_OP4_HGRUNT = TARGET_DIRECTORY_BASE"/""shared/animations/hgrunt_opfor/ld";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""massn/animations/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""massn/meshes/ld/Massn_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_ld_grunt_animations = {
            "dead_on_back",
            "dead_headcrabed",
            "hgrunt_dead_stomach",
            "dead_canyon",
            "sitting2",
            "sitting3",
            "open_floor_grate",
            "hgrunt_on_stretcher",
            "crawling",
            "of2a2_speech_idle",
            "of2a2_death_speech",
            "of1a6_get_on",
            "cpr",
            "mic_idle1",
            "mic_idle2",
            "mic_idle3",
            "mic_idle4",
            "mic_stand",
            "interogation",
            "interogation_idle",
        };

        p.RegisterFiles(
            op4_ld_grunt_animations,
            INPUT_DIRECTORY_OP4_HGRUNT,
            INPUT_DIRECTORY_OP4_HGRUNT); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Make foots stand where the foots are in the original animations.
        //=================================================================================

         SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
         );
         p.AddOperationToFiles(&op_solve_foot, {
            //"cpr",
            "crawling",
            //"dead_canyon",
            //"dead_headcrabed",
            //"dead_on_back",
            //"hgrunt_dead_stomach",
            "hgrunt_on_stretcher",
            "interogation",
            "interogation_idle",
            "mic_idle1",
            "mic_idle2",
            "mic_idle3",
            "mic_idle4",
            "mic_stand",
            "of1a6_get_on",
            "of2a2_death_speech",
            "of2a2_speech_idle",
            //"sitting",
            //"sitting2",
            //"sitting3",
         });

         //=================================================================================
         // Remove unnecessary bones.
         //=================================================================================
         RemoveBoneOperation op_rem_b_2({
             "saw_world",
             "Bone05",
             "Bone06",
        });

         p.AddOperationToAllFiles(&op_rem_b_2);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Op4_Intro_Grunts_To_LD_Op4_Massn_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_INTRO_COMMANDER = TARGET_DIRECTORY_BASE"/""shared/animations/intro_commander/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_MEDIC = "C:/Users/marc-/Documents/temp_conversion/intro_medic/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_REGULAR = "C:/Users/marc-/Documents/temp_conversion/intro_regular/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_SAW = "C:/Users/marc-/Documents/temp_conversion/intro_saw/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_TORCH = "C:/Users/marc-/Documents/temp_conversion/intro_torch/ld";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""massn/animations/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""massn/meshes/ld/Massn_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

       const std::list<const char*> op4_ld_intro_commander_animations = {
            "bark",
            "fall_out",
            "intro_stand_idle1",
            "intro_stand_idle2",
            "intro_stand_idle3",
            "stand_react",
        };

        const std::list<const char*> op4_ld_intro_medic_animations = {
            "intro_sitting_holster1",
            "intro_sitting_holster2",
            "intro_sitting_holster3",
            "intro_sitting_holster4",
            "intro_sitting_holster5",
            "intro_sitting_holster6",
            "intro_sitting_holster7",
        };

        const std::list<const char*> op4_ld_intro_regular_animations = {
            "intro_sitting_mp5_1",
            "intro_sitting_mp5_2",
            "intro_sitting_mp5_3",
            "intro_sitting_mp5_4",
            "intro_sitting_mp5_5",
        };

        const std::list<const char*> op4_ld_intro_saw_animations = {
            "intro_sitting_cower1",
            "sit_React",
            "stiff",
        };

        const std::list<const char*> op4_ld_intro_torch_animations = {
            "intro_sitting_cower2",
        };

        p.RegisterFiles(
            op4_ld_intro_commander_animations,
            INPUT_DIRECTORY_INTRO_COMMANDER,
            INPUT_DIRECTORY_INTRO_COMMANDER); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_intro_medic_animations,
            INPUT_DIRECTORY_INTRO_MEDIC,
            INPUT_DIRECTORY_INTRO_MEDIC); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_intro_regular_animations,
            INPUT_DIRECTORY_INTRO_REGULAR,
            INPUT_DIRECTORY_INTRO_REGULAR); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_intro_saw_animations,
            INPUT_DIRECTORY_INTRO_SAW,
            INPUT_DIRECTORY_INTRO_SAW); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_intro_torch_animations,
            INPUT_DIRECTORY_INTRO_TORCH,
            INPUT_DIRECTORY_INTRO_TORCH); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Make foots stand where the foots are in the original animations.
        //=================================================================================

         SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
         );
         p.AddOperationToFiles(&op_solve_foot, {
             // intro_commander
             "bark",
             "fall_out",
             "intro_stand_idle1",
             "intro_stand_idle2",
             "intro_stand_idle3",
             "stand_react",
        });

                 //=================================================================================
        // Missing MP5 bones
        //=================================================================================
        OperationList add_missing_bones_to_massn_sequences("Add missing bones to massn sequences");

        // Just give a non zero vector position to this bone.
        AddBoneOperation op_add_mp5_bone1(
            "Bip01 R Finger0",
            glm::vec3(6.829851, 0.000031, -0.000106),
            glm::vec3(0.001243, 0.000012, 0.000013),
            "Bip01 R Hand"
        ); add_missing_bones_to_massn_sequences.AddOperation(&op_add_mp5_bone1);

        // Just give a non zero vector position to this bone.
        AddBoneOperation op_add_mp5_bone2(
            "Line01", 
#if 1 // Fixed reference values
            glm::vec3(-4.603189, -0.446901, -0.308543),
            glm::vec3(-1.451630, -0.247183, 1.644017),
#else // Original reference values
            glm::vec3(-4.603189, -0.446901, 0.691457),
            glm::vec3(-1.751630, -0.047183, 1.544017), 
#endif
            "Bip01 R Finger0"
        ); add_missing_bones_to_massn_sequences.AddOperation(&op_add_mp5_bone2);

        p.AddOperationToFiles(&add_missing_bones_to_massn_sequences, {
            "intro_sitting_mp5_1",
            "intro_sitting_mp5_2",
            "intro_sitting_mp5_3",
            "intro_sitting_mp5_4",
            "intro_sitting_mp5_5",
            "intro_sitting_cower1",
            "sit_React",
            "stiff",
        });

         //=================================================================================
         // Remove unnecessary bones.
         //=================================================================================
         RemoveBoneOperation op_rem_b_2({
             "Bone05",
             "Bone06",
        });

         p.AddOperationToAllFiles(&op_rem_b_2);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_HD_Op4_Intro_Grunts_To_HD_Op4_Massn_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_INTRO_MEDIC = "C:/Users/marc-/Documents/temp_conversion/intro_medic/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_REGULAR = "C:/Users/marc-/Documents/temp_conversion/intro_regular/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_SAW = "C:/Users/marc-/Documents/temp_conversion/intro_saw/ld";
        constexpr const char* INPUT_DIRECTORY_INTRO_TORCH = "C:/Users/marc-/Documents/temp_conversion/intro_torch/ld";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""massn/animations/hd";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""massn/meshes/ld/Massn_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> op4_ld_intro_medic_animations = {
            "intro_sitting_holster1",
            "intro_sitting_holster2",
            "intro_sitting_holster3",
            "intro_sitting_holster4",
            "intro_sitting_holster5",
            "intro_sitting_holster6",
            "intro_sitting_holster7",
        };

        const std::list<const char*> op4_hd_intro_regular_animations = {
            "intro_sitting_mp5_1",
            "intro_sitting_mp5_2",
            "intro_sitting_mp5_3",
            "intro_sitting_mp5_4",
            "intro_sitting_mp5_5",
        };

        const std::list<const char*> op4_hd_intro_saw_animations = {
            "intro_sitting_cower1",
            "sit_React",
            "stiff",
        };

        const std::list<const char*> op4_ld_intro_torch_animations = {
            "intro_sitting_cower2",
        };

        p.RegisterFiles(
            op4_ld_intro_medic_animations,
            INPUT_DIRECTORY_INTRO_MEDIC,
            INPUT_DIRECTORY_INTRO_MEDIC); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_hd_intro_regular_animations,
            INPUT_DIRECTORY_INTRO_REGULAR,
            INPUT_DIRECTORY_INTRO_REGULAR); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_hd_intro_saw_animations,
            INPUT_DIRECTORY_INTRO_SAW,
            INPUT_DIRECTORY_INTRO_SAW); // First time, the original animation directory is the same as input directory.

        p.RegisterFiles(
            op4_ld_intro_torch_animations,
            INPUT_DIRECTORY_INTRO_TORCH,
            INPUT_DIRECTORY_INTRO_TORCH); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================

        OperationList common_transformations("Common transformations");

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Missing MP5 bones
        //=================================================================================
        OperationList add_missing_bones_to_massn_sequences("Add missing bones to massn sequences");

        // Just give a non zero vector position to this bone.
        AddBoneOperation op_add_mp5_bone1(
            "Bip01 R Finger0",
            glm::vec3(6.829851, 0.000031, -0.000106),
            glm::vec3(0.001243, 0.000012, 0.000013),
            "Bip01 R Hand"
        ); add_missing_bones_to_massn_sequences.AddOperation(&op_add_mp5_bone1);

        // Just give a non zero vector position to this bone.
        AddBoneOperation op_add_mp5_bone2(
            "Line01", 
#if 1 // Fixed reference values
            glm::vec3(-5.303184, -0.846868, 0.791483),
            glm::vec3(-1.451630, -0.407192, 1.654012),
#else // Original reference values
            glm::vec3(-4.603184, -0.446868, 0.691483),
            glm::vec3(-1.751630, -0.047192, 1.544012),
#endif
            "Bip01 R Finger0"
        ); add_missing_bones_to_massn_sequences.AddOperation(&op_add_mp5_bone2);

        p.AddOperationToFiles(&add_missing_bones_to_massn_sequences, {
            "intro_sitting_mp5_1",
            "intro_sitting_mp5_2",
            "intro_sitting_mp5_3",
            "intro_sitting_mp5_4",
            "intro_sitting_mp5_5",
            "intro_sitting_cower1",
            "sit_React",
            "stiff",
        });

         //=================================================================================
         // Remove unnecessary bones.
         //=================================================================================
         RemoveBoneOperation op_rem_b_2({
             "Bone05",
             "Bone06",
             //"m4",
        });

         p.AddOperationToAllFiles(&op_rem_b_2);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_LD_Massn_To_LD_HL1_Grunt_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/ld/massn";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/hgrunt/ld";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/ld/gruntbody.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> massn_ld_animations = {
            "standing",
            "working",
        };

        p.RegisterFiles(
            massn_ld_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

#if 1
        RemoveBoneOperation op_br({
           "Line01",
           "w_m40a1",
           "Bip01 R Finger0",
        }); common_transformations.AddOperation(&op_br);
#else
        ReplaceBoneParentOperation op_rp(
            "Line01", "Bip01 R Hand"
        ); common_transformations.AddOperation(&op_rp);

        RemoveBoneOperation op_br({
            "w_m40a1",
            "Bip01 R Finger0",
        }); common_transformations.AddOperation(&op_br);
#endif

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Make foots stand where the foots are in the original animations.
        //=================================================================================

         SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
         );
         p.AddOperationToAllFiles(&op_solve_foot);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Convert_HD_Massn_To_HD_HL1_Grunt_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY = INPUT_DIRECTORY_BASE"/""gearbox/hd/massn";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""shared/animations/hgrunt/hd";
        //constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt/meshes/hd/DC_soldier_reference.smd";
        /*
           TODO: This should be DC_soldier_reference.smd, but due to the forearm length difference between
           HD HL1 grunts and HD Op4 grunts, the hand position will be inconsistent. For now, target the
           HD Op4 grunt so the forearm length stays the same across LD and HD.
        */
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""hgrunt_opfor/meshes/hd/grunt_head_mask_reference.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        const std::list<const char*> massn_ld_animations = {
            "standing",
            "working",
        };

        p.RegisterFiles(
            massn_ld_animations,
            INPUT_DIRECTORY,
            INPUT_DIRECTORY); // First time, the original animation directory is the same as input directory.

        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        RemoveBoneOperation op_br({
           "m4",
           "w_m40a1",
           "Bip01 R Finger0",
        }); common_transformations.AddOperation(&op_br);

        // Additional bones renaming.
        RenameBoneOperation op_rb({

          { "Bip01 L Clavicle", "Bip01 L Arm" },
          { "Bip01 L UpperArm", "Bip01 L Arm1" },
          { "Bip01 L Forearm", "Bip01 L Arm2" },

          { "Bip01 L Thigh", "Bip01 L Leg" },
          { "Bip01 L Calf", "Bip01 L Leg1" },

          { "Bip01 R Clavicle", "Bip01 R Arm" },
          { "Bip01 R UpperArm", "Bip01 R Arm1" },
          { "Bip01 R Forearm", "Bip01 R Arm2" },

          { "Bip01 R Thigh", "Bip01 R Leg" },
          { "Bip01 R Calf", "Bip01 R Leg1" },

        }); common_transformations.AddOperation(&op_rb);

        FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01 Pelvis", "Bip01 Pelvis", "original_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Make foots stand where the foots are in the original animations.
        //=================================================================================

         SolveFootsOperation op_solve_foot(
            "Bip01 L Foot", "Bip01 R Foot", "Bip01 Pelvis",
            "Bip01 L Foot", "Bip01 R Foot"
         );
         p.AddOperationToAllFiles(&op_solve_foot);

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

class Fix_HD_Crossbow_Sequences
{
public:
    void Invoke()
    {
        constexpr const char* INPUT_DIRECTORY_CROSSBOW = INPUT_DIRECTORY_BASE"/""gearbox/hd/v_crossbow";

        constexpr const char* TARGET_DIRECTORY = TARGET_DIRECTORY_BASE"/""v_crossbow/animations/hd";
        constexpr const char* TARGET_REFERENCE = TARGET_DIRECTORY_BASE"/""v_crossbow/meshes/hd/v_crossbow_reference.smd";

        constexpr const char* IDLE2_HD_ANIMATION = TARGET_DIRECTORY_BASE"/""v_crossbow/animations/hd/idle2.smd";

        SMDFileLoader smd_loader;
        SMDSerializer smd_serializer;

        s_animation_t target_reference;
        smd_loader.LoadAnimation(TARGET_REFERENCE, target_reference);

        s_animation_t idle2_hd_animation;
        smd_loader.LoadAnimation(IDLE2_HD_ANIMATION, idle2_hd_animation);

        AnimationPipeline p(smd_loader);
        Variable var_target_reference;
        p.SetContextReference("input_reference", target_reference, &var_target_reference);

        Variable var_idle2_hd_animation;
        p.SetContextAnimation("idle2_hd_animation", idle2_hd_animation, &var_idle2_hd_animation);

        const std::list<const char*> op4_hd_crossbow_animations = {
            "idle2_new",
            "fidget2_new",
            "draw2",
            "holster2",
            "fire3",
        };

        p.RegisterFiles(
            op4_hd_crossbow_animations,
            INPUT_DIRECTORY_CROSSBOW,
            INPUT_DIRECTORY_CROSSBOW); // First time, the original animation directory is the same as input directory.


        //=================================================================================
        // Common transformations
        //=================================================================================
        OperationList common_transformations("Common transformations");

        // FixupBonesLengthsOperation op_fbl; common_transformations.AddOperation(&op_fbl);

#if 0
        // Move the pelvis to the same location as the original anim pelvis.
        TranslateToBoneInWorldSpaceOperation op_pelvis_translation(
            "Bip01", "Bip01", "sample_hd_animation"
        ); common_transformations.AddOperation(&op_pelvis_translation);
#endif

        p.AddOperationToAllFiles(&common_transformations);

        //=================================================================================
        // Move bolts away.
        //=================================================================================
        CopyBoneTransformationOperation op_copy_bolts_transform(
            "idle2_hd_animation", 0,
            {
                { "Ammobone01", "Ammobone01" },
                { "Ammobone02", "Ammobone02"},
                { "Ammobone03", "Ammobone03"},
                { "Ammobone",   "Ammobone" }
            }
        );

        //=================================================================================
        // Copy grip pose.
        //=================================================================================
        CopyBoneTransformationOperation op_copy_grip_transform(
            "idle2_hd_animation", 0,
            {
                { "LeftArm", "LeftArm" },
                { "LA1", "LA1"},
                { "LA2", "LA2"},
                { "LA3", "LA3" },
                { "LA4", "LA4" },
                { "RightArm", "RightArm" },
                { "RA1", "RA1" },
                { "RA2", "RA2" },
                { "RA3", "RA3" },
                { "RA4", "RA4" },
                { "Slide", "Slide" },
                { "Bolt", "Bolt" },
            }
        );

        p.AddOperationToAllFiles(&op_copy_bolts_transform);
        p.AddOperationToFiles(&op_copy_grip_transform, {
            "idle2_new",
            "fidget2_new",
            "draw2",
            "holster2",
        });

        //=================================================================================
        // Write operations.
        //=================================================================================

        OperationList write_operations("Write output files");
        //WriteOBJOperation op_wobj(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wobj);
        WriteAnimationOperation op_wanim(TARGET_DIRECTORY, smd_serializer); write_operations.AddOperation(&op_wanim);
        p.AddOperationToAllFiles(&write_operations);

        p.Invoke();
    }
};

int main()
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
#endif

#if 0
    Convert_LD_BlueShift_animations_to_LD_HL1().Invoke();
#endif
#if 0
    Convert_LD_HL1_and_LD_Op4_animations_to_LD_BlueShift().Invoke();
#endif
#if 0
    Convert_LD_HL1_and_LD_Op4_animations_to_HD_Op4().Invoke();
#endif
#if 0
    Convert_HD_Civilian_Blue_Shift_to_HD_Scientist().Invoke();
#endif
#if 0
    Fixup_Gman_LD_Briefcase().Invoke();
#endif
#if 0
    Convert_LD_BlueShift_Otis_Sequences_To_LD_Op4_Otis().Invoke();
#endif
#if 0
    Convert_LD_Otis_Sequences_To_HD_Barney().Invoke();
#endif
#if 0
    Fixup_HGrunt_LD_BlueShift_Sequences().Invoke();
#endif
#if 0
    Convert_LD_HL1_HGrunt_To_HD_HL1_HGrunt_Sequences().Invoke();
#endif
#if 0
    Fix_Op4_LD_Skeleton_Sequences().Invoke();
#endif
#if 0
    //Convert_Egon_LD_to_HD_Egon_Sequences().Invoke(); // UNFINISHED !! DON'T DO
#endif
#if 0
    Convert_LD_HL1_HGrunt_To_LD_Op4_Massn_Sequences().Invoke();
#endif
#if 0
    Fix_Op4_LD_Medic_Grunts_Sequences().Invoke();
#endif
#if 0
    Fix_Op4_LD_Engineer_Grunts_Sequences().Invoke();
#endif
#if 0
    Fix_Op4_LD_Opfor_Grunts_Sequences().Invoke();
#endif
#if 0
    Fix_Op4_LD_Opfor_Grunts_Shared_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Op4_Intro_Regular_To_LD_HL1_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Op4_Intro_SAW_To_LD_HL1_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Op4_Intro_Torch_To_LD_HL1_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Op4_Intro_Medic_To_LD_HL1_Sequences().Invoke();
#endif
#if 0
    Convert_HD_Op4_Intro_SAW_To_HD_HL1_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Op4_Grunt_To_LD_Op4_Massn_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Op4_Intro_Grunts_To_LD_Op4_Massn_Sequences().Invoke();
#endif
#if 0
    Convert_HD_Op4_Intro_Grunts_To_HD_Op4_Massn_Sequences().Invoke();
#endif
#if 0
    Convert_LD_Massn_To_LD_HL1_Grunt_Sequences().Invoke();
#endif
#if 0
    Convert_HD_Massn_To_HD_HL1_Grunt_Sequences().Invoke();
#endif
#if 0
    Fix_HD_Crossbow_Sequences().Invoke();
#endif


    //convert_HD_HL1_animations_to_LD_BlueShift(); // OBSOLETE !! DON'T DO

#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}

#pragma once

#include "Oyl3D/oylpch.h"

namespace oyl
{
    class Mesh;
    class VertexArray;
    
    namespace internal
    {
        class AnimationSystem;
        class RenderSystem;
        class SkeletalAnimationSystem;
    }
    
    struct Animation
    {
        struct KeyPose
        {
            Ref<Mesh> mesh;
            f32       duration = 0.0f;
        };

        std::vector<KeyPose> poses;
    };
}

namespace oyl::component
{    
    class VertexAnimatable
    {
    public:
        void pushAnimation(const std::string& alias, const Ref<Animation>& animation);

        void setNextAnimation(const std::string& alias, f32 transitionDuration = 0.0f);
        
        bool getBool(const std::string& name) const;
        void setBool(const std::string& name, bool value);

        const Ref<VertexArray>& getVertexArray() const;
    private:
        friend internal::AnimationSystem;
        friend internal::RenderSystem;
        std::unordered_map<std::string, Ref<Animation>> m_animations;

        std::unordered_map<std::string, bool> m_bools;

        Ref<Animation> m_currentAnimation;
        Ref<Animation> m_nextAnimation;

        Ref<VertexArray> m_vao;

        f32 m_currentElapsed = 0.0f;

        f32 m_transitionElapsed  = 0.0f;
        f32 m_transitionDuration = 0.0f;
    };

    struct SkeletonAnimatable
    {
        bool enabled = true;
        
        std::string animation;

        float time = 0.0f;

    private:
        std::string prevAnimation;

        friend ::oyl::internal::SkeletalAnimationSystem;
    };
}

#include "Animatable.inl"

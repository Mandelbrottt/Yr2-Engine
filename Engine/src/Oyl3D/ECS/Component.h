#pragma once

namespace oyl
{
    class Camera;
    class Material;
    class Texture2D;
    class TextureCubeMap;
    class Mesh;
    class VertexArray;

    namespace internal
    {
        class AnimationSystem;
        class PhysicsSystem;
        class RenderSystem;
        class TransformUpdateSystem;
        class GuiLayer;
    }
    
}

namespace oyl::component
{ 
    struct SceneObject
    {
        std::string name;
    };

    // TODO: Move larger components into seperate files?
    class Transform
    {
    public:        
        glm::vec3 getPosition()  const;
        f32       getPositionX() const;
        f32       getPositionY() const;
        f32       getPositionZ() const;

        glm::vec3 getPositionGlobal()  const;
        f32       getPositionXGlobal() const;
        f32       getPositionYGlobal() const;
        f32       getPositionZGlobal() const;
        
        glm::vec3 getRotationEuler()  const;
        f32       getRotationEulerX() const;
        f32       getRotationEulerY() const;
        f32       getRotationEulerZ() const;

        glm::quat getRotation() const;

        glm::vec3 getRotationEulerGlobal()  const;
        f32       getRotationEulerXGlobal() const;
        f32       getRotationEulerYGlobal() const;
        f32       getRotationEulerZGlobal() const;

        glm::quat getRotationGlobal() const;
        
        glm::vec3 getScale()  const;
        f32       getScaleX() const;
        f32       getScaleY() const;
        f32       getScaleZ() const;

        glm::vec3 getScaleGlobal()  const;
        f32       getScaleXGlobal() const;
        f32       getScaleYGlobal() const;
        f32       getScaleZGlobal() const;

        const glm::mat4& getMatrix()       const;
        glm::mat4        getMatrixGlobal() const;

        glm::vec3 getForward() const;
        glm::vec3 getRight()   const;
        glm::vec3 getUp()      const;

        glm::vec3 getForwardGlobal() const;
        glm::vec3 getRightGlobal()   const;
        glm::vec3 getUpGlobal()      const;

        void setPosition(glm::vec3 position);
        void setPositionX(f32 x);
        void setPositionY(f32 y);
        void setPositionZ(f32 z);

        void translate(glm::vec3 move, bool inLocalSpace = true);

        void setRotationEuler(glm::vec3 rotation);
        void setRotationEulerX(f32 x);
        void setRotationEulerY(f32 y);
        void setRotationEulerZ(f32 z);

        void rotate(glm::vec3 euler);

        void setRotation(glm::quat rotation);
        
        void setScale(glm::vec3 scale);
        void setScaleX(f32 x);
        void setScaleY(f32 y);
        void setScaleZ(f32 z);

        void scale(glm::vec3 scale);

        bool isLocalDirty() const;

    private:
        friend internal::PhysicsSystem;
        friend internal::TransformUpdateSystem;
        friend internal::GuiLayer;
        
        glm::vec3 m_localPosition      = glm::vec3(0.0f);
        glm::quat m_localRotation      = {};
        glm::vec3 m_localScale         = glm::vec3(1.0f);

        glm::vec3 m_deltaPosition = glm::vec3(0.0f);
        glm::vec3 m_deltaRotation = glm::vec3(0.0f);
        glm::vec3 m_deltaScale    = glm::vec3(0.0f);
        
        mutable glm::mat4 m_localMatrix = glm::mat4(1.0f);

        Ref<Transform>     m_localRef  = nullptr;
        WeakRef<Transform> m_parentRef = WeakRef<Transform>{};

        mutable bool m_isLocalDirty = true;
        
        bool m_isPositionOverridden = true;
        bool m_isRotationOverridden = true;
        bool m_isScaleOverridden    = true;
    };

    struct Renderable
    {
        Ref<Mesh>      mesh;
        Ref<Material>  material;

        bool enabled = true;
    };

    struct GuiRenderable
    {
        Ref<Texture2D> texture;

        bool enabled = true;
    };

    // Morph Target Animation
    struct Animation
    {
        struct KeyPose
        {
            Ref<Mesh> mesh;
            f32       duration;
        };

        std::vector<KeyPose> poses;
    };

    //struct Script
    //{
    //    void (*onUpdateCallback)(entt::registry&, entt::entity, Transform&, Timestep);
    //    void (*onCollisionEnterCallback)();
    //    void (*onCustomEventCallback)(entt::registry&, entt::entity, UniqueRef<Event>);
    //    
    //private:
    //    entt::registry m_reg;
    //    entt::entity m_entity;
    //};

    //#define getComponent(component_type) _registry.get<component_type>(_entity)
    //#define CALLBACK_ARGS entt::registry& _registry, entt::entity _entity, Transform& transform
    //
    //void foo()
    //{
    //    Script s;
    //    s.onUpdateCallback =
    //        [](CALLBACK_ARGS, Timestep dt)
    //        {
    //            for (int i = 0; i < 10; i++)
    //            {
    //                transform.setPosition(glm::vec3(0, 0, 0));
    //
    //                auto& something = getComponent(Parent);
    //            }
    //        };
    //
    //    s.onCustomEventCallback = [](entt::registry& registry, entt::entity entity, UniqueRef<Event> event)
    //    {
    //        switch (event->type)
    //        {
    //            case 
    //        }
    //    };
    //
    //
    //    registry->assign<Script>(e, s);
    //}
    
    class Animator
    {
    public:
        void pushAnimation(const std::string& alias, const Ref<Animation>& animation);

        void setNextAnimation(const std::string& alias, f32 transitionDuration = 0.0f);
        
        bool getBool(const std::string& name) const;
        void setBool(const std::string& name, bool value);

        const Ref<VertexArray>& getVertexArray() const;
    private:
        friend oyl::internal::AnimationSystem;
        friend oyl::internal::RenderSystem;
        std::unordered_map<std::string, Ref<Animation>> m_animations;

        std::unordered_map<std::string, bool> m_bools;

        Ref<Animation> m_currentAnimation;
        Ref<Animation> m_nextAnimation;

        Ref<VertexArray> m_vao;

        f32 m_currentElapsed = 0.0f;

        f32 m_transitionElapsed  = 0.0f;
        f32 m_transitionDuration = 0.0f;
    };
    
    enum class Direction
    {
        X_AXIS, Y_AXIS, Z_AXIS
    };

    namespace internal
    {
        class BaseCollider
        {
        public:
            
            glm::vec3 getCenter() const;

            void setCenter(glm::vec3 center);

            bool isDirty() const;

        protected:
            glm::vec3 m_center = glm::vec3(0.0f);

            bool m_isDirty = true;

            BaseCollider() = default;
            BaseCollider& operator =(const BaseCollider& other);

        private:
            friend oyl::internal::PhysicsSystem;
        };

        class BoxCollider : public BaseCollider
        {
        public:
            // TODO: Add getWidth() and so on
            glm::vec3 getSize() const;

            void setSize(glm::vec3 size);

        private:
            glm::vec3 m_size = glm::vec3(1.0f);
        };

        class SphereCollider : public BaseCollider
        {
        public:
            f32 getRadius() const;

            void setRadius(f32 radius);

        private:
            f32 m_radius = 0.5f;
        };

        class CapsuleCollider : public BaseCollider
        {
        public:
            f32       getRadius()    const;
            f32       getHeight()    const;
            Direction getDirection() const;

            void setRadius(f32 radius);
            void setHeight(f32 height);
            void setDirection(Direction direction);

        private:
            f32       m_radius    = 0.5f;
            f32       m_height    = 1.0f;
            Direction m_direction = Direction::Y_AXIS;
        };

        class CylinderCollider : public BaseCollider
        {
        public:
            f32       getRadius()    const;
            f32       getHeight()    const;
            Direction getDirection() const;

            void setRadius(f32 radius);
            void setHeight(f32 height);
            void setDirection(Direction direction);

        private:
            f32       m_radius = 0.5f;
            f32       m_height = 1.0f;
            Direction m_direction = Direction::Y_AXIS;
        };

        class OYL_DEPRECATED("Not fully implemented.")
            MeshCollider : public BaseCollider
        {
        public:
            MeshCollider() = default;
            ~MeshCollider() = default;
            MeshCollider(const MeshCollider& other) : BaseCollider(other), m_mesh(other.m_mesh) {}

            const Ref<Mesh>& getMesh() const;

            void setMesh(Ref<Mesh> mesh);

        private:
            Ref<Mesh> m_mesh = nullptr;
        };
    }

    class Collider
    {
    public:
        // TODO: Inherit from bullet3 classes
        struct ShapeInfo
        {
            ShapeInfo();
            ShapeInfo(OylEnum type);
            ShapeInfo(const ShapeInfo& shapeInfo);
            ~ShapeInfo() {}

            ShapeInfo& operator=(const ShapeInfo& shapeInfo);

            OylEnum getType() const;
            void setType(OylEnum type);
            
            bool isTrigger() const;
            void setIsTrigger(bool trigger);

            bool isDirty() const;
            
            union
            {
                internal::BoxCollider      box;
                internal::SphereCollider   sphere;
                internal::CapsuleCollider  capsule;
                internal::CylinderCollider cylinder;
                internal::MeshCollider     mesh;
            };

        private:
            friend oyl::internal::PhysicsSystem;
            friend oyl::internal::GuiLayer;

            void defaultInit();

            Ref<ShapeInfo> m_selfRef;

            OylEnum m_type = None;

            bool m_isTrigger = false;
            bool m_isDirty   = true;
        };

        ShapeInfo& pushShape(ShapeInfo shape);
        ShapeInfo& pushShape(OylEnum type);
        void eraseShape(u32 index);
        
        ShapeInfo& getShape(u32 index);
        const ShapeInfo& getShape(u32 index) const;

        std::vector<ShapeInfo>& getShapes();
        const std::vector<ShapeInfo>& getShapes() const;

        std::size_t size() const;
        bool empty() const;

        bool isDirty() const;

        std::vector<ShapeInfo>::iterator begin();
        std::vector<ShapeInfo>::iterator end();

        std::vector<ShapeInfo>::const_iterator begin() const;
        std::vector<ShapeInfo>::const_iterator end()   const;

    private:
        friend oyl::internal::PhysicsSystem;
        friend oyl::internal::GuiLayer;
        
        std::vector<ShapeInfo> m_shapes;
    };

    class RigidBody
    {
        friend oyl::internal::PhysicsSystem;
    public:
        enum Property : u32
        {
            IS_KINEMATIC      = 0x00,
            DETECT_COLLISIONS = 0x01,
            FREEZE_ROTATION_X = 0x02,
            FREEZE_ROTATION_Y = 0x04,
            FREEZE_ROTATION_Z = 0x08,
            DO_INTERPOLATION  = 0x10,
            USE_GRAVITY       = 0x20,
        };
        
        // TODO: Add getVelocityX() and so on
        glm::vec3 getVelocity() const;
        glm::vec3 getAcceleration() const;
        glm::vec3 getForce() const;
        glm::vec3 getImpulse() const;

        f32 getMass() const;
        f32 getFriction() const;

        bool getProperty(Property prop) const;
        bool getPropertyFlags() const;

        void setVelocity(glm::vec3 velocity);
        void addVelocity(glm::vec3 velocity);

        void setAcceleration(glm::vec3 acceleration);
        void addAcceleration(glm::vec3 acceleration);

        void setForce(glm::vec3 force);
        void addForce(glm::vec3 force);

        void addImpulse(glm::vec3 impulse);

        void setMass(f32 mass);
        void setFriction(f32 friction);

        void overwritePropertyFlags(u32 flags);
        void setProperties(u32 flags, bool value);

    private:
        // TODO: Add more stuff
        glm::vec3 m_velocity     = { 0, 0, 0 };
        glm::vec3 m_force        = { 0, 0, 0 };
        glm::vec3 m_impulse      = { 0, 0, 0 };

        f32 m_mass = 1.0f;
        f32 m_friction = 0.5f;

        u32 m_propertyFlags = DETECT_COLLISIONS | 
                              DO_INTERPOLATION | 
                              USE_GRAVITY;

        bool m_isDirty = true;
    };

    struct PlayerCamera
    {
        i32 player = -1;

        // TEMPORARY:
        glm::mat4 projection;

        Ref<TextureCubeMap> skybox;
        
        //Ref<Camera> camera;
    };

    struct Parent
    {
        entt::entity parent;
    };

    // TODO: Attenuation
    struct DirectionalLight
    {
        glm::vec3 direction = glm::vec3(-1.0f);
        
        glm::vec3 ambient = glm::vec3(0.2f);
        glm::vec3 diffuse = glm::vec3(0.5f);
        glm::vec3 specular = glm::vec3(1.0f);
    };

    struct PointLight
    {
        glm::vec3 ambient = glm::vec3(0.2f);
        glm::vec3 diffuse = glm::vec3(0.5f);
        glm::vec3 specular = glm::vec3(1.0f);
    };

    struct SpotLight
    {
        glm::vec3 direction;
        f32       angle;

        glm::vec3 ambient = glm::vec3(0.2f);
        glm::vec3 diffuse = glm::vec3(0.5f);
        glm::vec3 specular = glm::vec3(1.0f);
    };

    namespace internal
    {
        struct EditorCamera
        {
            Ref<Camera> camera;
        };

        struct ExcludeFromHierarchy { };
    }
}

#include "Component.inl"
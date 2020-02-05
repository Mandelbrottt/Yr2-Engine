#include "oylpch.h"

#include "App/Window.h"

#include "Components/Animatable.h"
#include "Components/Camera.h"
#include "Components/Lights.h"
#include "Components/Misc.h"
#include "Components/Renderable.h"
#include "Components/Transform.h"

#include "ECS/System.h"
#include "ECS/SystemImpl.h"

#include "Events/Event.h"
#include "Events/EventListener.h"
#include "Events/EventDispatcher.h"

#include "Graphics/EditorCamera.h"
#include "Graphics/Material.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"

#include "Input/Input.h"

#include "Physics/Raycast.h"

#include "Rendering/Renderer.h"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

namespace std
{
    template<>
    struct hash<std::pair<entt::entity, entt::entity>>
    {
        std::size_t operator()(const std::pair<entt::entity, entt::entity>& k) const noexcept
        {
            using std::size_t;
            using std::hash;
            using oyl::u32;

            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return ((hash<u32>()(static_cast<u32>(k.first))
                     ^ (hash<u32>()(static_cast<u32>(k.second)) << 1)) >> 1);
        }
    };
}

namespace oyl
{
    // vvv Generic System vvv //

    void System::onEnter() { }

    void System::onExit() { }

    void System::onUpdate() { }

    void System::onGuiRender() { }

    // ^^^ Generic System ^^^ //

    namespace internal
    {
        // vvv Render System vvv //

        void RenderSystem::onEnter()
        {
            listenForEventType(EventType::WindowResized);
        }

        void RenderSystem::onExit() { }

        void RenderSystem::onUpdate()
        {
            using component::Transform;
            using component::Renderable;
            using component::Camera;
            using component::PointLight;

            const auto& skybox = TextureCubeMap::get(DEFAULT_SKYBOX_ALIAS);
            const auto& shader = Shader::get(SKYBOX_SHADER_ALIAS);
            const auto& mesh = Mesh::get(CUBE_MESH_ALIAS);

            auto isRenderableValid = [](const Renderable& r)
            {
                return r.enabled && r.mesh && r.material && r.material->shader && r.material->albedoMap;
            };

            // We sort our mesh renderers based on material properties
            // This will group all of our meshes based on shader first, then material second
            registry->sort<Renderable>(
                [&isRenderableValid](const Renderable& lhs, const Renderable& rhs)
                {
                    if (!isRenderableValid(lhs))
                        return false;
                    if (!isRenderableValid(rhs))
                        return true;
                    if (lhs.material->shader != rhs.material->shader)
                        return lhs.material->shader < rhs.material->shader;
                    if (lhs.material->albedoMap != rhs.material->albedoMap)
                        return lhs.material->albedoMap < rhs.material->albedoMap;
                    return lhs.material < rhs.material;
                });

            Ref<Material> boundMaterial;

            static size_t lastNumCameras = 0;
            
            auto camView = registry->view<Transform, Camera>();
            
            int x = m_windowSize.x / 2;
            int y = camView.size() > 2 ? m_windowSize.y / 2 : 0;

            int width = m_windowSize.x;
            if (camView.size() > 1) width /= 2;

            int height = m_windowSize.y;
            if (camView.size() > 2) height /= 2;
            
            for (auto camera : camView)
            {
                Camera& pc = camView.get<Camera>(camera);

                if (!pc.m_forwardFrameBuffer)
                {
                    pc.m_forwardFrameBuffer = FrameBuffer::create(1);
                    pc.m_forwardFrameBuffer->initDepthTexture(1, 1);
                    pc.m_forwardFrameBuffer->initColorTexture(0, 1, 1,
                                                              TextureFormat::RGBA8,
                                                              TextureFilter::Nearest,
                                                              TextureWrap::ClampToEdge);
                }

                if (m_camerasNeedUpdate || lastNumCameras != camView.size())
                {
                    pc.m_forwardFrameBuffer->updateViewport(width, height);

                    pc.aspect((float) width / (float) height);
                }

                //if (m_intermediateFrameBuffer->getWidth() != width || 
                //    m_intermediateFrameBuffer->getHeight() != height)
                //    m_intermediateFrameBuffer->updateViewport(width, height);

                pc.m_forwardFrameBuffer->clear();
                pc.m_forwardFrameBuffer->bind();
                
                RenderCommand::setDrawRect(0, 0, width, height);

                glm::mat4 viewProj = pc.projectionMatrix();
                viewProj *= glm::mat4(glm::mat3(pc.viewMatrix()));
                
                shader->bind();
                shader->setUniformMat4("u_viewProjection", viewProj);

                RenderCommand::setDepthDraw(false);
                RenderCommand::setBackfaceCulling(false);
                Renderer::submit(mesh, shader, skybox);
                RenderCommand::setBackfaceCulling(true);
                RenderCommand::setDepthDraw(true);

                bool doCulling = true;

                auto view = registry->view<Renderable, Transform>();
                for (auto entity : view)
                {
                    Renderable& mr = view.get<Renderable>(entity);
                    
                    if (!isRenderableValid(mr))
                        break;

                    if (!(mr.cullingMask & pc.cullingMask))
                        continue;
                    
                    if (mr.material != boundMaterial)
                    {
                        boundMaterial = mr.material;
                        
                        boundMaterial->setUniformMat4("u_view", pc.viewMatrix());
                        boundMaterial->setUniformMat4("u_viewProjection", pc.viewProjectionMatrix());
                        glm::mat4 viewNormal = inverse(transpose(pc.viewMatrix()));
                        boundMaterial->setUniformMat3("u_viewNormal", glm::mat3(viewNormal));

                        auto lightView = registry->view<PointLight>();
                        int count = 0;
                        for (auto light : lightView)
                        {
                            auto lightProps =     lightView.get(light);
                            auto lightTransform = registry->get<Transform>(light);
                            
                            boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].position",
                                                        pc.viewMatrix() * glm::vec4(lightTransform.getPositionGlobal(), 1.0f));
                            boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].ambient",
                                                        lightProps.ambient);
                            boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].diffuse",
                                                        lightProps.diffuse);
                            boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].specular",
                                                        lightProps.specular);
                            count++;
                        }

                        // TEMPORARY:
                        boundMaterial->bind();
                        boundMaterial->applyUniforms();
                    }

                    auto& transformComponent = view.get<Transform>(entity);
                    glm::mat4 transform = transformComponent.getMatrixGlobal();

                    glm::bvec3 mirror = transformComponent.getMirrorGlobal();
                    if (!(mirror.x ^ mirror.y ^ mirror.z) != doCulling)
                    {
                        doCulling ^= 1;
                        RenderCommand::setBackfaceCulling(doCulling);
                    }

                    if (registry->has<component::Animatable>(entity))
                    {
                        auto& anim = registry->get<component::Animatable>(entity);
                        if (anim.getVertexArray())
                        {
                            anim.m_vao->bind();
                            
                            boundMaterial->shader->bind();
                            boundMaterial->shader->setUniform1f("lerpT_curr", glm::mod(anim.m_currentElapsed, 1.0f));
                            boundMaterial->shader->setUniform1f("lerpT_trans", glm::mod(anim.m_transitionElapsed, 1.0f));

                            Renderer::submit(boundMaterial, anim.m_vao, mr.mesh->getNumVertices(), transform);
                        }
                    }
                    else
                    {
                        Renderer::submit(mr.mesh, boundMaterial, transform);
                    }
                }
            }

            lastNumCameras = camView.size();
            m_camerasNeedUpdate = false;
        }

        void RenderSystem::onGuiRender() { }

        bool RenderSystem::onEvent(const Event& event)
        {
            switch (event.type)
            {
                case EventType::WindowResized:
                    auto e = event_cast<WindowResizedEvent>(event);
                    m_windowSize = { e.width, e.height };

                    //m_forwardFrameBuffer->updateViewport(e.width, e.height);
                    ////m_intermediateFrameBuffer->updateViewport(e.width, e.height);

                    //ViewportHandleChangedEvent hcEvent;
                    //hcEvent.handle = m_forwardFrameBuffer->getColorHandle(0);
                    //m_dispatcher->postEvent(hcEvent);

                    m_camerasNeedUpdate = true;

                    break;
            }
            return false;
        }

        // ^^^ Render System ^^^ //

        // vvv Gui Render System vvv //

        void GuiRenderSystem::onEnter()
        {
            listenForEventType(EventType::WindowResized);

            m_shader = Shader::create(
                {
                    { Shader::Vertex, ENGINE_RES + "shaders/gui.vert" },
                    { Shader::Pixel, ENGINE_RES + "shaders/gui.frag" }
                });

            float vertices[] = {
                -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
                 0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
                 0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
                -0.5f,  0.5f, 0.0f, 0.0f, 0.0f
            };

            u32 indices[] = {
                0, 2, 1,
                0, 3, 2
            };

            m_vao = VertexArray::create();
            Ref<VertexBuffer> vbo = VertexBuffer::create(vertices, sizeof(vertices));
            vbo->setLayout({
                { DataType::Float3, "in_position" },
                { DataType::Float2, "in_texCoord" }
            });
            Ref<IndexBuffer> ebo = IndexBuffer::create(indices, 6);
            m_vao->addVertexBuffer(vbo);
            m_vao->addIndexBuffer(ebo);
        }

        void GuiRenderSystem::onExit() {}

        void GuiRenderSystem::onUpdate()
        {
            using component::GuiRenderable;
            using component::Transform;
            using component::Camera;

            registry->sort<GuiRenderable>(
                [this](const entt::entity lhs, const entt::entity rhs)
                {
                    auto& lguir = registry->get<GuiRenderable>(lhs);
                    auto& rguir = registry->get<GuiRenderable>(rhs);

                    if (!lguir.enabled || lguir.texture == nullptr)
                        return false;
                    if (!rguir.enabled || rguir.texture == nullptr)
                        return true;

                    auto& lt = registry->get<Transform>(lhs);
                    auto& rt = registry->get<Transform>(rhs);

                    if (float lz = lt.getPositionZGlobal(), rz = rt.getPositionZGlobal();
                        lz != rz)
                        return lz > rz;

                    return lguir.texture < rguir.texture;
                });

            Ref<Texture2D> boundTexture;

            m_shader->bind();
            m_shader->setUniform1i("u_texture", 0);

            RenderCommand::setDepthDraw(false);

            auto camView = registry->view<Camera>();

            int x = m_windowSize.x / 2;
            int y = camView.size() > 2 ? m_windowSize.y / 2 : 0;

            int width = m_windowSize.x;
            if (camView.size() > 1) width /= 2;

            int height = m_windowSize.y;
            if (camView.size() > 2) height /= 2;

            glm::vec2 lastLowerClipping = glm::vec2(0.0f);
            glm::vec2 lastUpperClipping = glm::vec2(0.0f);
            
            for (auto camera : camView)
            {                
                auto& pc = camView.get(camera);

                pc.m_forwardFrameBuffer->bind();
                
                u32 playerNum = static_cast<u32>(pc.player);
                RenderCommand::setDrawRect(!!(playerNum & 1) * x, !(playerNum & 2) * y, width, height);

                m_shader->setUniformMat4("u_projection", pc.orthoMatrix());
                
                auto view = registry->view<Transform, GuiRenderable>();
                for (auto entity : view)
                {
                    auto& transform = view.get<Transform>(entity);
                    auto& gui = view.get<GuiRenderable>(entity);

                    if (!gui.enabled || !gui.texture)
                        break;

                    if (!(gui.cullingMask & pc.cullingMask))
                        continue;

                    if ((!boundTexture || boundTexture != gui.texture) && gui.texture->isLoaded())
                    {
                        boundTexture = gui.texture;
                        boundTexture->bind(0);
                    }

                    if (lastLowerClipping != gui.lowerClipping || lastUpperClipping != gui.upperClipping)
                    {
                        lastLowerClipping = gui.lowerClipping;
                        lastUpperClipping = gui.upperClipping;
                        glm::vec4 clippingRect = { lastLowerClipping, lastUpperClipping };
                        m_shader->setUniform4f("u_clippingCoords", clippingRect);
                    }

                    glm::vec3 texSize = glm::vec3(0.0f);
                    texSize.x = (float) gui.texture->getWidth() / (float) gui.texture->getHeight();
                    texSize.y = 1.0f;

                    glm::vec3 pos = transform.getPosition();
                    pos.y = -pos.y;
                    pos.z = 0.99f;
                    
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, pos);
                    model = glm::rotate(model, glm::radians(transform.getRotationEuler().z), glm::vec3(0.0f, 0.0f, -1.0f));
                    model = glm::scale(model, transform.getScale());
                    model = glm::scale(model, texSize);

                    Renderer::submit(m_shader, m_vao, model);
                }
            }

            RenderCommand::setDepthDraw(true);
        }

        void GuiRenderSystem::onGuiRender() { }

        bool GuiRenderSystem::onEvent(const Event& event)
        {
            switch (event.type)
            {
                case EventType::WindowResized:
                    auto e = event_cast<WindowResizedEvent>(event);
                    m_windowSize = { e.width, e.height };
                    m_shader->bind();
                    break;
            }
            return false;
        }

        void PostRenderSystem::onEnter()
        {
            listenForEventType(EventType::WindowResized);
            
            m_forwardFrameBuffer = FrameBuffer::create(1);

            m_forwardFrameBuffer->initColorTexture(0, 1, 1,
                                                   TextureFormat::RGBA8,
                                                   TextureFilter::Nearest,
                                                   TextureWrap::ClampToEdge);

            m_intermediateFrameBuffer = FrameBuffer::create(1);

            m_intermediateFrameBuffer->initColorTexture(0, 1, 1,
                                                        TextureFormat::RGBA8,
                                                        TextureFilter::Nearest,
                                                        TextureWrap::ClampToEdge);

            ViewportHandleChangedEvent hcEvent;
            hcEvent.handle = m_forwardFrameBuffer->getColorHandle(0);
            m_dispatcher->postEvent(hcEvent);

            m_shader = Shader::create(
                {
                    { Shader::Vertex, ENGINE_RES + "shaders/fbopassthrough.vert" },
                    { Shader::Pixel, ENGINE_RES + "shaders/fbopassthrough.frag" }
                });

            float vertices[] = {
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
            };

            u32 indices[] = {
                0, 1, 2,
                0, 2, 3
            };

            m_vao = VertexArray::create();

            Ref<VertexBuffer> vbo = VertexBuffer::create(vertices, sizeof(vertices));
            vbo->setLayout({
                { DataType::Float3, "in_position" },
                { DataType::Float2, "in_texCoord" }
            });

            Ref<IndexBuffer> ebo = IndexBuffer::create(indices, 6);
            m_vao->addVertexBuffer(vbo);
            m_vao->addIndexBuffer(ebo);
        }
        
        void PostRenderSystem::onExit() {}
        
        void PostRenderSystem::onUpdate()
        {
            using component::Camera;

            m_forwardFrameBuffer->clear();

            registry->view<Camera>().each([&](Camera& pc)
            {
                uint width = pc.m_forwardFrameBuffer->getWidth(), height = pc.m_forwardFrameBuffer->getHeight();
                if (m_intermediateFrameBuffer->getWidth() != width ||
                    m_intermediateFrameBuffer->getHeight() != height)
                    m_intermediateFrameBuffer->updateViewport(width, height);
                RenderCommand::setDrawRect(0, 0, width, height);
                
                m_vao->bind();

                RenderCommand::setDepthDraw(false);
                Shader* boundShader = nullptr;

                bool needsBlit = false;

                for (auto pass : pc.postProcessingPasses)
                {
                    if (!pass.shader) continue;

                    if (!needsBlit)
                    {
                        m_intermediateFrameBuffer->bind(FrameBufferContext::Write);
                        pc.m_forwardFrameBuffer->bind(FrameBufferContext::Read);
                        pc.m_forwardFrameBuffer->bindColorAttachment(0);
                    }
                    else
                    {
                        pc.m_forwardFrameBuffer->bind(FrameBufferContext::Write);
                        m_intermediateFrameBuffer->bind(FrameBufferContext::Read);
                        m_intermediateFrameBuffer->bindColorAttachment(0);
                    }

                    needsBlit ^= 1;

                    if (boundShader != pass.shader.get())
                    {
                        boundShader = pass.shader.get();
                        boundShader->bind();
                        boundShader->setUniform1i(0, 0);
                    }
                    pass.applyUniforms();

                    RenderCommand::drawIndexed(m_vao);
                }
                RenderCommand::setDepthDraw(true);

                if (needsBlit)
                    m_intermediateFrameBuffer->blit(pc.m_forwardFrameBuffer);

                RenderCommand::setDepthDraw(false);
                RenderCommand::setDrawRect(0, 0, m_windowSize.x, m_windowSize.y);

                m_forwardFrameBuffer->bind(FrameBufferContext::Write);
                m_shader->bind();

                //auto& pc = view.get<Camera>(camera);

                pc.m_forwardFrameBuffer->bindColorAttachment(0);

                m_shader->setUniform1i("u_texture", 0);

                glm::vec3 translation(0.0f);
                glm::vec3 scale(1.0f);

                if (registry->size<Camera>() > 1)
                    translation.x = 0.5f, scale.x = 0.5f;
                if (registry->size<Camera>() > 2)
                    translation.y = 0.5f, scale.y = 0.5f;

                uint playerNum = static_cast<uint>(pc.player);

                if (playerNum ^ 0x01)
                    translation.x *= -1;
                if (playerNum ^ 0x02)
                    translation.y *= -1;

                glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
                model           = glm::scale(model, scale);

                Renderer::submit(m_shader, m_vao, model);
            });

            m_forwardFrameBuffer->unbind();

            #if defined(OYL_DISTRIBUTION)
                    m_forwardFrameBuffer->blit();
            #endif

            RenderCommand::setDepthDraw(true);
        }
        
        void PostRenderSystem::onGuiRender() {}
        
        bool PostRenderSystem::onEvent(const Event& event)
        {
            switch (event.type)
            {
                case EventType::WindowResized:
                    auto e = event_cast<WindowResizedEvent>(event);
                    m_windowSize = { e.width, e.height };

                    m_forwardFrameBuffer->updateViewport(e.width, e.height);
                    //m_intermediateFrameBuffer->updateViewport(e.width, e.height);

                    ViewportHandleChangedEvent hcEvent;
                    hcEvent.handle = m_forwardFrameBuffer->getColorHandle(0);
                    m_dispatcher->postEvent(hcEvent);

                    break;
            }
            return false;
        }

        // ^^^ Gui Render System ^^^ //
        
        // vvv Animation System vvv //
        
        void AnimationSystem::onEnter() {}

        void AnimationSystem::onExit() {}

        void AnimationSystem::onUpdate()
        {
            auto view = registry->view<component::Animatable>();
            for (auto entity : view)
            {
                auto& anim = view.get(entity);

                if (!anim.m_currentAnimation)
                    anim.m_currentAnimation = anim.m_animations.begin()->second;

                if (!anim.m_vao)
                {
                    anim.m_vao = VertexArray::create();

                    anim.m_vao->addVertexBuffer(anim.m_currentAnimation->poses[0].mesh->m_vbo);
                    anim.m_vao->addVertexBuffer(anim.m_currentAnimation->poses[1].mesh->m_vbo);
                }

                if (anim.m_nextAnimation)
                {
                    anim.m_transitionElapsed += Time::deltaTime() / anim.m_transitionDuration;
                    if (anim.m_transitionElapsed >= 1.0f)
                    {
                        anim.m_currentAnimation = anim.m_nextAnimation;
                        anim.m_nextAnimation.reset();
                        anim.m_transitionElapsed  = 0.0f;
                        anim.m_transitionDuration = 0.0f;
                        anim.m_currentElapsed     = -0.0001f;
                    }
                }

                anim.m_currentElapsed =
                    glm::mod(anim.m_currentElapsed, (f32) anim.m_currentAnimation->poses.size());

                uint lastVal = glm::floor(anim.m_currentElapsed);
                anim.m_currentElapsed += Time::deltaTime() / anim.m_currentAnimation->poses[lastVal].duration;

                anim.m_currentElapsed =
                    glm::mod(anim.m_currentElapsed, (f32) anim.m_currentAnimation->poses.size());

                uint currVal = glm::floor(anim.m_currentElapsed);

                if (lastVal != currVal)
                {
                    ++lastVal %= anim.m_currentAnimation->poses.size();
                    ++currVal %= anim.m_currentAnimation->poses.size();
                    
                    auto lastMeshVbo = anim.m_currentAnimation->poses[lastVal].mesh->m_vbo;
                    auto currMeshVbo = anim.m_currentAnimation->poses[currVal].mesh->m_vbo;
                    
                    anim.m_vao->unload();
                    anim.m_vao->load();

                    anim.m_vao->addVertexBuffer(lastMeshVbo);
                    anim.m_vao->addVertexBuffer(currMeshVbo);
                    if (anim.m_nextAnimation)
                    {
                        auto transMeshVbo = anim.m_nextAnimation->poses[0].mesh->m_vbo;
                        anim.m_vao->addVertexBuffer(transMeshVbo);
                    }
                }
            }
        }

        void AnimationSystem::onGuiRender() {}

        bool AnimationSystem::onEvent(const Event& event) { return false; }

        // ^^^ Animation System ^^^ //
        
        // vvv Physics System vvv //

        static PhysicsSystem* g_currentPhysicsSystem = nullptr;

        static Ref<EventDispatcher> g_dispatcher;
        static Ref<entt::registry>  g_currentRegistry;

        
        static std::unordered_map<std::pair<entt::entity, entt::entity>, std::pair<int, glm::vec3>> g_contactMap;

        static int g_phase = 0;
        static void* g_obj1 = 0;
        static void* g_obj2 = 0;

        static void contactStartedCallback(btPersistentManifold* const& manifold)
        {
            auto body1 = manifold->getBody0();
            auto body2 = manifold->getBody1();

            if (g_phase == 0 && body1 == g_obj1 && body2 == g_obj2)
                return;

            g_phase = 0;
            g_obj1 = (void*) body1;
            g_obj2 = (void*) body2;
            
            auto entity1 = (entt::entity) reinterpret_cast<ENTT_ID_TYPE>(body1->getUserPointer());
            auto entity2 = (entt::entity) reinterpret_cast<ENTT_ID_TYPE>(body2->getUserPointer());

            if (!g_currentRegistry->valid(entity1) || !g_currentRegistry->valid(entity2))
                return;

            PhysicsCollisionEnterEvent event;
            event.entity1 = entity1;
            event.entity2 = entity2;

            if (body1->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE ||
                body2->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
            {
                event.type = EventType::PhysicsTriggerEnter;
            }

            g_dispatcher->postEvent(event);
        }

        static void contactEndedCallback(btPersistentManifold* const& manifold)
        {
            auto body1 = manifold->getBody0();
            auto body2 = manifold->getBody1();

            if (g_phase == 1 && body1 == g_obj1 && body2 == g_obj2)
                return;

            g_phase = 1;
            g_obj1 = (void*) body1;
            g_obj2 = (void*) body2;
            
            auto entity1 = (entt::entity) reinterpret_cast<ENTT_ID_TYPE>(body1->getUserPointer());
            auto entity2 = (entt::entity) reinterpret_cast<ENTT_ID_TYPE>(body2->getUserPointer());

            if (!g_currentRegistry->valid(entity1) || !g_currentRegistry->valid(entity2))
                return;

            PhysicsCollisionExitEvent event;
            event.entity1 = entity1;
            event.entity2 = entity2;

            if (body1->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE ||
                body2->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
            {
                event.type = EventType::PhysicsTriggerExit;
            }
            
            g_dispatcher->postEvent(event);
        }

        static bool contactProcessedCallback(btManifoldPoint& cp, void* obj1, void* obj2)
        {
            auto body1 = reinterpret_cast<btCollisionObject*>(obj1);
            auto body2 = reinterpret_cast<btCollisionObject*>(obj2);

            auto entity1 = (entt::entity) reinterpret_cast<ENTT_ID_TYPE>(body1->getUserPointer());
            auto entity2 = (entt::entity) reinterpret_cast<ENTT_ID_TYPE>(body2->getUserPointer());

            if (!g_currentRegistry->valid(entity1) || !g_currentRegistry->valid(entity2))
                return false;

            //static PhysicsCollisionStayEvent event;
            
            //if (g_phase == 2 && obj1 == g_obj1 && obj2 == g_obj2)
            //    numPoints++;
            //else
            //{
            //    event.entity1 = entity1;
            //    event.entity2 = entity2;
            //    
            //    event.contactPoint = avgContactPoint / static_cast<float>(numPoints);
            //    
            //    if (body1->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE ||
            //        body2->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
            //    {
            //        event.type = EventType::PhysicsTriggerStay;
            //    }

            //    g_dispatcher->postEvent(event);
            //    
            //    avgContactPoint = glm::vec3(0.0f);
            //    numPoints = 1;
            //}

            auto avgCp = (cp.getPositionWorldOnA() + cp.getPositionWorldOnB()) * 0.5f;

            auto pair = std::make_pair(entity1, entity2);
            if (auto it = g_contactMap.find(pair); it != g_contactMap.end())
                it->second.first++, it->second.second += glm::vec3(avgCp.x(), avgCp.y(), avgCp.z());
            else
                g_contactMap[pair] = std::make_pair(1, glm::vec3(avgCp.x(), avgCp.y(), avgCp.z()));
            
            return false;
        }

        void PhysicsSystem::onEnter()
        {
            g_currentPhysicsSystem = this;
            
            listenForEventType(EventType::PhysicsResetWorld);
            
            gContactStartedCallback   = contactStartedCallback;
            gContactEndedCallback     = contactEndedCallback;
            gContactProcessedCallback = contactProcessedCallback;
            
            g_dispatcher = m_dispatcher;

            g_currentRegistry = registry;
            
            m_fixedTimeStep = 1.0f / 60.0f;

            m_btCollisionConfig = UniqueRef<btDefaultCollisionConfiguration>::create();
            m_btDispatcher = UniqueRef<btCollisionDispatcher>::create(m_btCollisionConfig.get());
            m_btBroadphase = UniqueRef<btDbvtBroadphase>::create();
            m_btSolver = UniqueRef<btSequentialImpulseConstraintSolver>::create();
            m_btWorld = UniqueRef<btDiscreteDynamicsWorld>::create(m_btDispatcher.get(), 
                                                                   m_btBroadphase.get(),
                                                                   m_btSolver.get(),
                                                                   m_btCollisionConfig.get());
            
            m_rigidBodies.clear();
            
            //m_world->setGravity(btVector3(0.0f, -9.81f, 0.0f));
            m_btWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));
        }

        void PhysicsSystem::onExit()
        {
            m_btWorld.reset();
            m_btSolver.reset();
            m_btBroadphase.reset();
            m_btDispatcher.reset();
            m_btCollisionConfig.reset();

            m_rigidBodies.clear();
        }

        void PhysicsSystem::onUpdate()
        {
            using component::Transform;
            using component::RigidBody;
            using component::Collidable;

            // TEMPORARY: Change on proper collider integration
            for (auto it = m_rigidBodies.begin(); it != m_rigidBodies.end(); ++it)
            {
                if (registry->valid(it->first) &&
                    !registry->has<RigidBody>(it->first))
                {
                    m_btWorld->removeRigidBody(it->second->body.get());
                    it = m_rigidBodies.erase(it);
                }
            }
            
            auto view = registry->view<Transform, RigidBody>();
            for (auto entity : view)
            {
                auto& transform = view.get<Transform>(entity);
                auto& rigidBody = view.get<RigidBody>(entity);
                auto& collider  = registry->get_or_assign<Collidable>(entity);
                
                processIncomingRigidBody(entity, transform, collider, rigidBody);

                RigidBodyInfo& cachedBody = *m_rigidBodies[entity];

                // TODO: Separate lazy calculations into functions?
                if (transform.m_isPositionOverridden)
                {
                    //m_world->removeRigidBody(m_rigidBodies[entity]->body.get());
                    //this->addRigidBody(entity, transform, rigidBody);
                    
                    btTransform t;
                    if (cachedBody.body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT)
                        cachedBody.motion->getWorldTransform(t);
                    else
                        t = cachedBody.body->getWorldTransform();

                    t.setOrigin(btVector3(transform.getPositionXGlobal(),
                                          transform.getPositionYGlobal(),
                                          transform.getPositionZGlobal()));
                    
                    if (cachedBody.body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT)
                        cachedBody.motion->setWorldTransform(t);
                    else
                        cachedBody.body->setWorldTransform(t);
                    
                    transform.m_isPositionOverridden = false;

                    // TODO: Recursively recalculate every child transform
                }
                
                if (transform.m_isRotationOverridden)
                {
                    btTransform t = cachedBody.body->getWorldTransform();

                    glm::quat rotation = transform.getRotationGlobal();
                    t.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));

                    cachedBody.body->setWorldTransform(t);

                    transform.m_isRotationOverridden = false;
                }
                if (transform.m_isScaleOverridden)
                {
                    cachedBody.shape->setLocalScaling(btVector3(transform.getScaleXGlobal(),
                                                                transform.getScaleYGlobal(),
                                                                transform.getScaleZGlobal()));

                    transform.m_isScaleOverridden = false;
                }
                //if (rigidBody.m_isDirty)
                {
                    
                    // Velocity
                    cachedBody.body->setLinearVelocity(btVector3(rigidBody.m_velocity.x,
                                                                 rigidBody.m_velocity.y,
                                                                 rigidBody.m_velocity.z));

                    // Forces
                    //cachedBody.body->clearForces();

                    m_btWorld->removeRigidBody(cachedBody.body.get());

                    // Friction
                    cachedBody.body->setFriction(rigidBody.m_friction);
                    //cachedBody.body->setSpinningFriction(rigidBody.m_friction);
                    //cachedBody.body->setAnisotropicFriction(btVector3(rigidBody.m_friction, 
                    //                                                  rigidBody.m_friction, 
                    //                                                  rigidBody.m_friction), 
                    //                                        btCollisionObject::CF_ANISOTROPIC_FRICTION);
                    //cachedBody.body->setRollingFriction(rigidBody.m_friction);

                    // Flags
                    int flags = cachedBody.body->getCollisionFlags();

                    if (rigidBody.getProperty(RigidBody::IS_KINEMATIC) || rigidBody.getMass() == 0.0f)
                    {
                        flags |= btRigidBody::CF_KINEMATIC_OBJECT;
                        cachedBody.body->setActivationState(DISABLE_DEACTIVATION);
                        cachedBody.body->activate(true);
                        cachedBody.body->setMassProps(0.0f, btVector3(0.0f, 0.0f, 0.0f));
                        cachedBody.body->setLinearVelocity({ 0.0f, 0.0f, 0.0f });
                        cachedBody.body->setAngularVelocity({ 0.0f, 0.0f, 0.0f });
                    }
                    else
                    {
                        flags &= ~btRigidBody::CF_KINEMATIC_OBJECT;
                        cachedBody.body->setActivationState(ACTIVE_TAG);

                        btVector3 inertia = { 0, 0, 0 };
                        if (rigidBody.m_mass != 0.0f)
                            cachedBody.shape->calculateLocalInertia(rigidBody.m_mass, inertia);

                        cachedBody.body->setMassProps(rigidBody.m_mass, inertia);
                    }

                    cachedBody.body->updateInertiaTensor();

                    flags &= ~btRigidBody::CF_NO_CONTACT_RESPONSE;
                    flags |= rigidBody.getProperty(RigidBody::DETECT_COLLISIONS)
                                 ? 0
                                 : btRigidBody::CF_NO_CONTACT_RESPONSE;

                    cachedBody.body->setCollisionFlags(flags);

                    //m_btWorld->addRigidBody(cachedBody.body.get());

                    //// Rotation Locking
                    //btVector3 inertiaTensor = {};

                    //inertiaTensor.setX(rigidBody.getProperty(RigidBody::FREEZE_ROTATION_X) ? 0.0f : 1.0f);
                    //inertiaTensor.setY(rigidBody.getProperty(RigidBody::FREEZE_ROTATION_Y) ? 0.0f : 1.0f);
                    //inertiaTensor.setZ(rigidBody.getProperty(RigidBody::FREEZE_ROTATION_Z) ? 0.0f : 1.0f);

                    //cachedBody.body->setInvInertiaDiagLocal(inertiaTensor);

                    //cachedBody.body->updateInertiaTensor();

                    // Gravity
                    if (rigidBody.getProperty(RigidBody::USE_GRAVITY))
                    {
                        cachedBody.body->setGravity(m_btWorld->getGravity());
                    }
                    else
                    {
                        cachedBody.body->setGravity({ 0.0f, 0.0f, 0.0f });
                    }
                }
                
                cachedBody.body->setFriction(rigidBody.m_friction);

                float x = rigidBody.getProperty(RigidBody::FREEZE_ROTATION_X) ? 0.0f : 1.0f;
                float y = rigidBody.getProperty(RigidBody::FREEZE_ROTATION_Y) ? 0.0f : 1.0f;
                float z = rigidBody.getProperty(RigidBody::FREEZE_ROTATION_Z) ? 0.0f : 1.0f;
                
                //cachedBody.body->setLinearFactor(btVector3(1, 1, 1));
                cachedBody.body->setAngularFactor(btVector3(x, y, z));

                m_btWorld->addRigidBody(cachedBody.body.get());

                if (transform.m_isPositionOverridden || 
                    transform.m_isRotationOverridden ||
                    transform.m_isScaleOverridden ||
                    rigidBody.m_force != glm::vec3(0.0f) || 
                    rigidBody.m_impulse != glm::vec3(0.0f))
                    cachedBody.body->activate();

                cachedBody.body->applyCentralForce(btVector3(rigidBody.m_force.x,
                                                             rigidBody.m_force.y, 
                                                             rigidBody.m_force.z));
                
                cachedBody.body->applyCentralImpulse(btVector3(rigidBody.m_impulse.x,
                                                               rigidBody.m_impulse.y,
                                                               rigidBody.m_impulse.z));
            }

            // TODO: Iterate over ghost objects with tick callback
            m_btWorld->stepSimulation(Time::deltaTime(), 10, m_fixedTimeStep);

            for (auto& [entities, point] : g_contactMap)
            {
                PhysicsCollisionStayEvent event;
                auto rb1 = registry->try_get<RigidBody>(entities.first);
                auto rb2 = registry->try_get<RigidBody>(entities.second);
                if (rb1 && !rb1->getProperty(RigidBody::DETECT_COLLISIONS) ||
                    rb2 && !rb2->getProperty(RigidBody::DETECT_COLLISIONS))
                {
                    event.type = EventType::PhysicsTriggerStay;
                }

                event.entity1 = entities.first;
                event.entity2 = entities.second;

                event.contactPoint = point.second / static_cast<float>(point.first);

                postEvent(event);
            }

            g_obj1 = g_obj2 = 0;
            g_contactMap.clear();

            for (auto entity : view)
            {
                auto& transform = view.get<Transform>(entity);
                auto& rigidBody = view.get<RigidBody>(entity);

                if (rigidBody.getProperty(RigidBody::IS_KINEMATIC))
                    continue;

                RigidBodyInfo& cachedBody = *m_rigidBodies[entity];
                
                btVector3 _pos = {};
                btQuaternion _rot = {};
                
                btTransform t;
                cachedBody.motion->getWorldTransform(t);

                _pos = t.getOrigin();
                _rot = t.getRotation();

                if (auto p = transform.getParent(); p == nullptr)
                {
                    transform.m_localPosition = { _pos.x(), _pos.y(), _pos.z() };
                    transform.m_localRotation = glm::quat(_rot.w(), _rot.x(), _rot.y(), _rot.z());
                }
                else
                {
                    auto _t = p->getMatrixGlobal();
                    transform.m_localPosition = 
                        inverse(_t) * glm::vec4(_pos.x(), _pos.y(), _pos.z(), 1.0f);
                    transform.m_localRotation =
                        inverse(quat_cast(_t)) * glm::quat(_rot.w(), _rot.x(), _rot.y(), _rot.z());
                }
                
                transform.m_isLocalDirty = true;

                rigidBody.m_velocity = glm::vec3(cachedBody.body->getLinearVelocity().x(),
                                                 cachedBody.body->getLinearVelocity().y(), 
                                                 cachedBody.body->getLinearVelocity().z());

                rigidBody.m_force = glm::vec3(cachedBody.body->getTotalForce().x(),
                                              cachedBody.body->getTotalForce().y(), 
                                              cachedBody.body->getTotalForce().z());

                rigidBody.m_impulse = glm::vec3(0.0f);
            }
        }

        void PhysicsSystem::onGuiRender()
        {
            using component::Transform;
            using component::RigidBody;
            using component::EntityInfo;
            auto view = registry->view<EntityInfo, RigidBody>();

            ImGui::Begin("Physics");

            for (auto entity : view)
            {
                auto& rb = view.get<RigidBody>(entity);
                ImGui::Text("%s: X(%.3f) Y(%.3f) Z(%.3f)",
                            view.get<EntityInfo>(entity).name.c_str(),
                            rb.getVelocity().x,
                            rb.getVelocity().y, 
                            rb.getVelocity().z);
            }

            ImGui::End();
        }

        bool PhysicsSystem::onEvent(const Event& event)
        {
            switch (event.type)
            {
                case EventType::PhysicsResetWorld:
                {
                    onExit();
                    onEnter();
                    
                    return true;    
                }
            }
            return false;
        }

        // TODO: Currently need rigidbody for collider to register in world, make mutually exclusive
        void PhysicsSystem::processIncomingRigidBody(entt::entity entity, 
                                                     const component::Transform&  transformComponent, 
                                                     const component::Collidable& colliderComponent, 
                                                     const component::RigidBody&  rigidBodyComponent)
        {
            // Check if collider was emptied past frame
            if (colliderComponent.empty())
            {
                if (m_rigidBodies.find(entity) != m_rigidBodies.end()) 
                {
                    m_btWorld->removeRigidBody(m_rigidBodies.at(entity)->body.get());
                    m_rigidBodies.erase(entity);
                }
                return;
            }
            
            // TODO: Don't get rid of the body from the world, just update it
            if (colliderComponent.isDirty() && 
                m_rigidBodies.find(entity) != m_rigidBodies.end())
            {
                m_btWorld->removeRigidBody(m_rigidBodies.at(entity)->body.get());
                m_rigidBodies.erase(entity);

                for (auto& shape : const_cast<component::Collidable&>(colliderComponent))
                {
                    shape.m_isDirty          = false;
                    shape.box.m_isDirty      = false;
                    shape.sphere.m_isDirty   = false;
                    shape.capsule.m_isDirty  = false;
                    shape.cylinder.m_isDirty = false;
                    //shape.mesh.m_isDirty     = false;
                }
            }
            
            if (m_rigidBodies.find(entity) == m_rigidBodies.end())
            {
                // Add RigidBody to World

                Ref<btCollisionShape> shape  = nullptr;
                Ref<btMotionState>    motion = nullptr;
                Ref<btRigidBody>      body   = nullptr;

                m_rigidBodies[entity] = Ref<RigidBodyInfo>::create();

                if (colliderComponent.size() == 1)
                {
                    btTransform t;

                    const auto& shapeThing = colliderComponent.getShape(0);
                    switch (shapeThing.m_type)
                    {
                        case ColliderType::Box:
                        {
                            t.setIdentity();

                            t.setOrigin({
                                shapeThing.box.getCenter().x,
                                shapeThing.box.getCenter().y,
                                shapeThing.box.getCenter().z
                            });

                            btVector3 halfExtents = {
                                shapeThing.box.getSize().x / 2.0f,
                                shapeThing.box.getSize().y / 2.0f,
                                shapeThing.box.getSize().z / 2.0f
                            };

                            shape = Ref<btBoxShape>::create(halfExtents);
                            break;
                        }
                        case ColliderType::Sphere:
                        {
                            t.setIdentity();

                            t.setOrigin({
                                shapeThing.sphere.getCenter().x,
                                shapeThing.sphere.getCenter().y,
                                shapeThing.sphere.getCenter().z
                            });

                            shape = Ref<btSphereShape>::create(shapeThing.sphere.getRadius());
                            break;
                        }
                        case ColliderType::Capsule:
                        {
                            t.setIdentity();

                            t.setOrigin({
                                shapeThing.capsule.getCenter().x,
                                shapeThing.capsule.getCenter().y,
                                shapeThing.capsule.getCenter().z
                            });
                            
                            switch (shapeThing.capsule.getDirection())
                            {
                                case Direction::X_AXIS:
                                    shape = Ref<btCapsuleShapeX>::create(shapeThing.capsule.getRadius(),
                                                                         shapeThing.capsule.getHeight());
                                    break;
                                case Direction::Y_AXIS:
                                    shape = Ref<btCapsuleShape>::create(shapeThing.capsule.getRadius(),
                                                                        shapeThing.capsule.getHeight());
                                    break;
                                case Direction::Z_AXIS:
                                    shape = Ref<btCapsuleShapeZ>::create(shapeThing.capsule.getRadius(),
                                                                         shapeThing.capsule.getHeight());
                                    break;
                            }
                            break;
                        }
                    }
                    
                    RigidBodyInfo::ChildShapeInfo cInfo;

                    cInfo.btShape = shape;

                    cInfo.shapeInfo = shapeThing.m_selfRef;

                    m_rigidBodies[entity]->children.emplace_back(cInfo);

                    // TEMPORARY: Make relative to collider
                    t.setIdentity();
                    
                    //t.setFromOpenGLMatrix(value_ptr(transformComponent.getMatrixGlobal()));

                    t.setOrigin(btVector3(transformComponent.getPositionXGlobal(),
                                          transformComponent.getPositionYGlobal(),
                                          transformComponent.getPositionZGlobal()));

                    glm::quat q = transformComponent.getRotationGlobal();

                    btQuaternion btq = btQuaternion(q.x, q.y, q.z, q.w);
                    t.setRotation(btq);
                    
                    btVector3 inertia = { 0, 0, 0 };
                    if (rigidBodyComponent.getMass() != 0.0f)
                        shape->calculateLocalInertia(rigidBodyComponent.getMass(), inertia);

                    shape->setLocalScaling({
                        transformComponent.getScaleXGlobal(),
                        transformComponent.getScaleYGlobal(),
                        transformComponent.getScaleZGlobal()
                    });

                    motion = Ref<btDefaultMotionState>::create(t);

                    btRigidBody::btRigidBodyConstructionInfo info(rigidBodyComponent.getMass(),
                                                                  motion.get(),
                                                                  shape.get(),
                                                                  inertia);

                    body = Ref<btRigidBody>::create(info);
                }
                else
                {
                    auto workingShape = Ref<btCompoundShape>::create();
                    Ref<btCollisionShape> childShape;
                    auto childIter = colliderComponent.begin();
                    for (; childIter != colliderComponent.end(); ++childIter)
                    {
                        switch (childIter->m_type)
                        {
                            case ColliderType::Box:
                            {
                                btTransform t;

                                t.setIdentity();

                                t.setOrigin({
                                    childIter->box.getCenter().x,
                                    childIter->box.getCenter().y,
                                    childIter->box.getCenter().z
                                });
                                
                                btVector3 halfExtents = {
                                    childIter->box.getSize().x / 2.0f,
                                    childIter->box.getSize().y / 2.0f,
                                    childIter->box.getSize().z / 2.0f
                                };
                                
                                childShape = Ref<btBoxShape>::create(halfExtents);
                                workingShape->addChildShape(t, childShape.get());
                                break;
                            }
                            case ColliderType::Sphere:
                            {
                                btTransform t;

                                t.setIdentity();
                                
                                t.setOrigin({
                                    childIter->sphere.getCenter().x,
                                    childIter->sphere.getCenter().y,
                                    childIter->sphere.getCenter().z
                                });

                                childShape = Ref<btSphereShape>::create(childIter->sphere.getRadius());
                                workingShape->addChildShape(t, childShape.get());
                                break;
                            }
                            case ColliderType::Capsule:
                            {
                                btTransform t;

                                t.setIdentity();

                                t.setOrigin({
                                    childIter->capsule.getCenter().x,
                                    childIter->capsule.getCenter().y,
                                    childIter->capsule.getCenter().z
                                });

                                switch (childIter->capsule.getDirection())
                                {
                                    case Direction::X_AXIS:
                                        childShape = Ref<btCapsuleShapeX>::create(childIter->capsule.getRadius(), 
                                                                                  childIter->capsule.getHeight());
                                        break;
                                    case Direction::Y_AXIS:
                                        childShape = Ref<btCapsuleShape>::create(childIter->capsule.getRadius(),
                                                                                 childIter->capsule.getHeight());
                                        break;
                                    case Direction::Z_AXIS:
                                        childShape = Ref<btCapsuleShapeZ>::create(childIter->capsule.getRadius(),
                                                                                  childIter->capsule.getHeight());
                                        break;
                                }
                                workingShape->addChildShape(t, childShape.get());
                                break;
                            }
                        }

                        RigidBodyInfo::ChildShapeInfo info;

                        info.btShape = childShape;

                        info.shapeInfo = childIter->m_selfRef;

                        m_rigidBodies[entity]->children.emplace_back(info);
                    }

                    shape = workingShape;

                    btTransform t;
                    
                    //t.setFromOpenGLMatrix(value_ptr(transformComponent.getMatrixGlobal()));

                    t.setOrigin(btVector3(transformComponent.getPositionXGlobal(),
                                          transformComponent.getPositionYGlobal(),
                                          transformComponent.getPositionZGlobal()));

                    glm::quat q = transformComponent.getRotationGlobal();

                    btQuaternion btq = btQuaternion(q.x, q.y, q.z, q.w);
                    t.setRotation(btq);

                    btVector3 inertia = { 0, 0, 0 };
                    if (rigidBodyComponent.getMass() != 0.0f)
                        shape->calculateLocalInertia(rigidBodyComponent.getMass(), inertia);

                    shape->setLocalScaling({
                        transformComponent.getScaleXGlobal(),
                        transformComponent.getScaleYGlobal(),
                        transformComponent.getScaleZGlobal()
                    });
                    
                    motion = Ref<btDefaultMotionState>::create(t);

                    btRigidBody::btRigidBodyConstructionInfo info(rigidBodyComponent.getMass(),
                                                                  motion.get(),
                                                                  shape.get(),
                                                                  inertia);

                    body = Ref<btRigidBody>::create(info);

                    body->setRestitution(1.0f);
                }

                body->setCollisionFlags(body->getCollisionFlags() |
                                        btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
                
                m_btWorld->addRigidBody(body.get());

                m_rigidBodies[entity]->entity = entity;
                m_rigidBodies[entity]->body   = body;
                m_rigidBodies[entity]->shape  = shape;
                m_rigidBodies[entity]->motion = motion;

                body->setUserPointer((void*) entity);
            }
            else
            {
                // RigidBody is already in world, check if dirty
            }
        }

        Ref<ClosestRaycastResult> PhysicsSystem::raytestClosest(glm::vec3 position, glm::vec3 direction, f32 distance)
        {
            glm::vec3 endPos = position + direction * distance;
            btVector3 from = { position.x, position.y, position.z };
            btVector3 to   = { endPos.x, endPos.y, endPos.z };

            btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
            g_currentPhysicsSystem->m_btWorld->rayTest(from, to, rayCallback);

            RaycastResult::HitObject obj = {};
            
            if (rayCallback.hasHit())
            {
                obj.hitPosition = {
                    rayCallback.m_hitPointWorld.x(),
                    rayCallback.m_hitPointWorld.y(),
                    rayCallback.m_hitPointWorld.z()
                };
                
                obj.hitNormal = {
                    rayCallback.m_hitNormalWorld.x(),
                    rayCallback.m_hitNormalWorld.y(),
                    rayCallback.m_hitNormalWorld.z()
                };
                
                obj.hitFraction = rayCallback.m_closestHitFraction;
                
                obj.distanceTo = glm::length(direction * rayCallback.m_closestHitFraction);

                for (auto& kvp : g_currentPhysicsSystem->m_rigidBodies)
                {
                    if (kvp.second->body.get() == rayCallback.m_collisionObject)
                    {
                        obj.entity = kvp.first;
                        break;
                    }
                }

                OYL_ASSERT(obj.entity != entt::null);
            }
            
            Ref<ClosestRaycastResult> result = 
                Ref<ClosestRaycastResult>::create(position, endPos, rayCallback.hasHit(), std::move(obj));

            return result;
        }

        // ^^^ Physics System ^^^ //

        // vvv Transform Update System vvv //

        //void TransformUpdateSystem::onUpdate()
        //{
        //    using component::Transform;
        //    using component::Parent;

        //    auto view = registry->view<Transform>();
        //    for (auto entity : view)
        //    {
        //        auto& ct = view.get(entity);
        //        if (registry->has<Parent>(entity))
        //        {
        //            auto parent = registry->get<Parent>(entity).parent;
        //            if (parent != entt::null)
        //            {   
        //                auto& pt = registry->get<Transform>(parent);

        //                if (!pt.m_localRef) 
        //                    pt.m_localRef = Ref<Transform>(&pt, [](Transform*) {});

        //                ct.m_parentRef = pt.m_localRef;
        //            }
        //            else ct.m_parentRef = {};
        //        }
        //        else ct.m_parentRef = {};
        //    }
        //};

        // ^^^ Transform Update System ^^^ //
        
        // vvv Editor Camera System vvv //

        void EditorCameraSystem::onEnter()
        {
            //listenForEventType(EventType::KeyPressed);
            //listenForEventType(EventType::KeyReleased);
            //listenForEventType(EventType::MouseMoved);
            //listenForEventType(EventType::MousePressed);
            //listenForEventType(EventType::MouseReleased);
            listenForEventType(EventType::EditorViewportResized);
            listenForEventType(EventType::EditorCameraMoveRequest);

            m_camera = Ref<EditorCamera>::create();
            m_camera->setProjection(glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f));
            m_camera->setPosition(glm::vec3(10.0f, 5.0f, 10.0f));
            m_camera->lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

            EditorCameraChangedEvent ccEvent;
            ccEvent.camera = &m_camera;

            postEvent(ccEvent);
        }

        void EditorCameraSystem::onExit() { }

        void EditorCameraSystem::onUpdate()
        {
            if (!m_doMoveCamera)
            {
                m_cameraMove = glm::vec3(0.0f);
                m_cameraRotate = glm::vec3(0.0f);
                return;
            }

            m_camera->move(m_cameraMove * Time::unscaledDeltaTime());

            static glm::vec3 realRotation = glm::vec3(20.0f, -45.0f, 0.0f);

            realRotation += m_cameraRotate * m_cameraRotateSpeed * Time::unscaledDeltaTime();
            if (realRotation.x > 89.0f) realRotation.x = 89.0f;
            if (realRotation.x < -89.0f) realRotation.x = -89.0f;

            m_camera->setRotation(realRotation);

            m_cameraRotate = glm::vec3(0.0f);
        }

        void EditorCameraSystem::onGuiRender()
        {
            ImGui::Begin("Camera##CameraSettings");

            ImGui::SliderFloat("Move Speed", &m_cameraMoveSpeed, 5.0f, 30.f);
            ImGui::SliderFloat("Turn Speed", &m_cameraRotateSpeed, 0.1f, 50.0f);

            ImGui::End();
        }

        bool EditorCameraSystem::onEvent(const Event& event)
        {
            switch (event.type)
            {
                case EventType::KeyPressed:
                {
                    auto e = event_cast<KeyPressedEvent>(event);
                    if (!e.repeatCount)
                    {
                        if (e.keycode == Key::W)
                            m_cameraMove.z -= m_cameraMoveSpeed;
                        if (e.keycode == Key::S)
                            m_cameraMove.z += m_cameraMoveSpeed;
                        if (e.keycode == Key::D)
                            m_cameraMove.x += m_cameraMoveSpeed;
                        if (e.keycode == Key::A)
                            m_cameraMove.x -= m_cameraMoveSpeed;
                        if (e.keycode == Key::Space)
                            m_cameraMove.y += m_cameraMoveSpeed;
                        if (e.keycode == Key::LeftControl)
                            m_cameraMove.y -= m_cameraMoveSpeed;
                    }
                    break;
                }
                case EventType::KeyReleased:
                {
                    auto e = event_cast<KeyReleasedEvent>(event);
                    if (e.keycode == Key::W)
                        m_cameraMove.z += m_cameraMoveSpeed;
                    if (e.keycode == Key::S)
                        m_cameraMove.z -= m_cameraMoveSpeed;
                    if (e.keycode == Key::D)
                        m_cameraMove.x -= m_cameraMoveSpeed;
                    if (e.keycode == Key::A)
                        m_cameraMove.x += m_cameraMoveSpeed;
                    if (e.keycode == Key::Space)
                        m_cameraMove.y -= m_cameraMoveSpeed;
                    if (e.keycode == Key::LeftControl)
                        m_cameraMove.y += m_cameraMoveSpeed;
                    break;
                }
                case EventType::MouseMoved:
                {
                    auto e = event_cast<MouseMovedEvent>(event);
                    m_cameraRotate.y = e.dx;
                    m_cameraRotate.x = e.dy;

                    break;
                }
                case EventType::EditorCameraMoveRequest:
                {
                    auto e = event_cast<EditorCameraMoveRequestEvent>(event);
                    CursorStateRequestEvent cursorRequest;
                    if (e.doMove)
                    {
                        listenForEventType(EventType::MouseMoved);
                        listenForEventType(EventType::KeyPressed);
                        listenForEventType(EventType::KeyReleased);
                        cursorRequest.state = CursorState::Disabled;
                    }
                    else
                    {
                        ignoreEventType(EventType::MouseMoved);
                        ignoreEventType(EventType::KeyPressed);
                        ignoreEventType(EventType::KeyReleased);
                        cursorRequest.state = CursorState::Normal;
                    }

                    if (m_doMoveCamera != e.doMove)
                        postEvent(cursorRequest);
                    m_doMoveCamera = e.doMove;

                    break;
                }
                case EventType::EditorViewportResized:
                {
                    auto e = event_cast<EditorViewportResizedEvent>(event);
                    glm::mat4 proj = glm::perspective(glm::radians(60.0f), e.width / e.height, 0.1f, 1000.0f);
                    m_camera->setProjection(proj);

                    break;
                }
            }
            return false;
        }

        // ^^^ Editor Camera System vvv //

        // vvv Editor Render System vvv //

        void EditorRenderSystem::onEnter()
        {
            listenForEventType(EventType::WindowResized);
            listenForEventType(EventType::EditorCameraChanged);

            m_editorViewportBuffer = FrameBuffer::create(1);
            m_editorViewportBuffer->initDepthTexture(1, 1);
            m_editorViewportBuffer->initColorTexture(0, 1, 1, 
                                                     TextureFormat::RGBA8, 
                                                     TextureFilter::Nearest, 
                                                     TextureWrap::ClampToEdge);

            EditorViewportHandleChangedEvent handleChanged;
            handleChanged.handle = m_editorViewportBuffer->getColorHandle(0);
            postEvent(handleChanged);
        }

        void EditorRenderSystem::onExit() { }

        void EditorRenderSystem::onUpdate()
        {
            using component::Transform;
            using component::Renderable;
            using component::Camera;
            using component::PointLight;
            
            m_editorViewportBuffer->clear();
            m_editorViewportBuffer->bind();

            RenderCommand::setDrawRect(0, 0, m_windowSize.x, m_windowSize.y);

            const auto& skybox = TextureCubeMap::get(DEFAULT_SKYBOX_ALIAS);
            const auto& shader = Shader::get(SKYBOX_SHADER_ALIAS);
            const auto& mesh   = Mesh::get(CUBE_MESH_ALIAS);

            glm::mat4 viewProj = m_targetCamera->getProjectionMatrix();
            viewProj *= glm::mat4(glm::mat3(m_targetCamera->getViewMatrix()));
            shader->bind();
            shader->setUniformMat4("u_viewProjection", viewProj);

            RenderCommand::setDepthDraw(false);
            RenderCommand::setBackfaceCulling(false);
            Renderer::submit(mesh, shader, skybox);
            RenderCommand::setBackfaceCulling(true);
            RenderCommand::setDepthDraw(true);

            // We sort our mesh renderers based on material properties
            // This will group all of our meshes based on shader first, then material second
            registry->sort<Renderable>(
                [](const Renderable& lhs, const Renderable& rhs)
                {
                    if (lhs.material == nullptr || lhs.mesh == nullptr)
                        return false;
                    if (rhs.material == nullptr || rhs.mesh == nullptr)
                        return true;
                    if (lhs.material->shader != rhs.material->shader)
                        return lhs.material->shader < rhs.material->shader;
                    return lhs.material < rhs.material;
                });

            Ref<Material> boundMaterial = Material::create();
            Ref<Shader>   tempShader    = Shader::get(LIGHTING_SHADER_ALIAS);

            bool doCulling = true;

            auto view = registry->view<Renderable, Transform>();
            for (auto entity : view)
            {
                Renderable& mr = view.get<Renderable>(entity);

                if (mr.mesh == nullptr || mr.material == nullptr)
                    break;

                if (mr.material != boundMaterial)
                {                    
                    *boundMaterial = *mr.material;
                    boundMaterial->shader = tempShader;

                    boundMaterial->setUniformMat4("u_view", m_targetCamera->getViewMatrix());
                    boundMaterial->setUniformMat4("u_viewProjection", m_targetCamera->getViewProjectionMatrix());
                    glm::mat3 viewNormal = glm::mat3(m_targetCamera->getViewMatrix());
                    viewNormal = inverse(transpose(viewNormal));
                    boundMaterial->setUniformMat3("u_viewNormal", viewNormal);

                    auto lightView = registry->view<PointLight>();
                    int count = 0;
                    for (auto light : lightView)
                    {
                        auto lightProps = lightView.get(light);
                        auto lightTransform = registry->get<Transform>(light);

                        boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].position",
                                                    m_targetCamera->getViewMatrix() * glm::vec4(lightTransform.getPositionGlobal(), 1.0f));
                        boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].ambient",
                                                    lightProps.ambient);
                        boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].diffuse",
                                                    lightProps.diffuse);
                        boundMaterial->setUniform3f("u_pointLight[" + std::to_string(count) + "].specular",
                                                    lightProps.specular);
                        count++;
                    }
                    
                    boundMaterial->bind();
                    boundMaterial->applyUniforms();
                }

                auto& transformComponent = view.get<Transform>(entity);
                glm::mat4 transform = transformComponent.getMatrixGlobal();

                glm::bvec3 mirror = transformComponent.getMirrorGlobal();
                if (!(mirror.x ^ mirror.y ^ mirror.z) != doCulling)
                {
                    doCulling ^= 1;
                    RenderCommand::setBackfaceCulling(doCulling);
                }
                
                Renderer::submit(mr.mesh, boundMaterial, transform);
            }

            m_editorViewportBuffer->unbind();
        }

        void EditorRenderSystem::onGuiRender() { }

        bool EditorRenderSystem::onEvent(const Event& event)
        {
            switch (event.type)
            {
                case EventType::WindowResized:
                {
                    auto e = event_cast<WindowResizedEvent>(event);
                    m_editorViewportBuffer->updateViewport(e.width, e.height);
                    m_windowSize = { e.width, e.height };
                    return true;
                }
                case EventType::EditorCameraChanged:
                {
                    auto e = event_cast<EditorCameraChangedEvent>(event);
                    m_targetCamera = *e.camera;
                }
            }
            return false;
        }

        void EditorRenderSystem::init() { }

        void EditorRenderSystem::shutdown() { }

        // ^^^ Editor Render System ^^^ //
    }
}

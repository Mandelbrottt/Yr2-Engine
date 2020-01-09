#include <Oyl3D.h>

#include "SandboxLayer.h"

using namespace oyl;

void SandboxLayer::onEnter()
{
    auto mesh = Mesh::cache("res/assets/models/cube.obj");
    
    auto& mat = Material::cache(Shader::get(LIGHTING_SHADER_ALIAS), "container");
    mat->albedoMap   = Texture2D::cache("res/assets/textures/container2.jpg");
    mat->specularMap = Texture2D::cache("res/assets/textures/container2_specular.jpg");

    {
        component::Renderable mr;
        mr.mesh     = mesh;
        mr.material = mat;

		/////////////////////////////////////////////////////
		///////////////////// BLUE TEAM /////////////////////
		{
			//PLAYER
			entt::entity playerBlueEntity = registry->create();
			registry->assign<Player>(playerBlueEntity);
			registry->get<Player>(playerBlueEntity).team = Team::blue;

			registry->assign<component::Renderable>(playerBlueEntity, mr);

			component::Transform t2;
			t2.setPosition(glm::vec3(0.0f));
			registry->assign<component::Transform>(playerBlueEntity, t2);

			auto& so2 = registry->assign<component::SceneObject>(playerBlueEntity);
			so2.name = "Player";

			auto& rb = registry->assign<component::RigidBody>(playerBlueEntity);
			rb.setMass(5.0f);
			rb.setFriction(1.0f);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_X, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Y, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Z, true);

			auto& playerCollider = registry->assign<component::Collider>(playerBlueEntity);
			auto& shapeInfo = playerCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });

			registry->assign<entt::tag<"Player"_hs>>(playerBlueEntity);
		}
        
		{
			//CANNON
			entt::entity cannonBlueEntity = registry->create();

			component::Transform cannonTransform;
			cannonTransform.setPosition(glm::vec3(0.0f));
			registry->assign<component::Transform>(cannonBlueEntity, cannonTransform);

			auto& cannon = registry->assign<Cannon>(cannonBlueEntity);
			cannon.team = Team::blue;

			registry->assign<component::Renderable>(cannonBlueEntity, mr);

			auto& so = registry->assign<component::SceneObject>(cannonBlueEntity);
			so.name = "BlueCannon";

			auto& rb = registry->assign<component::RigidBody>(cannonBlueEntity);
			rb.setMass(0.0f);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_X, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Y, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Z, true);
			rb.setProperties(component::RigidBody::Property::IS_KINEMATIC, true);

			auto& cannonCollider = registry->assign<component::Collider>(cannonBlueEntity);
			auto& shapeInfo = cannonCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
		}

		{
			for (int i = 0; i < 3; i++)
			{
				//GARBAGE PILES
				entt::entity garbagePileBlueEntity = registry->create();

				component::Transform garbagePileTransform;
				garbagePileTransform.setPosition(glm::vec3(-3.0f, 0.0f, 0.0f));
				garbagePileTransform.setScale(glm::vec3(3.0f, 0.7f, 3.0f));
				registry->assign<component::Transform>(garbagePileBlueEntity, garbagePileTransform);

				auto& garbagePile = registry->assign<GarbagePile>(garbagePileBlueEntity);
				garbagePile.team = Team::blue;

				registry->assign<component::Renderable>(garbagePileBlueEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(garbagePileBlueEntity);
				so2.name = "BlueGarbagePile" + std::to_string(i);

				auto& garbagePileCollider = registry->assign<component::Collider>(garbagePileBlueEntity);
				auto& shapeInfo = garbagePileCollider.pushShape(Collider_Box);
				shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
			}
		}

		{
			//CANNONBALLS
			mr.mesh = Mesh::cache("res/assets/models/sphere.obj");
			for (int i = 0; i < 30; i++)
			{
				entt::entity cannonballEntity = registry->create();

				component::Transform cannonballTransform;
				cannonballTransform.setPosition(glm::vec3(1000.0f, 1000.0f, 1000.0f));
				cannonballTransform.setScale(glm::vec3(0.5f, 0.5f, 0.5f));
				registry->assign<component::Transform>(cannonballEntity, cannonballTransform);

				auto& carryableItem = registry->assign<CarryableItem>(cannonballEntity);
				carryableItem.team = Team::blue;
				carryableItem.type = CarryableItemType::cannonball;

				auto& cannonball = registry->assign<Cannonball>(cannonballEntity);

				mr.mesh = Mesh::get("sphere");
				registry->assign<component::Renderable>(cannonballEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(cannonballEntity);
				so2.name = "BlueCannonball" + std::to_string(i);

				auto& cannonballCollider = registry->assign<component::Collider>(cannonballEntity);
				auto& shapeInfo = cannonballCollider.pushShape(Collider_Sphere);
				shapeInfo.sphere.setRadius(0.5f);
			}
		}

        {
			//MOP
			entt::entity mopBlueEntity = registry->create();

			component::Transform mopTransform;
			mopTransform.setPosition(glm::vec3(3.0f, 0.1f, 0.0f));
			mopTransform.setScale(glm::vec3(2.0f, 0.2f, 0.2f));
			registry->assign<component::Transform>(mopBlueEntity, mopTransform);

			auto& carryableItem = registry->assign<CarryableItem>(mopBlueEntity);
			carryableItem.team = Team::blue;
			carryableItem.type = CarryableItemType::mop;

			mr.mesh = Mesh::get("cube");
			registry->assign<component::Renderable>(mopBlueEntity, mr);

			auto& so2 = registry->assign<component::SceneObject>(mopBlueEntity);
			so2.name = "BlueMop";

			/*auto& rb = registry->assign<component::RigidBody>(mopBlueEntity);
			rb.setMass(2.0f);
			rb.setFriction(1.0f);*/

			auto& mopCollider = registry->assign<component::Collider>(mopBlueEntity);
			auto& shapeInfo = mopCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
        }

		{
			//CLEANING SOLUTION
			for (int i = 0; i < 3; i++)
			{
				entt::entity cleaningSolutionEntity = registry->create();

				component::Transform cleaningSolutionTransform;
				cleaningSolutionTransform.setPosition(glm::vec3(3.0f, 0.22f, 3.0f));
				cleaningSolutionTransform.setScale(glm::vec3(0.2f, 0.44f, 0.2f));
				registry->assign<component::Transform>(cleaningSolutionEntity, cleaningSolutionTransform);

				auto& carryableItem = registry->assign<CarryableItem>(cleaningSolutionEntity);
				carryableItem.team = Team::blue;
				carryableItem.type = CarryableItemType::cleaningSolution;

				mr.mesh = Mesh::get("cube");
				registry->assign<component::Renderable>(cleaningSolutionEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(cleaningSolutionEntity);
				so2.name = "BlueCleaningSolution" + std::to_string(i);

				auto& cleaningSolutionCollider = registry->assign<component::Collider>(cleaningSolutionEntity);
				auto& shapeInfo = cleaningSolutionCollider.pushShape(Collider_Box);
				shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
			}
		}

		{
			//GLOOP
			for (int i = 0; i < 3; i++)
			{
				entt::entity gloopEntity = registry->create();

				component::Transform gloopTransform;
				gloopTransform.setPosition(glm::vec3(3.0f, 0.27f, 3.0f));
				gloopTransform.setScale(glm::vec3(0.32f, 0.54f, 0.32f));
				registry->assign<component::Transform>(gloopEntity, gloopTransform);

				auto& carryableItem = registry->assign<CarryableItem>(gloopEntity);
				carryableItem.team = Team::blue;
				carryableItem.type = CarryableItemType::gloop;

				registry->assign<Gloop>(gloopEntity);

				mr.mesh = Mesh::get("cube");
				registry->assign<component::Renderable>(gloopEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(gloopEntity);
				so2.name = "BlueGloop" + std::to_string(i);

				auto& gloopCollider = registry->assign<component::Collider>(gloopEntity);
				auto& shapeInfo = gloopCollider.pushShape(Collider_Box);
				shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
			}
		}

		{
			//CANNONBALL CRATE
			entt::entity cannonballCrateEntity = registry->create();

			component::Transform cannonballCrateTransform;
			cannonballCrateTransform.setPosition(glm::vec3(-1.0f, 0.5f, -3.0f));
			cannonballCrateTransform.setScale(glm::vec3(2.0f, 1.0f, 1.0f));
			registry->assign<component::Transform>(cannonballCrateEntity, cannonballCrateTransform);

			auto& cannonballCrate = registry->assign<CannonballCrate>(cannonballCrateEntity);
			cannonballCrate.team = Team::blue;

			mr.mesh = Mesh::get("cube");
			mr.material = mat;
			registry->assign<component::Renderable>(cannonballCrateEntity, mr);

			auto& so2 = registry->assign<component::SceneObject>(cannonballCrateEntity);
			so2.name = "BlueCannonballCrate";

			auto& rb = registry->assign<component::RigidBody>(cannonballCrateEntity);
			rb.setMass(0.0f);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_X, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Y, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Z, true);
			rb.setProperties(component::RigidBody::Property::IS_KINEMATIC, true);

			auto& cannonballCrateCollider = registry->assign<component::Collider>(cannonballCrateEntity);
			auto& shapeInfo = cannonballCrateCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
		}
		/////////////////////////////////////////////////////
		/////////////////////////////////////////////////////

		/////////////////////////////////////////////////////
		///////////////////// RED TEAM //////////////////////
		{
			//CANNON
			entt::entity cannonBlueEntity = registry->create();

			component::Transform cannonTransform;
			cannonTransform.setPosition(glm::vec3(0.0f));
			cannonTransform.setRotationEulerY(180.0f);
			registry->assign<component::Transform>(cannonBlueEntity, cannonTransform);

			auto& cannon = registry->assign<Cannon>(cannonBlueEntity);
			cannon.team = Team::red;

			registry->assign<component::Renderable>(cannonBlueEntity, mr);

			auto& so = registry->assign<component::SceneObject>(cannonBlueEntity);
			so.name = "RedCannon";

			auto& rb = registry->assign<component::RigidBody>(cannonBlueEntity);
			rb.setMass(0.0f);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_X, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Y, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Z, true);
			rb.setProperties(component::RigidBody::Property::IS_KINEMATIC, true);

			auto& cannonCollider = registry->assign<component::Collider>(cannonBlueEntity);
			auto& shapeInfo = cannonCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
		}

		{
			for (int i = 0; i < 3; i++)
			{
				//GARBAGE PILES
				entt::entity garbagePileBlueEntity = registry->create();

				component::Transform garbagePileTransform;
				garbagePileTransform.setPosition(glm::vec3(-3.0f, 0.0f, 0.0f));
				garbagePileTransform.setScale(glm::vec3(3.0f, 0.7f, 3.0f));
				registry->assign<component::Transform>(garbagePileBlueEntity, garbagePileTransform);

				auto& garbagePile = registry->assign<GarbagePile>(garbagePileBlueEntity);
				garbagePile.team = Team::red;

				registry->assign<component::Renderable>(garbagePileBlueEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(garbagePileBlueEntity);
				so2.name = "RedGarbagePile" + std::to_string(i);

				auto& garbagePileCollider = registry->assign<component::Collider>(garbagePileBlueEntity);
				auto& shapeInfo = garbagePileCollider.pushShape(Collider_Box);
				shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
			}
		}

		{
			//CANNONBALLS
			mr.mesh = Mesh::cache("res/assets/models/sphere.obj");
			for (int i = 0; i < 30; i++)
			{
				entt::entity cannonballEntity = registry->create();

				component::Transform cannonballTransform;
				cannonballTransform.setPosition(glm::vec3(1000.0f, 1000.0f, 1000.0f));
				cannonballTransform.setScale(glm::vec3(0.5f, 0.5f, 0.5f));
				registry->assign<component::Transform>(cannonballEntity, cannonballTransform);

				auto& carryableItem = registry->assign<CarryableItem>(cannonballEntity);
				carryableItem.team = Team::red;
				carryableItem.type = CarryableItemType::cannonball;

				auto& cannonball = registry->assign<Cannonball>(cannonballEntity);

				mr.mesh = Mesh::get("sphere");
				registry->assign<component::Renderable>(cannonballEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(cannonballEntity);
				so2.name = "RedCannonball" + std::to_string(i);

				auto& cannonballCollider = registry->assign<component::Collider>(cannonballEntity);
				auto& shapeInfo = cannonballCollider.pushShape(Collider_Sphere);
				shapeInfo.sphere.setRadius(0.5f);
			}
		}

		{
			//MOP
			entt::entity mopBlueEntity = registry->create();

			component::Transform mopTransform;
			mopTransform.setPosition(glm::vec3(3.0f, 0.1f, 0.0f));
			mopTransform.setScale(glm::vec3(2.0f, 0.2f, 0.2f));
			registry->assign<component::Transform>(mopBlueEntity, mopTransform);

			auto& carryableItem = registry->assign<CarryableItem>(mopBlueEntity);
			carryableItem.team = Team::red;
			carryableItem.type = CarryableItemType::mop;

			mr.mesh = Mesh::get("cube");
			registry->assign<component::Renderable>(mopBlueEntity, mr);

			auto& so2 = registry->assign<component::SceneObject>(mopBlueEntity);
			so2.name = "RedMop";

			/*auto& rb = registry->assign<component::RigidBody>(mopBlueEntity);
			rb.setMass(2.0f);
			rb.setFriction(1.0f);*/

			auto& mopCollider = registry->assign<component::Collider>(mopBlueEntity);
			auto& shapeInfo = mopCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
		}

		{
			//CLEANING SOLUTION
			for (int i = 0; i < 3; i++)
			{
				entt::entity cleaningSolutionEntity = registry->create();

				component::Transform cleaningSolutionTransform;
				cleaningSolutionTransform.setPosition(glm::vec3(3.0f, 0.22f, 3.0f));
				cleaningSolutionTransform.setScale(glm::vec3(0.2f, 0.44f, 0.2f));
				registry->assign<component::Transform>(cleaningSolutionEntity, cleaningSolutionTransform);

				auto& carryableItem = registry->assign<CarryableItem>(cleaningSolutionEntity);
				carryableItem.team = Team::red;
				carryableItem.type = CarryableItemType::cleaningSolution;

				mr.mesh = Mesh::get("cube");
				registry->assign<component::Renderable>(cleaningSolutionEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(cleaningSolutionEntity);
				so2.name = "RedCleaningSolution" + std::to_string(i);

				auto& cleaningSolutionCollider = registry->assign<component::Collider>(cleaningSolutionEntity);
				auto& shapeInfo = cleaningSolutionCollider.pushShape(Collider_Box);
				shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
			}
		}

		{
			//GLOOP
			for (int i = 0; i < 3; i++)
			{
				entt::entity gloopEntity = registry->create();

				component::Transform gloopTransform;
				gloopTransform.setPosition(glm::vec3(3.0f, 0.27f, 3.0f));
				gloopTransform.setScale(glm::vec3(0.32f, 0.54f, 0.32f));
				registry->assign<component::Transform>(gloopEntity, gloopTransform);

				auto& carryableItem = registry->assign<CarryableItem>(gloopEntity);
				carryableItem.team = Team::red;
				carryableItem.type = CarryableItemType::gloop;

				mr.mesh = Mesh::get("cube");
				registry->assign<component::Renderable>(gloopEntity, mr);

				auto& so2 = registry->assign<component::SceneObject>(gloopEntity);
				so2.name = "RedGloop" + std::to_string(i);

				auto& gloopCollider = registry->assign<component::Collider>(gloopEntity);
				auto& shapeInfo = gloopCollider.pushShape(Collider_Box);
				shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
			}
		}

		{
			//CANNONBALL CRATE
			entt::entity cannonballCrateEntity = registry->create();

			component::Transform cannonballCrateTransform;
			cannonballCrateTransform.setPosition(glm::vec3(-1.0f, 0.5f, -3.0f));
			cannonballCrateTransform.setScale(glm::vec3(2.0f, 1.0f, 1.0f));
			registry->assign<component::Transform>(cannonballCrateEntity, cannonballCrateTransform);

			auto& cannonballCrate = registry->assign<CannonballCrate>(cannonballCrateEntity);
			cannonballCrate.team = Team::red;

			mr.mesh = Mesh::get("cube");
			mr.material = mat;
			registry->assign<component::Renderable>(cannonballCrateEntity, mr);

			auto& so2 = registry->assign<component::SceneObject>(cannonballCrateEntity);
			so2.name = "RedCannonballCrate";

			auto& rb = registry->assign<component::RigidBody>(cannonballCrateEntity);
			rb.setMass(0.0f);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_X, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Y, true);
			rb.setProperties(component::RigidBody::Property::FREEZE_ROTATION_Z, true);
			rb.setProperties(component::RigidBody::Property::IS_KINEMATIC, true);

			auto& cannonballCrateCollider = registry->assign<component::Collider>(cannonballCrateEntity);
			auto& shapeInfo = cannonballCrateCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
		}
		/////////////////////////////////////////////////////
		/////////////////////////////////////////////////////

		{
			component::Collider boxCollider;
			auto& shi = boxCollider.pushShape(Collider_Box);
			shi.box.setSize({ 1.0f, 1.0f, 1.0f });

			entt::entity e2 = registry->create();
			registry->assign<component::Renderable>(e2, mr);

			component::Transform boxTransform;
			boxTransform.setPosition(glm::vec3(3.0f, 3.0f, 3.0f));
			boxTransform.setScale(glm::vec3(0.3f));
			registry->assign<component::Transform>(e2, boxTransform);

			auto& l = registry->assign<component::PointLight>(e2);
			l.ambient = glm::vec3(0.75f);

			auto& so3 = registry->assign<component::SceneObject>(e2);
			so3.name = "Light 1";
		}

		{
			//CHARACTER MESH
			entt::entity characterEntity = registry->create();

			component::Transform mopTransform;
			mopTransform.setPosition(glm::vec3(10.0f, 6.0f, 0.0f));
			mopTransform.setScale(glm::vec3(1.0f, 1.0f, 1.0f));
			registry->assign<component::Transform>(characterEntity, mopTransform);

			mr.mesh = Mesh::cache("res/assets/models/character.obj");
			registry->assign<component::Renderable>(characterEntity, mr);

			auto& so2 = registry->assign<component::SceneObject>(characterEntity);
			so2.name = "Character";

			/*auto& rb = registry->assign<component::RigidBody>(mopBlueEntity);
			rb.setMass(2.0f);
			rb.setFriction(1.0f);*/

			auto& mopCollider = registry->assign<component::Collider>(characterEntity);
			auto& shapeInfo = mopCollider.pushShape(Collider_Box);
			shapeInfo.box.setSize({ 1.0f, 1.0f, 1.0f });
		}
    }
    {
        component::Renderable mr;
        mr.mesh = Mesh::cache("res/assets/models/plane.obj");
        mr.material = mat;

        entt::entity e = registry->create();
        registry->assign<component::Renderable>(e, mr);

        component::Transform t;
        t.setPosition(glm::vec3(0.0f, -5.0f, 0.0f));
		t.setScale(glm::vec3(2.5f, 1.0f, 1.7f));
        registry->assign<component::Transform>(e, t);

        auto& so = registry->assign<component::SceneObject>(e);
        so.name = "Plane";

        auto& rb = registry->assign<component::RigidBody>(e);
        rb.setMass(0.0f);
		rb.setFriction(1.0f);

        auto& cl = registry->assign<component::Collider>(e);

        auto& shi = cl.pushShape(Collider_Box);
        shi.box.setSize({ 20.0f, 0.1f, 20.0f });
    }
    {
        component::Renderable mr;
        mr.mesh = Mesh::get("sphere");
        mr.material = mat;

        entt::entity e = registry->create();
        registry->assign<component::Renderable>(e, mr);

        component::Transform t;
        t.setPosition(glm::vec3(-3.0f, -1.0f, -2.0f));
        registry->assign<component::Transform>(e, t);

        auto& so = registry->assign<component::SceneObject>(e);
        so.name = "Sphere 1";
        
        auto& rb = registry->assign<component::RigidBody>(e);
        rb.setMass(1.0f);

        auto& cl = registry->assign<component::Collider>(e);

        auto& shi = cl.pushShape(Collider_Sphere);
        shi.sphere.setRadius(0.5f);
    }
    {
        component::Renderable mr;
        mr.mesh = Mesh::get("sphere");
        mr.material = mat;

        entt::entity e = registry->create();
        registry->assign<component::Renderable>(e, mr);

        component::Transform t;
        t.setPosition(glm::vec3(-3.0f, -1.0f, -2.0f));
        registry->assign<component::Transform>(e, t);

        auto& so = registry->assign<component::SceneObject>(e);
        so.name = "Sphere 2";

        auto& rb = registry->assign<component::RigidBody>(e);
        rb.setMass(1.0f);

        auto& cl = registry->assign<component::Collider>(e);

        auto& shi = cl.pushShape(Collider_Sphere);
        shi.sphere.setRadius(0.5f);
    }
}

void SandboxLayer::onUpdate(Timestep dt)
{

}

void SandboxLayer::onGuiRender(Timestep dt)
{
    /*ImGui::Begin("xdhaha");

    ImGui::SliderFloat("Force Speed", &forceSpeed, 0.1f, 10.0f);
    
    ImGui::End();*/
}

bool SandboxLayer::onEvent(Ref<Event> event)
{
    return false;
}

#include "Player.h"

glm::vec3 myLerp(const glm::vec3& v1, const glm::vec3& v2, float u)
{
	return (1.0f - u) * v1 + v2 * u;
}

void PlayerSystem::onEnter()
{
	addToCategoryMask(CategoryPlayer);
}

void PlayerSystem::onExit()
{
    
}

void PlayerSystem::onUpdate(Timestep dt)
{
	auto view = registry->view<Player, component::Transform, component::RigidBody>();
	for (auto& entity : view)
	{
		auto& player = registry->get<Player>(entity);
		auto& playerTransform = registry->get<component::Transform>(entity);
		auto& playerRB = registry->get<component::RigidBody>(entity);

		switch (player.state)
		{
		    case PlayerStates::idle:
		    {
				playerRB.impulse = glm::vec3(0.0f);
				break;
		    }
		    
		    case PlayerStates::walking:
			{
				glm::vec3 deltaVelocity = (player.moveDirection * player.speedForce) - playerRB.velocity;
				playerRB.impulse = playerRB.mass * deltaVelocity * static_cast<float>(dt);

				if (player.moveDirection == glm::vec3(0.0f))
					changeToIdle(&player);

				break;
			}

			case PlayerStates::jumping:
			{
		        
				break;
			}

			case PlayerStates::falling:
			{

				break;
			}
		    
		    case PlayerStates::adjustingPosition:
		    {
			    player.adjustingPositionStateData.interpolationParam = std::min(
					player.adjustingPositionStateData.interpolationParam + player.adjustingPositionStateData.speed * dt,
				    1.0f);

			    playerTransform.setPosition(glm::mix(player.adjustingPositionStateData.startPos, 
					                                 player.adjustingPositionStateData.desinationPos, 
					                                 player.adjustingPositionStateData.interpolationParam));

			    if (player.adjustingPositionStateData.interpolationParam >= 1.0f)
				    changeToPushing(&player, playerTransform, glm::vec3(10.0f, 0.0f, 0.0f));

				break;
		    }

		    case PlayerStates::pushing:
		    {
			    player.pushingStateData.interpolationParam = std::min(player.pushingStateData.interpolationParam + player.pushingStateData.speed * dt,
				    1.0f);

			    playerTransform.setPosition(glm::mix(player.pushingStateData.startPos, 
					                                 player.pushingStateData.desinationPos, 
					                                 player.pushingStateData.interpolationParam));

			    if (player.pushingStateData.interpolationParam >= 1.0f)
				    changeToIdle(&player);

				break;
		    }

			case PlayerStates::cleaning:
			{
		        
				break;
			}
		}

	    //reset player's movement before the next frame
		player.moveDirection = glm::vec3(0.0f);
	}
}

bool PlayerSystem::onEvent(Ref<Event> event)
{
	switch (event->type)
	{
		case TypePlayerInteract:
		{
			auto evt = (PlayerInteractEvent)* event;
		    
	        auto playerView = registry->view<Player, component::Transform>();
			for (auto& entity : playerView)
			{
				if (evt.player == entity)
				{

				}
			}
		}
		case TypePlayerMove:
		{
			auto evt = (PlayerMoveEvent)* event;

			auto playerView = registry->view<Player, component::Transform, component::RigidBody>();
		    for (auto& entity : playerView)
		    {
				auto& player = playerView.get<Player>(entity);
				auto& playerRB = playerView.get<component::RigidBody>(entity);
		        
		        if (evt.player == entity)
		        {
					player.moveDirection = evt.direction;
					changeToWalking(&player);
		        }
		    }
		}
	}
	return false;
}

void PlayerSystem::changeToIdle(Player* a_player)
{
	a_player->state = PlayerStates::idle;
}

void PlayerSystem::changeToWalking(Player* a_player)
{
    if (a_player->state == PlayerStates::idle)
	    a_player->state = PlayerStates::walking;
}

void PlayerSystem::changeToJumping(Player* a_player)
{
	a_player->state = PlayerStates::jumping;
}

void PlayerSystem::changeToFalling(Player* a_player)
{
	a_player->state = PlayerStates::falling;
}

void PlayerSystem::changeToAdjustingPosition(Player* a_player, component::Transform a_playerTransform, glm::vec3 a_destinationPos)
{
	a_player->adjustingPositionStateData.interpolationParam = 0.0f;
	a_player->adjustingPositionStateData.speed = 3.33f;
    
	a_player->adjustingPositionStateData.startPos = a_playerTransform.getPosition();
	a_player->adjustingPositionStateData.desinationPos = a_destinationPos;

	a_player->state = PlayerStates::adjustingPosition;
}

void PlayerSystem::changeToPushing(Player* a_player, component::Transform a_playerTransform, glm::vec3 a_destinationPos)
{
	a_player->pushingStateData.interpolationParam = 0.0f;
    
	a_player->pushingStateData.startPos = a_playerTransform.getPosition();
	a_player->pushingStateData.desinationPos = a_destinationPos;
    
	a_player->state = PlayerStates::pushing;
}

void PlayerSystem::changeToCleaning(Player* a_player)
{
	a_player->state = PlayerStates::cleaning;
}
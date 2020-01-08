#include "UIManager.h"

void UIManagerSystem::onEnter()
{
	this->listenForEventCategory((OylEnum)CategoryPlayer);
	this->listenForEventCategory((OylEnum)CategoryGarbagePile);
}

void UIManagerSystem::onExit()
{
}

void UIManagerSystem::onUpdate(Timestep dt)
{
}

bool UIManagerSystem::onEvent(Ref<Event> event)
{
	switch (event->type)
	{
		case TypePlayerInteractResult:
		{
			auto evt = (PlayerInteractResultEvent)* event;
			switch (evt.interactionType)
			{
				case PlayerInteractionResult::nothing:
				{
					auto guiView = registry->view<component::GuiRenderable, component::Transform, PlayerInteractionType>();
					for (auto guiEntity : guiView)
					{
						auto& renderable = registry->get<component::GuiRenderable>(guiEntity);
						auto& renderableTransform = registry->get<component::Transform>(guiEntity);

						renderableTransform.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));
					}

					auto reticleView = registry->view<component::GuiRenderable, component::Transform, Reticle>();
					for (auto& reticleEntity : reticleView)
					{
						auto& reticle = registry->get<Reticle>(reticleEntity);
						auto& reticleTransform = registry->get<component::Transform>(reticleEntity);

						if (reticle.type == ReticleType::normal)
							reticleTransform.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
						else
							reticleTransform.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));
					}

					break;
				}
				case PlayerInteractionResult::cannonFiringSoon:
				{
					enableInvalidUIType(PlayerInteractionResult::cannonFiringSoon);
					break;
				}
				case PlayerInteractionResult::loadCannon:
				{
					enableValidUIType(PlayerInteractionResult::loadCannon);
					break;
				}
				case PlayerInteractionResult::pushCannon:
				{
					enableValidUIType(PlayerInteractionResult::pushCannon);
					break;
				}
				case PlayerInteractionResult::takeCannonballFromCrate:
				{
					enableValidUIType(PlayerInteractionResult::takeCannonballFromCrate);
					break;
				}
				case PlayerInteractionResult::pickUpCannonball:
				{
					enableValidUIType(PlayerInteractionResult::pickUpCannonball);
					break;
				}
				case PlayerInteractionResult::pickUpMop:
				{
					enableValidUIType(PlayerInteractionResult::pickUpMop);
					break;
				}
				case PlayerInteractionResult::cleanGarbagePile:
				{
					enableValidUIType(PlayerInteractionResult::cleanGarbagePile);
					break;
				}
				case PlayerInteractionResult::invalid:
				{
					enableInvalidUIType(PlayerInteractionResult::invalid);
					break;
				}
			}

			break;
		}
		case TypeGarbageCleaned:
		{
			auto evt = (GarbageCleanedEvent)* event;

			auto garbageTickView = registry->view<component::Transform, GarbageTick>();
			int i = 0;
			for (auto& garbageTickEntity : garbageTickView)
			{
				auto& garbageTick = registry->get<GarbageTick>(garbageTickEntity);
				auto& garbageTickTransform = registry->get<component::Transform>(garbageTickEntity);

				garbageTickTransform.setPosition(glm::vec3(100.0f));

				if (evt.currentGarbageTicks >= i + 1)
				{
					garbageTick.screenDurationTimer.elapsed = 0.0f;
					garbageTick.screenDurationTimer.timeToWait = garbageTick.ON_SCREEN_DURATION;
					garbageTickTransform.setPosition(glm::vec3((i * 3.0f) - 3.0f, 4.0f, 0.0f));
				}

				i++;
			}
			break;
		}
	}

	return false;
}

void UIManagerSystem::enableValidUIType(PlayerInteractionResult a_type)
{
	auto guiView = registry->view<component::GuiRenderable, component::Transform, PlayerInteractionType> ();
	for (auto& guiEntity : guiView)
	{
		auto& interactionType = registry->get<PlayerInteractionType>(guiEntity);
		auto& renderableTransform = registry->get<component::Transform>(guiEntity);
		
		renderableTransform.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));

		if (interactionType.type == a_type)
		{
			renderableTransform.setPosition(glm::vec3(0.0f, 2.0f, 0.0f));
		}
	}

	auto reticleView = registry->view<component::GuiRenderable, component::Transform, Reticle>();
	for (auto& reticleEntity : reticleView)
	{
		auto& reticle = registry->get<Reticle>(reticleEntity);
		auto& reticleTransform = registry->get<component::Transform>(reticleEntity);

		if (reticle.type == ReticleType::normal)
			reticleTransform.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
		else
			reticleTransform.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));
	}
}

void UIManagerSystem::enableInvalidUIType(PlayerInteractionResult a_type)
{
	auto guiView = registry->view<component::GuiRenderable, component::Transform, PlayerInteractionType>();
	for (auto& guiEntity : guiView)
	{
		auto& interactionType = registry->get<PlayerInteractionType>(guiEntity);
		auto& renderableTransform = registry->get<component::Transform>(guiEntity);

		renderableTransform.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));

		if (interactionType.type == a_type)
		{
			renderableTransform.setPosition(glm::vec3(0.0f, 2.0f, 0.0f));
		}
	}

	auto reticleView = registry->view<component::GuiRenderable, component::Transform, Reticle>();
	for (auto& reticleEntity : reticleView)
	{
		auto& reticle = registry->get<Reticle>(reticleEntity);
		auto& reticleTransform = registry->get<component::Transform>(reticleEntity);

		if (reticle.type == ReticleType::invalid)
			reticleTransform.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
		else
			reticleTransform.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));
	}
}
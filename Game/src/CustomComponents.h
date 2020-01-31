#pragma once
#include "Oyl3D.h"

using oyl::PlayerNumber;

enum class Team
{
	blue,
	red
};

enum class PlayerState
{
	idle,
	walking,
	jumping,
	falling,
	pushing,
	inCleaningQuicktimeEvent,
	cleaning
};

enum class CannonState
{
	doingNothing,
	beingPushed,
	firingSoon
};

enum class CarryableItemType
{
	invalid,
	cannonball,
	mop,
	cleaningSolution,
	gloop
};

enum class PlayerInteractionResult
{
	nothing,
	invalid,
	cannonFiringSoon,
	loadCannon,
	pushCannon,
	takeCannonballFromCrate,
	pickUpCannonball,
	pickUpMop,
	pickUpCleaningSolution,
	pickUpGloop,
	useGloop,
	cleanGarbagePile,
	activateCleaningQuicktimeEvent
};

enum class ReticleType
{
	normal,
	invalid
};

struct MoveableUsingLerp
{
	glm::vec3 startPos;
	glm::vec3 destinationPos;

	float speed;
	bool  isMovingForward = true;
	float interpolationParam = 0.0f;
};

struct Player
{
	PlayerNumber playerNum = PlayerNumber::One; //should always correspond with appropriate PlayerCamera player number
	int controllerNum = -1; //the gid of the controller that will be used to control the player

	Team team;

	entt::entity primaryCarriedItem   = entt::null;
	entt::entity secondaryCarriedItem = entt::null;
    
	PlayerState state = PlayerState::idle;

	glm::vec3 moveDirection = glm::vec3(0.0f);

	float speedForce = 12.0f;
	float jumpForce  = 20.0f;
	bool isJumping   = false;

	float JUMP_COOLDOWN_DURATION = 0.1f;
	float jumpCooldownTimer      = 0.0f;

	float adjustingPositionSpeed = 3.333f;
	float pushingSpeed = 0.2f;

	MoveableUsingLerp adjustingPositionStateData;
	MoveableUsingLerp pushingStateData;

	float CLEANING_TIME_DURATION = 1.2f; //IF YOU CHANGE THIS, MAKE SURE TO ALSO CHANGE THE DEPENDANT VALUES IN GARBAGE PILE AND GARBAGE HP BAR COMPONENETS (check the comments in those components to figure out which ones)
	float cleaningTimeCountdown = CLEANING_TIME_DURATION;

	float yRotationClamp = 0.0f;

	entt::entity interactableEntity = entt::null;
};

struct Cannon
{
	Team team;

	CannonState state = CannonState::doingNothing;
	bool isLoaded     = false;

	float FUSE_DURATION = 20.0f;
	float fuseCountdown = FUSE_DURATION;

	glm::vec3 firingDirection = glm::vec3(1.0f, 1.0f, 1.0f);

	int cannonTrackPosition = 0;
	float pushDistance      = 10.0f;
	float beingPushedSpeed  = 0.2f;
	MoveableUsingLerp pushStateData;

	float WAIT_BEFORE_BEING_PUSHED_DURATION = 0.3f;
	float waitBeforeBeingPushedCountdown    = WAIT_BEFORE_BEING_PUSHED_DURATION;
};

struct CarryableItem
{
	Team team;

	bool isActive = true;

	CarryableItemType type = CarryableItemType::invalid; //must manually be set when spawning items

	bool isBeingCarried = false;
	bool hasBeenCarried = false;
};

struct Respawnable
{
	glm::vec3 spawnPosition = glm::vec3(0.0f);
	glm::vec3 spawnRotation = glm::vec3(0.0f);
};

struct RespawnManager
{
	entt::entity entityPrefab = entt::null;

	CarryableItemType type = CarryableItemType::invalid; //all respawnable items are also carryable items.. so this works fine
	Team team;

	float respawnTimerDuration  = 10.0f;
	float respawnTimerCountdown = respawnTimerDuration;

	bool isRespawnTimerActive = false;
};

struct Cannonball
{
	bool isWaitingToBeFired = false;
	bool isBeingFired       = false;

	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 v3;
	glm::vec3 v4;

	float interpolationParam = 0.0f;
	float speedWhenFired     = 0.7f;
};

struct Gloop
{
	int numUses = 2;
};

struct CannonballCrate
{
	Team team;
};

struct GarbagePile
{
	Team team;

	bool isGlooped = false;

	int MAX_GARBAGE_LEVEL = 5;
	int garbageLevel = 1;

	float GARBAGE_TICKS_PER_LEVEL = 4.0f;
	float garbageTicks = GARBAGE_TICKS_PER_LEVEL;

	float DELAY_BEFORE_ADDING_GARBAGE_DURATION = 1.0f; //this is used to add garbage to a pile that a cannon has shot at, or any other reason a delay could be wanted before adding garbage
	float delayBeforeAddingGarbageCountdown    = -1.0f; //timer shouldnt start at the beginning of the game

	float DELAY_BEFORE_REMOVING_GARBAGE_DURATION = 1.2f;  //this should be equal to the player's time it takes to clean
	float delayBeforeRemovingGarbageCountdown    = -1.0f; //timer shouldnt start at the beginning of the game

	int relativePositionOnShip = -10; //this will be -1 (left), 0 (middle), or 1 (right) to mirror the cannon track position. It is used to determine which pile the cannons are firing at
};

struct PlayerInteractionType
{
	PlayerInteractionResult type;
};

struct PlayerHUDElement
{
	PlayerNumber playerNum = PlayerNumber::One; //should always correspond with player camera's player number
	glm::vec3 positionWhenActive;
};

struct Reticle
{
	ReticleType type;
};

struct EndScreen
{
	bool isLoseScreen;
};

struct GarbagePileHealthBar
{
	entt::entity outlineEntity; //the outline of the HP bar

	Team team;
	PlayerNumber playerNum = PlayerNumber::One; //player to render to
	int garbagePileNum = 0;

	float interpolationSpeed = 0.8f; //this should be relative to the player's time it takes to clean (1 / (time it takes to clean) = the value this variable should be)
	float interpolationParam = 0.0f;
	float startValue         = 1.0f; //health bar starts at full
	float targetValue        = 1.0f;

	bool shouldHideAfterInterpolating = false;
	bool shouldBeHidden               = false;
};

struct CleaningQuicktimeEventIndicator
{
	entt::entity cleaningQuicktimeEventBackground;

	bool isActive = false;

	float LOWER_BOUND_FOR_SUCCESS = 0.37f;
	float UPPER_BOUND_FOR_SUCCESS = 0.63f;

	MoveableUsingLerp lerpInformation;

	float DELAY_BEFORE_DISAPPEARING_DURATION = 0.3f;
	float delayBeforeDisappearingCountdown   = 0.0f;

	bool shouldShake          = false;
	bool isNumberOfShakesEven = true;

	float SHAKE_START_VALUE         = 0.3f;
	float SHAKE_DECREASE_PER_SECOND = 1.0f;
	float currentShakeValue         = SHAKE_START_VALUE;
};

struct CameraBreathing
{
	float startPosY;

	float cameraHeightVariance = 0.04f;
	float interpolationParam   = 0.5f; //start halfway up (default camera height)
	float speed = 0.45f;

	bool isMovingUp = true;
};
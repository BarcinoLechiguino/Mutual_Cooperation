#include "j1App.h"
#include "j1Player1.h"
#include "j1Player2.h"
#include "j1Module.h"
#include "p2Point.h"
#include "j1Render.h"
#include "j1Input.h"
#include "p2Log.h"
#include "j1Map.h"
#include "j1Scene.h"
#include "j1Textures.h"
#include "j1Collisions.h"
#include "j1FadeScene.h"
#include "j1Audio.h"
#include "j1EntityManager.h"

j1Player1::j1Player1(int x, int y, ENTITY_TYPE type) : j1Entity(x, y, ENTITY_TYPE::PLAYER) //THIS HERE //Constructor. Called at the first frame.
{
	//String that will be given to the different functions (Awake(), Load()...) to generate the handler node.
	name.create("entities"); //The string has to be the same as the name of the node in the xml file.

	AddAnimationPushbacks();
};

j1Player1::~j1Player1()  //Destructor. Called at the last frame.
{

};

bool j1Player1::Init()
{
	return true;
};

bool j1Player1::Awake(pugi::xml_node& config)
{
	return true;
};

bool j1Player1::Start()
{
	LoadPlayerProperties();			//Loads the player's properties from the xml file. //THIS HERE
	LoadPlayer1();					//Loads P1 in game.
	//LoadPlayer1Textures();		//Loads P1's textures in game.

	p1.jumpFX = App->audio->LoadFx("audio/fx/Jump.wav");
	p1.deathFX = App->audio->LoadFx("audio/fx/Death.wav");
	p1.duoFX = App->audio->LoadFx("audio/fx/Jump_Duo.wav");
	p1.activateFX = App->audio->LoadFx("audio/fx/Activate.wav");
	p1.tpFX = App->audio->LoadFx("audio/fx/TP.wav");
	p1.goalFX = App->audio->LoadFx("audio/fx/Pass.wav");

	p1.airborne = true;
	p1.item_activated = false;

	p1.state = idle_P1;

	return true;
};

bool j1Player1::PreUpdate()
{
	if (p1.GodMode == false)
	{
		p1.state = idle_P1;

		if (p1.speed.y > 2)				//THIS HERE
		{
			p1.state = falling_P1;
		}

		LOG("Speed y is %f", p1.speed.y);

		if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)				//Move Right
		{
			/*if (p1.againstLeftWall == false)
			{
				p1.state = goingRight_P1;
			}*/

			p1.state = goingRight_P1;
		}

		if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)				//Move Left
		{
			/*if (p1.againstRightWall == false)				//THIS HERE, for some reason the bool here is always set to false now that the entity system has been implemented
			{
				p1.state = goingLeft_P1;
			}*/

			p1.state = goingLeft_P1;
		}

		if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)				//Crouch
		{
			/*if (p1.state != falling_P1)						//THIS HERE
			{

			}*/

			p1.state = crouching_P1;
		}

		if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)				//Jump
		{
			if (p1.grounded == true)
			{
				p1.state = jumping_P1;
			}
		}

		if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)				//Drop from platform
		{
			p1.platformDrop = true;
		}
		else
		{
			p1.platformDrop = false;
		}

		if (App->input->GetKey(SDL_SCANCODE_E) == KEY_DOWN)					//Teleport
		{
			p1.state = teleporting_P1;
		}

		if (App->input->GetKey(SDL_SCANCODE_M) == KEY_REPEAT)				//Suicide Button
		{
			p1.state = dying_P1;
		}
	}
	else
	{
		GodModeInput();														//GodMode 
	}

	//Switch Sprites Method Call
	/*if (App->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN)
	{
		App->tex->UnLoad(p1.texture);
		LoadPlayer1Textures();
	}*/

	return true;
};

bool j1Player1::Update(float dt, bool doLogic)
{
	switch (p1.state)
	{
	case idle_P1:

		animation = &p1.idle;					//ALL THIS HERE Change all p1.position or p1.current animation for position o animation.
		p1.isCrouching = false;

		break;

	case goingRight_P1:

		if (p1.againstLeftWall == false)
		{
			position.x += p1.speed.x * dt;

			p1.flip = false;
			animation = &p1.running;
			p1.isGoingRight = true;
		}

		break;

	case goingLeft_P1:

		if (p1.againstRightWall == false)
		{
			position.x -= p1.speed.x * dt;

			p1.flip = true;
			animation = &p1.running;
			p1.isGoingLeft = true;
		}

		break;

	case crouching_P1:

		animation = &p1.crouching;
		p1.isCrouching = true;

		break;

	case jumping_P1:

		App->audio->PlayFx(5, 0);
		p1.speed.y = -p1.acceleration.y * dt;
		p1.isJumping = true;					//Boolean for animations
		p1.airborne = true;
		p1.grounded = false;

		break;

	case falling_P1:		//When dropping from platforms

		p1.airborne = true;
		//p1.grounded = false;					//With this commented, jumping after falling off platforms is allowed.
		animation = &p1.falling;

		break;

	case teleporting_P1:

		TeleportP2ToP1();

		break;

	case dying_P1:

		//App->audio->PlayFx(2, 0);
		animation = &p1.death;
		p1.isDying = true;

		break;
	}

	//If P1 is in the air then this function brings him/her back down to the floor.
	if (p1.airborne == true)
	{
		p1.speed.y += p1.gravity * dt;

		if (p1.speed.y > p1.max_speed.y * dt)
		{
			p1.speed.y = p1.max_speed.y * dt;
		}

		position.y += p1.speed.y;				//Refreshes the vector speed of P1 in the Y axis.

		//Jump animation modifications.
		if (p1.isBoostJumping == true)				//If P1 is boost jumping then this set of animations is played.
		{
			if (p1.speed.y < p1.frontflipStart)
			{
				animation = &p1.jumping;
			}
			else if (p1.speed.y < p1.frontflipEnd)
			{
				animation = &p1.frontflip;
			}
			else
			{
				animation = &p1.falling;
			}
		}
		else if (p1.isJumping == true)				//If P1 is jumping then this set of animations is played.
		{
			if (p1.speed.y < 0)
			{
				animation = &p1.jumping;
			}
			else
			{
				animation = &p1.falling;
			}
		}
	}

	//--------------------------------------- Miscelaneous Checks ---------------------------------------
	LivesCheck(p1.lives);																	// If P1 empties the lives pool then both players die and reset their positions to the spawn point.

	//--------------------------------------- Tp skill Cooldown ---------------------------------------
	if (p1.tpInCd == true)
	{
		//TpCooldownCheck(dt);
		SkillCooldown(p1.tpInCd, p1.tpCdCount, p1.tpCdTime);
	}

	//THIS HERE
	p1.HitBox = animation->GetCurrentFrame(dt);												//THIS HERE //Sets the animation cycle that P1 will have. 
	collider->Set_Position(position.x, position.y);											//Makes P1's collider follow P1.
	//p1.atkCollider->Set_Position(position.x + sprite_size.x, sprite_size.y);				//Makes P1's attack collider follow P1.

	BlitEntity(position.x, position.y, p1.HitBox, p1.flip);									//Blits the player on screen with the data we have given the Blit() function.
	//
	return true;
};

bool j1Player1::PostUpdate()
{
	//Resetting the collider related bools after the collision happened in Update() so it can be checked the next update/frame.
	p1.againstCeiling = false;
	p1.againstRightWall = false;
	p1.againstLeftWall = false;

	return true;
};

bool j1Player1::CleanUp()
{
	App->tex->UnLoad(p1.texture);
	//App->player1->Disable();

	App->entityManager->player->Disable();	//THIS HERE

	collider = nullptr;						//THIS HERE
	animation = nullptr;					//THIS HERE

	return true;
};

//Collision Handling ---------------------------------------
void j1Player1::OnCollision(Collider* C1, Collider* C2)
{
	if (p1.GodMode == false)
	{
		if (C2->type == Object_Type::PLAYER)
		{
			Collider* temp = C1;
			C1 = C2;
			C2 = temp;
		}
		if (C1->type != Object_Type::PLAYER)
		{
			return;
		}

		if (C1->type == Object_Type::PLAYER)
		{
			//Player Colliding Against Another Player
			if (C2->type == Object_Type::PLAYER)
			{
				if (C1->collider.x + C1->collider.w > C2->collider.x || C1->collider.x < C2->collider.x + C2->collider.w) //As the boost can be done even if P1 is static, this allows for more precise jumps... hopefully.
				{
					if (App->player2->p2.state == crouching_P2 /*App->player2->p2.isCrouching == true*/)
					{
						if (p1.grounded == true)
						{
							p1.speed.y -= p1.boost_jump.y * App->GetDt();
							p1.isBoostJumping = true;
							p1.airborne = true;
							p1.grounded = false;
							p1.platformDrop = false;
							App->audio->PlayFx(3, 0);
						}
					}
					LOG("P1 IS COLLIDING WITH P2 FROM THE LEFT");
				}
			}

			//Player colliding against solids
			if (C2->type == Object_Type::SOLID)
			{
				//Player Colliding from TOP or BOTTOM. 
				if (C1->collider.x + C1->collider.w >= C2->collider.x && C1->collider.x <= C2->collider.x + C2->collider.y)		//The first part checks if C1 is contained in the X axis of C2. 
				{
					//Player Colliding from TOP.
					if (C1->collider.y + C1->collider.h >= C2->collider.y && C1->collider.y < C2->collider.y && p1.speed.y != 0)			//This second part checks if C1 is actually colliding vertically down.
					{
						p1.speed.y = 0;
						position.y = C2->collider.y - C1->collider.h + 1;		//THIS HERE
						p1.isJumping = false;
						p1.isBoostJumping = false;
						p1.grounded = true;
						LOG("P1 IS COLLIDING WITH A SOLID FROM ABOVE");
					}

					//Player Colliding from BOTTOM.
					else if (C1->collider.y < C2->collider.y + C2->collider.h && C1->collider.y + 20 > C2->collider.y + C2->collider.h && C1->collider.y > C2->collider.y)	//This second part checks if C1 is actually colliding vertically down.
					{
						p1.speed.y = 0;
						position.y = C2->collider.y + C2->collider.h + 1;		//THIS HERE

						p1.againstCeiling = true;
						LOG("P1 IS COLLIDING WITH A SOLID FROM BELOW");
					}
				}

				//Player is colliding from LEFT or RIGHT.
				if (C1->collider.y <= C2->collider.y + C2->collider.h && C1->collider.y + C1->collider.h - 4 >= C2->collider.y)		//The first part checks if C1 is contained in the Y axis of C2.
				{
					//Player is colliding from LEFT.
					if (C1->collider.x + C1->collider.w >= C2->collider.x && C1->collider.x <= C2->collider.x)						//This second part checks if C1 is actually colliding from the left side of the collider.
					{
						p1.againstLeftWall = true;
						p1.againstRightWall = false;
						LOG("P1 IS COLLIDING WITH A SOLID FROM THE LEFT");
					}

					//Player is colliding from RIGHT.
					if (C1->collider.x <= C2->collider.x + C2->collider.w && C1->collider.x >= C2->collider.x)						// This second part checks if C1 is actually colliding from the right side of the collider.
					{
						p1.againstRightWall = true;
						p1.againstLeftWall = false;
						LOG("P1 IS COLLIDING WITH A SOLID FROM THE RIGHT");
					}
				}
			}

			if (C2->type == Object_Type::PLATFORM)
			{
				if (p1.platformDrop == false)
				{
					if (C1->collider.x >= C2->collider.x && C1->collider.x + C1->collider.w <= C2->collider.x + C2->collider.w)
					{
						//Player Colliding from Above the Solid, ergo colliding with the ground. This second part checks if C1 is actually colliding vertically down.
						if (C1->collider.y + C1->collider.h >= C2->collider.y && C1->collider.y < C2->collider.y)
						{
							p1.speed.y = 0;
							p1.position.y = C2->collider.y - C1->collider.h + 1;
							p1.isJumping = false;
							p1.isBoostJumping = false;
							p1.grounded = true;
							LOG("P1 IS COLLIDING WITH A SOLID FROM ABOVE");
						}
					}
				}
			}

			if (C2->type == Object_Type::HAZARD)
			{
				//Death logic
				p1.lives--;
				p1.state = dying_P1;

				//Antes la animacion de muerte tiene que haber finalizado - Gerard
				RespawnP1ToP2();
			}

			//Player Colliding against an Activable Item
			if (C2->type == Object_Type::ITEM)
			{
				p1.item_activated = true;					//Records that P1 (or P2) has activated the item.
				App->player2->p2.item_activated = true;		//Activates P2's boolean as well.

				App->audio->PlayFx(4, 1);					//Item Activation sfx.
			}

			//Player colliding against Deactivable surfaces. 
			if (C2->type == Object_Type::DEACTIVABLE)
			{
				if (p1.item_activated == false || App->player2->p2.item_activated == false)
				{
					//Player Colliding vertically against the Solid. The first part checks if C1 is contained in the X axis of C2. 
					if (C1->collider.x + C1->collider.w >= C2->collider.x && C1->collider.x <= C2->collider.x + C2->collider.y)
					{
						//Player Colliding from Above the Solid, ergo colliding with the ground. This second part checks if C1 is actually colliding vertically down.
						if (C1->collider.y + C1->collider.h >= C2->collider.y && C1->collider.y < C2->collider.y)
						{
							p1.speed.y = 0;
							p1.isJumping = false;
							p1.isBoostJumping = false;
							p1.grounded = true;
							LOG("P1 IS COLLIDING WITH A SOLID FROM ABOVE");
						}

						//Player Colliding from Below the Solid. ergo colliding with the ceiling. This second part checks if C1 is actually colliding vertically down.
						else if (C1->collider.y < C2->collider.y + C2->collider.h && C1->collider.y > C2->collider.y)
						{
							p1.speed.y = 0;
							p1.position.y = C2->collider.y + C2->collider.h + 1;
							p1.againstCeiling = true;
							LOG("P1 IS COLLIDING WITH A SOLID FROM BELOW");
						}
					}

					//Player is colliding from the sides. The first part checks if C1 is contained in the Y axis of C2.
					if (C1->collider.y <= C2->collider.y + C2->collider.h && C1->collider.y + C1->collider.h - 4 >= C2->collider.y)
					{
						//Player is colliding from left (going right). This second part checks if C1 is actually colliding from the left side of the collider.
						if (C1->collider.x + C1->collider.w >= C2->collider.x && C1->collider.x < C2->collider.x)
						{
							p1.againstLeftWall = true;
							p1.againstRightWall = false;
							LOG("P1 IS COLLIDING WITH A SOLID FROM THE LEFT");
						}

						//Player is colliding from right (going left). This second part checks if  C1 is actually colliding from the right side of the collider.
						if (C1->collider.x <= C2->collider.x + C2->collider.w && C1->collider.x > C2->collider.x)
						{
							p1.againstRightWall = true;
							p1.againstLeftWall = false;
							LOG("P1 IS COLLIDING WITH A SOLID FROM THE RIGHT");
						}
					}
				}
			}

			if (C2->type == Object_Type::CHECKPOINT)
			{
				LOG("P1 HAS REACHED A CHECKPOINT");															//Call Safe() method here.
			}

			//Player colliding against the Goal
			if (C2->type == Object_Type::GOAL)
			{
				if (C1->collider.y > GOAL_Y && C1->collider.y < GOAL_HEIGHT)	//Dirty way to know which portal goal has been reached.
				{
					App->fadescene->FadeToBlack("1st_Level.tmx");				//Loads the 1st level.
					App->map->Restart_Cam();
				}
				else
				{
					App->fadescene->FadeToBlack("Tutorial_Level.tmx");			//Loads the 2nd level.
					App->map->Restart_Cam();
				}

				App->audio->PlayFx(6, 0);										//Sound effect of the Goal / Protal.
			}
		}
	}
}

bool j1Player1::Load(pugi::xml_node& data)
{
	p1.position.x = data.child("position1").attribute("x").as_int();	//Loads the position of the X axis saved in the save_file.xml.
	p1.position.y = data.child("position1").attribute("y").as_int();	//Loads the position of the X axis saved in the save_file.xml.
	return true;
}

bool j1Player1::Save(pugi::xml_node&  data) const
{
	pugi::xml_node pos = data.append_child("position1");	//Declares the node and with "append_" it is set to overwrite the data in the given xml. 

	pos.append_attribute("x") = p1.position.x;				//Saves the position of P1 on  the X axis the moment Save() is called. "append_" used again to overwrite previous position data.
	pos.append_attribute("y") = p1.position.y;				//Saves the position of P1 on  the Y axis the moment Save() is called. "append_" used again to overwrite previous position data.
	return true;
}

bool j1Player1::LoadPlayer1()								//Loads P1 on screen.
{
	//Loads the textures of P1. Switches them according to switch_sprites
	/*entity_sprite = new SDL_Texture();*/		//Revise this

	entity_sprite = App->tex->Load("textures/Spritesheets/Character 1/character_spritesheet_I_Buena.png");	//THIS HERE

	//--------------------------------------------Loading the data and colliders of P1--------------------------------------------

	//Loads the data of the rectangle that contains P1.
	p1.HitBox.x = position.x;							//Represents the position in the X axis of P1.		//THIS HERE
	p1.HitBox.y = position.y;							//Represents the position in the Y axis of P1.
	p1.HitBox.w = sprite_size.x;						//Represents the width of P1.
	p1.HitBox.h = sprite_size.y;						//Represents the height of P1.

	p1.atkHitBox.x = p1.position.x + p1.sprite_measures.x;	//Position in the X axis of P1's attack collider.
	p1.atkHitBox.y = p1.position.y;							//Position in the Y axis of P1's attack collider.
	p1.atkHitBox.w = p1.sprite_measures.x;					//Width of P1's attack collider.
	p1.atkHitBox.y = p1.sprite_measures.y;					//Height of P1's attack collider.

	//Adds a collider for the player.
	//collider = new Collider();			//THIS HERE

	collider = App->collisions->AddCollider(p1.HitBox, Object_Type::PLAYER, App->entityManager);				//THIS HERE

	//LOG("COLLIDER IS AT POS: (%d, %d)", collider->collider.x, collider->collider.y);

	p1.atkCollider = App->collisions->AddCollider(p1.atkHitBox, Object_Type::ATTACK, this);

	//Boolean resetting
	p1.grounded = false;
	p1.airborne = false;
	p1.flip = false;
	p1.isCrouching = false;
	p1.isJumping = false;
	p1.isBoostJumping = false;
	p1.item_activated = false;
	p1.isGoingRight = false;
	p1.isGoingLeft = false;
	p1.platformDrop = false;
	p1.tpInCd = false;
	p1.fading = false;
	p1.isAlive = true;
	p1.isDying = false;
	p1.againstRightWall = false;
	p1.againstLeftWall = false;
	p1.againstCeiling = false;
	p1.GodMode = false;
	p1.switch_sprites = false;

	return true;
}

bool j1Player1::AddAnimationPushbacks()
{
	p1.idle.LoadAnimation("idle");					//P1's Idle animation.
	p1.running.LoadAnimation("running");			//P1's Running animation.
	p1.crouching.LoadAnimation("crouching");		//P1's Crouching animation.	
	p1.jumping.LoadAnimation("jumping");			//P1's Jumping Animation.
	p1.frontflip.LoadAnimation("frontflip");		//P1's Frontflip Animation.
	p1.boosted_jump.LoadAnimation("boosted_jump");	//P1's Boosted Jump Animation.
	p1.falling.LoadAnimation("falling");			//P1's Falling Animation.
	p1.death.LoadAnimation("death");				//P1's Death Animation.

	return true;
}

bool j1Player1::LoadPlayer1Properties(pugi::xml_node& config)
{
	//Gets all the required player variables from the config xml file
	position.x = config.child("player").child("position").attribute("x").as_float();			//THIS HERE
	position.y = config.child("player").child("position").attribute("y").as_float();			//THIS HERE
	p1.spawn_position.x = config.child("player").child("position").attribute("x").as_float();
	p1.spawn_position.y = config.child("player").child("position").attribute("y").as_float();

	p1.speed.x = config.child("player").child("speed").attribute("x").as_float();
	p1.speed.y = config.child("player").child("speed").attribute("y").as_float();
	p1.max_speed.x = config.child("player").child("max_speed").attribute("x").as_float();
	p1.max_speed.y = config.child("player").child("max_speed").attribute("y").as_float();

	p1.acceleration.x = config.child("player").child("acceleration").attribute("x").as_float();
	p1.acceleration.y = config.child("player").child("acceleration").attribute("y").as_float();
	p1.gravity = config.child("player").child("gravity").attribute("value").as_float();

	p1.boost_jump.x = config.child("player").child("boost_jump").attribute("x").as_float();
	p1.boost_jump.y = config.child("player").child("boost_jump").attribute("y").as_float();

	sprite_size.x = config.child("player").child("sprite_measures").attribute("w").as_int();		//THIS HERE //Maybe change it to two separate variables (spirte_width, sprite_height)
	sprite_size.y = config.child("player").child("sprite_measures").attribute("h").as_int();		//THIS HERE

	/*p1.sprite_measures.x = config.child("player").child("sprite_measures").attribute("w").as_int();
	p1.sprite_measures.y = config.child("player").child("sprite_measures").attribute("h").as_int();*/

	p1.frontflipStart = config.child("player").child("frontflip").attribute("start").as_float();
	p1.frontflipEnd = config.child("player").child("frontflip").attribute("end").as_float();

	p1.lives = config.child("player").child("lives").attribute("lives").as_int();
	p1.max_lives = config.child("player").child("lives").attribute("lives").as_int();

	p1.tpCdCount = config.child("player").child("tpCooldown").attribute("timer").as_float();
	p1.tpCdTime = config.child("player").child("tpCooldown").attribute("cd").as_float();

	p1.godModeSpeed = config.child("player").child("godspeed").attribute("gspeed").as_float();

	LOG("Speed.x = %f", p1.speed.x);

	return true;
}

bool j1Player1::LoadPlayerProperties()
{
	config_file.load_file("config.xml");

	player = config_file.child("config").child("entities").child("player");

	//Gets all the required player variables from the config xml file
	position.x = player.child("position").attribute("x").as_float();			//THIS HERE
	position.y = player.child("position").attribute("y").as_float();			//THIS HERE
	p1.spawn_position.x = player.child("position").attribute("x").as_float();
	p1.spawn_position.y = player.child("position").attribute("y").as_float();

	p1.speed.x = player.child("speed").attribute("x").as_float();
	p1.speed.y = player.child("speed").attribute("y").as_float();
	p1.max_speed.x = player.child("max_speed").attribute("x").as_float();
	p1.max_speed.y = player.child("max_speed").attribute("y").as_float();

	p1.acceleration.x = player.child("acceleration").attribute("x").as_float();
	p1.acceleration.y = player.child("acceleration").attribute("y").as_float();
	p1.gravity = player.child("gravity").attribute("value").as_float();

	p1.boost_jump.x = player.child("boost_jump").attribute("x").as_float();
	p1.boost_jump.y = player.child("boost_jump").attribute("y").as_float();

	sprite_size.x = player.child("sprite_measures").attribute("w").as_int();		//THIS HERE //Maybe change it to two separate variables (spirte_width, sprite_height)
	sprite_size.y = player.child("sprite_measures").attribute("h").as_int();		//THIS HERE

	/*p1.sprite_measures.x = config.child("player").child("sprite_measures").attribute("w").as_int();
	p1.sprite_measures.y = config.child("player").child("sprite_measures").attribute("h").as_int();*/

	p1.frontflipStart = player.child("frontflip").attribute("start").as_float();
	p1.frontflipEnd = player.child("frontflip").attribute("end").as_float();

	p1.lives = player.child("lives").attribute("lives").as_int();
	p1.max_lives = player.child("lives").attribute("lives").as_int();

	p1.tpCdCount = player.child("tpCooldown").attribute("timer").as_float();
	p1.tpCdTime = player.child("tpCooldown").attribute("cd").as_float();

	p1.godModeSpeed = player.child("godspeed").attribute("gspeed").as_float();

	return true;
}

/*bool j1Player1::LoadPlayer1Textures()
{
	//Loads the textures of P1. Switches them according to switch_sprites
	if (p1.switch_sprites == true)
	{
		p1.texture = App->tex->Load("textures/Spritesheets/Character 1/character_spritesheet_I_Buena.png");
	}
	else
	{
		p1.texture = App->tex->Load("textures/Spritesheets/Character 2/Character_Spritesheet2_Buena.png");
	}

	return true;
}*/

void j1Player1::TeleportP2ToP1()		//Method that teleports P2 directly in front of P1. Takes into account which direction P1 is facing. Can trigger some fun interactions between players :)
{
	if (!p1.tpInCd)
	{
		if (p1.flip == false)			//The players will be always teleported directly in front of one another. 
		{
			App->player2->p2.position.x = position.x + collider->collider.w;		//THIS HERE
			App->player2->p2.position.y = position.y - 60;
		}
		else
		{
			App->player2->p2.position.x = position.x - collider->collider.w / 2;
			App->player2->p2.position.y = position.y - 60;
		}

		App->audio->PlayFx(1, 0);
		p1.tpInCd = true;
	}
}

void j1Player1::RespawnP1ToP2()		//Method that, on death, will respawn P1 behind P2.
{
	if (p1.flip == true)			//The players will be always respawned directly behind of one another. 
	{
		position.x = App->player2->p2.position.x + App->player2->p2.collider->collider.w;
		position.y = App->player2->p2.position.y - 40;
	}
	else
	{
		position.x = App->player2->p2.position.x + App->player2->p2.collider->collider.w / 2;
		position.y = App->player2->p2.position.y - 40;
	}
}

//---------------------------------------------- General Checks ----------------------------------------------
void j1Player1::LivesCheck(int lives)
{
	if (lives == 0)
	{
		p1.isAlive = false;
		App->player2->p2.isAlive = false;

		if (p1.isAlive == false && App->player2->p2.isAlive == false)
		{
			//App->audio->PlayFx(2, 0);
			Restart();
			App->player2->Restart();
			p1.lives = p1.max_lives;
		}
	}
	else
	{
		return;
	}
}

void j1Player1::SkillCooldown(bool& inCd, float& cdCounter, float& cdTime)			//Arguments need to be passed as reference (&) so the method can modify the actual variable instead of a copy.
{
	cdCounter += App->GetDt();				//Adds the time elapsed in a frame to a count.

	if (cdCounter > cdTime)					//Checks if the cd Count has reached the required amount of time.
	{
		inCd = false;						//Resets the bool so tp can be used again.
		cdCounter = 0;						//Resets the count so it can be used the next time tp is in cd.
	}

	//if (inCd == p1.tpInCd)					//Specific cooldown release sound for the tp. bool name needs to be passed as a string. Revise
	//{

	//}
}

void j1Player1::TpCooldownCheck(float dt)	//Revise if it is needed.
{
	p1.tpCdCount += dt;						//Adds the time elapsed in a frame to a count.

	if (p1.tpCdCount > p1.tpCdTime)			//Checks if the cd Count has reached the required amount of time.
	{
		p1.tpInCd = false;					//Resets the bool so tp can be used again.
		p1.tpCdCount = 0;					//Resets the count so it can be used the next time tp is in cd.
	}
}

void j1Player1::Restart()
{
	p1.position = p1.spawn_position;
	p1.isAlive = true;
}

void j1Player1::GodModeInput()
{
	p1.airborne = false;

	if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)		//THIS HERE
	{
		position.x += p1.godModeSpeed * App->GetDt();
	}
	if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
	{
		position.x -= p1.godModeSpeed * App->GetDt();
	}
	if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
	{
		position.y -= p1.godModeSpeed * App->GetDt();
	}
	if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
	{
		position.y += p1.godModeSpeed * App->GetDt();
	}
}
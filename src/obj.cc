#include "obj.h"
#include "world.h"
#include "battle.h" 

/* Basic Object */
Obj::Obj(){
	active=true;
	id = objtotal++;
	LOG("Object " << id << " created");
	world = NULL;
	persistent = false;
}
Obj::~Obj() { 
	while (!tweens.empty()) {
		delete tweens.front();
		tweens.pop();
		LOG(id << " destroyed active tween");
	}

	LOG("Object " << id << " destroyed");
}
void Obj::update() {
	if (tweens.size() > 0) {
		std::queue<GenericTween*> tempqueue;
		while (!tweens.empty()) {
			if (tweens.front()->step()) {
				tempqueue.push(tweens.front());
				tweens.pop();
			}
			else {
				delete tweens.front();
				tweens.pop();
				LOG("tween finished and destroyed");
			}
		}
		swap(tweens, tempqueue);
	}
}	
void Obj::map_start() {}
void Obj::map_end() {}
bool Obj::destroy() {
	dead=true;
	if (!world)
		return false;

	world->queue_destroy(this);
	return true;
	/* TODO make World just check for dead instead of keeping a vector of deads */
}
/*void Obj::collide(Obj *other) {} */
void Obj::set_active(bool active) { this->active = active; }
bool Obj::is_active() const { return active; }
void Obj::set_persistent(bool persistent) { this->persistent = persistent; }
bool Obj::is_persistent() const { return persistent; }
bool Obj::is_solid() const { return solid; }
void Obj::interact() {}

ObjType Obj::get_type() const { return OBJ; }

int Obj::objtotal = 0;

int Obj::get_id() const { return id; }
Box Obj::get_bbox() const { return Box(0.0f, 0.0f, 0.0f, 0.0f); }
void Obj::attach_to_world(World *world) { 
	this->world = world; 
	world->register_object(this);
}
bool Obj::operator ==(const Obj &rhs) {
	return this->get_id() == rhs.get_id();
}
bool Obj::operator !=(const Obj &rhs) {
	return this->get_id() != rhs.get_id();
}



/* Physical Object */
PhysicalObj::PhysicalObj(float x, float y, float w, float h) : x(x), y(y), z(0.0f), w(w), h(h) {
	LOG(get_bbox());
}
PhysicalObj::~PhysicalObj() {}
float PhysicalObj::get_x() const { return x; }
float PhysicalObj::get_y() const { return y; }
float PhysicalObj::get_z() const { return z; }
float PhysicalObj::get_w() const { return w; }
float PhysicalObj::get_h() const { return h; }
Box PhysicalObj::get_bbox() const { return Box(x,y,w,h); }
ObjType PhysicalObj::get_type() const { return OBJ_PHYSICAL; }

void PhysicalObj::set_position(float x, float y) {
	this->x = x;
	this->y = y;
}
void PhysicalObj::displace(float dx, float dy) {
	this->x += dx;
	this->y += dy;
}
void PhysicalObj::displace(Vec2f disp) {
	this->x += disp.get_x();
	this->y += disp.get_y();
}



/* Visible Object */
VisibleObj::VisibleObj(float x, float y, float w, float h, int depth, SpriteSheet *s) : PhysicalObj(x, y, w, h), depth(y), sheet(s) {
	if (sheet) {
		sprite = (*sheet)[0];
		sprite->sprite_center_origin(ORIGIN_CENTER_BOTTOM); 
	}
	else 
		sprite = NULL;

	aspeed = 2.0 / 60.0;
	loop = true;
	frame_index = 0;
	visible = true;
	hflip = false;
	vflip = false;
	alpha = 1.0f;
}

VisibleObj::~VisibleObj() {}
ObjType VisibleObj::get_type() const { return OBJ_VISIBLE; }

void VisibleObj::set_sprite(SpriteSheet *s, int index) {
	sheet = s;
	if (sheet) {
		sprite = (*sheet)[index];
		sprite->sprite_center_origin(ORIGIN_CENTER_BOTTOM);
	}
	else
		sprite = NULL;
}

void VisibleObj::set_sprite(Sprite *sprite) { this->sprite = sprite; }

void VisibleObj::draw() {

	if (sprite){
		int flags = 0;
		if (hflip)
			flags = flags | ALLEGRO_FLIP_HORIZONTAL;
		if (vflip)
			flags = flags | ALLEGRO_FLIP_VERTICAL;

		sprite->sprite_draw(x,y-z,frame_index, flags, al_map_rgba_f(1.0f * alpha, 1.0f * alpha, 1.0f * alpha, alpha));
		if (world && world->get_mode() == World::MODE_OVERWORLD)
			frame_index += aspeed;
		if (frame_index >= sprite->getframes() && !loop) {
			aspeed = 0;
			frame_index = sprite->getframes();
		}
	}
	else
		al_draw_filled_ellipse(x, y, w/2, h/2, al_map_rgb(0,0,125));	

#if DEBUG_DRAW
	get_bbox().draw(al_map_rgb(0,255,0));
	al_draw_filled_rectangle(x-1,y-1,x+1,y+1,al_map_rgb(0,0,0));
#endif
}
bool VisibleObj::destroy() {
	if (!world)
		return false;

	world->queue_destroy_visible(this);
	return true;
}
bool VisibleObj::operator<(const VisibleObj &rhs) {
	return depth > rhs.depth; //reverse order, since we draw depth high to low
}



/* Mobile Object */
MobileObj::MobileObj(float x, float y, float w, float h, int depth, SpriteSheet *s) : VisibleObj(x, y, w, h, depth, s) {
	dy = 0;
	dx = 0;
	direction = DIR_S;
}
MobileObj::~MobileObj() {}
void MobileObj::update() {
	x += dx;
	y += dy;
	depth = y; 

	super::update();
}
float MobileObj::get_dx() const { return dx; }
float MobileObj::get_dy() const { return dy; }
ObjType MobileObj::get_type() const { return OBJ_MOBILE; }
void MobileObj::face_point(float ox, float oy) {
	float slope = ((oy-y)/(ox-x));
	bool right = ox > x;
	float angle_low = 0.57f;
	float angle_high = 1.73f;

	if ((slope < angle_low && slope >= 0) ||
		(slope > -angle_low && slope < 0)) {
		if (right)
			direction = DIR_E;
		else
			direction = DIR_W;
	}

	if (slope > angle_low && slope < angle_high) {
			if (right)
				direction = DIR_SE;
			else
				direction = DIR_NW;
	}

	if (slope > angle_high) {
		if (right)
			direction = DIR_S;
		else
			direction = DIR_N;
	}

	if (slope < -angle_high) {
		if (right)
			direction = DIR_N;
		else
			direction = DIR_S;
	}
	if (int(x) == int(ox)) {
		if (y < oy)
			direction = DIR_S;
		else
			direction = DIR_N;
	}

	if (slope < -angle_low && slope > -angle_high) {
		if (right)
			direction = DIR_NE;
		else
			direction = DIR_SW;
	}
}
void MobileObj::collide_with_tiles() {
	/* Tile collision handling */
	if (world && (dx != 0 || dy != 0)) {
		Vec2f intersection = world->get_map()->get_collision_vec(get_bbox(), get_bbox()+Vec2f(dx,dy));

		if (intersection.get_x() == 0)
			intersection.set_x(world->get_object_collision_solid(get_bbox(), get_bbox()+Vec2f(dx,dy)).get_x());
		if (intersection.get_y() == 0)
			intersection.set_y(world->get_object_collision_solid(get_bbox(), get_bbox()+Vec2f(dx,dy)).get_y());

		dx += intersection.get_x();
		dy += intersection.get_y();
	}
}


/* Prop object */
Prop::Prop(float x, float y, PropType t) : VisibleObj(x,y,0.0f,0.0f), t(t) {
	solid = true;
	switch (t) {
		case PROP_CANDELABRUM:
			set_sprite(SheetManager::get_sheet(SH_CASTLE_PROPS), 0);
			w = 12;
			h = 8;
			break;
		case PROP_CANDELABRUM_LIT:
			set_sprite(SheetManager::get_sheet(SH_CASTLE_PROPS), 1);
			w = 12;
			h = 8;
			aspeed = 0.1;
			break;

		default:
			sheet=NULL;
			sprite=NULL;
			break;
	}
}
void Prop::update(){
	super::update();
}
ObjType Prop::get_type() const { return OBJ_PROP; }
void Prop::interact() {
	switch (t) {
		case PROP_CANDELABRUM:
			if (world) {
				world->sndmgr->play_sound(SND_IGNITE);
				Player *p = world->get_player();
				p->face_point(x,y);
			}
			set_sprite(SheetManager::get_sheet(SH_CASTLE_PROPS), 1);
			aspeed = 0.1;
			t = PROP_CANDELABRUM_LIT;
			break;
		case PROP_CANDELABRUM_LIT:
			break;
		default:
			break;
	}
	/*
	   if (world && mymsg) {
	   Player *p = world->get_player();
	   p->face_point(x,y);
	   world->show_text(mymsg);
	   }
	   */
}
Box Prop::get_bbox() const { return Box(x-w/2, y-h, w, h); }



/* Dummy Object */
Dummy::Dummy(float x, float y) : MobileObj(x, y, 12, 8, 0, SheetManager::get_sheet(SH_DUMMY)) {
	spr_shadow = (*SheetManager::get_sheet(SH_SHADOW))[0];
	spr_shadow->sprite_center_origin(ORIGIN_CENTER_MIDDLE);
	aspeed = 0;
	solid = true;

	set_sprite(sheet, 5);

	tweens.push(new Tween<float>(&alpha, 0.0f, 1.0f, 60));  
	tweens.push(new LoopTween<float>(&z, 8.0f, 4.0f, 60));  
	// TODO simultaneous tweens
	// OR they work? Check this out further.
}
Dummy::~Dummy() {}

void Dummy::update() {
	/*
	   Player *p = world->get_player();
	   if (get_bbox().check_collision(p->get_bbox())) {
	   world->sndmgr->play_sound(SoundManager::SND_ACCEPT); 
	   destroy(); 

	   }
	   */
	if (world) {
		Player *p = world->get_player();
		face_point(p->get_x(), p->get_y());
	}

	super::update();
}

Box Dummy::get_bbox() const { return Box(x-w/2, y-h, w, h); }
void Dummy::draw() {
	if (sprite) {
		if (aspeed == 0)
			frame_index = direction;

		spr_shadow->sprite_draw(x,y+1,0.0f);

		super::draw();
	}
	else
		al_draw_filled_ellipse(x, y, w/2, h/2, al_map_rgb(0,255,0));	
}
ObjType Dummy::get_type() const { return OBJ_DUMMY; }
void Dummy::interact() {
	if (world) {
		Player *p = world->get_player();
		face_point(p->get_x(), p->get_y());
		p->face_point(x,y);
		/*		world->show_text(mymsg); */
	}
}


Enemy::Enemy(float x, float y, float w, float h, SpriteSheet *s, std::string cname) : MobileObj(x,y,w,h,0, s) {
	spr_shadow = (*SheetManager::get_sheet(SH_SHADOW))[0];
	spr_shadow->sprite_center_origin(ORIGIN_CENTER_MIDDLE);
	aspeed = 0;
	
	/*
	set_sprite(SheetManager::get_sheet(SH_DUMMY), 5);
	*/
	combatant = new Combatant(cname);
	combatant->set_parent(this);
}
Enemy::~Enemy() {
	if (combatant) {
		delete combatant;
		combatant = NULL;
	}
}
void Enemy::update() {
	if (world) {
		Player *p = world->get_player();
		if (get_bbox().check_collision(p->get_bbox())) {
			if (!invincible) {
				invincible=true;
				world->start_battle(combatant);
			}
		}
		else {
/*			invincible = false; */
		}
	}

	super::update();
}
Box Enemy::get_bbox() const { return Box(x-w/2, y-h+2, w, h); }
ObjType Enemy::get_type() const { return OBJ_ENEMY; }
void Enemy::draw() {
	spr_shadow->sprite_draw(x,y+1,0.0f);
	super::draw();
}

Spirit::Spirit(float x, float y) : Enemy(x, y, 16, 8, SheetManager::get_sheet(SH_SPIRIT), "Stale Spirit") {
	set_sprite(sheet, 0);
	combatant->add_action(ACT_ATT);
	combatant->set_image(sheet->get_sprite(1)->get_bitmap());
	aspeed = 1.0f / 20.0f;
	z = 4.0f;
}
Spirit::~Spirit() {}
void Spirit::update() {
	if (world->get_player()->get_x() > x)
		dx = maxspeed;
	else if (world->get_player()->get_x() < x)
		dx = -maxspeed;
	else
		dx = 0;

	if (world->get_player()->get_y() > y)
		dy = maxspeed;
	else if (world->get_player()->get_y() < y)
		dy = -maxspeed;
	else
		dy = 0;

	collide_with_tiles();

	/* face the way yer going */
	if (dx > 0)
		hflip = true;
	if (dx < 0)
		hflip = false;

	super::update();
}
ObjType Spirit::get_type() const { return OBJ_ENEMY_SPIRIT; }



/* Player Object */
Player::Player(float x, float y) : MobileObj(x, y, 16, 8, 0, SheetManager::get_sheet(SH_DEATH)) {
	score = 0;
	aspeed = 0;
	spritenum = 6;

	for (int i = 0; i < spritenum; i++) {
		sprites[i] = (*sheet)[i];
		sprites[i]->sprite_center_origin(ORIGIN_CENTER_BOTTOM);
	}
	sprite = sprites[0];
	persistent = true;

	spr_shadow = (*SheetManager::get_sheet(SH_SHADOW))[0];
	spr_shadow->sprite_center_origin(ORIGIN_CENTER_MIDDLE);

	combatant = new Combatant("Death", sprites[SPR_STAND]->get_bitmap(DIR_N));
	combatant->set_parent(this);
	combatant->add_action(ACT_RUN);
	combatant->add_action(ACT_ATT);
	combatant->set_speed(2);
}
ObjType Player::get_type() const { return OBJ_PLAYER; }
Player::~Player() {
	if (world) {
		world->set_player(NULL);
		world->set_view_focus(NULL);
	}

	if (combatant) {
		delete combatant;
		combatant = NULL;
	}
}

bool Player::is_sneaking() const { return sneaking; }

void Player::update() {
	/* interact with x */
	if (key_press[ALLEGRO_KEY_X] && world) {
		Box bbox = get_bbox();
		switch(direction) {
			case DIR_S:
			case DIR_SE:
			case DIR_SW:
				world->interact_with_object(
						bbox+Vec2f(0.0f,bbox.get_h()));
				key_press[ALLEGRO_KEY_X] = false;
				break;
			case DIR_E:
				world->interact_with_object(
						bbox+Vec2f(bbox.get_w(),0.0f));
				key_press[ALLEGRO_KEY_X] = false;
				break;
			case DIR_NW:
			case DIR_NE:
			case DIR_N:
				world->interact_with_object(
						bbox+Vec2f(0.0f,-bbox.get_h()));
				key_press[ALLEGRO_KEY_X] = false;
				break;
			case DIR_W:
				world->interact_with_object(
						bbox+Vec2f(-bbox.get_w(),0.0f));
				key_press[ALLEGRO_KEY_X] = false;
				break;
			default:
				break;
		}
	}

	/* sneak timer */
	if (sneak_cooldown > 0) {
		sneak_cooldown--;
		if (sneak_cooldown <= 0) {
			can_sneak = true;
			sneak_cooldown = 0;
			/* TODO play a sound */
			LOG("cooldown");
		}
	}
	if (sneaking && (!kmap(ALLEGRO_KEY_Z) || sneak_time++ >= sneak_time_max)) {
		sneaking = false;
		sneak_time = 0;
		LOG("unsneak");
		sneak_cooldown = 60*2;
	}

	/* activate sneak with z */
	if (key_press[ALLEGRO_KEY_Z]) {
		if (can_sneak) {
			sneaking = true;
			can_sneak = false;
			/* TODO create mist effect */
			/* TODO play a sneaking sound */
			LOG("sneaky");
		}
	}

	/* vertical control */
	if (kmap(ALLEGRO_KEY_UP)) {
		dy = -maxspeed;
	} else if (kmap(ALLEGRO_KEY_DOWN)) {
		dy = maxspeed;
	} else
		dy = 0;

	if (kmap(ALLEGRO_KEY_UP) && kmap(ALLEGRO_KEY_DOWN))
		dy = 0;

	/* horizontal control */
	if (kmap(ALLEGRO_KEY_LEFT)) {
		dx = -maxspeed;
	} else if (kmap(ALLEGRO_KEY_RIGHT)) {
		dx = maxspeed;
	} else
		dx = 0;

	/* Not moving */
	if (kmap(ALLEGRO_KEY_LEFT) && kmap(ALLEGRO_KEY_RIGHT))
		dx = 0;

	/* which way ya facing */
	if (dy > 0 && dx == 0)
		direction = DIR_S;
	if (dy > 0 && dx > 0)
		direction = DIR_SE;
	if (dy > 0 && dx < 0)
		direction = DIR_SW;
	if (dy < 0 && dx == 0)
		direction = DIR_N;
	if (dy < 0 && dx < 0)
		direction = DIR_NW;
	if (dy < 0 && dx > 0)
		direction = DIR_NE;
	if (dy == 0 && dx > 0)
		direction = DIR_E;
	if (dy == 0 && dx < 0)
		direction = DIR_W;

	Sprite *temp = sprite;
	switch (direction) {
		case DIR_S:
			sprite = sprites[SPR_WALK_DOWN];
			hflip = false;
			break;
		case DIR_SE:
			sprite = sprites[SPR_WALK_DOWN_RIGHT];
			hflip = false;
			break;
		case DIR_E:
			sprite = sprites[SPR_WALK_RIGHT];
			hflip = false;
			break;
		case DIR_NE:
			sprite = sprites[SPR_WALK_UP_RIGHT];
			hflip = false;
			break;
		case DIR_N:
			sprite = sprites[SPR_WALK_UP];
			hflip = false;
			break;
		case DIR_SW:
			sprite = sprites[SPR_WALK_DOWN_RIGHT];
			hflip = true;
			break;
		case DIR_W:
			sprite = sprites[SPR_WALK_RIGHT];
			hflip = true;
			break;
		case DIR_NW:
			sprite = sprites[SPR_WALK_UP_RIGHT];
			hflip = true;
			break;
	}
	/* always start a walkcycle from the same place */
	/* (trust me it looks nicer) */
	if (sprite != temp){
		frame_index = 2;
	}

	/* stand tall and proud if not moving */
	if (dy ==0 && dx == 0) {
		sprite = sprites[SPR_STAND];
		aspeed = 0;
		hflip = false;
	}
	else
		aspeed = 6.0f/60.0f;

	/* Tile collision handling */
	collide_with_tiles();

	/* update position based on speed */
	super::update();
}

void Player::map_start(){
	world->set_view_focus(this);
	world->set_player(this);
	super::map_start();
}



void Player::draw() {
#if DEBUG_DRAW
	/*	for (auto &b : world->get_map()->get_collision_box(get_bbox()+Vec2f(dx,dy))) */
	/*		b.draw(al_map_rgb(128,0,0)); */
#endif

	if (sprite) {
		if (aspeed == 0)
			frame_index = direction;

		if (sneaking) {
			float shad_alpha = 0.5;
			spr_shadow->sprite_draw(x,y+1,0.0f, 0, al_map_rgba_f(1.0f * shad_alpha, 1.0f * shad_alpha, 1.0f * shad_alpha, shad_alpha));

			int flags = 0;
			if (hflip)
				flags = flags | ALLEGRO_FLIP_HORIZONTAL;
			if (vflip)
				flags = flags | ALLEGRO_FLIP_VERTICAL;
/*			al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);  */
			float a = 0.1f; 
			sprite->sprite_draw(x,y,frame_index, flags, al_map_rgba_f(a,a,a,a)); 
			sprite->sprite_draw(x+1,y,frame_index, flags, al_map_rgba_f(a,a,a,a)); 
			sprite->sprite_draw(x-1,y,frame_index, flags, al_map_rgba_f(a,a,a,a)); 
/*			al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);  */
			if (world && world->get_mode() == World::MODE_OVERWORLD)
				frame_index += aspeed;
			if (frame_index >= sprite->getframes() && !loop) {
				aspeed = 0;
				frame_index = sprite->getframes();
			}
		} else {
			spr_shadow->sprite_draw(x,y+1,0.0f);
			super::draw(); 
		}
	}
}

Box Player::get_bbox() const { return Box(x-w/2, y-h+2, w, h); }

Combatant *Player::get_combatant() const { return this->combatant; }

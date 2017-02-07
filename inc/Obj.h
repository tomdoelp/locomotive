#ifndef __OBJ_H__
#define __OBJ_H__

#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "../inc/global.h"

/* debug purposes? */
#include <typeinfo>

/* Basic object. Holds the total number of instances, an id, and can update (nop) */
class Obj {
	public:
		Obj();
		virtual void update();
	protected:
		int id;
		static int objtotal;
};


/* Physical Object. Has a location, a width, and a height */
class PhysicalObj : public Obj {
	public:
		PhysicalObj(float x=0.0, float y=0.0, float w=0, float h=0);
		~PhysicalObj();
	protected:
		typedef Obj super;
		float x;
		float y;
		float w;
		float h;
};

/* Solid object. Player can't move through these? */
class SolidObj : public virtual PhysicalObj {
	public:
		SolidObj(float x=0.0, float y=0.0, float w=0, float h=0);
		~SolidObj();
	protected:
		typedef PhysicalObj super;
};


/* Visible object. Has a position, size, and depth. Can draw itself. */
/* Compared by depth, so they can be drawn in order. */
class VisibleObj : public virtual PhysicalObj {
	public:
		VisibleObj(float x=0.0, float y=0.0, float w=0, float h=0, int depth=0);
		~VisibleObj();
		virtual void draw();
		bool operator<(const VisibleObj &r);
		int getDepth();
	protected:
		typedef PhysicalObj super;
		int depth;
		bool visible;
};


/* Wall. A solid, visible object. Player can't move through these. */
class Wall : public SolidObj, public VisibleObj {
	public:
		Wall(float x=0.0, float y=0.0, float w=0, float h=0, int depth=0);
		~Wall();
		virtual void draw();
	protected:
		typedef PhysicalObj super;
};


/* Mobile object. Has horizontal and vertical speed and moves at those speeds when updated. */
class MobileObj : public VisibleObj {
	public:
		MobileObj();
		MobileObj(float x=0.0, float y=0.0, float w=0, float h=0, int depth=0);
		~MobileObj();
		virtual void update();
	protected:
		typedef VisibleObj super;
		float hspeed;
		float vspeed;
};


/* Player object. Has a score. Horizontal and vertical motion controlled with arrow keys. */
class Player : public MobileObj {
	public:
		Player();
		Player(float x=0.0, float y=0.0, float w=0, float h=0, int depth=0);
		~Player();
		virtual void update();
		virtual void draw();
	protected:
		typedef MobileObj super;
		int score;

		bool bb_collision(float x, float y, float w, float h);
};

#endif

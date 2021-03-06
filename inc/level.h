#ifndef LEVEL_H
#define LEVEL_H

#include <stdlib.h>
#include <allegro5/allegro_image.h>
#include <pugixml.hpp>
#include <physfs.h>

#include "global.h"
#include "load.h"
#include "utils.h"

typedef enum layertype {
	LAYER_BACKGROUND, 
	LAYER_MIXED, 
	LAYER_FOREGROUND, 
	LAYER_COLLISIONS
} LayerType;

typedef enum solidtype {
	TILE_FREE, 
	TILE_SOLID_SQUARE, 
	TILE_SOLID_HALF_TOP, 
	TILE_SOLID_HALF_BOTTOM,
	TILE_SOLID_HALF_RIGHT,
	TILE_SOLID_HALF_LEFT,
	TILE_SOLID_DIAG_UPRIGHT,
	TILE_SOLID_DIAG_UPLEFT,
	TILE_SOLID_TRI_TOPLEFT,
	TILE_SOLID_TRI_TOPRIGHT,
	TILE_SOLID_TRI_BOTLEFT,
	TILE_SOLID_TRI_BOTRIGHT
} SolidType;

class World;
class Layer {
	public: 
		Layer(const char *name, int w, int h, std::vector<int> gids);
		Layer(pugi::xml_node &l);
		std::vector<int> get_gids();
	private:
		const char* name;
		int w, h;
		std::vector<int> gids;
};

class Map {
	public:
		Map();
		Map(World *world, const char *fname);
		~Map();

		int tilew, tileh;

		void draw_layer(float x, float y, int n);
		void draw_row(float x, float y, int r, int l);
		void draw_layer_from_row(float x, float y, int r, int l);
		int numlayers();
		std::vector<Box_Diag> get_collision_box(const Box &bbox);
		Vec2f get_collision_vec(const Box &now, const Box &next);

		void draw_layer_region(float x, float y, int n, Box b);
		void draw_row_region(float x, float y, int r, int l, Box b);
		void draw_layer_region_from_row(float x, float y, int r, int l, Box b);
	private:
		int w, h;
		int nextobj;
		World *world;

		std::vector<Layer> layers;
		std::vector<ALLEGRO_BITMAP*> tilesets;
		std::vector<ALLEGRO_BITMAP*> tiles;
		std::vector<SolidType> tiletype;
};

#endif

// World class
// contains the world and its physical rules (collisions, gravity etc)
// basically its a level of the game, other levels can be different child of world 
// class and have e.g. negative gravity, or different set of monsters, or no light, etc.
#include "world.h"
#include "player.h"
#include "creature.h"

#include <fstream>

#include <hgesprite.h>


World::World( Player * plr, const std::string & filename )
	: m_player(plr), m_world_width(0), m_world_height(0)
	, m_camera_pos(0, 0), m_pause_flag(false)
{
	LoadWorld( filename );

	// this will not create another HGE, instead we will get the global
	// unique HGE object which is already started
	m_hge = hgeCreate( HGE_VERSION );

	m_sprite_brick1 = m_sprite_manager.GetSprite("textures/brick1.png");
	m_sprite_sky = m_sprite_manager.GetSprite("textures/sky1.png");
}


World::~World()
{
	delete m_sprite_brick1;
	delete m_sprite_sky;

	// clear world contents 
	for( object_list_t::iterator i = m_objects.begin(); i != m_objects.end(); ++i ) {
		delete (*i);
	}
	m_objects.clear();

	m_hge->Release();
}


void World::LoadWorld( const std::string & filename )
{
	// Since we need to know exact world size to resize the m_world_cells array 
	// first we read the level file to determine the max length of all lines
	// second we read it again and put values in the m_world_cells
	std::ifstream f;
	f.open( filename );
	
	uint32_t	row = 0;
	size_t		max_line_width = 0;
	char		line_buf[MAX_WORLD_WIDTH];

	while( row < VISIBLE_ROWS || f.eof() || f.fail() )
	{
		f.getline( line_buf, sizeof line_buf );
		size_t line_width = strlen( line_buf );
		if( line_width > max_line_width ) {
			max_line_width = line_width;
		}
		row++;
	}
	f.close();

	// this can be actually more than 9 if your game can also scroll vertically
	// but for now we scroll horizontally only
	m_world_height = VISIBLE_ROWS;
	m_world_width = max_line_width;
	
	// resize the world
	m_world_cells.resize( m_world_height * m_world_width );

	// fill the world with empties (ASCII space, value 32)
	std::fill( m_world_cells.begin(), m_world_cells.end(), WORLD_CELL_EMPTY );

	// Now its time to properly load the world
	row = 0;
	f.open( filename );
	while( row < m_world_height || f.eof() || f.fail() )
	{
		f.getline( line_buf, sizeof line_buf );
		
		for( int col = 0; line_buf[col]; ++col ) {
			char contents = line_buf[col];
			if( contents == WORLD_CELL_MONEY ) {
				WorldObject * o = new WorldObject_Money( this, col * CELL_BOX_SIZE, row * CELL_BOX_SIZE );
				m_objects.push_back( o );
			}
			else {
				this->At( row, col ) = contents;
			}
		}

		row++;
	}
	f.close();
}


void World::Think()
{
	if( m_pause_flag ) return;

	float d = m_hge->Timer_GetDelta();
	m_camera_pos.x += d * 16;

	// test if player was pushed out of screen fully (to be pushed out partially is allowed)
	if (m_player->GetPos().x < CELL_BOX_SIZE) {
		m_player->Die();
		m_pause_flag = true;
	}
}


World::CellType & World::At( uint32_t row, uint32_t col )
{
	_ASSERTE( row >= 0 && row < m_world_height );
	_ASSERTE( col >= 0 && col < m_world_width );
	return m_world_cells[ row * m_world_width + col ];
}


void World::Render()
{
	// draw the sky, clearing is not needed anymore
	//m_hge->Gfx_Clear( ARGB(255, 80, 160, 190 ) );
	m_sprite_sky->Render( 0, 0 );

	// Assume world does not scroll vertically so we render 9 visible rows wholly, 
	// and carefully calculate visible columns to allow them to seamlessly slide as
	// the camera moves

	// calculate leftmost visible world column
	const uint32_t vis_column = (int)m_camera_pos.x / CELL_BOX_SIZE;

	// render one extra column incase if leftmost is partially visible, to avoid gaps
	// on the right side
	const uint32_t right_end_column = vis_column + VISIBLE_COLS + 1;
	

	// Now draw the visible window into the world
	for( uint32_t r = 0; r < VISIBLE_ROWS; ++r )
	{
		for( uint32_t c = vis_column; c <= right_end_column; ++c )
		{
			CellType cell_contents = this->At( r, c );
			switch( cell_contents )
			{
			case WORLD_CELL_SOLID:
				// find position in world and render it
				m_sprite_brick1->Render(
									c * CELL_BOX_SIZE - m_camera_pos.x,
									r * CELL_BOX_SIZE - m_camera_pos.y
									);
				break;
			} // end switch
		} // end for columns
	} // end for rows

	// Now draw creatures and other stuff (no clipping or other optimizations
	// here we attempt to draw everything)
	for( object_list_t::iterator i = m_objects.begin(); i != m_objects.end(); ++i )
	{
		(*i)->Render();
	}
}

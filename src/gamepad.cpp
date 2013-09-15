#include <nemu/gamepad.h>

#include <SDL/SDL.h>

#include <iostream>

// =====================================================================================================================
GamePad::GamePad()
    : m_data(new uint8_t[24]),
      m_position(0),
      m_resetInProgress(false)
{
    // reset all key button states
    memset(m_data, 0, 24);
}

// =====================================================================================================================
GamePad::~GamePad()
{
    delete[] m_data;
}

// =====================================================================================================================
void GamePad::pollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
	//std::cout << "event=" << (unsigned)event.type << " symbol=" << (unsigned)event.key.keysym.sym << std::endl;
	switch (event.type)
	{
	    case SDL_KEYDOWN :
	    case SDL_KEYUP :
	    {
		unsigned keyState = event.type == SDL_KEYDOWN ? 1 : 0;

		switch (event.key.keysym.sym)
		{
		    case SDLK_UP : m_data[4] = keyState; break;
		    case SDLK_DOWN : m_data[5] = keyState; break;
		    case SDLK_LEFT : m_data[6] = keyState; break;
		    case SDLK_RIGHT : m_data[7] = keyState; break;
		    case SDLK_RETURN : m_data[3] = keyState; break;
		    case SDLK_LCTRL : m_data[0] = keyState; break;
		    case SDLK_SPACE : m_data[1] = keyState; break;
		    case SDLK_s : m_data[2] = keyState; break;
		    default : break;
		}

		break;
	    }
	}

	//if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
	//m_running = false;
    }
}

// =====================================================================================================================
uint8_t GamePad::read(uint16_t address)
{
    uint8_t data;

    switch (address)
    {
	case 0 :
	    // $4016
	    data = m_data[m_position];
	    m_position = (m_position + 1) % 24;
	    //std::cout << "Gamepad data: " << (unsigned)data << std::endl;
	    break;

	default :
	    data = 0;
	    break;
    }

    return data;
}

// =====================================================================================================================
void GamePad::write(uint16_t address, uint8_t data)
{
    switch (address)
    {
	case 0 :
	    // $4016
	    if (data == 1)
		m_resetInProgress = true;
	    else if (data == 0 && m_resetInProgress)
	    {
		m_resetInProgress = false;
		m_position = 0;
		//std::cout << "strobe done" << std::endl;
	    }

	    break;
    }
}

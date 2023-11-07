#pragma once

#include <stdint.h>

class Input
{
public:
	Input();
	~Input();
	
	void Update();
	
	int16_t MouseX() const;
	int16_t MouseY() const;
	bool MouseButtonDown(const uint8_t button) const;
	bool MouseButtonClick(const uint8_t button) const;
	bool MouseMoved() const;
	
	bool GamepadButtonDown(const uint8_t gamepad, const uint8_t button) const;
	bool GamepadButtonPress(const uint8_t gamepad, const uint8_t button) const;
	bool GamepadActivity(const uint8_t gamepad) const;

	void ResetLastInput();
	
private:
	int16_t m_lastmousex;
	int16_t m_lastmousey;
	uint8_t m_lastmousebuttons;
	uint8_t m_lastgamepad[4];
	int16_t m_mousex;
	int16_t m_mousey;
	uint8_t m_mousebuttons;
	uint8_t m_gamepad[4];
	
};

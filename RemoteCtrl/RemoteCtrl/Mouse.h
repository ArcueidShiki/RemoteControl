#pragma once
#include "framework.h"

#define MOUSE_MOVE 0x00
#define MOUSE_LEFT 0x01
#define MOUSE_RIGHT 0x02
#define MOUSE_MIDDLE 0x04
#define MOUSE_CLICK 0x08
#define MOUSE_DBCLICK 0x10
#define MOUSE_DOWN 0x20
#define MOUSE_UP 0x40

typedef struct MouseEvent {
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		point.x = 0;
		point.y = 0;
	}
	MouseEvent(WORD button, WORD action, POINT pt)
	{
		nAction = action;
		nButton = button;
		point = pt;
	}
	WORD nAction; // move, click, double click
	WORD nButton; // left, middle, right
	POINT point;
} MOUSEEV, * PMOUSEEV;
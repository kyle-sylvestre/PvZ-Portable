/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 * 
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include <switch.h>
#include <unordered_map>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

static PadState pad;

// Everything the controller does in-game or in the menus is handled by the app's
// SexyAppBase::HandleEvent override (see CircleShootApp), which owns A, B, Plus
// and the d-pad. Only Minus is left to the framework, as a universal "back" that
// nothing on the controller path claims.
static std::unordered_map<u64, KeyCode> keyMaps = {
	{HidNpadButton_Minus, KEYCODE_ESCAPE},
};

// SDL's Switch driver maps the game controller positionally (Xbox layout), so its
// "A" is the physical bottom button -- Nintendo's B -- and its "B" is the physical
// right button. The app treats SDL_CONTROLLER_BUTTON_A as fire/confirm, so swap the
// pair on the way in to put confirm on the Switch's own A button, as players expect.
// Set this to 0 to use SDL's positional layout instead.
#define SWITCH_SWAP_AB_TO_LABELS 1

static void RelabelControllerButton(SDL_Event* theEvent)
{
#if SWITCH_SWAP_AB_TO_LABELS
	if (theEvent->type == SDL_CONTROLLERBUTTONDOWN || theEvent->type == SDL_CONTROLLERBUTTONUP)
	{
		if (theEvent->cbutton.button == SDL_CONTROLLER_BUTTON_A)
			theEvent->cbutton.button = SDL_CONTROLLER_BUTTON_B;
		else if (theEvent->cbutton.button == SDL_CONTROLLER_BUTTON_B)
			theEvent->cbutton.button = SDL_CONTROLLER_BUTTON_A;
	}
#endif
}

void SexyAppBase::InitInput()
{
	// Configure our supported input layout: a single player with standard controller styles
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);

	// Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
	padInitializeDefault(&pad);

	hidInitializeTouchScreen();

	// SDL_INIT_GAMECONTROLLER is requested in the SexyAppBase constructor, which has
	// already run by this point. Report what it found so a missing pad is diagnosable
	// from the on-device log rather than looking like dead input.
	SDL_Log("InitInput: %d joystick(s) attached", SDL_NumJoysticks());

	if (!mMouseIn)
		mMouseIn = true;
}

bool SexyAppBase::StartTextInput(std::string& theInput)
{
	char buf[512];

	SwkbdConfig kbd;
	swkbdCreate(&kbd, 0);
	swkbdConfigMakePresetDefault(&kbd);
	swkbdConfigSetType(&kbd, SwkbdType_Normal);

	swkbdConfigSetGuideText(&kbd, "Enter text...");
	swkbdConfigSetOkButtonText(&kbd, "OK");

	Result rc = swkbdShow(&kbd, buf, sizeof(buf));
	swkbdClose(&kbd);

	if (R_SUCCEEDED(rc))
	{
		theInput = buf;
		return true;
	}

	return false;
}

void SexyAppBase::StopTextInput()
{
	
}

bool SexyAppBase::ProcessDeferredMessages(bool singleMessage)
{
	if (!appletMainLoop())
	{
		mShutdown = true;
		return false;
	}

	static s32 prev_touchcount = 0;

	// Scan the gamepad. This should be done once for each frame
	padUpdate(&pad);

	u64 kDown = padGetButtonsDown(&pad);
	u64 kUp = padGetButtonsUp(&pad);

	if (kDown)
	{
		mLastUserInputTick = mLastTimerTime;

		for (auto& k : keyMaps)
		{
			if (kDown & k.first)
				mWidgetManager->KeyDown(k.second);
		}
	}

	if (kUp)
	{
		mLastUserInputTick = mLastTimerTime;

		for (auto& k : keyMaps)
		{
			if (kUp & k.first)
				mWidgetManager->KeyUp(k.second);
		}
	}

	// The Switch build drives its window through EGL rather than SDL, so nothing else
	// pumps the SDL event queue. Drain it here: it carries the gamepad events that the
	// app's HandleEvent override navigates with, and the synthetic mouse events that
	// same code pushes back to move and click its virtual cursor.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		RelabelControllerButton(&event);
		HandleEvent(&event);

		switch (event.type)
		{
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERAXISMOTION:
				mLastUserInputTick = mLastTimerTime;
				break;

			case SDL_MOUSEMOTION:
			{
				int x = event.motion.x;
				int y = event.motion.y;
				mWidgetManager->RemapMouse(x, y);

				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->MouseMove(x, y);
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int x = event.button.x;
				int y = event.button.y;
				mWidgetManager->RemapMouse(x, y);

				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->MouseMove(x, y);

				// These are pushed by the app with button left zeroed, so treat
				// anything that is not an explicit right-click as a left click.
				int btn = (event.button.button == SDL_BUTTON_RIGHT) ? -1 : 1;
				if (event.type == SDL_MOUSEBUTTONDOWN)
					mWidgetManager->MouseDown(x, y, btn);
				else
					mWidgetManager->MouseUp(x, y, btn);
				break;
			}
		}
	}

	HidTouchScreenState state = {0};
	hidGetTouchScreenStates(&state, 1);
	static int x=0, y=0;
	if (state.count)
	{
		mLastUserInputTick = mLastTimerTime;

		x = (int)state.touches[0].x;
		y = (int)state.touches[0].y;
		mWidgetManager->RemapMouse(x, y);
		mWidgetManager->MouseMove(x, y);
	}

	if (state.count && !prev_touchcount)
		mWidgetManager->MouseDown(x, y, 1);
	else if (!state.count && prev_touchcount)
		mWidgetManager->MouseUp(x, y, 1);

	prev_touchcount = state.count;

	return false;
}

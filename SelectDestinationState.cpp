/*
 * Copyright 2010 Daniel Albano
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "SelectDestinationState.h"
#include "Game.h"
#include "ResourcePack.h"
#include "Language.h"
#include "LangString.h"
#include "Font.h"
#include "Palette.h"
#include "Surface.h"
#include "Window.h"
#include "Globe.h"
#include "Text.h"
#include "TextButton.h"
#include "Timer.h"
#include "SavedGame.h"
#include "Craft.h"
#include "Ufo.h"
#include "ConfirmDestinationState.h"

#define CLICK_RADIUS 25

/**
 * Initializes all the elements in the Select Destination window.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft to target.
 * @param globe Pointer to the Geoscape globe.
 */
SelectDestinationState::SelectDestinationState(Game *game, Craft *craft, Globe *globe) : State(game), _craft(craft), _globe(globe)
{
	_screen = false;

	// Create objects
	_btnRotateLeft = new InteractiveSurface(12, 10, 259, 176);
	_btnRotateRight = new InteractiveSurface(12, 10, 283, 176);
	_btnRotateUp = new InteractiveSurface(13, 12, 271, 162);
	_btnRotateDown = new InteractiveSurface(13, 12, 271, 187);
	_btnZoomIn = new InteractiveSurface(23, 23, 295, 156);
	_btnZoomOut = new InteractiveSurface(13, 17, 300, 182);

	_window = new Window(this, 256, 28, 0, 0);
	_btnCancel = new TextButton(game->getResourcePack()->getFont("BIGLETS.DAT"), game->getResourcePack()->getFont("SMALLSET.DAT"), 60, 12, 110, 8);
	_txtTitle = new Text(game->getResourcePack()->getFont("BIGLETS.DAT"), game->getResourcePack()->getFont("SMALLSET.DAT"), 100, 9, 10, 10);

	// Set palette
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)), Palette::backPos, 16);
	
	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut);

	add(_window);
	add(_btnCancel);
	add(_txtTitle);
	
	// Set up objects
	_globe->onMouseClick((EventHandler)&SelectDestinationState::globeClick);

	_btnRotateLeft->onMousePress((EventHandler)&SelectDestinationState::btnRotateLeftPress);
	_btnRotateLeft->onMouseRelease((EventHandler)&SelectDestinationState::btnRotateLeftRelease);

	_btnRotateRight->onMousePress((EventHandler)&SelectDestinationState::btnRotateRightPress);
	_btnRotateRight->onMouseRelease((EventHandler)&SelectDestinationState::btnRotateRightRelease);

	_btnRotateUp->onMousePress((EventHandler)&SelectDestinationState::btnRotateUpPress);
	_btnRotateUp->onMouseRelease((EventHandler)&SelectDestinationState::btnRotateUpRelease);

	_btnRotateDown->onMousePress((EventHandler)&SelectDestinationState::btnRotateDownPress);
	_btnRotateDown->onMouseRelease((EventHandler)&SelectDestinationState::btnRotateDownRelease);

	_btnZoomIn->onMouseClick((EventHandler)&SelectDestinationState::btnZoomInClick);

	_btnZoomOut->onMouseClick((EventHandler)&SelectDestinationState::btnZoomOutClick);

	_window->setColor(Palette::blockOffset(15)+2);
	_window->setBackground(game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnCancel->setColor(Palette::blockOffset(8)+8);
	_btnCancel->setText(_game->getResourcePack()->getLanguage()->getString(STR_CANCEL_UC));
	_btnCancel->onMouseClick((EventHandler)&SelectDestinationState::btnCancelClick);

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setText(_game->getResourcePack()->getLanguage()->getString(STR_SELECT_DESTINATION));
}

/**
 *
 */
SelectDestinationState::~SelectDestinationState()
{
	
}

/**
 * Resets the palette since it's bound to change on other screens.
 */
void SelectDestinationState::init()
{
	// Set palette
	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_0")->getColors());
}

/**
 * Runs the globe rotation timer.
 */
void SelectDestinationState::think()
{
	State::think();
	_globe->think();
}

/**
 * Handles the globe.
 * @param ev Pointer to a SDL_Event.
 * @param scale Current screen scale (used to correct mouse input).
 */
void SelectDestinationState::handle(SDL_Event *ev, int scale)
{
	State::handle(ev, scale);
	_globe->handle(ev, scale, this);
}

/**
 * Processes any left-clicks for picking a target,
 * or right-clicks to scroll the globe.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::globeClick(SDL_Event *ev, int scale)
{
	double lon, lat;
	int mouseX = ev->button.x / scale, mouseY = ev->button.y / scale;
	_globe->cartToPolar(mouseX, mouseY, &lon, &lat);

	if (ev->button.button == SDL_BUTTON_LEFT)
	{
		// Clicking on UFOs
		for (vector<Ufo*>::iterator i = _game->getSavedGame()->getUfos()->begin(); i != _game->getSavedGame()->getUfos()->end(); i++)
		{
			if ((*i)->getDetected())
			{
				Sint16 x, y;
				_globe->polarToCart((*i)->getLongitude(), (*i)->getLatitude(), &x, &y);

				int dx = mouseX - x;
				int dy = mouseY - y;
				if (dx * dx + dy * dy <= CLICK_RADIUS)
				{
					_game->pushState(new ConfirmDestinationState(_game, _craft, (*i)));
				}
			}
		}
	}
	else if (ev->button.button == SDL_BUTTON_RIGHT)
	{
		// Rotating the globe
		_globe->center(lon, lat);
	}
	else if (ev->button.button == SDL_BUTTON_WHEELUP)
	{
		// Zooming in the globe
		_globe->zoomIn();
	}
	else if (ev->button.button == SDL_BUTTON_WHEELDOWN)
	{
		// Zooming out the globe
		_globe->zoomOut();
	}
}

/**
 * Starts rotating the globe to the left.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateLeftPress(SDL_Event *ev, int scale)
{
	_globe->rotateLeft();
}

/**
 * Stops rotating the globe to the left.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateLeftRelease(SDL_Event *ev, int scale)
{
	_globe->rotateStop();
}

/**
 * Starts rotating the globe to the right.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateRightPress(SDL_Event *ev, int scale)
{
	_globe->rotateRight();
}

/**
 * Stops rotating the globe to the right.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateRightRelease(SDL_Event *ev, int scale)
{
	_globe->rotateStop();
}

/**
 * Starts rotating the globe upwards.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateUpPress(SDL_Event *ev, int scale)
{
	_globe->rotateUp();
}

/**
 * Stops rotating the globe upwards.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateUpRelease(SDL_Event *ev, int scale)
{
	_globe->rotateStop();
}

/**
 * Starts rotating the globe downwards.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateDownPress(SDL_Event *ev, int scale)
{
	_globe->rotateDown();
}

/**
 * Stops rotating the globe downwards.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnRotateDownRelease(SDL_Event *ev, int scale)
{
	_globe->rotateStop();
}

/**
 * Zooms into the globe.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnZoomInClick(SDL_Event *ev, int scale)
{
	if (ev->button.button == SDL_BUTTON_LEFT)
		_globe->zoomIn();
	else if (ev->button.button == SDL_BUTTON_RIGHT)
		_globe->zoomMax();
}

/**
 * Zooms out of the globe.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnZoomOutClick(SDL_Event *ev, int scale)
{
	if (ev->button.button == SDL_BUTTON_LEFT)
		_globe->zoomOut();
	else if (ev->button.button == SDL_BUTTON_RIGHT)
		_globe->zoomMin();
}

/**
 * Returns to the previous screen.
 * @param ev Pointer to the SDL_Event.
 * @param scale Scale of the screen.
 */
void SelectDestinationState::btnCancelClick(SDL_Event *ev, int scale)
{
	_game->popState();
}

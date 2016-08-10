//========================================================================//
// Copyright (C) 2016 Evan Bowman                                         //
//                                                                        //
// This program is free software: you can redistribute it and/or modify   //
// it under the terms of the GNU General Public License as published by   //
// the Free Software Foundation, either version 3 of the License, or      //
// (at your option) any later version.                                    //
//                                                                        //
// This program is distributed in the hope that it will be useful,        //
// but WITHOUT ANY WARRANTY; without even the implied warranty of         //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
// GNU General Public License for more details.                           //
//                                                                        //
// You should have received a copy of the GNU General Public License      //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.  //
//========================================================================//

#include "mappingFunctions.hpp"
#include "initMapVectors.hpp"
#include <cmath>
#include "ResourcePath.hpp"
#include "enemyPlacementFn.hpp"
#include "lightingMap.h"
#include "pillarPlacement.h"
#include "math.h"
#include "game.hpp"
#include "easingTemplates.hpp"

std::mutex globalOverworldMutex, globalUIMutex, globalTransitionMutex;

Game::Game(const sf::Vector2f viewPort, InputController * _pInput, FontController * _pFonts)
	: windowW(viewPort.x),
	  windowH(viewPort.y),
	  transitionState(TransitionState::None),
	  pInput(_pInput),
	  sounds(&globalResourceHandler),
	  player(viewPort.x / 2, viewPort.y / 2),
	  camera(&player, viewPort),
	  pFonts(_pFonts),
	  level(0),
	  stashed(false),
	  preload(false),
	  teleporterCond(false),
	  worldView(sf::Vector2f(viewPort.x / 2, viewPort.y / 2), viewPort),
	  timer(0)
{
	target.create(windowW, windowH);
	secondPass.create(windowW, windowH);
	secondPass.setSmooth(true);
	thirdPass.create(windowW, windowH);
	thirdPass.setSmooth(true);
	stash.create(windowW, windowH);
	stash.setSmooth(true);
	lightingMap.create(windowW, windowH);
	vignetteSprite.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::vignette));
	vignetteSprite.setScale(windowW / 450, windowH / 450);
	vignetteShadowSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::vignetteShadow));
	vignetteShadowSpr.setScale(windowW / 450, windowH / 450);
	vignetteShadowSpr.setColor(sf::Color(255,255,255,100));
	hudView.setSize(windowW, windowH);
	hudView.setCenter(windowW / 2, windowH / 2);
	bkg.giveWindowSize(windowW, windowH);
	UI.setView(&worldView);
	tiles.setPosition((windowW / 2) - 16, (windowH / 2));
	tiles.setWindowSize(windowW, windowH);
	// Completely non-general code only used by the intro level (it's not procedurally generated)
	details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (6 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (0 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<2>(tiles.posX - 180 + 16 + (11 * 32), tiles.posY + 200 - 3 + (11 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<2>(tiles.posX - 180 + 16 + (10 * 32), tiles.posY + 204 + 8 + (-9 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<4>(tiles.posX - 192 + 6 * 32, tiles.posY + 301,
					   globalResourceHandler.getTexture(ResourceHandler::Texture::introWall));
	sf::Sprite podSpr;
	podSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	podSpr.setTextureRect(sf::IntRect(164, 145, 44, 50));
	details.add<6>(tiles.posX + 3 * 32, tiles.posY + 4 + 17 * 26, podSpr);;
	tiles.teleporterLocation.x = 8;
	tiles.teleporterLocation.y = -7;
	for (auto it = global_levelZeroWalls.begin(); it != global_levelZeroWalls.end(); ++it) {
		wall w;
		w.setXinit(it->first);
		w.setYinit(it->second);
		tiles.walls.push_back(w);
	}
	en.setWindowSize(windowW, windowH);
	pFonts->setWaypointText(level);
	details.add<0>(tiles.posX - 178 + 8 * 32, tiles.posY + 284 + -7 * 26,
				   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
				   globalResourceHandler.getTexture(ResourceHandler::Texture::teleporterGlow));
	bkg.setBkg(0);
	beamGlowTxr.loadFromFile(resourcePath() + "teleporterBeamGlow.png");
	beamGlowSpr.setTexture(beamGlowTxr);
	beamGlowSpr.setPosition(windowW / 2 - 200, windowH / 2 - 200 + 30);
	beamGlowSpr.setColor(sf::Color(0, 0, 0, 255));
	transitionShape.setSize(sf::Vector2f(windowW, windowH));
	transitionShape.setFillColor(sf::Color(0, 0, 0, 0));
	vignetteSprite.setColor(sf::Color(255, 255, 255));
}

void Game::draw(sf::RenderWindow & window) {
	target.clear(sf::Color::Transparent);
	if (!stashed || preload) {
		std::unique_lock<std::mutex> overworldLock(globalOverworldMutex);
		lightingMap.setView(camera.getView());
		bkg.drawBackground(target, worldView, camera);
		tiles.draw(target, &glowSprs1, level, worldView, camera.getView());
		glowSprs2.clear();
		glowSprs1.clear();
		gameShadows.clear();
		gameObjects.clear();
		target.setView(camera.getView());
		drawGroup(details, gameObjects, gameShadows, glowSprs1, glowSprs2, target);
		if (player.visible) {
			player.draw(gameObjects, gameShadows);
		}
		drawGroup(effectGroup, gameObjects, glowSprs1);
		en.draw(gameObjects, gameShadows, camera);
		sounds.poll();
		overworldLock.unlock();
		if (!gameShadows.empty()) {
			for (auto & element : gameShadows) {
				target.draw(std::get<0>(element));
			}
		}
		target.setView(worldView);
		lightingMap.clear(sf::Color::Transparent);
		// Sort the game objects based on y-position for z-ordering
		std::sort(gameObjects.begin(), gameObjects.end(), [](const std::tuple<sf::Sprite, float, Rendertype, float> & arg1, const std::tuple<sf::Sprite, float, Rendertype, float> & arg2) {
				return (std::get<1>(arg1) < std::get<1>(arg2));
			});
		window.setView(worldView);
		sf::Shader & colorShader = globalResourceHandler.getShader(ResourceHandler::Shader::color);
		for (auto & element : gameObjects) {
			switch (std::get<2>(element)) {
			case Rendertype::shadeDefault:
				std::get<0>(element).setColor(sf::Color(190, 190, 210, std::get<0>(element).getColor().a));
				lightingMap.draw(std::get<0>(element));
				break;
					
			case Rendertype::shadeNone:
				lightingMap.draw(std::get<0>(element));
				break;
					
			case Rendertype::shadeWhite:
				colorShader.setParameter("amount", std::get<3>(element));
				colorShader.setParameter("targetColor", sf::Vector3f(1.00, 1.00, 1.00));
				lightingMap.draw(std::get<0>(element), &colorShader);
				break;
					
			case Rendertype::shadeRed:
				colorShader.setParameter("amount", std::get<3>(element));
				colorShader.setParameter("targetColor", sf::Vector3f(0.98, 0.22, 0.03));
				lightingMap.draw(std::get<0>(element), &colorShader);
				break;
					
			case Rendertype::shadeCrimson:
				colorShader.setParameter("amount", std::get<3>(element));
				colorShader.setParameter("targetColor", sf::Vector3f(0.94, 0.09, 0.34));
				lightingMap.draw(std::get<0>(element), &colorShader);
				break;
					
			case Rendertype::shadeNeon:
				colorShader.setParameter("amount", std::get<3>(element));
				colorShader.setParameter("targetColor", sf::Vector3f(0.29, 0.99, 0.99));
				lightingMap.draw(std::get<0>(element), &colorShader);
				break;
			}
		}
		sf::Color blendAmount(185, 185, 185, 255);
		sf::Sprite tempSprite;
		for (auto element : glowSprs2) {
			element.setColor(blendAmount);
			element.setPosition(element.getPosition().x, element.getPosition().y - 12);
			lightingMap.draw(element, sf::BlendMode(sf::BlendMode(sf::BlendMode::SrcAlpha, sf::BlendMode::One,
																	 sf::BlendMode::Add, sf::BlendMode::DstAlpha,
																	 sf::BlendMode::Zero, sf::BlendMode::Add)));
		}
		lightingMap.display();
		target.draw(sf::Sprite(lightingMap.getTexture()));
		target.setView(camera.getView());
		bkg.drawForeground(target);
		target.setView(worldView);
		target.draw(vignetteSprite, sf::BlendMultiply);
		target.draw(vignetteShadowSpr);
		target.display();
	}
	if (UI.blurEnabled() && UI.desaturateEnabled()) {
		if (stashed) {
			window.setView(worldView);
			window.draw(sf::Sprite(stash.getTexture()));
		} else {
			sf::Shader & blurShader = globalResourceHandler.getShader(ResourceHandler::Shader::blur);
			sf::Shader & desaturateShader = globalResourceHandler.getShader(ResourceHandler::Shader::desaturate);
			secondPass.clear(sf::Color::Transparent);
			thirdPass.clear(sf::Color::Transparent);
			sf::Vector2u textureSize = target.getSize();
			// Get the blur amount from the UI controller
			float blurAmount = UI.getBlurAmount();
			blurShader.setParameter("blur_radius", sf::Vector2f(0.f, blurAmount / textureSize.y));
			secondPass.draw(sf::Sprite(target.getTexture()), &blurShader);
			secondPass.display();
			blurShader.setParameter("blur_radius", sf::Vector2f(blurAmount / textureSize.x, 0.f));
			thirdPass.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
			thirdPass.display();
			desaturateShader.setParameter("amount", UI.getDesaturateAmount());
			window.draw(sf::Sprite(thirdPass.getTexture()), &desaturateShader);
			if (!stashed && (UI.getState() == UserInterface::State::statsScreen
							 || UI.getState() == UserInterface::State::menuScreen)) {
				stash.clear(sf::Color::Black);
				stash.draw(sf::Sprite(thirdPass.getTexture()), &desaturateShader);
				stash.display();
				stashed = true;
			}
		}
	} else if (UI.blurEnabled() && !UI.desaturateEnabled()) {
		if (stashed) {
			if (pInput->pausePressed()) {
				preload = true;
			}
			window.setView(worldView);
			window.draw(sf::Sprite(stash.getTexture()));
		} else {
			sf::Shader & blurShader = globalResourceHandler.getShader(ResourceHandler::Shader::blur);
			secondPass.clear(sf::Color::Transparent);
			sf::Vector2u textureSize = target.getSize();
			// Get the blur amount from the UI controller
			float blurAmount = UI.getBlurAmount();
			blurShader.setParameter("blur_radius", sf::Vector2f(0.f, blurAmount / textureSize.y));
			secondPass.draw(sf::Sprite(target.getTexture()), &blurShader);
			secondPass.display();
			blurShader.setParameter("blur_radius", sf::Vector2f(blurAmount / textureSize.x, 0.f));
			window.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
			if (!stashed && (UI.getState() == UserInterface::State::statsScreen
							 || UI.getState() == UserInterface::State::menuScreen)) {
				stash.clear(sf::Color::Black);
				stash.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
				stash.display();
				stashed = true;
				preload = false;
			}
		}
	} else if (!UI.blurEnabled() && UI.getDesaturateAmount()) {
		sf::Shader & desaturateShader = globalResourceHandler.getShader(ResourceHandler::Shader::desaturate);
		desaturateShader.setParameter("amount", UI.getDesaturateAmount());
		window.draw(sf::Sprite(target.getTexture()), &desaturateShader);
	} else {
		window.draw(sf::Sprite(target.getTexture()));
	}
	std::unique_lock<std::mutex> UILock(globalUIMutex);
	if (player.getState() == Player::State::dead) {
		UI.draw(window, *pFonts);
	} else {
		if (transitionState == TransitionState::None) {
			UI.draw(window, *pFonts);
		}
		pFonts->print(window);
	}
	UILock.unlock();
	window.setView(worldView);
	globalUIMutex.unlock();
	drawTransitions(window);
}

void Game::update(sf::Time & elapsedTime) {
	float xOffset = 0;
	float yOffset = 0;
	// Blurring is graphics intensive, the game caches frames in a RenderTexture when possible
	if (stashed && UI.getState() != UserInterface::State::statsScreen && UI.getState() != UserInterface::State::menuScreen) {
		stashed = false;
	}
	if (!stashed || preload) {
		std::lock_guard<std::mutex> overworldLock(globalOverworldMutex);
		if (level != 0) {
			const sf::Vector2f & cameraOffset = camera.getOffset();
			bkg.setOffset(cameraOffset.x, cameraOffset.y);
		} else { // TODO: why is this necessary...?
			bkg.setOffset(0, 0);
		}
		tiles.update(xOffset, yOffset);
		details.apply([&](auto & vec) {
			for (auto it = vec.begin(); it != vec.end();) {
				if (it->getKillFlag()) {
					it = vec.erase(it);
				} else {
					it->update(elapsedTime);
					++it;
				}
			}
		});
		std::vector<sf::Vector2f> cameraTargets;
		en.update(player.getXpos(), player.getYpos(), effectGroup, tiles.walls, !UI.isOpen(), &tiles, elapsedTime, camera, cameraTargets);
		camera.update(elapsedTime, cameraTargets);
		if (player.visible) {
			player.update(this, elapsedTime, sounds);
		}
		if (!UI.isOpen() || (UI.isOpen() && player.getState() == Player::State::dead)) {
			effectGroup.apply([&](auto & vec) {
				for (auto it = vec.begin(); it != vec.end();) {
					if (it->getKillFlag()) {
						it = vec.erase(it);
					} else {
						it->update(elapsedTime);
						++it;
					}
				}
			});
		}
	}
	std::unique_lock<std::mutex> UILock(globalUIMutex);
    if (player.getState() == Player::State::dead) {
		UI.dispDeathSeq();
		// If the death sequence is complete and the UI controller is finished playing its animation
		if (UI.isComplete()) {
			// Reset the UI controller
			UI.reset();
			// Reset the player
			player.reset();
			// Reset the map. Reset() increments level, set to -1 so that it will be zero
			level = -1;
			Reset();
			pFonts->reset();
			pFonts->updateMaxHealth(4, 4);
			pFonts->setWaypointText(level);
		}
		UI.update(player, *pFonts, pInput, elapsedTime);
	} else {
		if (transitionState == TransitionState::None) {
			UI.update(player, *pFonts, pInput, elapsedTime);
		}
		pFonts->updateHealth(player.getHealth());
	}
	UILock.unlock();
	updateTransitions(elapsedTime);
}

void Game::drawTransitions(sf::RenderWindow & window) {
	std::lock_guard<std::mutex> grd(globalTransitionMutex);
	switch (transitionState) {
	case TransitionState::None:
    	break;

	case TransitionState::ExitBeamEnter:
    	window.draw(beamShape);
		break;

	case TransitionState::ExitBeamInflate:
    	window.draw(beamShape);
		break;

	case TransitionState::ExitBeamDeflate:
    	window.draw(beamShape);
		break;

	case TransitionState::TransitionOut:
		if (level != 0) {
			if (timer > 100) {
				window.draw(transitionShape);
			}
		} else {
			if (timer > 1600) {
				window.draw(transitionShape);
				uint8_t alpha = Easing::easeOut<1>(timer - 1600, static_cast<int_fast32_t>(600)) * 255;
				pFonts->drawTitle(alpha, window);
			} else {
				pFonts->drawTitle(255, window);
			}
		}
		break;

	case TransitionState::TransitionIn:
    	window.draw(transitionShape);
		break;

	case TransitionState::EntryBeamDrop:
    	window.draw(beamShape);
		break;

	case TransitionState::EntryBeamFade:
    	window.draw(beamShape);
		break;
	}
}

void Game::updateTransitions(const sf::Time & elapsedTime) {
	std::lock_guard<std::mutex> grd(globalTransitionMutex);
	switch (transitionState) {
	case TransitionState::None:
		{
			// If the player is near the teleporter, snap to its center and deactivate the player
			float teleporterX = details.get<0>().back().getPosition().x;
			float teleporterY = details.get<0>().back().getPosition().y;
			if ((std::abs(player.getXpos() - teleporterX) < 8 && std::abs(player.getYpos() - teleporterY + 12) < 8)) {
				// Todo: snap player to the teleporter
				player.setPosition(teleporterX - 2.f, teleporterY - 16.f);
				player.setState(Player::State::deactivated);
				const sf::Vector2f playerPos = player.getPosition();
				const sf::Vector2f & cameraPos = camera.getView().getCenter();
				if (std::abs(playerPos.x - cameraPos.x) < 1.f && std::abs(playerPos.y - cameraPos.y) < 1.f) {
					transitionState = TransitionState::ExitBeamEnter;
				}
			}
			beamShape.setFillColor(sf::Color(114, 255, 229, 6));
			beamShape.setPosition(windowW / 2 - 1.5, windowH / 2 + 36);
			beamShape.setSize(sf::Vector2f(2, 0));
		}
		break;

	case TransitionState::ExitBeamEnter:
		timer += elapsedTime.asMilliseconds();
		{
			// About the static casts--macOS and Linux use different underlying types for int_fast32_t
			float beamHeight = Easing::easeIn<1>(timer, static_cast<int_fast32_t>(500)) * -(windowH / 2 + 36);
			beamShape.setSize(sf::Vector2f(2, beamHeight));
			uint_fast8_t alpha = Easing::easeIn<1>(timer, static_cast<int_fast32_t>(450)) * 255;
			beamShape.setFillColor(sf::Color(114, 255, 229, alpha));
		}
		if (timer > 500) {
			timer = 0;
			beamShape.setSize(sf::Vector2f(2, -(windowH / 2 + 36)));
			beamShape.setFillColor(sf::Color(114, 255, 229, 255));
			transitionState = TransitionState::ExitBeamInflate;
		}
		break;

	case TransitionState::ExitBeamInflate:
		timer += elapsedTime.asMilliseconds();
		{
			float beamWidth = std::max(2.f, Easing::easeIn<2>(timer, static_cast<int_fast32_t>(250)) * 20.f);
			float beamHeight = beamShape.getSize().y;
			beamShape.setSize(sf::Vector2f(beamWidth, beamHeight));
			beamShape.setPosition(windowW / 2 - 0.5f - beamWidth / 2.f, windowH / 2 + 36);
		}
		if (timer > 250) {
			timer = 0;
			transitionState = TransitionState::ExitBeamDeflate;
			player.visible = false;
		}
		break;

	case TransitionState::ExitBeamDeflate:
		timer += elapsedTime.asMilliseconds();
		{
			// beamWidth is carefully calibrated, be sure to recalculate the regression based on the timer if you change it...
		    float beamWidth = 0.9999995 * std::exp(-0.0050125355 * static_cast<double>(timer)) * 20.f;
			float beamHeight = beamShape.getSize().y;
			beamShape.setSize(sf::Vector2f(beamWidth, beamHeight));
			beamShape.setPosition(windowW / 2 - 0.5f - beamWidth / 2.f, windowH / 2 + 36);
			if (timer >= 640) {
				timer = 0;
				transitionState = TransitionState::TransitionOut;
			}
		}
		break;

	case TransitionState::TransitionOut:
		timer += elapsedTime.asMilliseconds();
		if (level != 0) {
			if (timer > 100) {
				uint_fast8_t alpha = Easing::easeIn<1>(timer - 100, static_cast<int_fast32_t>(900)) * 255;
				transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
			}
			if (timer >= 1000) {
				transitionState = TransitionState::TransitionIn;
				timer = 0;
				transitionShape.setFillColor(sf::Color(0, 0, 0, 255));
				Reset();
			}
		} else {
			if (timer > 1600) {
				uint_fast8_t alpha = Easing::easeIn<1>(timer - 1600, static_cast<int_fast32_t>(1400)) * 255;
				transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
			}
			if (timer > 3000) {
				transitionState = TransitionState::TransitionIn;
				timer = 0;
				transitionShape.setFillColor(sf::Color(0, 0, 0, 255));
				Reset();
			}
		}
		break;

	case TransitionState::TransitionIn:
	    timer += elapsedTime.asMilliseconds();
		{
			uint_fast8_t alpha = Easing::easeOut<1>(timer, static_cast<int_fast32_t>(800)) * 255;
			transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
		}
		if (timer >= 800) {
			transitionState = TransitionState::EntryBeamDrop;
			timer = 0;
			beamShape.setSize(sf::Vector2f(4, 0));
			beamShape.setPosition(windowW / 2 - 2, 0);
			beamShape.setFillColor(sf::Color(114, 255, 229, 240));
		}
		break;

	case TransitionState::EntryBeamDrop:
		timer += elapsedTime.asMilliseconds();
		{
			float beamHeight = Easing::easeIn<2>(timer, static_cast<int_fast32_t>(300)) * (windowH / 2 + 26);
			beamShape.setSize(sf::Vector2f(4, beamHeight));
		}
		if (timer > 300) {
			transitionState = TransitionState::EntryBeamFade;
			timer = 0;
			player.visible = true;
			Feedback::sleep(milliseconds(20));
			camera.shake(0.15f);
		}
		break;

	case TransitionState::EntryBeamFade:
		timer += elapsedTime.asMilliseconds();
		{
			uint_fast8_t alpha = Easing::easeOut<1>(timer, static_cast<int_fast32_t>(250)) * 240;
			beamShape.setFillColor(sf::Color(114, 255, 229, alpha));
		}
		if (timer > 250) {
			transitionState = TransitionState::None;
			player.setState(Player::State::nominal);
			timer = 0;
		}
		break;
	}
}

void Game::Reset() {
	level += 1;
	pFonts->setWaypointText(level);
	tiles.clear();
	effectGroup.clear();
	details.clear();
	player.setPosition(windowW / 2 - 17, windowH / 2);
	camera.reset();
	en.clear();
	teleporterCond = 0;
	set = tileController::Tileset::intro;
	if (level == 0) {
		set = tileController::Tileset::intro;
	} else {
		set = tileController::Tileset::regular;
	}
	vignetteSprite.setColor(sf::Color(255, 255, 255, 255));
	if (set != tileController::Tileset::intro) {
		int count;
		do {
			count = generateMap(tiles.mapArray);
		} while (count < 150);
	}
	tiles.rebuild(set);
	bkg.setBkg(tiles.getWorkingSet());
	tiles.setPosition((windowW / 2) - 16, (windowH / 2));
	bkg.setPosition((tiles.posX / 2) + 206, tiles.posY / 2);
	auto pickLocation = [](std::vector<Coordinate>& emptyLocations) {
		int locationSelect = std::abs(static_cast<int>(globalRNG())) % emptyLocations.size();
		Coordinate c = emptyLocations[locationSelect];
		emptyLocations[locationSelect] = emptyLocations.back();
		emptyLocations.pop_back();
		return c;
	};
	if (level != 0) {
		Coordinate c = tiles.getTeleporterLoc();
		details.add<0>(tiles.posX + c.x * 32 + 2, tiles.posY + c.y * 26 - 4,
				   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
				   globalResourceHandler.getTexture(ResourceHandler::Texture::teleporterGlow));
		
		initEnemies(this);
		if (std::abs(static_cast<int>(globalRNG())) % 2) {
			Coordinate c = pickLocation(tiles.emptyMapLocations);
			details.add<1>(c.x * 32 + tiles.posX, c.y * 26 + tiles.posY,
						   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects), 0);
		}
		
		glowSprs1.clear();
		glowSprs2.clear();
		Circle teleporterFootprint;
		teleporterFootprint.x = tiles.getTeleporterLoc().x;
		teleporterFootprint.y = tiles.getTeleporterLoc().y;
		teleporterFootprint.r = 50;
		std::vector<Coordinate> detailPositions;
		if (set == tileController::Tileset::regular) {
			getRockPositions(tiles.mapArray, detailPositions, teleporterFootprint);
			for (auto element : detailPositions) {
				details.add<3>(tiles.posX + 32 * element.x, tiles.posY + 26 * element.y - 35,
							   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
			}
			detailPositions.clear();
    	}
		getLightingPositions(tiles.mapArray, detailPositions, teleporterFootprint);
		size_t len = detailPositions.size();
		for (size_t i = 0; i < len; i++) {
			details.add<2>(tiles.posX + 16 + (detailPositions[i].x * 32), tiles.posY - 3 + (detailPositions[i].y * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		}
		detailPositions.clear();
	} else if (set == tileController::Tileset::intro) {
		details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (6 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (0 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<2>(tiles.posX - 180 + 16 + (11 * 32), tiles.posY + 200 - 3 + (11 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<2>(tiles.posX - 180 + 16 + (10 * 32), tiles.posY + 204 + 8 + (-9 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<4>(tiles.posX - 192 + 6 * 32, tiles.posY + 301,
					   globalResourceHandler.getTexture(ResourceHandler::Texture::introWall));
		sf::Sprite podSpr;
		podSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
		podSpr.setTextureRect(sf::IntRect(164, 145, 44, 50));
		details.add<6>(tiles.posX + 3 * 32, tiles.posY + 4 + 17 * 26, podSpr);;
		tiles.teleporterLocation.x = 8;
		tiles.teleporterLocation.y = -7;
		details.add<0>(tiles.posX - 178 + 8 * 32, tiles.posY + 284 + -7 * 26,
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::teleporterGlow));
		for (auto it = global_levelZeroWalls.begin(); it != global_levelZeroWalls.end(); ++it) {
			wall w;
			w.setXinit(it->first);
			w.setYinit(it->second);
			tiles.walls.push_back(w);
		}
	}
}

DetailGroup & Game::getDetails() {
	return details;
}

enemyController & Game::getEnemyController() {
	return en;
}

tileController & Game::getTileController() {
	return tiles;
}

Player & Game::getPlayer() {
	return player;
}

EffectGroup & Game::getEffects() {
	return effectGroup;
}

InputController * Game::getPInput() {
	return pInput;
}

UserInterface & Game::getUI() {
	return UI;
}

FontController * Game::getPFonts() {
	return pFonts;
}

bool Game::getTeleporterCond() {
	return teleporterCond;
}

int Game::getLevel() {
	return level;
}


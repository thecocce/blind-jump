//
//  main.cpp
//  BlindJump
//
//  Created by Evan Bowman on 10/9/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include "player.hpp"
#include "backgroundHandler.hpp"
#include <cmath>
#include <SFML/Window.hpp>
#include "gameMap.hpp"
#include "ResourcePath.hpp"
#include "inputController.hpp"

#define CREDITS_DISP_LENGTH 370
	
int main(int, char const**) {
	//Set the seed for random generation
	srand(static_cast<unsigned int>(time(0)));

	sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
	
	float aspectRatio = static_cast<float>(desktop.width) / static_cast<float>(desktop.height);
	float windowWidth = 450.f;
	float windowHeight = 450.f;
	if (aspectRatio > 1.f) {
		float windowAspect;
		do {
			windowWidth += 0.025f;
			windowHeight -= 0.025f;
			windowAspect = windowWidth / windowHeight;
		}
		while (fabs(aspectRatio - windowAspect) > 0.005f);
	} else {
		float windowAspect;
		do {
			windowWidth -= 0.025f;
			windowHeight += 0.025f;
			windowAspect = windowWidth / windowHeight;
		}
		while (fabs(aspectRatio - windowAspect) > 0.005f);
	}
	
	// Boolean that stores whether the game is on the title screen
	bool title = false;
	// Variable that enables drawing the cutscene
	// For transitions, simply use a rectangle primitive shape
	sf::RectangleShape overlayRect;
	sf::Vector2f v1(windowWidth, windowHeight);
	overlayRect.setSize(v1);
	overlayRect.setFillColor(sf::Color(5,5,25,255));
	
	sf::Texture vignetteTexture;
	vignetteTexture.loadFromFile(resourcePath() + "vignetteMask.png");
	vignetteTexture.setSmooth(true);
	sf::Sprite vignetteSpr;
	vignetteSpr.setTexture(vignetteTexture);
	vignetteSpr.setScale(windowWidth / 450, windowHeight / 450);
	
	// Wraps input from keyboard and gamepad
	InputController input;
	
	// Don't want the fonts to be scaled and blurry, so define another view for drawing them
	sf::View fontView(sf::FloatRect(0, 0, desktop.width, desktop.height));
	// Since the view is half the size of the window, the interface will have a sort of stylistic pixelated apperance
	sf::View view(sf::FloatRect(0, 0, windowWidth, windowHeight));
	
	// Declare a font controller
	FontController fonts(fontView, windowWidth / 2, windowHeight / 2);
	
	//Initialize the map
	GameMap Map(windowWidth, windowHeight, &vignetteTexture, &input, &fonts);
	
	// Setup the window context settings
	sf::ContextSettings settings;
	settings.antialiasingLevel = 6;
	
	sf::RenderWindow window(sf::VideoMode(desktop.width, desktop.height), "Blind Jump", sf::Style::Fullscreen, settings);
	window.setMouseCursorVisible(false);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
	
	// Set the Icon
	sf::Image icon;
	if (!icon.loadFromFile(resourcePath() + "icon.png")) {
		return EXIT_FAILURE;
	}
	
	// Set the game icon
	window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

	// For scene transitions:
	sf::RectangleShape transition;
	transition.setScale(windowWidth / 450, windowHeight / 450);
	transition.setFillColor(sf::Color(37, 36, 72, 255));
	
	// Load opening credits text and create a background
	sf::Texture creditsTextTxtr;
	sf::Sprite creditsTextSpr;
	sf::RectangleShape creditsBkg;
	creditsBkg.setSize(sf::Vector2f(windowWidth, windowHeight));
	creditsBkg.setFillColor(sf::Color(47, 51, 98, 255));
	
	sf::Clock gameClock;
	sf::Time elapsedTime;
	
	// Start the game loop
	while (window.isOpen()) {
		// Process events
		sf::Event event;
		while (window.pollEvent(event)) {
			// Close window: exit
			if (event.type == sf::Event::Closed) {
				window.close();
			}

			// Escape pressed: exit
			if (event.type == sf::Event::KeyPressed/* && event.key.code == sf::Keyboard::Escape*/) {
				switch(event.key.code) {
				case sf::Keyboard::Escape:
					window.close();
					break;

				case sf::Keyboard::Left:
					input.setLeft(true);
					break;

				case sf::Keyboard::Right:
					input.setRight(true);
					break;

				case sf::Keyboard::Up:
					input.setUp(true);
					break;

				case sf::Keyboard::Down:
					input.setDown(true);
					break;

				case sf::Keyboard::X:
					input.setX(true);
					break;

				case sf::Keyboard::Z:
					input.setZ(true);
					break;

				case sf::Keyboard::C:
					input.setC(true);
					break;

				default:
					break;
				}
			} else if (event.type == sf::Event::KeyReleased) {
				switch(event.key.code) {
				case sf::Keyboard::Left:
					input.setLeft(false);
					break;

				case sf::Keyboard::Right:
					input.setRight(false);
					break;

				case sf::Keyboard::Up:
					input.setUp(false);
					break;

				case sf::Keyboard::Down:
					input.setDown(false);
					break;

				case sf::Keyboard::X:
					input.setX(false);
					break;

				case sf::Keyboard::Z:
					input.setZ(false);
					break;

				case sf::Keyboard::C:
					input.setC(false);
					break;

				default:
					break;
				}
			}
		}
		
		// Clear screen
		window.clear();
		
		//input.update();
		
		//Update and draw all of the game objects by calling the update function within mapInit and passing in a reference to the window
		if (!title) {   // If not on the title screen
			// Apply the view to the window.
			window.setView(view);
			
			elapsedTime = gameClock.restart();
			Map.update(window, elapsedTime);
			
			if (Map.getTeleporterCond()) {					 //Change the condition later to check for a value within Map
				Map.Reset();
			}
		} else {
			if (input.zPressed() && title) {
				title = false;
			}
		}
		
		// Update the window
		window.display();
	}

	return EXIT_SUCCESS;
}


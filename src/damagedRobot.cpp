//
//  damagedRobot.cpp
//  Blind Jump
//
//  Created by Evan Bowman on 3/5/16.
//  Copyright © 2016 Evan Bowman. All rights reserved.
//

#include "damagedRobot.hpp"

DamagedRobot::DamagedRobot(float _xInit, float _yInit, const sf::Texture & inpTxtr)
	: Detail{_xInit, _yInit}, robotSheet{inpTxtr}
{
	robotSheet[rand() % 2];
}

void DamagedRobot::update(float xOffset, float yOffset, const sf::Time & elapsedTime) {
	yPos = yOffset + yInit;
	robotSheet.setPosition(xInit + xOffset, yPos);
}

const sf::Sprite & DamagedRobot::getSprite() const {
	return robotSheet.getSprite();
}

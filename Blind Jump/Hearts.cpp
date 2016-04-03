//
//  Hearts.cpp
//  Blind Jump
//
//  Created by Evan Bowman on 4/2/16.
//  Copyright © 2016 Evan Bowman. All rights reserved.
//

#include "Hearts.hpp"

Hearts::Hearts(sf::Sprite* inpSpr, sf::Sprite glow, float xInit, float yInit) {
    this->xInit = xInit;
    this->yInit = yInit;
    this->glow = glow;
    spr = *inpSpr;
}

void Hearts::update(float xoffset, float yoffset) {
    xPos = xInit + xoffset;
    yPos = yInit + yoffset;
}

sf::Sprite Hearts::getSprite() {
    spr.setPosition(xPos, yPos);
    return spr;
}

sf::Sprite* Hearts::getGlow() {
    glow.setPosition(xPos, yPos + 8);
    return &glow;
}

bool Hearts::getKillFlag() {
    return killFlag;
}

void Hearts::setKillFlag(bool killFlag) {
    this->killFlag = killFlag;
}

float Hearts::getXpos() {
    return xPos;
}

float Hearts::getYpos() {
    return yPos;
}
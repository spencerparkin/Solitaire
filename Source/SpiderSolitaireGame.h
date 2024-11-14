#pragma once

#include "SolitaireGame.h"

class SpiderSolitaireGame : public SolitaireGame
{
public:
	SpiderSolitaireGame();
	virtual ~SpiderSolitaireGame();

	virtual void NewGame() override;
	virtual void Clear() override;

private:
	std::vector<std::shared_ptr<Card>> cardArray;
};
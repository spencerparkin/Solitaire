#pragma once

#include "SolitaireGame.h"

class SpiderSolitaireGame : public SolitaireGame
{
public:
	SpiderSolitaireGame(const Box& worldExtents, const Box& cardSize);
	virtual ~SpiderSolitaireGame();

	virtual void NewGame() override;
	virtual void Clear() override;
	virtual void OnGrabAt(DirectX::XMVECTOR worldPoint) override;
	virtual void OnReleaseAt(DirectX::XMVECTOR worldPoint) override;

private:
	std::vector<std::shared_ptr<Card>> cardArray;
};
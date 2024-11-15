#pragma once

#include "SolitaireGame.h"

class SpiderSolitaireGame : public SolitaireGame
{
public:
	SpiderSolitaireGame(const Box& worldExtents, const Box& cardSize);
	virtual ~SpiderSolitaireGame();

	virtual void NewGame() override;
	virtual void Clear() override;
	virtual void OnMouseGrabAt(DirectX::XMVECTOR worldPoint) override;
	virtual void OnMouseReleaseAt(DirectX::XMVECTOR worldPoint) override;
	virtual void OnMouseMove(DirectX::XMVECTOR worldPoint) override;

private:
	std::vector<std::shared_ptr<Card>> cardArray;
	DirectX::XMVECTOR grabDelta;
	std::shared_ptr<CardPile> originCardPile;
};
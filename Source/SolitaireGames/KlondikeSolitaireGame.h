#pragma once

#include "SolitaireGame.h"
#include <list>

class KlondikeSolitaireGame : public SolitaireGame
{
public:
	KlondikeSolitaireGame(const Box& worldExtents, const Box& cardSize);
	virtual ~KlondikeSolitaireGame();

	virtual std::shared_ptr<SolitaireGame> AllocNew() const override;
	virtual std::shared_ptr<SolitaireGame> Clone() const override;
	virtual void NewGame() override;
	virtual void Clear() override;
	virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const override;
	virtual bool OnMouseGrabAt(DirectX::XMVECTOR worldPoint) override;
	virtual void OnMouseReleaseAt(DirectX::XMVECTOR worldPoint) override;
	virtual void OnMouseMove(DirectX::XMVECTOR worldPoint) override;
	virtual void OnCardsNeeded() override;
	virtual void OnKeyUp(uint32_t keyCode) override;
	virtual void Tick(double deltaTimeSeconds) override;
	virtual bool GameWon() const override;

private:
	std::list<std::shared_ptr<Card>> cardList;
	std::shared_ptr<CardPile> drawPile;
	std::vector<std::shared_ptr<CardPile>> suitPileArray;
};
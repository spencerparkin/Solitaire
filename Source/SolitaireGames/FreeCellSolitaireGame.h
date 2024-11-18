#pragma once

#include "SolitaireGame.h"

class FreeCellSolitaireGame : public SolitaireGame
{
public:
	FreeCellSolitaireGame(const Box& worldExtents, const Box& cardSize);
	virtual ~FreeCellSolitaireGame();

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
	int GetFreeCellCount() const;

	std::vector<std::shared_ptr<CardPile>> suitPileArray;
	std::vector<std::shared_ptr<CardPile>> freePileArray;
};
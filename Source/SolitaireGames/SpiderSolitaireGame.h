#pragma once

#include "SolitaireGame.h"

class SpiderSolitaireGame : public SolitaireGame
{
public:
	enum DifficultyLevel
	{
		LOW,
		MEDIUM,
		HARD
	};

	SpiderSolitaireGame(const Box& worldExtents, const Box& cardSize, DifficultyLevel difficultyLevel);
	virtual ~SpiderSolitaireGame();

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
	std::vector<std::shared_ptr<Card>> cardArray;
	std::vector<std::shared_ptr<Card>> exitingCardArray;
	DifficultyLevel difficultyLevel;
};
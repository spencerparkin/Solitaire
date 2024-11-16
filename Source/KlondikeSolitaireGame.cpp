#include "KlondikeSolitaireGame.h"

using namespace DirectX;

KlondikeSolitaireGame::KlondikeSolitaireGame(const Box& worldExtents, const Box& cardSize) : SolitaireGame(worldExtents, cardSize)
{
}

/*virtual*/ KlondikeSolitaireGame::~KlondikeSolitaireGame()
{
}

/*virtual*/ void KlondikeSolitaireGame::NewGame()
{
	this->Clear();

	//...
}

/*virtual*/ void KlondikeSolitaireGame::Clear()
{
	SolitaireGame::Clear();
	this->cardArray.clear();
	this->choicePile = nullptr;
	this->suitePileArray.clear();
}

/*virtual*/ void KlondikeSolitaireGame::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
}

/*virtual*/ bool KlondikeSolitaireGame::OnMouseGrabAt(DirectX::XMVECTOR worldPoint)
{
	return false;
}

/*virtual*/ void KlondikeSolitaireGame::OnMouseReleaseAt(DirectX::XMVECTOR worldPoint)
{
}

/*virtual*/ void KlondikeSolitaireGame::OnMouseMove(DirectX::XMVECTOR worldPoint)
{
}

/*virtual*/ void KlondikeSolitaireGame::OnCardsNeeded()
{
}

/*virtual*/ void KlondikeSolitaireGame::OnKeyUp(uint32_t keyCode)
{
}

/*virtual*/ void KlondikeSolitaireGame::Tick(double deltaTimeSeconds)
{
}

/*virtual*/ bool KlondikeSolitaireGame::GameWon() const
{
	return false;
}
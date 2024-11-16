#include "FreeCellSolitaireGame.h"

using namespace DirectX;

FreeCellSolitaireGame::FreeCellSolitaireGame(const Box& worldExtents, const Box& cardSize) : SolitaireGame(worldExtents, cardSize)
{
}

/*virtual*/ FreeCellSolitaireGame::~FreeCellSolitaireGame()
{
}

/*virtual*/ void FreeCellSolitaireGame::NewGame()
{
	this->Clear();

	std::vector<std::shared_ptr<Card>> cardArray;
	this->GenerateDeck(cardArray);
	this->SuffleCards(cardArray);

	int numPiles = 8;
	int numCards = (int)cardArray.size();
	for (int i = 0; i < numCards; i++)
	{
		if (i < numPiles)
		{
			auto pile = std::make_shared<CascadingCardPile>();
			this->cardPileArray.push_back(pile);

			pile->position = XMVectorSet(
				(float(i) / float(numPiles - 1)) * (this->worldExtents.GetWidth() - this->cardSize.GetWidth()),
				this->worldExtents.GetHeight() - this->cardSize.GetHeight() * 2.4f,
				0.0f,
				1.0f);
		}

		CardPile* pile = this->cardPileArray[i % numPiles].get();
		std::shared_ptr<Card> card = cardArray.back();
		cardArray.pop_back();
		pile->cardArray.push_back(card);
		card->orientation = Card::Orientation::FACE_UP;
	}

	for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
		cardPile->LayoutCards(this->cardSize);

	for (int i = 0; i < numPiles; i++)
	{
		auto pile = std::make_shared<SingularCardPile>();

		if (i < int(Card::Suit::NUM_SUITS))
			this->freePileArray.push_back(pile);
		else
			this->suitPileArray.push_back(pile);

		pile->position = XMVectorSet(
			(float(i) / float(numPiles - 1)) * (this->worldExtents.GetWidth() - this->cardSize.GetWidth()),
			this->worldExtents.GetHeight() - this->cardSize.GetHeight() * 1.2f,
			0.0f,
			1.0f);
		pile->LayoutCards(this->cardSize);
	}
}

/*virtual*/ void FreeCellSolitaireGame::Clear()
{
	SolitaireGame::Clear();
	this->suitPileArray.clear();
	this->freePileArray.clear();
}

/*virtual*/ void FreeCellSolitaireGame::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
	SolitaireGame::GenerateRenderList(cardRenderList);

	for (const std::shared_ptr<CardPile>& cardPile : this->suitPileArray)
		cardPile->GenerateRenderList(cardRenderList);

	for (const std::shared_ptr<CardPile>& cardPile : this->freePileArray)
		cardPile->GenerateRenderList(cardRenderList);
}

/*virtual*/ bool FreeCellSolitaireGame::OnMouseGrabAt(DirectX::XMVECTOR worldPoint)
{
	return false;
}

/*virtual*/ void FreeCellSolitaireGame::OnMouseReleaseAt(DirectX::XMVECTOR worldPoint)
{
}

/*virtual*/ void FreeCellSolitaireGame::OnMouseMove(DirectX::XMVECTOR worldPoint)
{
}

/*virtual*/ void FreeCellSolitaireGame::OnCardsNeeded()
{
}

/*virtual*/ void FreeCellSolitaireGame::OnKeyUp(uint32_t keyCode)
{
}

/*virtual*/ void FreeCellSolitaireGame::Tick(double deltaTimeSeconds)
{
}

/*virtual*/ bool FreeCellSolitaireGame::GameWon() const
{
	for (const std::shared_ptr<CardPile>& suitPile : this->suitPileArray)
		if (suitPile->cardArray.size() < Card::Value::NUM_VALUES)
			return false;

	return true;
}
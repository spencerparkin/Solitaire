#include "SpiderSolitaireGame.h"

using namespace DirectX;

SpiderSolitaireGame::SpiderSolitaireGame()
{
}

/*virtual*/ SpiderSolitaireGame::~SpiderSolitaireGame()
{
}

/*virtual*/ void SpiderSolitaireGame::NewGame(const Box& worldExtents, const Box& cardSize)
{
	this->Clear();

	this->GenerateDeck(this->cardArray);
	this->GenerateDeck(this->cardArray);
	this->SuffleCards(this->cardArray);

	int numPiles = 10;

	for (int i = 0; i < numPiles; i++)
	{
		auto pile = std::make_shared<CascadingCardPile>();
		this->cardPileArray.push_back(pile);

		for (int j = 0; j < ((i < 5) ? 6 : 5); j++)
		{
			std::shared_ptr<Card> card = this->cardArray.back();
			card->orientation = Card::Orientation::FACE_DOWN;
			this->cardArray.pop_back();
			pile->cardArray.push_back(card);
		}

		pile->cardArray[pile->cardArray.size() - 1]->orientation = Card::Orientation::FACE_UP;
		pile->position = XMVectorSet(
			(float(i) / float(numPiles - 1)) * (worldExtents.GetWidth() - cardSize.GetWidth()),
			worldExtents.GetHeight() - cardSize.GetHeight(),
			0.0f,
			1.0f);
		pile->LayoutCards(cardSize);
	}
}

/*virtual*/ void SpiderSolitaireGame::Clear()
{
	SolitaireGame::Clear();
	this->cardArray.clear();
}
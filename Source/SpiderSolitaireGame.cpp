#include "SpiderSolitaireGame.h"

using namespace DirectX;

SpiderSolitaireGame::SpiderSolitaireGame(const Box& worldExtents, const Box& cardSize) : SolitaireGame(worldExtents, cardSize)
{
}

/*virtual*/ SpiderSolitaireGame::~SpiderSolitaireGame()
{
}

/*virtual*/ void SpiderSolitaireGame::NewGame()
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

		for (int j = 0; j < ((i < 4) ? 6 : 5); j++)
		{
			std::shared_ptr<Card> card = this->cardArray.back();
			card->orientation = Card::Orientation::FACE_DOWN;
			this->cardArray.pop_back();
			pile->cardArray.push_back(card);
		}

		pile->cardArray[pile->cardArray.size() - 1]->orientation = Card::Orientation::FACE_UP;
		pile->position = XMVectorSet(
			(float(i) / float(numPiles - 1)) * (this->worldExtents.GetWidth() - this->cardSize.GetWidth()),
			this->worldExtents.GetHeight() - this->cardSize.GetHeight(),
			0.0f,
			1.0f);
		pile->LayoutCards(this->cardSize);
	}
}

/*virtual*/ void SpiderSolitaireGame::Clear()
{
	SolitaireGame::Clear();
	this->cardArray.clear();
}

/*virtual*/ void SpiderSolitaireGame::OnGrabAt(DirectX::XMVECTOR worldPoint)
{
	std::shared_ptr<CardPile> foundCardPile;
	int foundCardOffset = -1;
	if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
	{
		foundCardOffset = 0;
	}
}

/*virtual*/ void SpiderSolitaireGame::OnReleaseAt(DirectX::XMVECTOR worldPoint)
{
}
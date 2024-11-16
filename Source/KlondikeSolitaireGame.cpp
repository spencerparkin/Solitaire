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

	std::vector<std::shared_ptr<Card>> cardArray;
	this->GenerateDeck(cardArray);
	this->SuffleCards(cardArray);

	int numPiles = 7;

	for (int i = 0; i < int(Card::Suit::NUM_SUITS); i++)
	{
		auto pile = std::make_shared<SingularCardPile>();
		this->suitPileArray.push_back(pile);

		pile->position = XMVectorSet(
			(float(i + 3) / float(numPiles - 1)) * (this->worldExtents.GetWidth() - this->cardSize.GetWidth()),
			this->worldExtents.GetHeight() - this->cardSize.GetHeight() * 1.2f,
			0.0f,
			1.0f);
		pile->LayoutCards(this->cardSize);
	}

	for (int i = 0; i < numPiles; i++)
	{
		auto pile = std::make_shared<CascadingCardPile>();
		this->cardPileArray.push_back(pile);

		for (int j = 0; j <= i; j++)
		{
			std::shared_ptr<Card> card = cardArray.back();
			card->orientation = (j == i) ? Card::Orientation::FACE_UP : Card::Orientation::FACE_DOWN;
			cardArray.pop_back();
			pile->cardArray.push_back(card);
		}

		pile->position = XMVectorSet(
			(float(i) / float(numPiles - 1)) * (this->worldExtents.GetWidth() - this->cardSize.GetWidth()),
			this->worldExtents.GetHeight() - this->cardSize.GetHeight() * 2.4f,
			0.0f,
			1.0f);
		pile->LayoutCards(this->cardSize);
	}

	this->drawPile = std::make_shared<CascadingCardPile>(CascadingCardPile::CascadeDirection::RIGHT, 3);
	this->drawPile->position = XMVectorSet(
		0.0f,
		this->worldExtents.GetHeight() - this->cardSize.GetHeight() * 1.2f,
		0.0f,
		1.0f);
	this->drawPile->LayoutCards(this->cardSize);

	for (std::shared_ptr<Card>& card : cardArray)
		this->cardList.push_back(card);
}

/*virtual*/ void KlondikeSolitaireGame::Clear()
{
	SolitaireGame::Clear();
	this->cardList.clear();
	this->drawPile = nullptr;
	this->suitPileArray.clear();
}

/*virtual*/ void KlondikeSolitaireGame::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
	SolitaireGame::GenerateRenderList(cardRenderList);

	for (const std::shared_ptr<CardPile>& pile : this->suitPileArray)
		pile->GenerateRenderList(cardRenderList);

	this->drawPile->GenerateRenderList(cardRenderList);
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
	if (this->cardList.size() == 0)
	{
		while (this->drawPile->cardArray.size() > 0)
		{
			std::shared_ptr<Card> card = this->drawPile->cardArray.back();
			this->drawPile->cardArray.pop_back();
			this->cardList.push_back(card);
		}
	}
	
	for (int i = 0; i < 3; i++)
	{
		if (this->cardList.size() == 0)
			break;

		std::shared_ptr<Card> card = this->cardList.back();
		this->cardList.pop_back();
		this->drawPile->cardArray.push_back(card);
	}

	this->drawPile->LayoutCards(this->cardSize);
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
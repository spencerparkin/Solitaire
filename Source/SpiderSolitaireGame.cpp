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

/*virtual*/ void SpiderSolitaireGame::OnMouseGrabAt(DirectX::XMVECTOR worldPoint)
{
	assert(this->movingCardPile.get() == nullptr);

	std::shared_ptr<CardPile> foundCardPile;
	int foundCardOffset = -1;
	if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
	{
		Card* card = foundCardPile->cardArray[foundCardOffset].get();
		if (card->orientation == Card::Orientation::FACE_UP)
		{
			this->movingCardPile = std::make_shared<CascadingCardPile>();

			for (int i = foundCardOffset; i < foundCardPile->cardArray.size(); i++)
				this->movingCardPile->cardArray.push_back(foundCardPile->cardArray[i]);
			
			for (int i = 0; i < this->movingCardPile->cardArray.size(); i++)
				foundCardPile->cardArray.pop_back();

			this->movingCardPile->position = this->movingCardPile->cardArray[0]->position;
			this->grabDelta = this->movingCardPile->position - worldPoint;
			this->originCardPile = foundCardPile;
		}
	}
}

/*virtual*/ void SpiderSolitaireGame::OnMouseReleaseAt(DirectX::XMVECTOR worldPoint)
{
	if (this->movingCardPile.get())
	{
		bool abortCardPileMove = true;
		std::shared_ptr<CardPile> foundCardPile;
		int foundCardOffset = -1;
		if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
		{
			Card* card = foundCardPile->cardArray[foundCardOffset].get();
			if (foundCardOffset == foundCardPile->cardArray.size() - 1 &&
				int(card->value) - 1 == int(this->movingCardPile->cardArray[0]->value))
			{
				abortCardPileMove = false;

				for (auto& movingCard : this->movingCardPile->cardArray)
					foundCardPile->cardArray.push_back(movingCard);

				foundCardPile->LayoutCards(this->cardSize);

				if (this->originCardPile->cardArray.size() > 0)
					this->originCardPile->cardArray[this->originCardPile->cardArray.size() - 1]->orientation = Card::Orientation::FACE_UP;
			}
		}

		if (abortCardPileMove)
		{
			for (auto& movingCard : this->movingCardPile->cardArray)
				this->originCardPile->cardArray.push_back(movingCard);

			this->originCardPile->LayoutCards(this->cardSize);
		}

		this->movingCardPile = nullptr;
	}
}

/*virtual*/ void SpiderSolitaireGame::OnMouseMove(DirectX::XMVECTOR worldPoint)
{
	if (this->movingCardPile.get())
	{
		this->movingCardPile->position = worldPoint + this->grabDelta;
		this->movingCardPile->LayoutCards(this->cardSize);
	}
}

/*virtual*/ void SpiderSolitaireGame::OnCardsNeeded()
{
	// According to the rules of Spider, you can't deal out
	// cards unless all 10 piles have at least one or more cards.
	for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
		if (cardPile->cardArray.size() == 0)
			return;

	// Deal out 10 more cards or as many as we have left.
	for (int i = 0; i < 10 && this->cardArray.size() > 0; i++)
	{
		std::shared_ptr<Card> card = this->cardArray.back();
		this->cardArray.pop_back();
		
		card->orientation = Card::Orientation::FACE_UP;

		std::shared_ptr<CardPile>& cardPile = this->cardPileArray[i];
		cardPile->cardArray.push_back(card);
		cardPile->LayoutCards(this->cardSize);

		card->targetPosition = card->position;
		card->position = this->worldExtents.min;
		card->animationRate = 200.0;
	}
}

/*virtual*/ void SpiderSolitaireGame::OnKeyUp(uint32_t keyCode)
{
}
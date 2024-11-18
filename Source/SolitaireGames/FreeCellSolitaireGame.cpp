#include "FreeCellSolitaireGame.h"

using namespace DirectX;

FreeCellSolitaireGame::FreeCellSolitaireGame(const Box& worldExtents, const Box& cardSize) : SolitaireGame(worldExtents, cardSize)
{
}

/*virtual*/ FreeCellSolitaireGame::~FreeCellSolitaireGame()
{
}

/*virtual*/ std::shared_ptr<SolitaireGame> FreeCellSolitaireGame::AllocNew() const
{
	return std::make_shared<FreeCellSolitaireGame>(this->worldExtents, this->cardSize);
}

/*virtual*/ std::shared_ptr<SolitaireGame> FreeCellSolitaireGame::Clone() const
{
	auto game = SolitaireGame::Clone();

	auto freeCell = dynamic_cast<FreeCellSolitaireGame*>(game.get());

	for (const auto& cardPile : this->suitPileArray)
		freeCell->suitPileArray.push_back(cardPile->Clone());

	for (const auto& cardPile : this->freePileArray)
		freeCell->freePileArray.push_back(cardPile->Clone());

	return game;
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
	for (const std::shared_ptr<CardPile>& cardPile : this->suitPileArray)
		cardPile->GenerateRenderList(cardRenderList);

	for (const std::shared_ptr<CardPile>& cardPile : this->freePileArray)
		cardPile->GenerateRenderList(cardRenderList);

	SolitaireGame::GenerateRenderList(cardRenderList);
}

/*virtual*/ bool FreeCellSolitaireGame::OnMouseGrabAt(DirectX::XMVECTOR worldPoint)
{
	assert(this->movingCardPile.get() == nullptr);

	int foundCardOffset = -1;
	std::shared_ptr<CardPile> foundCardPile;
	if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
	{
		Card* card = foundCardPile->cardArray[foundCardOffset].get();
		if(foundCardPile->CardsInOrder(foundCardOffset, int(foundCardPile->cardArray.size()) - 1) &&
			foundCardPile->CardsAlternateColor(foundCardOffset, int(foundCardPile->cardArray.size()) - 1))
		{
			int moveCardCount = int(foundCardPile->cardArray.size()) - foundCardOffset;
			int freeCellCount = this->GetFreeCellCount();
			if (moveCardCount <= freeCellCount + 1)
			{
				this->StartCardMoving(foundCardPile, foundCardOffset, worldPoint);
				return true;
			}
		}
	}
	else
	{
		for (int i = 0; i < int(this->freePileArray.size()); i++)
		{
			std::shared_ptr<CardPile>& freePile = this->freePileArray[i];
			if (freePile->ContainsPoint(worldPoint, this->cardSize) && freePile->cardArray.size() == 1)
			{
				this->StartCardMoving(freePile, 0, worldPoint);
				return true;
			}
		}
	}

	return false;
}

/*virtual*/ void FreeCellSolitaireGame::OnMouseReleaseAt(DirectX::XMVECTOR worldPoint)
{
	if (this->movingCardPile.get())
	{
		bool moveCards = false;
		std::shared_ptr<CardPile> foundCardPile;
		int foundCardOffset = -1;
		if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
		{
			const Card* card = foundCardPile->cardArray[foundCardOffset].get();
			if (card->GetColor() != this->movingCardPile->cardArray[0]->GetColor() &&
				int(card->value) - 1 == int(this->movingCardPile->cardArray[0]->value))
			{
				moveCards = true;
			}
		}
		else if (this->movingCardPile->cardArray.size() == 1)
		{
			for (int i = 0; i < int(Card::Suit::NUM_SUITS); i++)
			{
				std::shared_ptr<CardPile>& suitPile = this->suitPileArray[i];
				if (suitPile->ContainsPoint(worldPoint, this->cardSize))
				{
					if (suitPile->cardArray.size() == 0 && this->movingCardPile->cardArray[0]->value == Card::Value::ACE ||
						(int(suitPile->cardArray[suitPile->cardArray.size() - 1]->value) + 1 == int(this->movingCardPile->cardArray[0]->value) &&
							suitPile->cardArray[suitPile->cardArray.size() - 1]->suit == this->movingCardPile->cardArray[0]->suit))
					{
						foundCardPile = suitPile;
						moveCards = true;
						break;
					}
				}
			}

			if (!moveCards)
			{
				for (int i = 0; i < int(this->freePileArray.size()); i++)
				{
					std::shared_ptr<CardPile>& freePile = this->freePileArray[i];
					if (freePile->ContainsPoint(worldPoint, this->cardSize))
					{
						if (freePile->cardArray.size() == 0)
						{
							foundCardPile = freePile;
							moveCards = true;
							break;
						}
					}
				}
			}
		}
		
		if (!moveCards)
		{
			for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
			{
				if (cardPile->cardArray.size() == 0 && cardPile->ContainsPoint(worldPoint, this->cardSize))
				{
					foundCardPile = cardPile;
					moveCards = true;
					break;
				}
			}
		}

		this->FinishCardMoving(foundCardPile, moveCards);
	}
}

/*virtual*/ void FreeCellSolitaireGame::OnMouseMove(DirectX::XMVECTOR worldPoint)
{
	this->ManageCardMoving(worldPoint);
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

int FreeCellSolitaireGame::GetFreeCellCount() const
{
	int count = 0;
	for (const std::shared_ptr<CardPile>& freePile : this->freePileArray)
		if (freePile->cardArray.size() == 0)
			count++;

	return count;
}
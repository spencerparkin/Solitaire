#include "SolitaireGame.h"
#include <format>

using namespace DirectX;

//----------------------------------- SolitaireGame -----------------------------------

SolitaireGame::SolitaireGame(const Box& worldExtents, const Box& cardSize)
{
	this->worldExtents = worldExtents;
	this->cardSize = cardSize;
}

/*virtual*/ SolitaireGame::~SolitaireGame()
{
}

/*virtual*/ void SolitaireGame::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
	for (const std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
		cardPile->GenerateRenderList(cardRenderList);

	if (this->movingCardPile.get())
		this->movingCardPile->GenerateRenderList(cardRenderList);
}

/*static*/ void SolitaireGame::GenerateDeck(std::vector<std::shared_ptr<Card>>& cardArray)
{
	for (int i = 0; i < (int)Card::NUM_SUITES; i++)
	{
		for (int j = 0; j < (int)Card::NUM_VALUES; j++)
		{
			auto card = std::make_shared<Card>();
			card->suite = (Card::Suite)i;
			card->value = (Card::Value)j;
			cardArray.push_back(card);
		}
	}
}

/*static*/ void SolitaireGame::SuffleCards(std::vector<std::shared_ptr<Card>>& cardArray)
{
	for (int i = int(cardArray.size()) - 1; i >= 0; i--)
	{
		int j = RandomInteger(0, i);
		if (j != i)
		{
			std::shared_ptr<Card> card = cardArray[i];
			cardArray[i] = cardArray[j];
			cardArray[j] = card;
		}
	}
}

/*static*/ int SolitaireGame::RandomInteger(int min, int max)
{
	double alpha = double(std::rand()) / double(RAND_MAX);
	double value = (1.0 - alpha) * (double(min) - 0.5) + alpha * (double(max) + 0.5);
	int roundedValue = (int)::round(value);
	if (roundedValue < min)
		roundedValue = min;
	if (roundedValue > max)
		roundedValue = max;
	return roundedValue;
}

/*virtual*/ void SolitaireGame::Clear()
{
	this->cardPileArray.clear();
	this->movingCardPile.reset();
}

bool SolitaireGame::FindCardAndPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile>& foundCardPile, int& foundCardOffset)
{
	for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
	{
		// Search from top to bottom to account for Z-order.
		for (int i = int(cardPile->cardArray.size()) - 1; i >= 0; i--)
		{
			std::shared_ptr<Card>& card = cardPile->cardArray[i];
			if (card->ContainsPoint(worldPoint, this->cardSize))
			{
				foundCardPile = cardPile;
				foundCardOffset = i;
				return true;
			}
		}
	}

	return false;
}

//----------------------------------- SolitaireGame::Card -----------------------------------

SolitaireGame::Card::Card()
{
	this->value = Value::ACE;
	this->suite = Suite::SPADES;
	this->orientation = Orientation::FACE_UP;
	this->position = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

/*virtual*/ SolitaireGame::Card::~Card()
{
}

bool SolitaireGame::Card::ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const
{
	Box cardBox = cardSize;
	cardBox.min += this->position;
	cardBox.max += this->position;
	return cardBox.ContainsPoint(point);
}

std::string SolitaireGame::Card::GetRenderKey() const
{
	if (this->orientation == Orientation::FACE_DOWN)
		return "card_back";

	std::string prefix;
	switch (this->value)
	{
	case Value::ACE:
		prefix = "ace";
		break;
	case Value::JACK:
		prefix = "jack";
		break;
	case Value::QUEEN:
		prefix = "queen";
		break;
	case Value::KING:
		prefix = "king";
		break;
	default:
		prefix = std::format("{}", int(this->value) + 1);
		break;
	}

	std::string postfix;
	switch (this->suite)
	{
	case Suite::SPADES:
		postfix = "spades";
		break;
	case Suite::CLUBS:
		postfix = "clubs";
		break;
	case Suite::DIAMONDS:
		postfix = "diamonds";
		break;
	case Suite::HEARTS:
		postfix = "hearts";
		break;
	}

	return prefix + "_of_" + postfix;
}

//----------------------------------- SolitaireGame::CardPile -----------------------------------

SolitaireGame::CardPile::CardPile()
{
	this->position = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

/*virtual*/ SolitaireGame::CardPile::~CardPile()
{
}

//----------------------------------- SolitaireGame::CascadingCardPile -----------------------------------

SolitaireGame::CascadingCardPile::CascadingCardPile()
{
	this->cascadeDirection = CascadeDirection::DOWN;
}

/*virtual*/ SolitaireGame::CascadingCardPile::~CascadingCardPile()
{
}

/*virtual*/ void SolitaireGame::CascadingCardPile::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
	if (this->cardArray.size() > 0)
	{
		// Render the cards from bottom to top of the pile.
		for (const std::shared_ptr<Card>& card : this->cardArray)
			cardRenderList.push_back(card.get());
	}
	else
	{
		// TODO: Should add an "empty" card to the render list.
	}
}

/*virtual*/ void SolitaireGame::CascadingCardPile::LayoutCards(const Box& cardSize)
{
	XMVECTOR location = this->position;
	XMVECTOR delta = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	switch (this->cascadeDirection)
	{
	case CascadeDirection::DOWN:
		delta = XMVectorSet(0.0f, -cardSize.GetHeight() * 0.2f, 0.0f, 0.0f);
		break;
	case CascadeDirection::RIGHT:
		delta = XMVectorSet(cardSize.GetWidth() * 0.2f, 0.0f, 0.0f, 0.0f);
		break;
	}

	for (std::shared_ptr<Card>& card : this->cardArray)
	{
		card->position = location;
		location += delta;
	}
}

//----------------------------------- SolitaireGame::SingularCardPile -----------------------------------

SolitaireGame::SingularCardPile::SingularCardPile()
{
}

/*virtual*/ SolitaireGame::SingularCardPile::~SingularCardPile()
{
}

/*virtual*/ void SolitaireGame::SingularCardPile::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
	if (this->cardArray.size() > 0)
	{
		// Only the card on top of the pile renders.
		cardRenderList.push_back(this->cardArray[this->cardArray.size() - 1].get());
	}
	else
	{
		// TODO: Should add an "empty" card to the render list.
	}
}

/*virtual*/ void SolitaireGame::SingularCardPile::LayoutCards(const Box& cardSize)
{
	for (std::shared_ptr<Card>& card : this->cardArray)
		card->position = this->position;
}
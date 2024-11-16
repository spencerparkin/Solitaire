#include "SolitaireGame.h"
#include <format>

using namespace DirectX;

//----------------------------------- SolitaireGame -----------------------------------

SolitaireGame::SolitaireGame(const Box& worldExtents, const Box& cardSize)
{
	this->worldExtents = worldExtents;
	this->cardSize = cardSize;
	this->grabDelta = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
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
			card->suit = (Card::Suit)i;
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

bool SolitaireGame::FindEmptyPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile>& foundCardPile)
{
	for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
	{
		if (cardPile->cardArray.size() > 0)
			continue;

		if (cardPile->ContainsPoint(worldPoint, this->cardSize))
		{
			foundCardPile = cardPile;
			return true;
		}
	}

	return false;
}

/*virtual*/ void SolitaireGame::Tick(double deltaTimeSeconds)
{
	for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
		for (std::shared_ptr<Card>& card : cardPile->cardArray)
			card->Tick(deltaTimeSeconds);
}

//----------------------------------- SolitaireGame::Card -----------------------------------

SolitaireGame::Card::Card()
{
	this->value = Value::ACE;
	this->suit = Suit::SPADES;
	this->orientation = Orientation::FACE_UP;
	this->position = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	this->targetPosition = this->position;
	this->animationRate = 0.0;
}

/*virtual*/ SolitaireGame::Card::~Card()
{
}

void SolitaireGame::Card::Tick(double deltaTimeSeconds)
{
	if (this->animationRate > 0.0f)
	{
		XMVECTOR delta = this->targetPosition - this->position;
		double currentDistance = XMVectorGetX(XMVector3Length(delta));
		double travelDistance = this->animationRate * deltaTimeSeconds;
		if (currentDistance <= travelDistance)
		{
			this->position = this->targetPosition;
			this->animationRate = 0.0;
		}
		else
		{
			delta = XMVectorScale(delta, 1.0 / currentDistance) * travelDistance;
			this->position += delta;
		}
	}
}

bool SolitaireGame::Card::ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const
{
	Box cardBox = cardSize;
	cardBox.min += this->position;
	cardBox.max += this->position;
	return cardBox.ContainsPoint(point);
}

SolitaireGame::Card::Color SolitaireGame::Card::GetColor() const
{
	switch (this->suit)
	{
	case Suit::SPADES:
	case Suit::CLUBS:
		return Color::BLACK;
	case Suit::DIAMONDS:
	case Suit::HEARTS:
		return Color::RED;
	}

	return Color::BLACK;
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
	switch (this->suit)
	{
	case Suit::SPADES:
		postfix = "spades";
		break;
	case Suit::CLUBS:
		postfix = "clubs";
		break;
	case Suit::DIAMONDS:
		postfix = "diamonds";
		break;
	case Suit::HEARTS:
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

bool SolitaireGame::CardPile::ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const
{
	Box pileBox = cardSize;
	pileBox.min += this->position;
	pileBox.max += this->position;
	return pileBox.ContainsPoint(point);
}

bool SolitaireGame::CardPile::IndexValid(int i) const
{
	return 0 <= i && i < int(this->cardArray.size());
}

bool SolitaireGame::CardPile::CardsInOrder(int start, int finish) const
{
	assert(start <= finish);
	assert(this->IndexValid(start));
	assert(this->IndexValid(finish));

	if (start == finish)
		return true;

	for (int i = start; i < finish; i++)
	{
		const Card* cardA = this->cardArray[i].get();
		const Card* cardB = this->cardArray[i + 1].get();

		if (int(cardA->value) - 1 != int(cardB->value))
			return false;
	}

	return true;
}

bool SolitaireGame::CardPile::CardsSameColor(int start, int finish) const
{
	assert(start <= finish);
	assert(this->IndexValid(start));
	assert(this->IndexValid(finish));

	Card::Color color = this->cardArray[start]->GetColor();
	for (int i = start + 1; i <= finish; i++)
		if (this->cardArray[i]->GetColor() != color)
			return false;

	return true;
}

bool SolitaireGame::CardPile::CardsSameSuit(int start, int finish) const
{
	assert(start <= finish);
	assert(this->IndexValid(start));
	assert(this->IndexValid(finish));

	Card::Suit suit = this->cardArray[start]->suit;
	for (int i = start + 1; i <= finish; i++)
		if (this->cardArray[i]->suit != suit)
			return false;

	return true;
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
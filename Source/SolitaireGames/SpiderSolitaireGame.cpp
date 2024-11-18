#include "SpiderSolitaireGame.h"

using namespace DirectX;

SpiderSolitaireGame::SpiderSolitaireGame(const Box& worldExtents, const Box& cardSize, DifficultyLevel difficultyLevel) : SolitaireGame(worldExtents, cardSize)
{
	this->difficultyLevel = DifficultyLevel::LOW;
}

/*virtual*/ SpiderSolitaireGame::~SpiderSolitaireGame()
{
}

/*virtual*/ std::shared_ptr<SolitaireGame> SpiderSolitaireGame::AllocNew() const
{
	return std::make_shared<SpiderSolitaireGame>(this->worldExtents, this->cardSize, this->difficultyLevel);
}

/*virtual*/ std::shared_ptr<SolitaireGame> SpiderSolitaireGame::Clone() const
{
	auto game = SolitaireGame::Clone();

	assert(exitingCardArray.size() == 0);

	auto spider = dynamic_cast<SpiderSolitaireGame*>(game.get());

	for (const auto& card : this->cardArray)
		spider->cardArray.push_back(card);

	return game;
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
			this->worldExtents.GetHeight() - this->cardSize.GetHeight() * 1.2f,
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

/*virtual*/ void SpiderSolitaireGame::GenerateRenderList(std::vector<const Card*>& cardRenderList) const
{
	SolitaireGame::GenerateRenderList(cardRenderList);

	for (const std::shared_ptr<Card>& card : this->exitingCardArray)
		cardRenderList.push_back(card.get());
}

/*virtual*/ void SpiderSolitaireGame::Tick(double deltaTimeSeconds)
{
	SolitaireGame::Tick(deltaTimeSeconds);

	int animationCount = 0;
	for (std::shared_ptr<Card>& card : this->exitingCardArray)
	{
		card->Tick(deltaTimeSeconds);
		if (card->animationRate == 0.0)
			animationCount++;
	}

	if (animationCount == this->exitingCardArray.size())
		this->exitingCardArray.clear();

	// Note that we continuously check here for cards that can
	// exit just so we don't have to think about checking for it
	// everywhere that it can happen during game-play.  Hmmmm.
	// A thought comes to mind.  The player may create a sequence
	// that can exit, but maybe thay're not ready for it to exit?
	// Would that ever be necessary or desired?
	for (std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
	{
		if (cardPile->cardArray.size() < Card::Value::NUM_VALUES)
			continue;

		if (cardPile->cardArray[cardPile->cardArray.size() - 1]->value != Card::Value::ACE)
			continue;

		int start = int(cardPile->cardArray.size()) - int(Card::Value::NUM_VALUES);
		int finish = int(cardPile->cardArray.size()) - 1;
		if (!cardPile->CardsInOrder(start, finish))
			continue;

		bool exitCards = false;
		if (this->difficultyLevel == DifficultyLevel::LOW)
			exitCards = true;
		else if (this->difficultyLevel == DifficultyLevel::MEDIUM)
			exitCards = cardPile->CardsSameColor(start, finish);
		else if (this->difficultyLevel == DifficultyLevel::HARD)
			exitCards = cardPile->CardsSameSuit(start, finish);

		if (exitCards)
		{
			for (int i = 0; i < int(Card::Value::NUM_VALUES); i++)
			{
				std::shared_ptr<Card> card = cardPile->cardArray.back();
				cardPile->cardArray.pop_back();
				card->targetPosition = this->worldExtents.max;
				card->animationRate = 200.0;
				this->exitingCardArray.push_back(card);
			}

			if (cardPile->cardArray.size() > 0)
				cardPile->cardArray[cardPile->cardArray.size() - 1]->orientation = Card::Orientation::FACE_UP;
		}
	}
}

/*virtual*/ bool SpiderSolitaireGame::OnMouseGrabAt(DirectX::XMVECTOR worldPoint)
{
	assert(this->movingCardPile.get() == nullptr);

	std::shared_ptr<CardPile> foundCardPile;
	int foundCardOffset = -1;
	if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
	{
		Card* card = foundCardPile->cardArray[foundCardOffset].get();
		if (card->orientation == Card::Orientation::FACE_UP)
		{
			int start = foundCardOffset;
			int finish = int(foundCardPile->cardArray.size()) - 1;
			if (foundCardPile->CardsInOrder(start, finish))
			{
				bool canMoveCards = false;

				if (this->difficultyLevel == DifficultyLevel::LOW)
					canMoveCards = true;
				else if (this->difficultyLevel == DifficultyLevel::MEDIUM)
					canMoveCards = foundCardPile->CardsSameColor(start, finish);
				else if (this->difficultyLevel == DifficultyLevel::HARD)
					canMoveCards = foundCardPile->CardsSameSuit(start, finish);

				if (canMoveCards)
				{
					this->StartCardMoving(foundCardPile, foundCardOffset, worldPoint);
					return true;
				}
			}
		}
	}

	return false;
}

/*virtual*/ bool SpiderSolitaireGame::OnMouseReleaseAt(DirectX::XMVECTOR worldPoint)
{
	if (!this->movingCardPile.get())
		return false;
	
	bool moveCards = false;
	std::shared_ptr<CardPile> foundCardPile;
	int foundCardOffset = -1;
	if (this->FindCardAndPile(worldPoint, foundCardPile, foundCardOffset))
	{
		const Card* card = foundCardPile->cardArray[foundCardOffset].get();
		if (foundCardOffset == foundCardPile->cardArray.size() - 1 &&
			int(card->value) - 1 == int(this->movingCardPile->cardArray[0]->value))
		{
			moveCards = true;
		}
	}
	else if (this->FindEmptyPile(worldPoint, foundCardPile))
	{
		moveCards = true;
	}

	this->FinishCardMoving(foundCardPile, moveCards);
	return moveCards;
}

/*virtual*/ void SpiderSolitaireGame::OnMouseMove(DirectX::XMVECTOR worldPoint)
{
	this->ManageCardMoving(worldPoint);
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

/*virtual*/ bool SpiderSolitaireGame::GameWon() const
{
	for (const std::shared_ptr<CardPile>& cardPile : this->cardPileArray)
		if (cardPile->cardArray.size() > 0)
			return false;

	if (this->cardArray.size() > 0)
		return false;

	if (this->exitingCardArray.size() > 0)
		return false;

	return true;
}
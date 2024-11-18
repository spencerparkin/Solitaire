#pragma once

#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <random>
#include "Box.h"

// TODO: It wouldn't be hard to add undo/redo to any game type.
//       Just add a virtual clone method and then keep a history
//       list of game snap-shots.
class SolitaireGame
{
public:
	SolitaireGame(const Box& worldExtents, const Box& cardSize);
	virtual ~SolitaireGame();

	class Card;

	virtual std::shared_ptr<SolitaireGame> AllocNew() const = 0;
	virtual std::shared_ptr<SolitaireGame> Clone() const;
	virtual void NewGame() = 0;
	virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const;
	virtual void Clear();
	virtual bool OnMouseGrabAt(DirectX::XMVECTOR worldPoint) = 0;
	virtual void OnMouseReleaseAt(DirectX::XMVECTOR worldPoint) = 0;
	virtual void OnMouseMove(DirectX::XMVECTOR worldPoint) = 0;
	virtual void OnCardsNeeded() = 0;
	virtual void OnKeyUp(uint32_t keyCode) = 0;
	virtual void Tick(double deltaTimeSeconds);
	virtual bool GameWon() const = 0;

	class Card
	{
	public:
		Card();
		virtual ~Card();

		std::string GetRenderKey() const;
		bool ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const;
		void Tick(double deltaTimeSeconds);

		enum Value
		{
			ACE,
			TWO,
			THREE,
			FOUR,
			FIVE,
			SIX,
			SEVEN,
			EIGHT,
			NINE,
			TEN,
			JACK,
			QUEEN,
			KING,
			NUM_VALUES
		};

		enum Suit
		{
			SPADES,
			CLUBS,
			DIAMONDS,
			HEARTS,
			NUM_SUITS
		};

		enum Color
		{
			BLACK,
			RED
		};

		enum Orientation
		{
			FACE_UP,
			FACE_DOWN
		};

		Color GetColor() const;

		Value value;
		Suit suit;
		Orientation orientation;
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR targetPosition;
		double animationRate;
	};

	class CardPile
	{
	public:
		CardPile();
		virtual ~CardPile();

		virtual std::shared_ptr<CardPile> AllocNew() const = 0;
		virtual std::shared_ptr<CardPile> Clone() const;
		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const = 0;
		virtual void LayoutCards(const Box& cardSize) = 0;

		bool CardsInOrder(int start, int finish) const;
		bool CardsSameColor(int start, int finish) const;
		bool CardsSameSuit(int start, int finish) const;
		bool CardsAlternateColor(int start, int finish) const;
		bool IndexValid(int i) const;
		bool ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const;

		std::vector<std::shared_ptr<Card>> cardArray;
		DirectX::XMVECTOR position;
		std::shared_ptr<Card> emptyCard;
	};

	class CascadingCardPile : public CardPile
	{
	public:
		enum CascadeDirection
		{
			DOWN,
			RIGHT
		};

		CascadingCardPile();
		CascadingCardPile(CascadeDirection cascadeDirection, int cascadeNumber);
		virtual ~CascadingCardPile();

		virtual std::shared_ptr<CardPile> AllocNew() const override;
		virtual std::shared_ptr<CardPile> Clone() const override;
		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const override;
		virtual void LayoutCards(const Box& cardSize) override;

		CascadeDirection cascadeDirection;
		int cascadeNumber;
	};

	class SingularCardPile : public CardPile
	{
	public:
		SingularCardPile();
		virtual ~SingularCardPile();

		virtual std::shared_ptr<CardPile> AllocNew() const override;
		virtual std::shared_ptr<CardPile> Clone() const override;
		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const override;
		virtual void LayoutCards(const Box& cardSize) override;
	};

protected:

	static void GenerateDeck(std::vector<std::shared_ptr<Card>>& cardArray);
	static void SuffleCards(std::vector<std::shared_ptr<Card>>& cardArray);
	static int RandomInteger(int min, int max);

	bool FindCardInPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile> givenCardPile, int& foundCardOffset);
	bool FindCardAndPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile>& foundCardPile, int& foundCardOffset);
	bool FindEmptyPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile>& foundCardPile);

	void StartCardMoving(std::shared_ptr<CardPile> cardPile, int grabOffset, DirectX::XMVECTOR grabPoint);
	void FinishCardMoving(std::shared_ptr<CardPile> targetPile, bool commitMove);
	void ManageCardMoving(DirectX::XMVECTOR grabPoint);

	std::vector<std::shared_ptr<CardPile>> cardPileArray;
	std::shared_ptr<CardPile> movingCardPile;
	Box cardSize;
	Box worldExtents;
	DirectX::XMVECTOR grabDelta;
	std::shared_ptr<CardPile> originCardPile;
};
#pragma once

#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <random>
#include "Box.h"

class SolitaireGame
{
public:
	SolitaireGame(const Box& worldExtents, const Box& cardSize);
	virtual ~SolitaireGame();

	class Card;

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
			NUM_SUITES
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

		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const = 0;
		virtual void LayoutCards(const Box& cardSize) = 0;

		bool CardsInOrder(int start, int finish) const;
		bool CardsSameColor(int start, int finish) const;
		bool CardsSameSuit(int start, int finish) const;
		bool IndexValid(int i) const;
		bool ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const;

		std::vector<std::shared_ptr<Card>> cardArray;
		DirectX::XMVECTOR position;
	};

	class CascadingCardPile : public CardPile
	{
	public:
		CascadingCardPile();
		virtual ~CascadingCardPile();

		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const override;
		virtual void LayoutCards(const Box& cardSize) override;

		enum CascadeDirection
		{
			DOWN,
			RIGHT
		};

		CascadeDirection cascadeDirection;
	};

	class SingularCardPile : public CardPile
	{
	public:
		SingularCardPile();
		virtual ~SingularCardPile();

		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const override;
		virtual void LayoutCards(const Box& cardSize) override;
	};

protected:

	static void GenerateDeck(std::vector<std::shared_ptr<Card>>& cardArray);
	static void SuffleCards(std::vector<std::shared_ptr<Card>>& cardArray);
	static int RandomInteger(int min, int max);

	bool FindCardAndPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile>& foundCardPile, int& foundCardOffset);
	bool FindEmptyPile(DirectX::XMVECTOR worldPoint, std::shared_ptr<CardPile>& foundCardPile);

	std::vector<std::shared_ptr<CardPile>> cardPileArray;
	std::shared_ptr<CardPile> movingCardPile;
	Box cardSize;
	Box worldExtents;
	DirectX::XMVECTOR grabDelta;
	std::shared_ptr<CardPile> originCardPile;
};
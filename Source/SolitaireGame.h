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
	virtual void OnGrabAt(DirectX::XMVECTOR worldPoint) = 0;
	virtual void OnReleaseAt(DirectX::XMVECTOR worldPoint) = 0;

	class Card
	{
	public:
		Card();
		virtual ~Card();

		std::string GetRenderKey() const;
		bool ContainsPoint(DirectX::XMVECTOR point, const Box& cardSize) const;

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
			KING,
			QUEEN,
			NUM_VALUES
		};

		enum Suite
		{
			SPADES,
			CLUBS,
			DIAMONDS,
			HEARTS,
			NUM_SUITES
		};

		enum Orientation
		{
			FACE_UP,
			FACE_DOWN
		};

		Value value;
		Suite suite;
		Orientation orientation;
		DirectX::XMVECTOR position;
	};

	class CardPile
	{
	public:
		CardPile();
		virtual ~CardPile();

		virtual void GenerateRenderList(std::vector<const Card*>& cardRenderList) const = 0;
		virtual void LayoutCards(const Box& cardSize) = 0;

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

	std::vector<std::shared_ptr<CardPile>> cardPileArray;
	std::shared_ptr<CardPile> movingCardPile;
	Box cardSize;
	Box worldExtents;
};
#pragma once

#include <DirectXMath.h>

class Box
{
public:
	Box();
	Box(const Box& box);
	virtual ~Box();

	void operator=(const Box& box);

	double GetWidth() const;
	double GetHeight() const;
	double GetAspectRatio() const;
	DirectX::XMVECTOR PointToUVs(DirectX::XMVECTOR point) const;
	DirectX::XMVECTOR PointFromUVs(DirectX::XMVECTOR uvs) const;
	bool ContainsPoint(DirectX::XMVECTOR point) const;

	void ExpandToMatchAspectRatio(double aspectRatio);

public:
	DirectX::XMVECTOR min;
	DirectX::XMVECTOR max;
};
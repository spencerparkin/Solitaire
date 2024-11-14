#pragma once

#include <DirectXMath.h>

class Box
{
public:
	Box();
	virtual ~Box();

	double GetWidth() const;
	double GetHeight() const;
	double GetAspectRatio() const;
	
	void ExpandToMatchAspectRatio(double aspectRatio);

	DirectX::XMVECTOR min;
	DirectX::XMVECTOR max;
};
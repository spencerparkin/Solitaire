#include "Box.h"

using namespace DirectX;

Box::Box()
{
	this->min = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	this->max = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

/*virtual*/ Box::~Box()
{
}

double Box::GetWidth() const
{
	return XMVectorGetX(this->max) - XMVectorGetX(this->min);
}

double Box::GetHeight() const
{
	return XMVectorGetY(this->max) - XMVectorGetY(this->min);
}

double Box::GetAspectRatio() const
{
	return this->GetWidth() / this->GetHeight();
}

void Box::ExpandToMatchAspectRatio(double aspectRatio)
{
	double currentAspectRatio = this->GetAspectRatio();

	XMVECTOR delta = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	if (currentAspectRatio < aspectRatio)
		delta = XMVectorSet((this->GetHeight() * aspectRatio - this->GetWidth()) / 2.0, 0.0f, 0.0f, 1.0f);
	else if (currentAspectRatio > aspectRatio)
		delta = XMVectorSet(0.0f, (this->GetWidth() / aspectRatio - this->GetHeight()) / 2.0, 0.0f, 1.0f);

	this->min = XMVectorSubtract(this->min, delta);
	this->max = XMVectorAdd(this->max, delta);

#if defined _DEBUG
	double newAspectRatio = this->GetAspectRatio();
	double eps = 1e-5;
	assert(::fabsf(newAspectRatio - aspectRatio) < eps);
#endif
}
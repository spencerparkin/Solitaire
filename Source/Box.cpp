#include "Box.h"

using namespace DirectX;

Box::Box()
{
	this->min = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	this->max = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

Box::Box(const Box& box)
{
	this->min = box.min;
	this->max = box.max;
}

/*virtual*/ Box::~Box()
{
}

void Box::operator=(const Box& box)
{
	this->min = box.min;
	this->max = box.max;
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

XMVECTOR Box::PointToUVs(XMVECTOR point) const
{
	return XMVectorSet(
		(XMVectorGetX(point) - XMVectorGetX(this->min)) / this->GetWidth(),
		(XMVectorGetY(point) - XMVectorGetY(this->min)) / this->GetHeight(),
		0.0f,
		1.0f
	);
}

XMVECTOR Box::PointFromUVs(XMVECTOR uvs) const
{
	return XMVectorSet(
		XMVectorGetX(min) + XMVectorGetX(uvs) * this->GetWidth(),
		XMVectorGetY(min) + XMVectorGetY(uvs) * this->GetHeight(),
		0.0f,
		1.0f
	);
}

bool Box::ContainsPoint(DirectX::XMVECTOR point) const
{
	if (XMVectorGetX(point) < XMVectorGetX(this->min))
		return false;

	if (XMVectorGetX(point) > XMVectorGetX(this->max))
		return false;

	if (XMVectorGetY(point) < XMVectorGetY(this->min))
		return false;

	if (XMVectorGetY(point) > XMVectorGetY(this->max))
		return false;

	return true;
}

XMVECTOR Box::GetCenter() const
{
	return (this->min + this->max) / 2.0f;
}

void Box::ScaleAboutCenter(double scaleFactor)
{
	XMVECTOR center = this->GetCenter();
	XMVECTOR delta = this->max - center;
	delta *= scaleFactor;
	this->min = center - delta;
	this->max = center + delta;
}
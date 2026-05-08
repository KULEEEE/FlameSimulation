#include "Particle.h"
#include <math.h>


using namespace DirectX;

const XMFLOAT2 Particles::mTexcoord[Particles::particlevert] = {
	XMFLOAT2(1.0f, 1.0f),
	XMFLOAT2(1.0f, 0.0f),
	XMFLOAT2(0.0f, 1.0f),
	XMFLOAT2(0.0f, 0.0f),
};

Particles::Particles(int num, float dt, float size, XMFLOAT3 initPosition)
{
	mNumParticles = num;
	mParticleSize = size;
	mTimeStep = dt;

	mCurrParticle.resize(num);
	minitPosition = initPosition;

	const XMVECTOR initVec = XMVectorSet(minitPosition.x, minitPosition.y, minitPosition.z, 1.0f);
	const XMVECTOR zero = XMVectorZero();
	const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	for (int i = 0; i < num; i++)
	{
		mCurrParticle[i].midPoint = initVec;
		mCurrParticle[i].lifespan = 0.0f;
		mCurrParticle[i].fade = float(rand() % 100) / 1000.0f + 0.003f;
		mCurrParticle[i].speed = XMFLOAT3(float((rand() % 50) - 25.0f) * 10.0f, 0.0f, 0.0f);
		mCurrParticle[i].gravity = XMFLOAT3(0.0f, 0.8f, 0.0f);

		makeParticle(i, zero, up);
	}
}

Particles::~Particles()
{
}

void Particles::Update(float dt, XMVECTOR eyePos, XMVECTOR up, float particleSize, float temp, float width)
{
	mParticleSize = particleSize;

	// The simulation is split into 4 "lobes" with slightly different vertical
	// damping factors so the visible flame has asymmetric motion. Lobes 0 and 3
	// damp the y-component more (divisor 2000) than lobes 1 and 2 (divisor 1500).
	struct LobeParams { int begin; int end; float yDivisor; };
	const LobeParams lobes[4] = {
		{ 0,                    mNumParticles / 4,     2000.0f },
		{ mNumParticles / 4,    mNumParticles * 2 / 4, 1500.0f },
		{ mNumParticles * 2 / 4,mNumParticles * 3 / 4, 1500.0f },
		{ mNumParticles * 3 / 4,mNumParticles,         2000.0f },
	};

	for (const LobeParams& lobe : lobes)
	{
		const float invY = 1.0f / lobe.yDivisor;
		for (int i = lobe.begin; i < lobe.end; ++i)
		{
			particle& p = mCurrParticle[i];

			p.midPoint = XMVectorAdd(p.midPoint,
				XMVectorSet(p.speed.x / 1500.0f, p.speed.y * invY, p.speed.z / 1500.0f, 0.0f));

			const float mx = XMVectorGetX(p.midPoint);
			const float my = XMVectorGetY(p.midPoint);
			const float mz = XMVectorGetZ(p.midPoint);

			if (my > minitPosition.y)
			{
				if (mx > minitPosition.x)       p.gravity.x = -width;
				else if (mx < minitPosition.x)  p.gravity.x =  width;
				else                            p.gravity.x = MathHelper::RandF(-0.5f, 0.5f);

				if (mz > minitPosition.z)       p.gravity.z = -width;
				else if (mz < minitPosition.z)  p.gravity.z =  width;
				else                            p.gravity.z = MathHelper::RandF(-0.5f, 0.5f);
			}
			else
			{
				p.gravity.x = MathHelper::RandF(-0.5f, 0.5f);
				p.gravity.z = MathHelper::RandF(-0.5f, 0.5f);
			}

			p.speed.x += p.gravity.x;
			p.speed.y += p.gravity.y;
			p.speed.z += p.gravity.z;

			p.lifespan -= p.fade;
			if (p.lifespan < 0.0f)
			{
				p.lifespan = MathHelper::RandF(0.6f, 1.0f);
				p.fade = float(rand() % 100) / 300.0f + 0.015f;
				p.midPoint = XMVectorSet(
					minitPosition.x + MathHelper::RandF(temp, -temp),
					minitPosition.y,
					minitPosition.z + MathHelper::RandF(temp, -temp), 0.0f);
				p.speed = XMFLOAT3(
					float((rand() % 70) - 35.0f),
					float((rand() % 60) - 30.0f),
					float((rand() % 70) - 35.0f));
				p.gravity.y = 3.0f;
			}

			makeParticle(i, eyePos, up);
		}
	}
}

void Particles::makeParticle(int i, XMVECTOR eyePos, XMVECTOR up)
{
	particle& p = mCurrParticle[i];

	const XMVECTOR epv = XMVector3Normalize(XMVectorSubtract(eyePos, p.midPoint));
	const XMVECTOR a1  = XMVector3Normalize(up);
	const XMVECTOR a2  = XMVector3Cross(epv, a1);

	const XMVECTOR du1 = XMVectorScale(a1, mParticleSize);
	const XMVECTOR du2 = XMVectorScale(a2, mParticleSize);

	p.position[0] = XMVectorAdd(p.midPoint,  du1);
	p.position[1] = XMVectorSubtract(p.midPoint, du2);
	p.position[2] = XMVectorAdd(p.midPoint,  du2);
	p.position[3] = XMVectorSubtract(p.midPoint, du1);
}

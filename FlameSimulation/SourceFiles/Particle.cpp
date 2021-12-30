#include "Particle.h"
#include <algorithm>
#include <math.h>
#include <vector>


using namespace DirectX;

Particles::Particles(int num, float dt, float size, XMFLOAT3 initPosition)
{
	mNumParticles = num;

	mParticleSize = size;

	mTimeStep = dt;
	mTime = 0;

	mCurrParticle.resize(num);
	minitPosition = XMFLOAT3(initPosition.x, initPosition.y, initPosition.z);


	for (int i = 0; i < num; i++)
	{

		mCurrParticle[i].midPoint = XMVectorSet(minitPosition.x,minitPosition.y,minitPosition.z,1.0f);
		mCurrParticle[i].lifespan = 0.0f;
		mCurrParticle[i].fade = float(rand() % 100) / 1000.0f + 0.003f;
		mCurrParticle[i].speed = XMFLOAT3(float((rand() % 50) - 25.0f) * 10.0f, 0.0f, 0.0f);
		mCurrParticle[i].gravity = XMFLOAT3(0.0f, 0.8f, 0.0f);
		

		makeParticle(i, XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),XMVectorSet(0.0f,1.0f,0.0f,0.0f));
	}
	mVertexCount = mCurrParticle.size() * 4;

}
Particles::~Particles()
{
}



int Particles::VertexCount()
{
	return mVertexCount;
}

void Particles::Update(float dt, XMVECTOR eyePos, XMVECTOR up, float particleSize, float temp, float width)
{
	
		for (int i = 0; i < (mNumParticles)/4; i++)
		{

			//mCurrParticle[i].midPoint = XMVectorSet(i * 0.5f, 1.0f, i * -1.0f, 0.0f);
			//mCurrParticle[i].lifespan = 1.0f;



			mCurrParticle[i].midPoint = XMVectorAdd(mCurrParticle[i].midPoint, XMVectorSet(mCurrParticle[i].speed.x / 1500, mCurrParticle[i].speed.y / 2000, mCurrParticle[i].speed.z / 1500, 0.0f));




			if ((XMVectorGetX(mCurrParticle[i].midPoint) > minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{

				mCurrParticle[i].gravity.x = -width;
			}
			else if ((XMVectorGetX(mCurrParticle[i].midPoint) < minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.x = width;
			}
			else
			{
				mCurrParticle[i].gravity.x = MathHelper::RandF(-0.5f, 0.5f);
			}


			if ((XMVectorGetZ(mCurrParticle[i].midPoint) > minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = -width;
			}
			else if ((XMVectorGetZ(mCurrParticle[i].midPoint) < minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = width;
			}
			else
			{
				mCurrParticle[i].gravity.z = MathHelper::RandF(-0.5f, 0.5f);
			}




			mCurrParticle[i].speed = XMFLOAT3(mCurrParticle[i].speed.x + mCurrParticle[i].gravity.x, mCurrParticle[i].speed.y + mCurrParticle[i].gravity.y, mCurrParticle[i].speed.z + mCurrParticle[i].gravity.z);
			mCurrParticle[i].lifespan -= mCurrParticle[i].fade;
			if (mCurrParticle[i].lifespan < 0.0f)
			{
				mCurrParticle[i].lifespan = MathHelper::RandF(0.6f, 1.0f);
				mCurrParticle[i].fade = float(rand() % 100) / 300.0f + 0.015f;
				mCurrParticle[i].midPoint = XMVectorSet(minitPosition.x + MathHelper::RandF(temp, -temp), minitPosition.y, minitPosition.z + MathHelper::RandF(temp, -temp), 0.0f);
				mCurrParticle[i].speed = XMFLOAT3(float((rand() % 70) - 35.0f), float((rand() % 60) - 30.0f), float((rand() % 70) - 35.0f));
				mCurrParticle[i].gravity.y = 3.0f;
			}
			mParticleSize = particleSize;
			//mParticleSize = particleSize*mCurrParticle[i].sizeWeight*(1-mCurrParticle[i].lifespan)/ (2-mCurrParticle[i].lifespan)*3.0f;
			makeParticle(i, eyePos, up);
		}

		for (int i = (mNumParticles); i < (mNumParticles)*2 / 4; i++)
		{

			//mCurrParticle[i].midPoint = XMVectorSet(i * 0.5f, 1.0f, i * -1.0f, 0.0f);
			//mCurrParticle[i].lifespan = 1.0f;



			mCurrParticle[i].midPoint = XMVectorAdd(mCurrParticle[i].midPoint, XMVectorSet(mCurrParticle[i].speed.x / 1500, mCurrParticle[i].speed.y / 1500, mCurrParticle[i].speed.z / 1500, 0.0f));




			if ((XMVectorGetX(mCurrParticle[i].midPoint) > minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{

				mCurrParticle[i].gravity.x = -width;
			}
			else if ((XMVectorGetX(mCurrParticle[i].midPoint) < minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.x = width;
			}
			else
			{
				mCurrParticle[i].gravity.x = MathHelper::RandF(-0.5f, 0.5f);
			}


			if ((XMVectorGetZ(mCurrParticle[i].midPoint) > minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = -width;
			}
			else if ((XMVectorGetZ(mCurrParticle[i].midPoint) < minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = width;
			}
			else
			{
				mCurrParticle[i].gravity.z = MathHelper::RandF(-0.5f, 0.5f);
			}




			mCurrParticle[i].speed = XMFLOAT3(mCurrParticle[i].speed.x + mCurrParticle[i].gravity.x, mCurrParticle[i].speed.y + mCurrParticle[i].gravity.y, mCurrParticle[i].speed.z + mCurrParticle[i].gravity.z);
			mCurrParticle[i].lifespan -= mCurrParticle[i].fade;
			if (mCurrParticle[i].lifespan < 0.0f)
			{
				mCurrParticle[i].lifespan = MathHelper::RandF(0.6f, 1.0f);
				mCurrParticle[i].fade = float(rand() % 100) / 300.0f + 0.015f;
				mCurrParticle[i].midPoint = XMVectorSet(minitPosition.x + MathHelper::RandF(temp, -temp), minitPosition.y, minitPosition.z + MathHelper::RandF(temp, -temp), 0.0f);
				mCurrParticle[i].speed = XMFLOAT3(float((rand() % 70) - 35.0f), float((rand() % 60) - 30.0f), float((rand() % 70) - 35.0f));
				mCurrParticle[i].gravity.y = 3.0f;
			}
			mParticleSize = particleSize;
			//mParticleSize = particleSize*mCurrParticle[i].sizeWeight*(1-mCurrParticle[i].lifespan)/ (2-mCurrParticle[i].lifespan)*3.0f;
			makeParticle(i, eyePos, up);
		}

		for (int i = (mNumParticles) *2 / 4; i < (mNumParticles) * 3 / 4; i++)
		{

			//mCurrParticle[i].midPoint = XMVectorSet(i * 0.5f, 1.0f, i * -1.0f, 0.0f);
			//mCurrParticle[i].lifespan = 1.0f;



			mCurrParticle[i].midPoint = XMVectorAdd(mCurrParticle[i].midPoint, XMVectorSet(mCurrParticle[i].speed.x / 1500, mCurrParticle[i].speed.y / 1500, mCurrParticle[i].speed.z / 1500, 0.0f));




			if ((XMVectorGetX(mCurrParticle[i].midPoint) > minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{

				mCurrParticle[i].gravity.x = -width;
			}
			else if ((XMVectorGetX(mCurrParticle[i].midPoint) < minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.x = width;
			}
			else
			{
				mCurrParticle[i].gravity.x = MathHelper::RandF(-0.5f, 0.5f);
			}


			if ((XMVectorGetZ(mCurrParticle[i].midPoint) > minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = -width;
			}
			else if ((XMVectorGetZ(mCurrParticle[i].midPoint) < minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = width;
			}
			else
			{
				mCurrParticle[i].gravity.z = MathHelper::RandF(-0.5f, 0.5f);
			}




			mCurrParticle[i].speed = XMFLOAT3(mCurrParticle[i].speed.x + mCurrParticle[i].gravity.x, mCurrParticle[i].speed.y + mCurrParticle[i].gravity.y, mCurrParticle[i].speed.z + mCurrParticle[i].gravity.z);
			mCurrParticle[i].lifespan -= mCurrParticle[i].fade;
			if (mCurrParticle[i].lifespan < 0.0f)
			{
				mCurrParticle[i].lifespan = MathHelper::RandF(0.6f, 1.0f);
				mCurrParticle[i].fade = float(rand() % 100) / 300.0f + 0.015f;
				mCurrParticle[i].midPoint = XMVectorSet(minitPosition.x + MathHelper::RandF(temp, -temp), minitPosition.y, minitPosition.z + MathHelper::RandF(temp, -temp), 0.0f);
				mCurrParticle[i].speed = XMFLOAT3(float((rand() % 70) - 35.0f), float((rand() % 60) - 30.0f), float((rand() % 70) - 35.0f));
				mCurrParticle[i].gravity.y = 3.0f;
			}
			mParticleSize = particleSize;
			//mParticleSize = particleSize*mCurrParticle[i].sizeWeight*(1-mCurrParticle[i].lifespan)/ (2-mCurrParticle[i].lifespan)*3.0f;
			makeParticle(i, eyePos, up);
		}

		for (int i = (mNumParticles) *3 / 4; i < (mNumParticles); i++)
		{

			//mCurrParticle[i].midPoint = XMVectorSet(i * 0.5f, 1.0f, i * -1.0f, 0.0f);
			//mCurrParticle[i].lifespan = 1.0f;



			mCurrParticle[i].midPoint = XMVectorAdd(mCurrParticle[i].midPoint, XMVectorSet(mCurrParticle[i].speed.x / 1500, mCurrParticle[i].speed.y / 2000, mCurrParticle[i].speed.z / 1500, 0.0f));




			if ((XMVectorGetX(mCurrParticle[i].midPoint) > minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{

				mCurrParticle[i].gravity.x = -width;
			}
			else if ((XMVectorGetX(mCurrParticle[i].midPoint) < minitPosition.x) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.x = width;
			}
			else
			{
				mCurrParticle[i].gravity.x = MathHelper::RandF(-0.5f, 0.5f);
			}


			if ((XMVectorGetZ(mCurrParticle[i].midPoint) > minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = -width;
			}
			else if ((XMVectorGetZ(mCurrParticle[i].midPoint) < minitPosition.z) && (XMVectorGetY(mCurrParticle[i].midPoint) > minitPosition.y))
			{
				mCurrParticle[i].gravity.z = width;
			}
			else
			{
				mCurrParticle[i].gravity.z = MathHelper::RandF(-0.5f, 0.5f);
			}




			mCurrParticle[i].speed = XMFLOAT3(mCurrParticle[i].speed.x + mCurrParticle[i].gravity.x, mCurrParticle[i].speed.y + mCurrParticle[i].gravity.y, mCurrParticle[i].speed.z + mCurrParticle[i].gravity.z);
			mCurrParticle[i].lifespan -= mCurrParticle[i].fade;
			if (mCurrParticle[i].lifespan < 0.0f)
			{
				mCurrParticle[i].lifespan = MathHelper::RandF(0.6f,1.0f);
				mCurrParticle[i].fade = float(rand() % 100) / 300.0f + 0.015f;
				mCurrParticle[i].midPoint = XMVectorSet(minitPosition.x + MathHelper::RandF(temp, -temp), minitPosition.y , minitPosition.z + MathHelper::RandF(temp, -temp), 0.0f);
				mCurrParticle[i].speed = XMFLOAT3(float((rand() % 70) - 35.0f), float((rand() % 60) - 30.0f), float((rand() % 70) - 35.0f));
				mCurrParticle[i].gravity.y = 3.0f;
			}
			mParticleSize = particleSize;
			//mParticleSize = particleSize*mCurrParticle[i].sizeWeight*(1-mCurrParticle[i].lifespan)/ (2-mCurrParticle[i].lifespan)*3.0f;
			makeParticle(i, eyePos, up);
		}

}
void Particles::makeParticle(int i, XMVECTOR eyePos, XMVECTOR up)
{
	//XMVECTOR ep = XMVectorSet(2.0f, 1.0f, 1.0f, 0.0f);

	XMVECTOR epv = XMVector3Normalize(XMVectorAdd(eyePos, XMVectorNegate(mCurrParticle[i].midPoint)));
	XMVECTOR a1 = XMVector3Normalize(up);
	XMVECTOR a2 = XMVector3Cross(epv, a1);

	

	mCurrParticle[i].position.clear();
	mCurrParticle[i].texcoord.clear();


	
	
	mCurrParticle[i].texcoord.push_back(XMFLOAT2(1.0f, 1.0f));
	mCurrParticle[i].texcoord.push_back(XMFLOAT2(1.0f,0.0f));
	mCurrParticle[i].texcoord.push_back(XMFLOAT2(0.0f, 1.0f));
	mCurrParticle[i].texcoord.push_back(XMFLOAT2(0.0f, 0.0f));
	
	
	mCurrParticle[i].position.push_back(XMVectorAdd(XMVectorSet(mParticleSize * XMVectorGetX(a1), mParticleSize * XMVectorGetY(a1), mParticleSize * XMVectorGetZ(a1), 0.0f), mCurrParticle[i].midPoint));
	mCurrParticle[i].position.push_back(XMVectorAdd(XMVectorSet(-mParticleSize * XMVectorGetX(a2), -mParticleSize * XMVectorGetY(a2), -mParticleSize * XMVectorGetZ(a2), 0.0f), mCurrParticle[i].midPoint));
	mCurrParticle[i].position.push_back(XMVectorAdd(XMVectorSet(mParticleSize * XMVectorGetX(a2), mParticleSize * XMVectorGetY(a2), mParticleSize * XMVectorGetZ(a2), 0.0f), mCurrParticle[i].midPoint));
	mCurrParticle[i].position.push_back(XMVectorAdd(XMVectorSet(-mParticleSize * XMVectorGetX(a1), -mParticleSize * XMVectorGetY(a1), -mParticleSize * XMVectorGetZ(a1), 0.0f), mCurrParticle[i].midPoint));

}

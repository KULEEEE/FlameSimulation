#pragma once

#include <vector>
#include <DirectXMath.h>
#include "../Common/MathHelper.h"


using namespace std;

class Particles
{

public:
	static const int particlevert = 4;

	Particles(int num, float dt, float size, DirectX::XMFLOAT3 initPosition);
	Particles(const Particles& rhs) = delete;
	Particles& operator=(const Particles& rhs) = delete;
	~Particles();


	const DirectX::XMVECTOR& Position(int i, int j)const { return mCurrParticle[i].position[j]; }
	const DirectX::XMVECTOR& MidPosition(int i)const { return mCurrParticle[i].midPoint; }
	const float Lifespan(int i) const{ return mCurrParticle[i].lifespan; }
	const DirectX::XMFLOAT2& Texcoord(int j)const { return mTexcoord[j]; }
	const int Num() const { return mNumParticles; }

	void Update(float dt, DirectX::XMVECTOR eyePos, DirectX::XMVECTOR up,float particleSize, float temp, float width);
	int VertexCount() const { return mNumParticles * particlevert; }
	void makeParticle(int i, DirectX::XMVECTOR eyePos, DirectX::XMVECTOR up);
private:
	int mNumParticles = 0;
	float mParticleSize = 0.0f;
	float mTimeStep = 0.0f;

	DirectX::XMFLOAT3 minitPosition = DirectX::XMFLOAT3(0.0f,0.0f,0.0f);

	struct particle
	{
		DirectX::XMVECTOR midPoint;
		DirectX::XMVECTOR position[particlevert];

		DirectX::XMFLOAT3 speed;
		DirectX::XMFLOAT3 gravity;
		float lifespan;
		float fade;
	};

	vector<particle> mCurrParticle;

	// Quad texcoords are constant per particle; store once for the whole class.
	static const DirectX::XMFLOAT2 mTexcoord[particlevert];
};

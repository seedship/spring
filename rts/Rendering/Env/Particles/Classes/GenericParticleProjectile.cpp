/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "GenericParticleProjectile.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/creg/DefTypes.h"

CR_BIND_DERIVED(CGenericParticleProjectile, CProjectile, )

CR_REG_METADATA(CGenericParticleProjectile,(
	CR_MEMBER(gravity),
	CR_IGNORED(texture),
	CR_MEMBER(colorMap),
	CR_MEMBER(directional),
	CR_MEMBER(life),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(airdrag),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod),
	CR_SERIALIZER(Serialize)
))


CGenericParticleProjectile::CGenericParticleProjectile(const CUnit* owner, const float3& pos, const float3& speed, const CProjectile* parent)
	: CProjectile(pos, speed, owner, false, false, false)

	, gravity(ZeroVector)
	, texture(nullptr)
	, colorMap(nullptr)
	, directional(false)
	, life(0.0f)
	, decayrate(0.0f)
	, size(0.0f)
	, airdrag(0.0f)
	, sizeGrowth(0.0f)
	, sizeMod(0.0f)
{
	useAirLos = parent->useAirLos;
	checkCol  = parent->checkCol;

	// ugly workaround
	rotParams = parent->rotParams;
	rotParams *= float3(math::DEG_TO_RAD / GAME_SPEED, math::DEG_TO_RAD / (GAME_SPEED * GAME_SPEED), math::DEG_TO_RAD);
	UpdateRotation();

	animParams = parent->animParams;

	deleteMe  = false;
}

void CGenericParticleProjectile::Serialize(creg::ISerializer* s)
{
	std::string name;
	if (s->IsWriting())
		name = projectileDrawer->textureAtlas->GetTextureName(texture);
	creg::GetType(name)->Serialize(s, &name);
	if (!s->IsWriting())
		texture = projectileDrawer->textureAtlas->GetTexturePtr(name);
}

void CGenericParticleProjectile::Update()
{
	SetPosition(pos + speed);
	SetVelocityAndSpeed((speed + gravity) * airdrag);

	life += decayrate;
	size = size * sizeMod + sizeGrowth;

	deleteMe |= (life > 1.0f);
}

void CGenericParticleProjectile::Draw()
{
	UpdateRotation();
	UpdateAnimParams();

	float3 zdir;
	float3 ydir;
	float3 xdir;
	const bool shadowPass = (camera->GetCamType() == CCamera::CAMTYPE_SHADOW);
	if (directional && !shadowPass) {
		zdir = (pos - camera->GetPos()).SafeANormalize();
		if (math::fabs(zdir.dot(speed)) < 0.99f) {
			ydir = zdir.cross(speed).SafeANormalize();
			xdir = ydir.cross(zdir);
		}
		else {
			zdir = camera->GetForward();
			xdir = camera->GetRight();
			ydir = camera->GetUp();
		}
	} else {
		zdir = camera->GetForward();
		xdir = camera->GetRight();
		ydir = camera->GetUp();
	}

	std::array<float3, 4> bounds = {
		-ydir * size - xdir * size,
		-ydir * size + xdir * size,
		 ydir * size + xdir * size,
		 ydir * size - xdir * size
	};

	if (math::fabs(rotVal) > 0.01f) {
		float3::rotate<false>(rotVal, zdir, bounds);
	}

	unsigned char color[4];
	colorMap->GetColor(color, life);
	AddEffectsQuad(
		{ drawPos + bounds[0], texture->xstart, texture->ystart, color },
		{ drawPos + bounds[1], texture->xend,   texture->ystart, color },
		{ drawPos + bounds[2], texture->xend,   texture->yend,   color },
		{ drawPos + bounds[3], texture->xstart, texture->yend,   color }
	);
}
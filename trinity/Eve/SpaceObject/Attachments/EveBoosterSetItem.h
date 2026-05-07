#pragma once
#ifndef EveBoosterSetItem_H
#define EveBoosterSetItem_H

BLUE_DECLARE( EveBoosterSetItem );
BLUE_DECLARE_VECTOR( EveBoosterSetItem );

// --------------------------------------------------------------------------------
// Description:
//   Persisted per-booster source data. Holds only the fields needed to reconstruct
//   a booster after a .red file load. Runtime-derived fields (lightPosition,
//   lightRadius, lightPhase) are NOT stored here — they are recomputed by
//   EveBoosterSet2::Add() at load time.
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveBoosterSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveBoosterSetItem( IRoot* lockobj = NULL ) {}

	Matrix      transform;
	Vector4     functionality;
	uint32_t    atlasIndex0;
	uint32_t    atlasIndex1;
	bool        hasTrail;
	float       lightScale;
};

TYPEDEF_BLUECLASS( EveBoosterSetItem );

#endif // EveBoosterSetItem_H



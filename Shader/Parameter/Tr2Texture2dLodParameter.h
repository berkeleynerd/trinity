////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2Texture2dLodParameter_h
#define Tr2Texture2dLodParameter_h

#include "TriTextureParameter.h"
#include "Resources/Tr2LodResourceCache.h"

BLUE_DECLARE( Tr2LodResource );

BLUE_CLASS( Tr2Texture2dLodParameter ) : public TriTextureParameter, public ITr2LodResourceListener
{
public:
	EXPOSE_TO_BLUE();

	Tr2Texture2dLodParameter( IRoot* lockobj = nullptr );
	~Tr2Texture2dLodParameter();

	bool Initialize() override;

	Tr2LodResourcePtr GetLodResource() const;
	void SetLodResource( Tr2LodResource* newLodResource );

protected:
	void OnAddedToMaterial( Tr2Material* material ) override;
	void OnRemovedFromMaterial( Tr2Material* material ) override;

	void OnLodResourceChanged( Tr2LodResource* resource ) override;

	Tr2LodResourcePtr m_lodResource;
	mutable Tr2LodResourceCache<ITr2TextureProvider> m_lodCache;
	std::vector<Tr2Material*> m_materials;
};

TYPEDEF_BLUECLASS( Tr2Texture2dLodParameter );

#endif // Tr2Texture2dLodParameter_h
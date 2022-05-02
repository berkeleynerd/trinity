////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2MeshLod_H
#define Tr2MeshLod_H

#include "Tr2MeshBase.h"
#include "Resources/Tr2LodResource.h"
#include "Resources/Tr2LodResourceCache.h"

BLUE_DECLARE_VECTOR( Tr2LodResource );

BLUE_CLASS( Tr2MeshLod ):
	public Tr2MeshBase,
	public IInitialize,
	public ITr2LodResourceListener,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2MeshLod( IRoot* lockobj = NULL );
	~Tr2MeshLod();

	bool Initialize() override;

	virtual bool IsLoading() const { return false; }

	void GetBatches( ITriRenderBatchAccumulator* batches,
		const Tr2MeshAreaVector* areas,
		const Tr2PerObjectData* data,
		float screenSize = std::numeric_limits<float>::max(),
		ITr2MeshBatchCallback* callback = nullptr ) const override;

	// Selects the given level of detail for the mesh
	void SelectLod( Tr2Lod lod );
	Tr2Lod GetSelectedLod() const { return m_selectedLod; }

	// Add an associated resource, such as a texture used on an area of the mesh.
	// When level of detail is selected, it is also applied to associated
	// resources.
	void AddAssociatedResource( Tr2LodResource* lr );

	// Remove an associated resource.
	void RemoveAssociatedResource( Tr2LodResource* lr );

	// Clear all associated resources.
	void ClearAssociatedResources();

	virtual TriGeometryRes* GetGeometryResource() const;
	Tr2LodResourcePtr GetGeometryLodResource() const;
	void SetGeometryResource( Tr2LodResource* lodResource );
	
	bool IsGeometryUsingSelectedLod() const;

protected:
	void ReleaseCachedData( BlueAsyncRes * p ) override;
	void RebuildCachedData( BlueAsyncRes * p ) override;

	void OnLodResourceChanged( Tr2LodResource * resource ) override;

	Tr2LodResourcePtr m_geometryRes;
	mutable Tr2LodResourceCache<TriGeometryRes> m_geometryCache;
	TriGeometryResPtr m_lowResGeometryResource;

	PTr2LodResourceVector m_associatedResources;
	Tr2Lod m_selectedLod;

	bool m_triedLoadingLowRes;
};

TYPEDEF_BLUECLASS( Tr2MeshLod );
BLUE_DECLARE_VECTOR( Tr2MeshLod );

#endif

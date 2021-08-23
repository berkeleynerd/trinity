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
	public Tr2MeshBase
{
public:
	EXPOSE_TO_BLUE();

	Tr2MeshLod( IRoot* lockobj = NULL );
	~Tr2MeshLod();

	virtual bool IsLoading() const { return false; }

	void GetBatches( ITriRenderBatchAccumulator* batches,
		const Tr2MeshAreaVector* areas,
		const Tr2PerObjectData* data,
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
	void SetGeometryResource( Tr2LodResource* lodResource );

	virtual bool GetBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool GetBoundingSphere( Vector4& sphere );
	
	bool IsGeometryUsingSelectedLod() const;

protected:
	Tr2LodResourcePtr m_geometryRes;
	mutable Tr2LodResourceCache<TriGeometryRes> m_geometryCache;

	PTr2LodResourceVector m_associatedResources;
	Tr2Lod m_selectedLod;
};

TYPEDEF_BLUECLASS( Tr2MeshLod );
BLUE_DECLARE_VECTOR( Tr2MeshLod );

#endif

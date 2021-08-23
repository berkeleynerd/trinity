////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshLod.h"
#include "Resources/TriGeometryRes.h"


Tr2MeshLod::Tr2MeshLod( IRoot* lockobj /*= NULL */ ) :
	PARENTLOCK( m_associatedResources ),
	m_selectedLod( TR2_LOD_UNSPECIFIED )
{

}

Tr2MeshLod::~Tr2MeshLod()
{

}

void Tr2MeshLod::SelectLod( Tr2Lod lod )
{
	if( m_selectedLod == lod )
	{
		return;
	}

	m_geometryRes->SelectLod( lod );
	for( auto it = m_associatedResources.begin(); it != m_associatedResources.end(); ++it )
	{
		(*it)->SelectLod( lod );
	}
	m_selectedLod = lod;
}

void Tr2MeshLod::AddAssociatedResource( Tr2LodResource* lr )
{
	m_associatedResources.Append( lr );
}

void Tr2MeshLod::RemoveAssociatedResource( Tr2LodResource* lr )
{
	auto key = m_associatedResources.FindKey( lr );
	if( key != -1 )
	{
		m_associatedResources.Remove( key );
	}
}

void Tr2MeshLod::ClearAssociatedResources()
{
	m_associatedResources.Remove( -1 );
}

TriGeometryRes* Tr2MeshLod::GetGeometryResource() const
{
	return m_geometryCache.GetResource( m_geometryRes );
}

void Tr2MeshLod::SetGeometryResource( Tr2LodResource* lodResource )
{
	m_geometryRes = lodResource;
}

bool Tr2MeshLod::GetBoundingBox( Vector3& min, Vector3& max ) const
{
	auto const geomRes = GetGeometryResource();
	if( geomRes && geomRes->IsGood() )
	{
		return geomRes->GetBoundingBox( m_meshIndex, min, max );
	}
	return false;
}

bool Tr2MeshLod::GetBoundingSphere( Vector4& sphere )
{
	const auto geomRes = GetGeometryResource();
	if ( geomRes && geomRes->IsGood() )
	{
		return geomRes->GetBoundingSphere( m_meshIndex, sphere );
	}
	return false;
}

void Tr2MeshLod::GetBatches( ITriRenderBatchAccumulator* batches,
	const Tr2MeshAreaVector* areas,
	const Tr2PerObjectData* data,
	ITr2MeshBatchCallback* callback ) const
{
	if( !GetDisplay() )
	{
		return;
	}

	for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;

		if( !area->GetDisplay() || area->GetMinLod() > m_selectedLod )
		{
			continue;
		}

		auto shadMat = area->GetMaterialInterface();

		if( !shadMat )
		{
			continue;
		}

		TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetShaderMaterial( shadMat );
			batch->SetPerObjectData( data );
			batch->SetGeometryResource( GetGeometryResource() );
			batch->SetMeshParameters( m_meshIndex, area->GetIndex(), area->GetCount(), area->GetReversed() );

			if( callback )
			{
				if( !callback->ProcessBatch( area, batch ) )
				{
					continue;
				}
			}

			batches->Commit( batch );
		}
	}
}

bool Tr2MeshLod::IsGeometryUsingSelectedLod() const
{
	return m_geometryRes->IsUsingSelectedLod();
}

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
	m_selectedLod( TR2_LOD_UNSPECIFIED ),
	m_triedLoadingLowRes( false )
{

}

Tr2MeshLod::~Tr2MeshLod()
{
	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
	}
	if( m_lowResGeometryResource )
	{
		m_lowResGeometryResource->RemoveNotifyTarget( this );
	}
}

bool Tr2MeshLod::Initialize()
{
	if( m_geometryRes )
	{
		m_geometryRes->AddNotifyTarget( this );
	}
	SelectLod( TR2_LOD_HIGH );
	return true;
}

void Tr2MeshLod::SelectLod( Tr2Lod lod )
{
	if( m_selectedLod == lod )
	{
		return;
	}

	if( !m_triedLoadingLowRes )
	{
		m_triedLoadingLowRes = true;
		auto lowResPath = m_geometryRes->GetResourcePath( TR2_LOD_LOW );
		auto resPath = m_geometryRes->GetResourcePath( TR2_LOD_HIGH );
		if( !lowResPath.empty() && BePaths->FileExistsLocally( CA2W( lowResPath.c_str() ) ) && !BePaths->FileExistsLocally( CA2W( resPath.c_str() ) ) )
		{
			BeResMan->GetResource( lowResPath, "", m_lowResGeometryResource );
			m_lowResGeometryResource->AddNotifyTarget( this );
		}
	}

	m_geometryRes->SelectLod( TR2_LOD_HIGH );
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
	auto res = m_geometryCache.GetResource( m_geometryRes );
	if( res && res->IsGood() )
	{
		return res;
	}
	return m_lowResGeometryResource;
}

Tr2LodResourcePtr Tr2MeshLod::GetGeometryLodResource() const
{
	return m_geometryRes;
}

void Tr2MeshLod::SetGeometryResource( Tr2LodResource* lodResource )
{
	if( m_geometryRes == lodResource )
	{
		return;
	}
	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
	}
	m_geometryRes = lodResource;
	if( m_geometryRes )
	{
		m_geometryRes->AddNotifyTarget( this );
	}
}

void Tr2MeshLod::GetBatches( ITriRenderBatchAccumulator* batches,
	const Tr2MeshAreaVector* areas,
	const Tr2PerObjectData* data,
	float screenSize,
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
			batch->SetMeshParameters( m_meshIndex, area->GetIndex(), area->GetCount(), screenSize, area->GetReversed() );

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

void Tr2MeshLod::ReleaseCachedData( BlueAsyncRes* )
{
}

void Tr2MeshLod::RebuildCachedData( BlueAsyncRes* p ) 
{
	if( p == m_lowResGeometryResource && ( !m_geometryRes->GetResource() || !m_geometryRes->GetResource()->IsGood() ) )
	{
		CacheBounds();
	}
}

void Tr2MeshLod::OnLodResourceChanged( Tr2LodResource* resource )
{
	CCP_ASSERT( resource == m_geometryRes );

	CacheBounds();
}
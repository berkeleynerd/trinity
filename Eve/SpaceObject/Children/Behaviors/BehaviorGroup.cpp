#include "StdAfx.h"
#include "BehaviorGroup.h"
#include "include/TriMath.h"
#include "IBehavior.h"
#include "Tr2InstancedMesh.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierTransformCommon.h" 
#include "Resources/TriGeometryRes.h"
#include "PlayFX.h"

BehaviorGroup::BehaviorGroup( IRoot* lockobj ) :
	PARENTLOCK( m_behaviors ),
	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_count( 0 ),
	m_display( true ),
	m_meshToggle( false ),
	m_maxVelocity( 100 ),
	m_changeBufferVertexCount( nullptr ),
	m_scale( Vector3( 1, 1, 1 ) ),
	m_spriteScale( Vector3( 7.0, 7.0, 7.0 ) ),
	m_currentScreenSize( 0.0 ),
	m_renderThreshold( 1.0 ),
	m_blendScreenSizeMin( 5.0 ),
	m_blendScreenSizeMax( 15.0 ),
	m_boundingSphereRadius( 5.f ),
	m_spawnPosition( 0.f, 0.f, 0.f )
{
	m_behaviors.SetNotify( this );
	m_tree = nullptr;
	m_tree.CreateInstance();
}

bool BehaviorGroup::Initialize()
{
	m_scratchData.resize( m_behaviors.size() );
	return true;
}

BehaviorGroup::~BehaviorGroup()
{
}

void BehaviorGroup::OnListModified(
	long event,		// BLUELISTEVENT values
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const struct IList* list
)
{
	if( list == &m_behaviors && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
			case BELIST_INSERTED:
			{
				m_scratchData.insert( m_scratchData.begin() + key, CcpMallocBuffer() );
				if( !m_agents.empty() )
				{
					auto size = m_behaviors[key]->GetScratchMemorySize();
					auto& scratchData = m_scratchData[key];
					scratchData.resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
					for( size_t i = 0; i < m_agents.size(); ++i )
					{
						m_behaviors[key]->InitializeScratch( scratchData.get() + size * i );
					}
				}
				CreateAgentTree();
				break;
			}
			case BELIST_REMOVED:
				m_scratchData.erase( m_scratchData.begin() + key );
				CreateAgentTree();
				break;
			case BELIST_SWAPPED:
				std::swap( m_scratchData[key], m_scratchData[key2] );
				break;
			case BELIST_UNLOADSTART:
				m_scratchData.clear();
				break;
			default:
				break;
		}
	}
	InitializeGeometryResource();
}

// --------------------------------------------------------------------------------
// Description:
//   Load the geometry resource, might be a re-load
// SeeAlso:
//   IBlueResource, TriGeometryRes
// --------------------------------------------------------------------------------
void BehaviorGroup::InitializeGeometryResource()
{
	//to make resource load correctly they must be regenerated for this instance
	m_agents.clear();
	for( auto it = begin( m_scratchData ); it != end( m_scratchData ); ++it )
	{
		it->clear();
	}

	SortBehaviorIndexes();

	const int t = m_count;
	m_count = 0;
	SetCount( t );
}

void BehaviorGroup::SetVertexFunctionReferance( const std::function<void( void )> &F )
{
	m_changeBufferVertexCount = F;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return how many agents have been created
// --------------------------------------------------------------------------------------
size_t BehaviorGroup::GetSize()
{
	return m_agents.size();
}

// --------------------------------------------------------------------------------------
// Description:
//   Return local counter of agent count
// --------------------------------------------------------------------------------------
unsigned int BehaviorGroup::GetCount()
{
	return unsigned( m_count );
}

// --------------------------------------------------------------------------------------
// Description:
//   Create a local KD tree
// --------------------------------------------------------------------------------------
void BehaviorGroup::CreateAgentTree()
{
	m_tree = nullptr;
	if ( !m_tree.CreateInstance() )
	{
		return;
	}
	m_tree->CreateTree( m_agents, m_behaviors.size());
}

// --------------------------------------------------------------------------------------
// Description:
//   Loop through behaviors and returns the one that matches the name
// Argument:
//	name - name of the behavior you want to find
// Returns:
//	The behavior object if it found a match, if not return a nullptr
// --------------------------------------------------------------------------------------
IBehavior* BehaviorGroup::GetBehaviorByName(std::string name)
{
	for( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior )
	{
		if( name == ( *behavior )->GetBehaviorName() )
		{
			return *behavior;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Loop through elements in ProcessPriority enum and re-arranges behavior indices 
//   based on priority
// Returns:
//	A vector of sorted indices
// --------------------------------------------------------------------------------------
void BehaviorGroup::SortBehaviorIndexes()
{
	m_sortedBehaviorIndexes.clear();

	// Hard-coded 5 because the enum ProcessPriority has 5 elements
	for ( int i = 0; i < 5; ++i )
	{
		int p = 0;
		for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior, ++p )
		{
			if ( ( *behavior )->GetProcessPriority() == i )
			{
				m_sortedBehaviorIndexes.push_back( p );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   For artists when they are creating the sprite to easily swap between meshes
// --------------------------------------------------------------------------------------
void BehaviorGroup::ToggleMesh()
{
	m_meshToggle = !m_meshToggle;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return either the instanced mesh or the lodded out mesh
// --------------------------------------------------------------------------------------
Tr2MeshPtr BehaviorGroup::GetMesh() const
{
	auto mesh = m_meshToggle ? m_spriteMesh : m_mesh;
	return mesh;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the lodded out mesh
// --------------------------------------------------------------------------------------
Tr2MeshPtr BehaviorGroup::GetSpriteMesh() const
{
	return m_spriteMesh;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the max velocity set for agents in this behaviorGroup
// --------------------------------------------------------------------------------------
float BehaviorGroup::GetMaxVelocity() const
{
	return m_maxVelocity;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the vertex declaration handle for the BehaviorGroup instanced mesh 
// --------------------------------------------------------------------------------------
unsigned int BehaviorGroup::GetVertexDeclarationHandle() const
{
	return m_vertexDeclarationHandle;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the vertex declaration handle for the BehaviorGroup sprite(lodded out) mesh 
// --------------------------------------------------------------------------------------
unsigned int BehaviorGroup::GetSpriteVertexDeclarationHandle() const
{
	return m_spriteVertexDeclarationHandle;
}

// --------------------------------------------------------------------------------------
// Description:
//   Set the group index, used by BehaviorSystem
// --------------------------------------------------------------------------------------
void BehaviorGroup::SetGroupIndexIndicator( int index )
{
	m_groupIndex = index;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get the group index, used by BehaviorSystem
// --------------------------------------------------------------------------------------
int BehaviorGroup::GetGroupIndexIndicator() const
{
	return m_groupIndex;
}

// --------------------------------------------------------------------------------------
// Description:
//   Exposed function used to add an agent, calls the private function which does all the work
// --------------------------------------------------------------------------------------
void BehaviorGroup::AddAgent()
{
	// The function without arguments to be called from the UI
	AddAgentPrivate();
	if ( m_changeBufferVertexCount )
	{
		(m_changeBufferVertexCount)();
	}
	m_count++;
	CreateAgentTree();
}

// --------------------------------------------------------------------------------------
// Description:
//   Create an agent and add it to the vector. Updates scratchdata for that new agent
// --------------------------------------------------------------------------------------
void BehaviorGroup::AddAgentPrivate()
{
 	DroneAgent agent;
	// This is because the process priority behavior can also affect where drones spawn
	if( m_spawnPosition != Vector3( 0.f, 0.f, 0.f ) )
	{
		agent.position = m_spawnPosition;
	}

	m_agents.push_back( agent );

	while( m_scratchData.size() < m_behaviors.size() )
	{
		m_scratchData.push_back( CcpMallocBuffer() );
	}

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[ m_sortedBehaviorIndexes[i] ]->GetScratchMemorySize();
		if( size > 0 )
		{
			m_scratchData[ m_sortedBehaviorIndexes[i] ].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
			m_behaviors[ m_sortedBehaviorIndexes[i] ]->InitializeScratch( m_scratchData[ m_sortedBehaviorIndexes[i] ].get() + size * ( m_agents.size() - 1 ) );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Exposed function to set agent count. Based on if adding multiple agents or removing
//	 them, calls the appropriate function
// --------------------------------------------------------------------------------------
void BehaviorGroup::SetCount( int count )
{
	if ( count == m_count || count < 0 )
	{
		return;
	}

	if ( m_count < count )
	{
		AddAgentsByCount( count );
	}
	else
	{
		RemoveAgentsByCount( count );
	}
		
	m_count = count;
	CreateAgentTree();

	if ( m_changeBufferVertexCount == nullptr )
	{
		return;
	}
	(m_changeBufferVertexCount)();
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if all the agents have lodded out or not.
// Returns:
//	 1 if all agents are sprites, 0 if all agents are instanced meshes, -1 if they are
//	 not the same or neither (no agents)
// --------------------------------------------------------------------------------------
float BehaviorGroup::AllTheSame()
{
	float same = -1;
	// if none of the agents need either of the meshes we let the system know
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if ( same == -1 ) same = (agent->xfade);
		if ( same != (agent->xfade) ) return -1;
	}
	return same;
}

// --------------------------------------------------------------------------------------
// Description:
//   Exposed function used to remove an agent, calls the private function which does all the work
// --------------------------------------------------------------------------------------
void BehaviorGroup::RemoveAgent()
{
	if( m_agents.size() <= 0 )
	{
		return;
	}
	// Removes the last agent
	RemoveSpecificAgent( m_count - 1 );

	if ( m_changeBufferVertexCount )
	{
		(m_changeBufferVertexCount)();
	}

	m_count--;
	CreateAgentTree();
}

// --------------------------------------------------------------------------------------
// Description:
//   Remove the last agent from the vector and update scratch data.
// --------------------------------------------------------------------------------------
void BehaviorGroup::RemoveSpecificAgent( int index )
{
	m_agents[index] = m_agents.back();
	m_agents.pop_back();

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[m_sortedBehaviorIndexes[i]]->GetScratchMemorySize();
		if( size == 0 )
		{
			continue;
		}

		memcpy( m_scratchData[m_sortedBehaviorIndexes[i]].get() + size * index, m_scratchData[m_sortedBehaviorIndexes[i]].get() + size * m_agents.size(), size );
		m_scratchData[m_sortedBehaviorIndexes[i]].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Set agent count to size. Update scratch data for how many agents were added
// Argument:
//	 size: The new size of agent vector
// --------------------------------------------------------------------------------------
void BehaviorGroup::AddAgentsByCount( int size )
{
	size_t sizeBeforeResize = m_agents.size();
	m_agents.resize( size );

	while( m_scratchData.size() < m_behaviors.size() )
	{
		m_scratchData.push_back( CcpMallocBuffer() );
	}

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
 		auto size = m_behaviors[m_sortedBehaviorIndexes[i]]->GetScratchMemorySize();
		if( size > 0 )
		{
			m_scratchData[m_sortedBehaviorIndexes[i]].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
			for( size_t j = sizeBeforeResize; j < m_agents.size(); j++ )
			{
				m_behaviors[m_sortedBehaviorIndexes[i]]->InitializeScratch( m_scratchData[m_sortedBehaviorIndexes[i]].get() + size * ( j ) );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Set agent count to size. Update scratch data for how many agents were removed
// Argument:
//	 size: The new size of agent vector
// --------------------------------------------------------------------------------------
void BehaviorGroup::RemoveAgentsByCount( int size )
{
	m_agents.resize( size );

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[m_sortedBehaviorIndexes[i]]->GetScratchMemorySize();
		if( size == 0 )
		{
			continue;
		}

		m_scratchData[m_sortedBehaviorIndexes[i]].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Update agents forces based on behaviors calculations.
// Arguments:
//	 dt: delta time, system: parent object
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateAgents(const float dt, EveChildBehaviorSystem& system )
{
	if ( m_agents.empty() )
	{
		return;
	}

	std::vector<float> ranges;

	float searchRad = 0;
	for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior )
	{
		searchRad = ( *behavior )->GetBehaviorSearchRadius();
		if ( searchRad == -1.f )
		{
			ranges.push_back( -1.f );
		}
		else
		{
			ranges.push_back( searchRad + m_boundingSphereRadius );
		}
	}

	const std::vector < std::vector<std::vector<DroneAgent*>>>* dronesInRange = m_tree->FindDronesInRange( m_agents, ranges, m_boundingSphereRadius );

	//Calculate the behaviors
	if( m_collectForces )
	{
		m_forces.clear();
		auto scratch = m_scratchData.begin();

		for ( int i = 0; i < static_cast< int >(m_behaviors.size()); ++i)
		{
			std::vector<Vector3> forces = m_behaviors[ m_sortedBehaviorIndexes[i] ]->CalculateBehavior( m_agents, (scratch + m_sortedBehaviorIndexes[i])->get(), dt, *this, system, (*dronesInRange)[ m_sortedBehaviorIndexes[i] ]);
			for ( auto force = forces.begin(); force != forces.end(); ++force )
			{
				m_forces.push_back( *force );
			}
		}
	}
	else
	{
		auto scratch = m_scratchData.begin();
		for ( int i = 0; i < static_cast< int >( m_behaviors.size()); ++i )
		{
			m_behaviors[ m_sortedBehaviorIndexes[ i ] ]->CalculateBehavior( m_agents, ( scratch + m_sortedBehaviorIndexes[ i ] )->get(), dt, *this, system, ( *dronesInRange )[ m_sortedBehaviorIndexes[i] ] );
		}
	}

	// Move the agents based on the behaviors
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		agent->lifetime += dt;

		static const Vector3 zAxis( 0.f, 0.f, 1.f );
		Vector3 test = agent->velocity - agent->acceleration;
		
		agent->velocity += agent->acceleration;
		Vector3 facingDir = agent->velocity;
		Vector3 interestPoint = Vector3();
		TriVectorRotateQuaternion( &interestPoint, &zAxis, &agent->rotation );
		Vector3 actualFacingDir = Lerp( interestPoint, facingDir, LengthSq(agent->velocity) / max(1.0f, m_maxVelocity * m_maxVelocity) );
		TriQuaternionRotationArc( &agent->rotation, &zAxis, &actualFacingDir);
		agent->targetDirection = actualFacingDir;

		agent->velocity = ClampLength( agent->velocity, m_maxVelocity );
		agent->position = agent->position + agent->velocity * dt;
		agent->acceleration = Vector3( 0, 0, 0 );
	}

	// later on we could have the updateTree input dynamically adjust based on dt
	// one of my ideas was input = max( const - dt , minimumUpdateFreq )
	// this would make it update less often on big dt-s
	m_tree->UpdateTree( 0.015 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if each agent is still visible or not and update it's visibility based on that.
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateVisibility( const TriFrustum & frustum, const Matrix & parentTransform )
{
	// Check if an agent is visible and calculate the xfade value
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if( frustum.IsSphereVisible( agent->position, m_boundingSphereRadius ) )
		{
			float pixelSize = frustum.GetPixelSizeAccross( agent->position, m_boundingSphereRadius );
			agent->screenSize = pixelSize * m_scale[0]; // Store the screen size for each agent

			if( pixelSize >= m_blendScreenSizeMax )
			{
				agent->xfade = 0.0; // Render as mesh
			}
			else if( pixelSize <= m_blendScreenSizeMin )
			{
				agent->xfade = 1.0; // Render as sprite
			}
			else
			{
				float s = 1.0;
				agent->xfade = s - ( pixelSize - m_blendScreenSizeMin ) / ( m_blendScreenSizeMax - m_blendScreenSizeMin );
			}
			agent->isVisible = true;
		}
		else
		{
			agent->isVisible = false;
		}
	}

	this->UpdateCurrentScreenSize();

	m_frustum = frustum;
	m_parentTransform = parentTransform;
}

// --------------------------------------------------------------------------------------
// Description:
//   Update the current screensize read-only attribute in Jessica using the first agent.
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateCurrentScreenSize()
{
	if ( !m_agents.empty() )
	{
		m_currentScreenSize = m_agents.begin()->screenSize;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//  Check if any of the agents are visible.
// --------------------------------------------------------------------------------------
bool BehaviorGroup::IsGroupVisible()
{
	bool isAnyAgentVisible = false;

	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if ( agent->screenSize >= m_renderThreshold ) {
			isAnyAgentVisible = true;
			break;
		}
	}
	return isAnyAgentVisible;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get LOD info for buffer. Called from BehaviorSystem
// --------------------------------------------------------------------------------------
void BehaviorGroup::GetInfoForBuffer( uint8_t* data, Matrix& parentWorldLocation )
{
	for( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		float LOD = ( *agent ).xfade;
		float LODmod;
		if( LOD != 1 && agent->isVisible )
		{
			LODmod = ( 1 - LOD ) *( 0.5f + ( 1 - LOD ) * 0.5f );
			Vector3 meshScale = m_scale * Vector3( LODmod, LODmod, LODmod );

			if( m_meshToggle )
			{
				meshScale *= m_spriteScale;
			}

			Matrix m = Transpose( TransformationMatrix( meshScale, agent->rotation, agent->position ) );
			memcpy( data, &m, 12 * sizeof( float ) );
		}
		else
		{
			Matrix m = Transpose( TransformationMatrix( Vector3( 0, 0, 0 ), Quaternion( 0, 0, 0, 0 ), Vector3( 0, 0, 0 ) ) );
			memcpy( data, &m, 12 * sizeof( float ) );
		}

		data += 12 * sizeof( float );

		// sprite
		if( LOD != 0 && agent->isVisible )
		{
			LODmod = ( 1.0f - LOD ) * ( LOD * 0.3f ) + ( LOD * 1.0f );
			Matrix agentMatrix = TransformationMatrix( m_scale * m_spriteScale * Vector3( LODmod, LODmod, LODmod ),
				agent->rotation, agent->position );
			agentMatrix = Billboard2D( agentMatrix );
			Matrix m2 = Transpose( agentMatrix );
			memcpy( data, &m2, 12 * sizeof( float ) );
		}
		data += 12 * sizeof( float );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Create lodded out mesh vertex declaration.
// --------------------------------------------------------------------------------------
void BehaviorGroup::CreateSpriteVertexDeclaration()
{
	Tr2MeshPtr meshPtr = GetSpriteMesh();

	if ( meshPtr )
	{
		if ( nullptr == meshPtr->GetGeometryResource() )
		{
			return;
		}

		if ( (meshPtr->GetGeometryResource())->IsGood() )
		{
			TriGeometryResMeshData* meshData = meshPtr->GetGeometryResource()->GetMeshData( meshPtr->GetMeshIndex() );
			if ( meshData->m_vertexDeclaration != m_cachedSVD )
			{
				Tr2VertexDefinition s_agentInstancedVertex;
				Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, s_agentInstancedVertex );

				Tr2VertexDefinition& def = s_agentInstancedVertex;
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 8, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 9, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 10, 1, 1 );

				// create vertex-declarartion
				m_spriteVertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_agentInstancedVertex );

				m_cachedSVD = meshData->m_vertexDeclaration;
			}
			return;
		}
	}
	m_cachedSVD = -1;
	m_spriteVertexDeclarationHandle = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Create instanced mesh vertex declaration
// --------------------------------------------------------------------------------------
void BehaviorGroup::CreateVertexDeclaration()
{
	Tr2MeshPtr meshPtr = GetMesh();

	if ( meshPtr )
	{
		if ( nullptr == meshPtr->GetGeometryResource() )
		{
			return;
		}

		if ( (meshPtr->GetGeometryResource())->IsGood() )
		{
			TriGeometryResMeshData* meshData = meshPtr->GetGeometryResource()->GetMeshData( meshPtr->GetMeshIndex() );
			if ( meshData->m_vertexDeclaration != m_cachedVD )
			{
				Tr2VertexDefinition s_agentInstancedVertex;
				Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, s_agentInstancedVertex );

				Tr2VertexDefinition& def = s_agentInstancedVertex;
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 8, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 9, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 10, 1, 1 );

				// create vertex-declarartion
				m_vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_agentInstancedVertex );

				m_cachedVD = meshData->m_vertexDeclaration;
			}
			return;
		}
	}
	m_cachedVD = -1;
	m_vertexDeclarationHandle = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   For the effect in PlayFX to be rendered this is needed.
// --------------------------------------------------------------------------------------
void BehaviorGroup::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	auto behavior = GetBehaviorByName( "PlayFX" );

	if( behavior != nullptr )
	{
		auto tmp = dynamic_cast<PlayFX*>( behavior );
		if( tmp )
		{
			tmp->GetRenderables( renderables );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   For the effect in PlayFX to be updated this is needed.
// --------------------------------------------------------------------------------------
void BehaviorGroup::Update( EveUpdateContext& updateContext )
{
	auto behavior = GetBehaviorByName( "PlayFX" );

	if( behavior != nullptr )
	{
		auto tmp = dynamic_cast<PlayFX*>( behavior );
		if( tmp )
		{
			tmp->Update( updateContext, m_frustum, m_parentTransform );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void BehaviorGroup::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "AgentsKDTree" );
	options.insert( "Bounding Sphere" );
	options.insert( "DebugBehaviors" );
}

float BehaviorGroup::GetBoundingSphereRadius()
{
	return m_boundingSphereRadius;
}

EveKDdroneManagementTreePtr BehaviorGroup::GetKDTree()
{
	return m_tree;
}

void BehaviorGroup::RenderDebugInfo( ITr2DebugRenderer2& renderer, Matrix& parentWorldLocation )
{
	for ( auto group = m_behaviors.begin(); group != m_behaviors.end(); ++group )
	{
		( *group )->RenderDebugInfo( renderer, m_agents, parentWorldLocation );
	}

	if ( renderer.HasOption( this, "AgentsKDTree" ) )
	{
		if ( m_tree != nullptr )
		{
			m_tree->RenderDebugInfo( renderer, parentWorldLocation );
		}
		else
		{
			CreateAgentTree();
		}
	}

	if (renderer.HasOption( this, "Bounding Sphere" ))
	{
		for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent) 
		{
			renderer.DrawSphere( this, TranslationMatrix( agent->position ) * parentWorldLocation, m_boundingSphereRadius, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
		}
	}

	if ( renderer.HasOption( this, "DebugBehaviors" ) )
	{
		if( m_collectForces )
		{
			bool loopToggle = true;
			Vector3 point;
			for ( auto force = m_forces.begin(); force != m_forces.end(); ++force )
			{
				if(loopToggle)
				{
					point = ( *force ) + parentWorldLocation.GetTranslation();
					loopToggle = false;
				}
				else
				{
					float lengthLerp = min( 1.f, max( 0.f, Length( *force ) / m_maxVelocity ) );
					renderer.DrawLine(this, point, point + (*force),
					                  Lerp(Color(0xff11ff11), Color(0xffff1111), lengthLerp));
					Color(0xff11ff11);
					loopToggle = true;
				}
			}
		}
		else
		{
			m_collectForces = true;
		}
	}
	else
	{
		m_collectForces = false;
	}
}


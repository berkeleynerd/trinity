#include "StdAfx.h"
#include "EveAudioObject.h"
#include "Eve/EveUpdateContext.h"

EveAudioObject::EveAudioObject( IRoot* lockobj ) :
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_display( true ),
	m_mute( false ),
	m_audioEvent( L"" )
{
}

bool EveAudioObject::Initialize()
{
	if( SUCCEEDED( BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>(&m_audioEmitter.p) ) ) )
	{
		m_audioEmitter->SetName( m_name.empty() ? "audio_object" : m_name.c_str() );
		
		Vector3 position = GetWorldPosition();
		Vector3 front( 0, 1, 0 ), top( 0, 0, 1 );
		m_audioEmitter->SetPosition( front, top, position );
		
		if( !m_audioEvent.empty() )
		{
			PlayAudioEvent( m_audioEvent );
		}
		
		return true;
	}
	
	return false;
}

void EveAudioObject::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	UpdateWorldTransform( updateContext.GetTime() );
	
	m_lastUpdateMatrix = TransformationMatrix( m_scaling, m_rotation, m_translation ) * m_worldTransform;
	
	if( m_audioEmitter && !m_mute )
	{
		Vector3 position = GetWorldPosition();
		Vector3 front( 0, 1, 0 ), top( 0, 0, 1 );
		m_audioEmitter->SetPosition( front, top, position );
	}
}

void EveAudioObject::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void EveAudioObject::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
}

void EveAudioObject::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
}

bool EveAudioObject::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	Vector3 pos = m_worldTransform.GetTranslation();
	sphere = Vector4( pos.x, pos.y, pos.z, 1.0f );
	return true;
}

void EveAudioObject::GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const
{
}

void EveAudioObject::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	UpdateWorldTransform( t );
	Matrix currentTransform = TransformationMatrix( m_scaling, m_rotation, m_translation ) * m_worldTransform;
	position = currentTransform.GetTranslation();
}

void EveAudioObject::GetModelCenterWorldPosition( Vector3 &position ) const
{
	Matrix currentTransform = TransformationMatrix( m_scaling, m_rotation, m_translation ) * m_worldTransform;
	position = currentTransform.GetTranslation();
}

bool EveAudioObject::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	min = Vector3( -1.0f, -1.0f, -1.0f );
	max = Vector3( 1.0f, 1.0f, 1.0f );
	return true;
}

void EveAudioObject::GetLocalToWorldTransform( Matrix &transform ) const
{
	transform = m_lastUpdateMatrix;
}

Vector3 EveAudioObject::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}

Quaternion EveAudioObject::GetWorldRotation()
{
	return Normalize( m_rotation * RotationQuaternion( m_worldTransform ) );
}

void EveAudioObject::SetEmitterName( const std::string& name )
{
	if( m_audioEmitter )
	{
		m_audioEmitter->SetName( name.c_str() );
	}
}

void EveAudioObject::SetPosition( const Vector3& position )
{
	m_translation = position;
}

void EveAudioObject::SetRotation( const Quaternion& rotation )
{
	m_rotation = rotation;
}

unsigned int EveAudioObject::PlayAudioEvent( const std::wstring& eventName )
{
	if( m_audioEmitter && !eventName.empty() )
	{
		return m_audioEmitter->SendEvent( eventName );
	}
	return 0;
}

void EveAudioObject::SetMute( bool mute )
{
	m_mute = mute;
	if( m_audioEmitter )
	{
		mute ? m_audioEmitter->Mute() : m_audioEmitter->Unmute();
	}
}

void EveAudioObject::UpdateWorldTransform( Be::Time time )
{
	Quaternion rotation;
	Vector3 translation;

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}
	else
	{
		translation = m_translation;
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}
	else
	{
		rotation = m_rotation;
	}

	m_worldTransform = RotationMatrix( rotation ) * TranslationMatrix( translation );
}

void EveAudioObject::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Bounding Sphere" );

	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_audioEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void EveAudioObject::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Bounding Sphere" ) )
	{
		Vector4 boundingSphere;
		if( GetBoundingSphere( boundingSphere ) )
		{
			renderer.DrawSphere( this, boundingSphere.GetXYZ(), boundingSphere.w, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
		}
	}

	auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_audioEmitter.p );
	if( tmp )
	{
		tmp->RenderDebugInfo( renderer );
	}
}

bool EveAudioObject::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_display ) )
	{
		if( m_audioEmitter )
		{
			m_display ? m_audioEmitter->Unmute() : m_audioEmitter->Mute();
		}
	}
	else if( IsMatch( val, m_mute ) )
	{
		SetMute( m_mute );
	}
	else if( IsMatch( val, m_audioEvent ) )
	{
		if( m_audioEmitter && !m_audioEvent.empty() )
		{
			PlayAudioEvent( m_audioEvent );
		}
	}
	return true;
}
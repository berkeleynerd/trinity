////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#include "StdAfx.h"
#include "EveSocketParameter.h"

#include "Tr2ExternalParameter.h"
#include "TriValueBinding.h"


EveSocketParameterBase::EveSocketParameterBase( IRoot* lockobj ) :
	PARENTLOCK( m_bindings ),
	m_name()
{
}

EveSocketParameterBase::~EveSocketParameterBase()
{

}

void EveSocketParameterBase::ClearBindings()
{
	m_bindings.Clear();
}

bool EveSocketParameterBase::BindToExternalParameter( const Tr2ExternalParameter& externalParameter )
{
	if ( !externalParameter.IsValid() || externalParameter.GetName() != m_name )
	{
		return false;
	}

	TriValueBindingPtr binding = externalParameter.CreateBinding();
	// the value attribute of any derived class should be exposed in blue as "value" to make this work...
	binding->SetSource( "value", this );
	binding->Initialize();
	if ( !binding->IsValid() )
	{
		return false;
	}
	
	if ( !ExtractDefault( externalParameter ) )
	{
		return false;
	}
	m_bindings.Append( dynamic_cast<ITr2ValueBinding*>( binding.Detach() ) );

	return true;
}

void EveSocketParameterBase::Propagate()
{
	for ( auto it = begin( m_bindings ); it != end( m_bindings ); ++it )
	{
		( *it )->CopyValue();
	}
}

#define SOCKET_PARAMETER_DEFINE(_className, _valueType)\
	void _className##::ClearBindings()\
	{\
		m_defaults.clear();\
		EveSocketParameterBase::ClearBindings();\
	}\
	void _className##::Reset()\
	{\
		for ( size_t i = 0; i < m_bindings.size(); ++i )\
		{\
			m_value = m_defaults[i];\
			m_bindings[i]->CopyValue();\
		}\
		ClearBindings();\
	}\
	bool _className##::ExtractDefault( const Tr2ExternalParameter& externalParameter )\
	{\
		m_defaults.push_back( *reinterpret_cast<_valueType*>( externalParameter.GetDestination() ) );\
		return true;\
	}\

SOCKET_PARAMETER_DEFINE( EveSocketParameterFloat, float );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector2, Vector2 );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector3, Vector3 );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector4, Vector4 );
SOCKET_PARAMETER_DEFINE( EveSocketParameterColor, Color );

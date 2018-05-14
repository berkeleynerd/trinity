////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ControllerExpression.h"
#include "Tr2Controller.h"
#include "Tr2StateMachine.h"
#include "Tr2ControllerFloatVariable.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2ExpressionTermInfo.h"
#include <regex>


namespace
{
	const Tr2StateMachine* s_stateMachine = nullptr;
	IRoot* s_owner = nullptr;

	float StateTime()
	{
		return s_stateMachine ? s_stateMachine->GetStateRunTime() : 0;
	}

	float AnimationTime( const char* name )
	{
		EveSpaceObject2Ptr spaceObject = BlueCastPtr( s_owner );
		if( !spaceObject )
		{
			return 0;
		}
		auto ac = spaceObject->GetAnimationController();
		if( !ac )
		{
			return 0;
		}
		auto animation = ac->FindAnimationByName( name );
		if( !animation )
		{
			return 0;
		}
		return animation->Duration;
	}

	float CurveSetTime( const char* name )
	{
		ITr2CurveSetOwnerPtr csOwner = BlueCastPtr( s_owner );
		if( !csOwner )
		{
			return 0;
		}
		return csOwner->GetCurveSetDuration( name );
	}

	float IsAnimationPlaying( const char* layerName )
	{
		EveSpaceObject2Ptr spaceObject = BlueCastPtr( s_owner );
		if( !spaceObject )
		{
			return 0;
		}
		auto ac = spaceObject->GetAnimationController();
		if( !ac )
		{
			return 0;
		}
		auto layer = ac->GetAnimationLayer( layerName && !*layerName ? nullptr : layerName );
		if( !layer )
		{
			return 0;
		}
		auto remaining = layer->GetAnimationRemainingTime();
		return remaining > 0;
	}

	bool IsValidVariableName( const char* name )
	{
		static std::regex namePattern( "[a-zA-Z_][a-zA-Z_0-9]*" );
		return std::regex_match( name, namePattern );
	}
}


Tr2ControllerExpression::Tr2ControllerExpression()
	:m_stateMachine( nullptr ),
	m_controller( nullptr )
{
}

std::string Tr2ControllerExpression::SetExpr( const char* expression, const Tr2StateMachine& stateMachine )
{
	m_stateMachine = &stateMachine;
	m_controller = stateMachine.GetController();
	return CreateParser( expression, nullptr );
}

std::string Tr2ControllerExpression::SetExpr( const char* expression, const Tr2Controller& controller, ModifyParser modifyParser )
{
	m_stateMachine = nullptr;
	m_controller = &controller;
	return CreateParser( expression, modifyParser );
}

std::string Tr2ControllerExpression::CreateParser( const char* expression, ModifyParser modifyParser )
{
	m_expressionParser = mu::Parser();
	auto& variables = m_controller->GetVariables();
	for( auto it = begin( variables ); it != end( variables ); ++it )
	{
		if( !IsValidVariableName( ( *it )->GetName().c_str() ) )
		{
			continue;
		}
		m_expressionParser.DefineVar( ( *it )->GetName(), ( *it )->GetPointerForParser() );
	}

	m_expressionParser.DefineFun( "StateTime", StateTime, false );
	m_expressionParser.DefineFun( "AnimationTime", AnimationTime, false );
	m_expressionParser.DefineFun( "IsAnimationPlaying", IsAnimationPlaying, false );
	m_expressionParser.DefineFun( "CurveSetTime", CurveSetTime, false );
	if( modifyParser )
	{
		( *modifyParser )( m_expressionParser );
	}
	
	m_expressionParser.SetExpr( expression );
	s_stateMachine = m_stateMachine;
	s_owner = m_controller->GetOwner();
	try
	{
		m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& e )
	{
		s_stateMachine = nullptr;
		s_owner = nullptr;
		return e.GetMsg();
	}
	s_stateMachine = nullptr;
	s_owner = nullptr;
	return std::string();
}

std::pair<bool, float> Tr2ControllerExpression::Eval() const
{
	if( !m_controller )
	{
		return std::make_pair( false, 0.f );
	}
	s_stateMachine = m_stateMachine;
	s_owner = m_controller->GetOwner();
	bool success = true;
	float result = 0.f;
	try
	{
		result = m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& )
	{
		success = false;
	}
	s_stateMachine = nullptr;
	s_owner = nullptr;
	return std::make_pair( success, result );
}

void Tr2ControllerExpression::Clear()
{
	m_stateMachine = nullptr;
	m_controller = nullptr;
	m_expressionParser = mu::Parser();
}

bool Tr2ControllerExpression::IsExpressionValid() const
{
	try
	{
		m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& )
	{
		return false;
	}
	return true;
}

void Tr2ControllerExpression::GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& info ) const
{
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "StateTime", "time in seconds the current state is running" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "AnimationTime", "name", "geometry animation duration (in seconds) for the given name" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "CurveSetTime", "name", "duration (in seconds) of the curve set with the given name" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "IsAnimationPlaying", "name", "return 1 if the geometry animation in the given layer is playing; 0 otheriwise" ) );
}
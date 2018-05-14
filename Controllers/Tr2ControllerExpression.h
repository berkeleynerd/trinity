////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

#include <muParser.h>

BLUE_DECLARE( Tr2StateMachine );
BLUE_DECLARE( Tr2Controller );
BLUE_DECLARE( Tr2ExpressionTermInfo );


class Tr2ControllerExpression
{
public:
	Tr2ControllerExpression();

	typedef void( *ModifyParser )( mu::Parser& );

	std::string SetExpr( const char* expression, const Tr2StateMachine& stateMachine );
	std::string SetExpr( const char* expression, const Tr2Controller& controller, ModifyParser modifyParser = nullptr );
	std::pair<bool, float> Eval() const;
	void Clear();
	bool IsExpressionValid() const;

	void GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& info ) const;
private:
	std::string CreateParser( const char* expression, ModifyParser modifyParser );

	mu::Parser m_expressionParser;
	const Tr2StateMachine* m_stateMachine;
	const Tr2Controller* m_controller;
};

////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

#include "Include/ITr2Updateable.h"
#include "Include/ITriFunction.h"
#include "ITr2ControllerAction.h"
#include "Controllers/Tr2BindingPoint.h"
#include "Controllers/Tr2ControllerExpression.h"


BLUE_DECLARE( Tr2ExpressionTermInfo );


BLUE_CLASS( Tr2ActionAnimateValue ) :
	public ITr2ControllerAction,
	public ITr2Updateable,
	public INotify
{
public:
	Tr2ActionAnimateValue( IRoot* = nullptr );

	EXPOSE_TO_BLUE();

	virtual void Link( Tr2Controller& controller );
	virtual void Unlink();
	virtual void Start( Tr2Controller& controller );
	virtual void Stop( Tr2Controller& controller );

	virtual void Update( Be::Time realTime, Be::Time simTime );

	virtual bool OnModified( Be::Var* value );

	bool IsBindingValid() const;
	bool IsExpressionValid() const;

	float GetCurveValue( float time ) const;

	IRootPtr GetDestination() const;
	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
private:
	bool IsAttrExpressionValid( const char* attributeName ) const;

	Tr2BindingPoint m_destination;
	std::string m_value;
	ITriScalarFunctionPtr m_curve;

	Tr2ControllerExpression m_evaluator;

	Be::Time m_startTime;
	const Tr2Controller* m_controller;
};

TYPEDEF_BLUECLASS( Tr2ActionAnimateValue );

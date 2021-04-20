////////////////////////////////////////////////////////////
//
//    Created:   October 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2ControllerAction.h"


BLUE_CLASS( Tr2ActionPlaySound ) : public ITr2ControllerAction
{
public:
	Tr2ActionPlaySound( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( Tr2Controller& controller ) override;
	void StartWithController( PyObject * controller );
private:
	std::string m_emitterName;
	std::wstring m_soundEvent;
	bool m_bypassPrefix;
};

TYPEDEF_BLUECLASS( Tr2ActionPlaySound );

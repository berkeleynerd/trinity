////////////////////////////////////////////////////////////
//
//    Created:   April 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef EveCustomMask_H
#define EveCustomMask_H

// forwards
struct EveSpaceObjectPSData;

// --------------------------------------------------------------------------------
// Description:
//   This class holds data about custom masks for spaceobjects
// SeeAlso:
//   EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveCustomMask ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveCustomMask( IRoot* lockobj = NULL );
	~EveCustomMask();

	// access
	void GetDebugDrawMatrix( Matrix* matrix, float objectRadius ) const;
	void FillPerObjectDataPS( EveSpaceObjectPSData* psData ) const;
	void Setup( const Vector3& position, const Vector3& scaling, const Quaternion& rotation );

private:
	/////////////////////////////////////////////////////////////////////////////////////
	// mask projection data
	Vector3 m_position;
	Vector3 m_scaling;
	Quaternion m_rotation;

	// materials index
	uint8_t m_materialIndex1;

	// options
	bool m_isMirrored;
};

TYPEDEF_BLUECLASS( EveCustomMask );

#endif // EveCustomMask_H

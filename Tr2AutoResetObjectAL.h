#pragma once
#ifndef Tr2AutoResetObjectAL_H
#define Tr2AutoResetObjectAL_H

class Tr2PrimaryRenderContextAL;

class Tr2AutoResetObjectAL
{
public:
	static void ReleaseALResources();
	static void PrepareALResources( Tr2PrimaryRenderContextAL& renderContext );
protected:
	Tr2AutoResetObjectAL();
	~Tr2AutoResetObjectAL();
	Tr2AutoResetObjectAL( const Tr2AutoResetObjectAL& );

	virtual void ReleaseALResource() = 0;
	virtual void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext ) = 0;
};

#endif
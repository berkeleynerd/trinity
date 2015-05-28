/* 
	*************************************************************************

	TriTextureParameter.h

	Created:   March 2015
	OS:        Win32
	Project:   Trinity

	Description:   
		
		Under DX9, this parameter acts like a cut down version of TriTexture, 
	loading texture resources.

		Under DX10, this parameter will load any form of buffer object resource,
	primarily textures, but also eventually instance data etc.

	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2006

	*************************************************************************
*/
#pragma once
#if !defined( _TriTextureParameter_H_)
#define _TriTextureParameter_H_

#include "include/ITriEffectParameter.h"
#include "Tr2EffectDescription.h"

BLUE_DECLARE_INTERFACE( ITr2ShaderState );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( TriTextureParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriTextureParameter );

BLUE_CLASS( TriTextureParameter ):
	public ITriEffectResourceParameter,
	public IInitialize,
	public INotify,
	public ICopierCustomAssignment
{

public:
	TriTextureParameter(IRoot* lockobj = NULL);
	~TriTextureParameter();

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	const char* GetParameterName() const;
	virtual bool IsZeroOrNull( void ) const;
	void RebuildEffectHandles( ITr2ShaderState* effectRes );

	//////////////////////////////////////////////////////////////////////////
	// ITriEffectResourceParameter
	void ReloadResources();
	bool LoadResources();
	void UnloadResources();
	void* GetResourcePointer() const;
	bool IsPrepared() const;
	void CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
							unsigned char* destHandle, 
							size_t isSRGB,
							Tr2RenderContext &renderContext ) const;
	unsigned GetHashValue( unsigned startingHash ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ICopierCustomAssignment
	/////////////////////////////////////////////////////////////////////////////////////
	virtual bool AssignTo(
		ICopierCustomAssignment* other,
		ICopier* copier
		);

	// access resource
	void SetResource( TriTextureRes* newRes );
	
	virtual TriTextureRes* GetResource() const;
	
	// access strings
	void SetParameterName( const BlueSharedString& name );
	const wchar_t* GetResourcePath() const;
	void SetResourcePath( const char* resourcePath );
private:
	ITr2ShaderStatePtr m_cachedEffect;

	BlueSharedString m_name;
	std::string m_resourcePath;

	TriTextureResPtr m_resource;
	Tr2EffectResource::Type m_resourceType;

	bool m_isUsedByEffect;
};

TYPEDEF_BLUECLASS(TriTextureParameter);
#endif 

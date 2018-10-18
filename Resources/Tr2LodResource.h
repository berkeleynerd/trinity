////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2LodResource_h
#define Tr2LodResource_h

enum Tr2Lod {
	TR2_LOD_UNSPECIFIED = -1,
	
	TR2_LOD_LOW = 0,
	TR2_LOD_MEDIUM,
	TR2_LOD_HIGH,
	TR2_LOD_ULTRA,

	TR2_LOD_COUNT
};

extern bool g_lodLevelUltraEnabled;


BLUE_DECLARE( Tr2LodResource );

struct ITr2LodResourceListener
{
	virtual void OnLodResourceChanged( Tr2LodResource* resource ) = 0;
};


BLUE_CLASS( Tr2LodResource ) : 
	public INotify,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2LodResource( IRoot* lockobj = nullptr );
	~Tr2LodResource();

	virtual bool OnModified( Be::Var* value );

	const char* GetName() const;
	void SetName( const BlueSharedString& val );

	// access
	void SetResourcePath( Tr2Lod lod, const char* resPath );
	IBlueResource* GetResource();

	void SelectLod( Tr2Lod lod );

	void AddNotifyTarget( ITr2LodResourceListener* p );
	void RemoveNotifyTarget( ITr2LodResourceListener* p );
protected:
	void ReleaseCachedData( BlueAsyncRes* p ) override;
	void RebuildCachedData( BlueAsyncRes* p ) override;

	void SetRequested( IBlueResource* requested );
	void SetActive( BlueAsyncRes* active );
	void NotifyListeners();

	BlueSharedString m_name;
	std::string m_resPath[TR2_LOD_COUNT];
	Tr2Lod m_currentLod;

	std::vector<ITr2LodResourceListener*> m_notifies;

	BlueAsyncResPtr m_active;
	BlueAsyncResPtr m_requested;
};

TYPEDEF_BLUECLASS( Tr2LodResource );

#endif // Tr2LodResource_h

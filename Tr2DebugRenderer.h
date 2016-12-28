////////////////////////////////////////////////////////////
//
//    Created:   December 2016
//    Copyright: CCP 2016
//

#pragma once

#include "TriDebugTextRenderer.h"
#include "Tr2PickBuffer.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2DebugRenderer );

typedef std::string Tr2DebugRendererOption;
typedef std::set<Tr2DebugRendererOption> Tr2DebugRendererOptions;


BLUE_INTERFACE( ITr2DebugRenderable ) : public IRoot
{
    virtual void GetDebugOptions( Tr2DebugRendererOptions& options ) = 0;
    virtual void RenderDebugInfo( Tr2DebugRenderer& renderer ) = 0;
};

struct Tr2DebugColor
{
	// front color
	uint32_t m_color;
	// color for things behind other geometry
	uint32_t m_zFailColor;

	Tr2DebugColor( const Color& color );
	Tr2DebugColor( const Color& color, const Color& zFailColor );
};

struct Tr2DebugObjectReference
{
	IRootPtr m_object;
	uint32_t m_area;

	Tr2DebugObjectReference( IRoot* object );
	Tr2DebugObjectReference( IRoot* object, uint32_t area );

	operator bool() const;
	bool operator==( const Tr2DebugObjectReference& other ) const;
	bool operator!=( const Tr2DebugObjectReference& other ) const;
};

BLUE_CLASS( Tr2DebugRenderer ): public IRoot
{
public:
    enum Effect
    {
        Wireframe,
        Solid,
        Lit,
    };

	Tr2DebugRenderer( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

    bool HasOption( IRoot* owner, const char* option ) const;
    bool IsSelected( IRoot* owner ) const;
    
    void DrawLine( Tr2DebugObjectReference owner, const Vector3& from, const Vector3& to, Tr2DebugColor color );
	void DrawTriangle( 
		Tr2DebugObjectReference owner, 
		const Vector3& vertex1, 
		const Vector3& normal1, 
		const Vector3& vertex2, 
		const Vector3& normal2, 
		const Vector3& vertex3, 
		const Vector3& normal3, 
		Tr2DebugColor color );
	void DrawTriangle( 
		Tr2DebugObjectReference owner, 
		const Vector3& vertex1, 
		const Vector3& vertex2, 
		const Vector3& vertex3, 
		const Vector3& normal, 
		Tr2DebugColor color );

    void DrawBox( Tr2DebugObjectReference owner, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color );
    void DrawBox( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color );

    void DrawSphere( Tr2DebugObjectReference owner, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );
    void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, uint32_t segments, Effect effect, Tr2DebugColor color );
    void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );
    void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

    void DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCylinder( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCone( Tr2DebugObjectReference owner, const Vector3& base, const Vector3& focal, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

    void DrawCapsule( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCapsule( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawDoubleArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color );

    void DrawSphereArrow( Tr2DebugObjectReference owner, const Vector3& center, const Vector3& direction, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawAxis( Tr2DebugObjectReference owner, const Matrix& transform, Effect effect );

	void DrawExtrusionShape( Tr2DebugObjectReference owner, const Matrix& transform, const Vector2* vertices, const Vector2* normals, uint32_t vertexCount, uint32_t segments, Effect effect, Tr2DebugColor color );
    
    void Printf( TriDebugFont font, const Vector3& pos, const Color& color, const char* msg, ... );
    
    void BeginRender();
    void EndRender( Tr2RenderContext& renderContext );
    
    Tr2DebugObjectReference Pick( float& depth, Tr2RenderContext& renderContext );
    
    void SetSelectedObjects( const std::vector<IRoot*>& objects );
    void SetOptions( IRoot* owner, std::vector<Tr2DebugRendererOption>& options );
    void SetDefaultOptions( const std::vector<Tr2DebugRendererOption>& options );
private:
	struct Vertex
	{
		Vector3 m_position;
		Vector3 m_normal;
		float m_object;
		float m_line;
		uint32_t m_color;
		uint32_t m_zFailColor;

		Vertex();
		Vertex( const Vector3& position, Tr2DebugColor color, size_t objectIndex );
		Vertex( const Vector3& position, const Vector3& normal, Tr2DebugColor color, size_t objectIndex );
	};

	Tr2PickBuffer m_pickBuffer;

	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_pickingEffect;

	std::vector<Vertex> m_lines;
	std::vector<Vertex> m_triangles;
	std::vector<std::pair<Tr2DebugObjectReference, size_t>> m_objectLineOffsets;
	std::vector<std::pair<Tr2DebugObjectReference, size_t>> m_objectTriangleOffsets;

	Tr2DebugRendererOptions m_defaultOptions;
	std::map<IRootPtr, Tr2DebugRendererOptions> m_options;
	std::set<IRootPtr> m_selectedObjects;
};

TYPEDEF_BLUECLASS( Tr2DebugRenderer );

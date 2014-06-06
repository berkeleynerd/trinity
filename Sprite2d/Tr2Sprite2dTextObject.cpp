////////////////////////////////////////////////////////////////////////////////
//
// Created:		12 2011
// Copyright:	CCP 2011
//

#include "StdAfx.h"
#include "Tr2Sprite2dTextObject.h"
#include "Font/Tr2FontMeasurer.h"
#include "Tr2Sprite2dScene.h"

Tr2Sprite2dTextObject::Tr2Sprite2dTextObject( IRoot* lockobj /*= nullptr */ ) :
	m_dropShadow( false ),
	m_textWidth( 0.0f ),
	m_textHeight( 0.0f ),
	m_pickRadius( 0.0f ),
	m_hasAuxiliaryTooltip( false ),
	m_useSizeFromTexture( false )
{

}

Tr2Sprite2dTextObject::~Tr2Sprite2dTextObject()
{
	if( m_fontMeasurer )
	{
		m_fontMeasurer->UnregisterForChangeNotification( this );
	}
}

void Tr2Sprite2dTextObject::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display || (m_textWidth <= 0.0f) || (m_textHeight <= 0.0f) )
	{
		return;
	}

	if( m_fontMeasurer )
	{
		if( m_isDirty )
		{
			m_fontMeasurer->PrepareSprites( renderer, m_translation, m_color, m_spriteEffect );
			m_isDirty = false;
		}

		if( m_spriteEffect == TR2_SFX_BLUR )
		{
			renderer->PushClipRectangle( m_translation.x - 1, m_translation.y - 1, m_textWidth + 2, m_textHeight + 2 );
		}
		else
		{
			renderer->PushClipRectangle( m_translation.x, m_translation.y, m_textWidth, m_textHeight );
		}
		m_fontMeasurer->SubmitSprites( renderer );
		renderer->PopClipRectangle();
	}
}

ITr2SpriteObject* Tr2Sprite2dTextObject::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return nullptr;
	}

	if( m_pickState == TR2_SPS_ON || m_hasAuxiliaryTooltip)
	{
		if( renderer->IsInside( Vector2( x, y ), m_translation, m_displayWidth, m_displayHeight, m_pickRadius ) )
		{
			return this;
		}
	}
	
	return nullptr;
}

unsigned int Tr2Sprite2dTextObject::GetVertexCount()
{
	if( !m_display || !m_fontMeasurer )
	{
		return 0;
	}
	return m_fontMeasurer->GetVertexCount();
}

void Tr2Sprite2dTextObject::ReleaseResources( TriStorage s )
{
	SetDirty();
	if( m_fontMeasurer )
	{
		m_fontMeasurer->ClearSprites();
	}
}

bool Tr2Sprite2dTextObject::OnPrepareResources()
{
	return true;
}

void Tr2Sprite2dTextObject::SetFontMeasurer( Tr2FontMeasurer* m )
{
	if( m != m_fontMeasurer )
	{
		if( m_fontMeasurer )
		{
			m_fontMeasurer->UnregisterForChangeNotification( this );
		}

		m_fontMeasurer = m;

		if( m_fontMeasurer )
		{
			m_fontMeasurer->RegisterForChangeNotification( this );
		}

		SetDirty();
	}
}

bool Tr2Sprite2dTextObject::IsAuxMouseover()
{
	// Return true if the label is not pickable and is a localization tooltip
	return (m_pickState != TR2_SPS_ON && m_hasAuxiliaryTooltip);
}


Tr2FontMeasurer* Tr2Sprite2dTextObject::GetFontMeasurer() const
{
	return m_fontMeasurer;
}

void Tr2Sprite2dTextObject::FontMeasurerChanged( Tr2FontMeasurer* p )
{
	SetDirty();
}

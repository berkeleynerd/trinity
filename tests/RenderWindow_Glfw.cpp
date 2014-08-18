#include "StdAfx.h"
#if !defined(__ORBIS__) && !defined(_WIN32) && !defined(TRINITY_AL_MOBILE) && (TRINITY_PLATFORM != TRINITY_STUB)
#include "RenderWindow.h"
#include "WithWindowFixture.h"
#include "GLFW/glfw3.h"

RenderWindow::RenderWindow( uint32_t width, uint32_t height )
{
    static bool glfwInitialized = false;
    if( !glfwInitialized )
    {
        glfwInitialized = true;
        glfwInit();
    }
    m_handle = (Tr2WindowHandle) glfwCreateWindow( width, height, g_moduleName, nullptr, reinterpret_cast<GLFWwindow*>( WithWindow::GetWindowHandle() ) );
}

RenderWindow::~RenderWindow()
{
    glfwDestroyWindow( reinterpret_cast<GLFWwindow*>( m_handle ) );
}

uint32_t RenderWindow::GetClientWidth() const
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( m_handle ), &width, &height );
	return uint32_t( width );
}

uint32_t RenderWindow::GetClientHeight() const
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( m_handle ), &width, &height );
	return uint32_t( height );
}
#endif
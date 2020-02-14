#include "StdAfx.h"
#if !defined(_WIN32) && !defined(__ANDROID__)

#include "App.h"
#include "../TriDevice.h"
#include "Scancodes.h"
#include "GLFW/glfw3.h"

namespace
{
    const int32_t WM_DESTROY = 0x0002;
    const int32_t WM_ACTIVATE = 0x0006;
    const int32_t WM_CLOSE = 0x0010;
    const int32_t WM_MOUSEMOVE = 0x0200;
    const int32_t WM_LBUTTONDOWN = 0x0201;
    const int32_t WM_LBUTTONUP = 0x0202;
    const int32_t WM_RBUTTONDOWN = 0x0204;
    const int32_t WM_RBUTTONUP = 0x0205;
    const int32_t WM_MBUTTONDOWN = 0x0207;
    const int32_t WM_MBUTTONUP = 0x0207;
    const int32_t WM_MOUSEWHEEL = 0x020A;
    const int32_t WM_KEYDOWN = 0x0100;
    const int32_t WM_SYSKEYDOWN = 0x0104;
    const int32_t WM_KEYUP = 0x0101;
    const int32_t WM_SYSKEYUP = 0x0105;
    const int32_t WM_CHAR = 0x0102;
    
    
    
void OnAppWindowResized( GLFWwindow*, int width, int height )
{
    if( g_app )
    {
        g_app->OnAppWindowResized( width, height );
    }
}
    
    void OnAppWindowClosed( GLFWwindow* )
    {
        if( g_app )
        {
            g_app->OnAppWindowClosed();
        }
    }
    
    void OnAppWindowFocused( GLFWwindow*, int focused )
    {
        if( g_app )
        {
            g_app->OnAppWindowFocused( focused == GL_TRUE );
        }
    }
    
    void OnAppMouseMove( GLFWwindow*, double x, double y )
    {
        if( g_app )
        {
            g_app->OnAppMouseMove( int( x ), int( y ) );
        }
    }
    
    void OnAppMouseButton( GLFWwindow*, int button, int action, int mods )
    {
        if( g_app )
        {
            uint32_t message;
            if( action == GLFW_PRESS )
            {
                switch ( button )
                {
                    case GLFW_MOUSE_BUTTON_LEFT:
                        message = WM_LBUTTONDOWN;
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        message = WM_RBUTTONDOWN;
                        break;
                    case GLFW_MOUSE_BUTTON_MIDDLE:
                        message = WM_MBUTTONDOWN;
                        break;
                    default:
                        return;
                }
            }
            else
            {
                switch ( button )
                {
                    case GLFW_MOUSE_BUTTON_LEFT:
                        message = WM_LBUTTONUP;
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        message = WM_RBUTTONUP;
                        break;
                    case GLFW_MOUSE_BUTTON_MIDDLE:
                        message = WM_MBUTTONUP;
                        break;
                    default:
                        return;
                }
            }
            g_app->OnAppMouseButton( message, button, mods );
        }
    }
    
    void OnAppMouseScroll( GLFWwindow*, double xoffset, double yoffset )
    {
        if( g_app )
        {
            g_app->OnAppMouseScroll( int( xoffset * 120 ), int( yoffset * 120 ) );
        }
    }
    
    uint32_t VKeyToGlfwKey( uint8_t vKey )
    {
        static uint32_t map[256];
        static bool initialized = false;
        if( !initialized )
        {
            memset( map, 0, sizeof( map ) );
            map[VK_SPACE] = GLFW_KEY_SPACE;
            map['\''] = GLFW_KEY_APOSTROPHE;
            map[','] = GLFW_KEY_COMMA;
            map['-'] = GLFW_KEY_MINUS;
            map['.'] = GLFW_KEY_PERIOD;
            map['/'] = GLFW_KEY_SLASH;
            map[VK_0] = GLFW_KEY_0;
            map[VK_1] = GLFW_KEY_1;
            map[VK_2] = GLFW_KEY_2;
            map[VK_3] = GLFW_KEY_3;
            map[VK_4] = GLFW_KEY_4;
            map[VK_5] = GLFW_KEY_5;
            map[VK_6] = GLFW_KEY_6;
            map[VK_7] = GLFW_KEY_7;
            map[VK_8] = GLFW_KEY_8;
            map[VK_9] = GLFW_KEY_9;
            map[';'] = GLFW_KEY_SEMICOLON;
            map['='] = GLFW_KEY_EQUAL;
            map[VK_A] = GLFW_KEY_A;
            map[VK_B] = GLFW_KEY_B;
            map[VK_C] = GLFW_KEY_C;
            map[VK_D] = GLFW_KEY_D;
            map[VK_E] = GLFW_KEY_E;
            map[VK_F] = GLFW_KEY_F;
            map[VK_G] = GLFW_KEY_G;
            map[VK_H] = GLFW_KEY_H;
            map[VK_I] = GLFW_KEY_I;
            map[VK_J] = GLFW_KEY_J;
            map[VK_K] = GLFW_KEY_K;
            map[VK_L] = GLFW_KEY_L;
            map[VK_M] = GLFW_KEY_M;
            map[VK_N] = GLFW_KEY_N;
            map[VK_O] = GLFW_KEY_O;
            map[VK_P] = GLFW_KEY_P;
            map[VK_Q] = GLFW_KEY_Q;
            map[VK_R] = GLFW_KEY_R;
            map[VK_S] = GLFW_KEY_S;
            map[VK_T] = GLFW_KEY_T;
            map[VK_U] = GLFW_KEY_U;
            map[VK_V] = GLFW_KEY_V;
            map[VK_W] = GLFW_KEY_W;
            map[VK_X] = GLFW_KEY_X;
            map[VK_Y] = GLFW_KEY_Y;
            map[VK_Z] = GLFW_KEY_Z;
            map['['] = GLFW_KEY_LEFT_BRACKET;
            map['\\'] = GLFW_KEY_BACKSLASH;
            map[']'] = GLFW_KEY_RIGHT_BRACKET;
            map['~'] = GLFW_KEY_GRAVE_ACCENT;
            map[VK_ESCAPE] = GLFW_KEY_ESCAPE;
            map[VK_RETURN] = GLFW_KEY_ENTER;
            map[VK_TAB] = GLFW_KEY_TAB;
            map[VK_BACK] = GLFW_KEY_BACKSPACE;
            map[VK_INSERT] = GLFW_KEY_INSERT;
            map[VK_DELETE] = GLFW_KEY_DELETE;
            map[VK_RIGHT] = GLFW_KEY_RIGHT;
            map[VK_LEFT] = GLFW_KEY_LEFT;
            map[VK_DOWN] = GLFW_KEY_DOWN;
            map[VK_UP] = GLFW_KEY_UP;
            map[VK_PRIOR] = GLFW_KEY_PAGE_UP;
            map[VK_NEXT] = GLFW_KEY_PAGE_DOWN;
            map[VK_HOME] = GLFW_KEY_HOME;
            map[VK_END] = GLFW_KEY_END;
            map[VK_CAPITAL] = GLFW_KEY_CAPS_LOCK;
            map[VK_SCROLL] = GLFW_KEY_SCROLL_LOCK;
            map[VK_NUMLOCK] = GLFW_KEY_NUM_LOCK;
            map[VK_PRINT] = GLFW_KEY_PRINT_SCREEN;
            map[VK_PAUSE] = GLFW_KEY_PAUSE;
            map[VK_F1] = GLFW_KEY_F1;
            map[VK_F2] = GLFW_KEY_F2;
            map[VK_F3] = GLFW_KEY_F3;
            map[VK_F4] = GLFW_KEY_F4;
            map[VK_F5] = GLFW_KEY_F5;
            map[VK_F6] = GLFW_KEY_F6;
            map[VK_F7] = GLFW_KEY_F7;
            map[VK_F8] = GLFW_KEY_F8;
            map[VK_F9] = GLFW_KEY_F9;
            map[VK_F10] = GLFW_KEY_F10;
            map[VK_F11] = GLFW_KEY_F11;
            map[VK_F12] = GLFW_KEY_F12;
            map[VK_F13] = GLFW_KEY_F13;
            map[VK_F14] = GLFW_KEY_F14;
            map[VK_F15] = GLFW_KEY_F15;
            map[VK_F16] = GLFW_KEY_F16;
            map[VK_F17] = GLFW_KEY_F17;
            map[VK_F18] = GLFW_KEY_F18;
            map[VK_F19] = GLFW_KEY_F19;
            map[VK_F20] = GLFW_KEY_F20;
            map[VK_F21] = GLFW_KEY_F21;
            map[VK_F22] = GLFW_KEY_F22;
            map[VK_F23] = GLFW_KEY_F23;
            map[VK_F24] = GLFW_KEY_F24;
            map[VK_NUMPAD0] = GLFW_KEY_KP_0;
            map[VK_NUMPAD1] = GLFW_KEY_KP_1;
            map[VK_NUMPAD2] = GLFW_KEY_KP_2;
            map[VK_NUMPAD3] = GLFW_KEY_KP_3;
            map[VK_NUMPAD4] = GLFW_KEY_KP_4;
            map[VK_NUMPAD5] = GLFW_KEY_KP_5;
            map[VK_NUMPAD6] = GLFW_KEY_KP_6;
            map[VK_NUMPAD7] = GLFW_KEY_KP_7;
            map[VK_NUMPAD8] = GLFW_KEY_KP_8;
            map[VK_NUMPAD9] = GLFW_KEY_KP_9;
            map[VK_DECIMAL] = GLFW_KEY_KP_DECIMAL;
            map[VK_DIVIDE] = GLFW_KEY_KP_DIVIDE;
            map[VK_MULTIPLY] = GLFW_KEY_KP_MULTIPLY;
            map[VK_SUBTRACT] = GLFW_KEY_KP_SUBTRACT;
            map[VK_ADD] = GLFW_KEY_KP_ADD;
            map[VK_RETURN] = GLFW_KEY_KP_ENTER;
            map[VK_LSHIFT] = GLFW_KEY_LEFT_SHIFT;
            map[VK_LCONTROL] = GLFW_KEY_LEFT_CONTROL;
            map[VK_LMENU] = GLFW_KEY_LEFT_ALT;
            map[VK_LWIN] = GLFW_KEY_LEFT_SUPER;
            map[VK_RSHIFT] = GLFW_KEY_RIGHT_SHIFT;
            map[VK_RCONTROL] = GLFW_KEY_RIGHT_CONTROL;
            map[VK_RMENU] = GLFW_KEY_RIGHT_ALT;
            map[VK_RWIN] = GLFW_KEY_RIGHT_SUPER;
            map[VK_MENU] = GLFW_KEY_MENU;
            initialized = true;
        }
        return map[vKey];
    }
    
    uint8_t GlfwKeyToVKey( uint32_t glfwKey )
    {
        static uint8_t map[GLFW_KEY_LAST + 1];
        static bool initialized = false;
        if( !initialized )
        {
            memset( map, 0, sizeof( map ) );
            map[GLFW_KEY_SPACE] = VK_SPACE;
            map[GLFW_KEY_APOSTROPHE] = '\'';
            map[GLFW_KEY_COMMA] = ',';
            map[GLFW_KEY_MINUS] = '-';
            map[GLFW_KEY_PERIOD] = '.';
            map[GLFW_KEY_SLASH] = '/';
            map[GLFW_KEY_0] = VK_0;
            map[GLFW_KEY_1] = VK_1;
            map[GLFW_KEY_2] = VK_2;
            map[GLFW_KEY_3] = VK_3;
            map[GLFW_KEY_4] = VK_4;
            map[GLFW_KEY_5] = VK_5;
            map[GLFW_KEY_6] = VK_6;
            map[GLFW_KEY_7] = VK_7;
            map[GLFW_KEY_8] = VK_8;
            map[GLFW_KEY_9] = VK_9;
            map[GLFW_KEY_SEMICOLON] = ';';
            map[GLFW_KEY_EQUAL] = '=';
            map[GLFW_KEY_A] = VK_A;
            map[GLFW_KEY_B] = VK_B;
            map[GLFW_KEY_C] = VK_C;
            map[GLFW_KEY_D] = VK_D;
            map[GLFW_KEY_E] = VK_E;
            map[GLFW_KEY_F] = VK_F;
            map[GLFW_KEY_G] = VK_G;
            map[GLFW_KEY_H] = VK_H;
            map[GLFW_KEY_I] = VK_I;
            map[GLFW_KEY_J] = VK_J;
            map[GLFW_KEY_K] = VK_K;
            map[GLFW_KEY_L] = VK_L;
            map[GLFW_KEY_M] = VK_M;
            map[GLFW_KEY_N] = VK_N;
            map[GLFW_KEY_O] = VK_O;
            map[GLFW_KEY_P] = VK_P;
            map[GLFW_KEY_Q] = VK_Q;
            map[GLFW_KEY_R] = VK_R;
            map[GLFW_KEY_S] = VK_S;
            map[GLFW_KEY_T] = VK_T;
            map[GLFW_KEY_U] = VK_U;
            map[GLFW_KEY_V] = VK_V;
            map[GLFW_KEY_W] = VK_W;
            map[GLFW_KEY_X] = VK_X;
            map[GLFW_KEY_Y] = VK_Y;
            map[GLFW_KEY_Z] = VK_Z;
            map[GLFW_KEY_LEFT_BRACKET] = '[';
            map[GLFW_KEY_BACKSLASH] = '\\';
            map[GLFW_KEY_RIGHT_BRACKET] = ']';
            map[GLFW_KEY_GRAVE_ACCENT] = '~';
            map[GLFW_KEY_ESCAPE] = VK_ESCAPE;
            map[GLFW_KEY_ENTER] = VK_RETURN;
            map[GLFW_KEY_TAB] = VK_TAB;
            map[GLFW_KEY_BACKSPACE] = VK_BACK;
            map[GLFW_KEY_INSERT] = VK_INSERT;
            map[GLFW_KEY_DELETE] = VK_DELETE;
            map[GLFW_KEY_RIGHT] = VK_RIGHT;
            map[GLFW_KEY_LEFT] = VK_LEFT;
            map[GLFW_KEY_DOWN] = VK_DOWN;
            map[GLFW_KEY_UP] = VK_UP;
            map[GLFW_KEY_PAGE_UP] = VK_PRIOR;
            map[GLFW_KEY_PAGE_DOWN] = VK_NEXT;
            map[GLFW_KEY_HOME] = VK_HOME;
            map[GLFW_KEY_END] = VK_END;
            map[GLFW_KEY_CAPS_LOCK] = VK_CAPITAL;
            map[GLFW_KEY_SCROLL_LOCK] = VK_SCROLL;
            map[GLFW_KEY_NUM_LOCK] = VK_NUMLOCK;
            map[GLFW_KEY_PRINT_SCREEN] = VK_PRINT;
            map[GLFW_KEY_PAUSE] = VK_PAUSE;
            map[GLFW_KEY_F1] = VK_F1;
            map[GLFW_KEY_F2] = VK_F2;
            map[GLFW_KEY_F3] = VK_F3;
            map[GLFW_KEY_F4] = VK_F4;
            map[GLFW_KEY_F5] = VK_F5;
            map[GLFW_KEY_F6] = VK_F6;
            map[GLFW_KEY_F7] = VK_F7;
            map[GLFW_KEY_F8] = VK_F8;
            map[GLFW_KEY_F9] = VK_F9;
            map[GLFW_KEY_F10] = VK_F10;
            map[GLFW_KEY_F11] = VK_F11;
            map[GLFW_KEY_F12] = VK_F12;
            map[GLFW_KEY_F13] = VK_F13;
            map[GLFW_KEY_F14] = VK_F14;
            map[GLFW_KEY_F15] = VK_F15;
            map[GLFW_KEY_F16] = VK_F16;
            map[GLFW_KEY_F17] = VK_F17;
            map[GLFW_KEY_F18] = VK_F18;
            map[GLFW_KEY_F19] = VK_F19;
            map[GLFW_KEY_F20] = VK_F20;
            map[GLFW_KEY_F21] = VK_F21;
            map[GLFW_KEY_F22] = VK_F22;
            map[GLFW_KEY_F23] = VK_F23;
            map[GLFW_KEY_F24] = VK_F24;
            //map[GLFW_KEY_F25] = VK_F25;
            map[GLFW_KEY_KP_0] = VK_NUMPAD0;
            map[GLFW_KEY_KP_1] = VK_NUMPAD1;
            map[GLFW_KEY_KP_2] = VK_NUMPAD2;
            map[GLFW_KEY_KP_3] = VK_NUMPAD3;
            map[GLFW_KEY_KP_4] = VK_NUMPAD4;
            map[GLFW_KEY_KP_5] = VK_NUMPAD5;
            map[GLFW_KEY_KP_6] = VK_NUMPAD6;
            map[GLFW_KEY_KP_7] = VK_NUMPAD7;
            map[GLFW_KEY_KP_8] = VK_NUMPAD8;
            map[GLFW_KEY_KP_9] = VK_NUMPAD9;
            map[GLFW_KEY_KP_DECIMAL] = VK_DECIMAL;
            map[GLFW_KEY_KP_DIVIDE] = VK_DIVIDE;
            map[GLFW_KEY_KP_MULTIPLY] = VK_MULTIPLY;
            map[GLFW_KEY_KP_SUBTRACT] = VK_SUBTRACT;
            map[GLFW_KEY_KP_ADD] = VK_ADD;
            map[GLFW_KEY_KP_ENTER] = VK_RETURN;
            map[GLFW_KEY_LEFT_SHIFT] = VK_LSHIFT;
            map[GLFW_KEY_LEFT_CONTROL] = VK_LCONTROL;
            map[GLFW_KEY_LEFT_ALT] = VK_LMENU;
            map[GLFW_KEY_LEFT_SUPER] = VK_LWIN;
            map[GLFW_KEY_RIGHT_SHIFT] = VK_RSHIFT;
            map[GLFW_KEY_RIGHT_CONTROL] = VK_RCONTROL;
            map[GLFW_KEY_RIGHT_ALT] = VK_RMENU;
            map[GLFW_KEY_RIGHT_SUPER] = VK_RWIN;
            map[GLFW_KEY_MENU] = VK_MENU;
            initialized = true;
        }
        if( glfwKey <= GLFW_KEY_LAST )
        {
            return map[glfwKey];
        }
        return 0;
    }

    const char* VKeyToString( uint8_t vKey )
    {
        static const char* map[256];
        static bool initialized = false;
        if( !initialized )
        {
            memset( map, 0, sizeof( map ) );
            map[VK_SPACE] = " ";
            map['\''] = "\'";
            map[','] = ",";
            map['-'] = "-";
            map['.'] = ".";
            map['/'] = "/";
            map[VK_0] = "0";
            map[VK_1] = "1";
            map[VK_2] = "2";
            map[VK_3] = "3";
            map[VK_4] = "4";
            map[VK_5] = "5";
            map[VK_6] = "6";
            map[VK_7] = "7";
            map[VK_8] = "8";
            map[VK_9] = "9";
            map[';'] = ";";
            map['='] = "=";
            map[VK_A] = "A";
            map[VK_B] = "B";
            map[VK_C] = "C";
            map[VK_D] = "D";
            map[VK_E] = "E";
            map[VK_F] = "F";
            map[VK_G] = "G";
            map[VK_H] = "H";
            map[VK_I] = "I";
            map[VK_J] = "J";
            map[VK_K] = "K";
            map[VK_L] = "L";
            map[VK_M] = "M";
            map[VK_N] = "N";
            map[VK_O] = "O";
            map[VK_P] = "P";
            map[VK_Q] = "Q";
            map[VK_R] = "R";
            map[VK_S] = "S";
            map[VK_T] = "T";
            map[VK_U] = "U";
            map[VK_V] = "V";
            map[VK_W] = "W";
            map[VK_X] = "X";
            map[VK_Y] = "Y";
            map[VK_Z] = "Z";
            map['['] = "[";
            map['\\'] = "\\";
            map[']'] = "]";
            map['~'] = "~";
            map[VK_ESCAPE] = "Esc";
            map[VK_RETURN] = "Return";
            map[VK_TAB] = "Tab";
            map[VK_BACK] = "Backspace";
            map[VK_INSERT] = "Isert";
            map[VK_DELETE] = "Delete";
            map[VK_RIGHT] = "Right";
            map[VK_LEFT] = "Left";
            map[VK_DOWN] = "Down";
            map[VK_UP] = "Up";
            map[VK_PRIOR] = "PgUp";
            map[VK_NEXT] = "PgDn";
            map[VK_HOME] = "Home";
            map[VK_END] = "End";
            map[VK_CAPITAL] = "Caps";
            map[VK_SCROLL] = "Scroll";
            map[VK_NUMLOCK] = "Num";
            map[VK_PRINT] = "PtSc";
            map[VK_PAUSE] = "Pause";
            map[VK_F1] = "F1";
            map[VK_F2] = "F2";
            map[VK_F3] = "F3";
            map[VK_F4] = "F4";
            map[VK_F5] = "F5";
            map[VK_F6] = "F6";
            map[VK_F7] = "F7";
            map[VK_F8] = "F8";
            map[VK_F9] = "F9";
            map[VK_F10] = "F10";
            map[VK_F11] = "F11";
            map[VK_F12] = "F12";
            map[VK_F13] = "F13";
            map[VK_F14] = "F14";
            map[VK_F15] = "F15";
            map[VK_F16] = "F16";
            map[VK_F17] = "F17";
            map[VK_F18] = "F18";
            map[VK_F19] = "F19";
            map[VK_F20] = "F20";
            map[VK_F21] = "F21";
            map[VK_F22] = "F22";
            map[VK_F23] = "F23";
            map[VK_F24] = "F24";
            map[VK_NUMPAD0] = "0";
            map[VK_NUMPAD1] = "1";
            map[VK_NUMPAD2] = "2";
            map[VK_NUMPAD3] = "3";
            map[VK_NUMPAD4] = "4";
            map[VK_NUMPAD5] = "5";
            map[VK_NUMPAD6] = "6";
            map[VK_NUMPAD7] = "7";
            map[VK_NUMPAD8] = "8";
            map[VK_NUMPAD9] = "9";
            map[VK_DECIMAL] = ".";
            map[VK_DIVIDE] = "/";
            map[VK_MULTIPLY] = "*";
            map[VK_SUBTRACT] = "-";
            map[VK_ADD] = "+";
            map[VK_RETURN] = "Enter";
            map[VK_LSHIFT] = "LShift";
            map[VK_LCONTROL] = "LControl";
            map[VK_LMENU] = "LAlt";
            map[VK_LWIN] = "LSup";
            map[VK_RSHIFT] = "RShift";
            map[VK_RCONTROL] = "RControl";
            map[VK_RMENU] = "RAlt";
            map[VK_RWIN] = "RSup";
            map[VK_MENU] = "Alt";
            initialized = true;
        }
        return map[vKey];
    }

    void OnAppKey( GLFWwindow*, int key, int mods, int action )
    {
        if( g_app )
        {
            switch( action )
            {
                case GLFW_PRESS:
                    g_app->OnAppKeyDown( key, mods );
                    break;
                case GLFW_RELEASE:
                    g_app->OnAppKeyUp( key, mods );
                    break;
            }
        }
    }
    
    void OnAppChar( GLFWwindow*, unsigned character )
    {
        if( g_app )
        {
            g_app->OnAppChar( character );
        }
    }
    
}

void App::RegisterWindowCallbacks()
{
    glfwSetWindowSizeCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppWindowResized );
    glfwSetWindowCloseCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppWindowClosed );
    glfwSetWindowFocusCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppWindowFocused );
    glfwSetCursorPosCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppMouseMove );
    glfwSetMouseButtonCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppMouseButton );
    glfwSetScrollCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppMouseScroll );
    glfwSetKeyCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppKey );
    glfwSetCharCallback( reinterpret_cast<GLFWwindow*>( mHwnd ), &::OnAppChar );
}

void App::OnAppWindowResized( int width, int height )
{
    mActive = true;
    if( width == 0 || height == 0 )
    {
        mActive = false;
    }
    else if( !mIsResizing && !mFixedWindow )
    {
        mSendResizeEvent = true;
    }
    mIsHidden = !mActive;
}

void App::OnAppWindowClosed()
{
    long lRes = 0;
    if( CallEventHandler( WM_CLOSE, 0, 0, lRes ) )
    {
        glfwSetWindowShouldClose( reinterpret_cast<GLFWwindow*>( mHwnd ), 0 );
    }
    else
    {
        mHwnd = NULL;
        Destroy();
    }
}

void App::OnAppWindowFocused( bool focused )
{
    if( focused )
    {
        mActive = true;
        gTriDev->ApplicationActivated( TriDevice::APP_ACTIVATED );
    }
    else
    {
        mActive = false;
        gTriDev->ApplicationActivated( TriDevice::APP_DEACTIVATED );
    }
    
	if( gTriDev )
	{
		gTriDev->SetTickInterval( mActive ? 0 : 10 );
	}

    long lRes;
    CallEventHandler( WM_ACTIVATE, focused ? 1 : 0, 0, lRes );
}

void App::OnAppMouseMove( int x, int y )
{
    int width, height;
    glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( mHwnd ), &width, &height );
    if( x < 0 || y < 0 || x >= width || y >= height )
    {
        return;
    }
    long lRes;
    x = std::min( std::max( x, 0 ), 0xffff );
    y = std::min( std::max( y, 0 ), 0xffff );
    uintptr_t wParam = 0;
    if( glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_LCTRL ) == GLFW_PRESS ||
       glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_RCTRL ) == GLFW_PRESS )
    {
        wParam |= 0x0008;
    }
    if( glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_LSHIFT ) == GLFW_PRESS ||
       glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_RSHIFT ) == GLFW_PRESS )
    {
        wParam |= 0x0004;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS )
    {
        wParam |= 0x0001;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS )
    {
        wParam |= 0x0002;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_MIDDLE ) == GLFW_PRESS )
    {
        wParam |= 0x0010;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_4 ) == GLFW_PRESS )
    {
        wParam |= 0x0020;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_5 ) == GLFW_PRESS )
    {
        wParam |= 0x0040;
    }
    CallEventHandler(
                     WM_MOUSEMOVE,
                     wParam,
                     ( uintptr_t( y ) << 16 ) | uintptr_t( x ),
                     lRes );
}

void App::OnAppMouseButton( uint32_t message, int button, int mods )
{
    // TODO: ClipCursor?
    double xPos, yPos;
    glfwGetCursorPos( reinterpret_cast<GLFWwindow*>( mHwnd ), &xPos, &yPos );
    int x = int( xPos );
    int y = int( yPos );
    x = std::min( std::max( x, 0 ), 0xffff );
    y = std::min( std::max( y, 0 ), 0xffff );
    uintptr_t wParam = 0;
    if( mods & GLFW_MOD_CONTROL )
    {
        wParam |= 0x0008;
    }
    if( mods & GLFW_MOD_SHIFT )
    {
        wParam |= 0x0004;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS )
    {
        wParam |= 0x0001;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS )
    {
        wParam |= 0x0002;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_MIDDLE ) == GLFW_PRESS )
    {
        wParam |= 0x0010;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_4 ) == GLFW_PRESS )
    {
        wParam |= 0x0020;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_5 ) == GLFW_PRESS )
    {
        wParam |= 0x0040;
    }
    long lRes;
    CallEventHandler(
                     message,
                     wParam,
                     ( uintptr_t( y ) << 16 ) | uintptr_t( x ),
                     lRes );
}

void App::OnAppMouseScroll( int xoffset, int yoffset )
{
    if( yoffset == 0 )
    {
        return;
    }
    double xPos, yPos;
    glfwGetCursorPos( reinterpret_cast<GLFWwindow*>( mHwnd ), &xPos, &yPos );
    int x = int( xPos );
    int y = int( yPos );
    x = std::min( std::max( x, 0 ), 0xffff );
    y = std::min( std::max( y, 0 ), 0xffff );
    long lRes;
    x = std::min( std::max( x, 0 ), 0xffff );
    y = std::min( std::max( y, 0 ), 0xffff );
    uintptr_t wParam = 0;
    if( glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_LCTRL ) == GLFW_PRESS ||
       glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_RCTRL ) == GLFW_PRESS )
    {
        wParam |= 0x0008;
    }
    if( glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_LSHIFT ) == GLFW_PRESS ||
       glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_KEY_RSHIFT ) == GLFW_PRESS )
    {
        wParam |= 0x0004;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS )
    {
        wParam |= 0x0001;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS )
    {
        wParam |= 0x0002;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_MIDDLE ) == GLFW_PRESS )
    {
        wParam |= 0x0010;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_4 ) == GLFW_PRESS )
    {
        wParam |= 0x0020;
    }
    if( glfwGetMouseButton( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_MOUSE_BUTTON_5 ) == GLFW_PRESS )
    {
        wParam |= 0x0040;
    }
    wParam |= ( (uintptr_t)( yoffset ) ) << 16;
    CallEventHandler(
                     WM_MOUSEWHEEL,
                     wParam,
                     ( uintptr_t( y ) << 16 ) | uintptr_t( x ),
                     lRes );
}

void App::OnAppKeyDown( int key, int mods )
{
    uintptr_t wParam = GlfwKeyToVKey( key );
    if( wParam == 0 )
    {
        return;
    }
    uint32_t message = WM_KEYDOWN;
    long lRes;
    CallEventHandler(
                     message,
                     wParam,
                     0,
                     lRes );
}

void App::OnAppKeyUp( int key, int mods )
{
    uintptr_t wParam = GlfwKeyToVKey( key );
    if( wParam == 0 )
    {
        return;
    }
    uint32_t message = WM_KEYUP;
    long lRes;
    CallEventHandler(
                     message,
                     wParam,
                     0,
                     lRes );
    
}

void App::OnAppChar( unsigned character )
{
    wchar_t ustr[2] = { wchar_t( character ), 0 };
    CW2A str( ustr );
    long lRes;
    CallEventHandler(
                     WM_CHAR,
                     uintptr_t( ( (const char*)str )[0] ),
                     0,
                     lRes );
}

uint32_t App::GetKeyState( uint32_t vKeyCode )
{
    if( vKeyCode > 255 || !mHwnd )
    {
        return 0;
    }
    uint32_t glfwKey = VKeyToGlfwKey( uint8_t( vKeyCode ) );
    return glfwGetKey( reinterpret_cast<GLFWwindow*>( mHwnd ), glfwKey ) == GLFW_PRESS ? 0x8000 : 0;
}

bool App::GetKeyName( uint32_t vKeyCode, char* buffer, size_t bufferSize )
{
    if( vKeyCode > 255 )
    {
        return false;
    }
    const char* name = VKeyToString( vKeyCode );
    if( !name )
    {
        return false;
    }
    strcpy( buffer, name );
    return true;
}

// -------------------------------------------------------------
// Description:
//   Get the virtual screen width(total desktop width)
// Return Value:
//   Total desktop width
// -------------------------------------------------------------
int App::GetVirtualScreenWidth() const
{
	return 640;
}

// -------------------------------------------------------------
// Description:
//   Get the virtual screen width(total desktop height)
// Return Value:
//   Total desktop height
// -------------------------------------------------------------
int App::GetVirtualScreenHeight() const
{
	return 480;
}


#endif
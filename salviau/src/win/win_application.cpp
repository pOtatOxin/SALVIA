#include <salviau/include/win/win_application.h>

#include <salviau/src/win/resource.h>
#include <salviau/include/common/window.h>

#include <eflib/include/platform/constant.h>
#include <eflib/include/string/string.h>

#include <eflib/include/platform/boost_begin.h>
#include <boost/signals2.hpp>
#include <eflib/include/platform/boost_end.h>

#include <string>
#include <sstream>

#include <Windows.h>

using boost::signals2::signal;
using std::string;

BEGIN_NS_SALVIAU();

class win_application;
static win_application* g_app = nullptr;

class win_window: public window
{
	signal<void()> on_idle;
	signal<void()> on_paint;
	signal<void()> on_create;

public:
	win_window(win_application* app) : app_(app), hwnd_(nullptr)
	{
	}

	void show()
	{
		ShowWindow(hwnd_, SW_SHOWDEFAULT);
	}

	void set_idle_handler(idle_handler_t const& handler)
	{
		on_idle.connect( handler );
	}

	void set_draw_handler(draw_handler_t const& handler)
	{
		on_paint.connect(handler);
	}

	void set_create_handler(create_handler_t const& handler)
	{
		on_create.connect(handler);
	}

	void set_title( string const& title )
	{
		SetWindowText( hwnd_, eflib::to_tstring(title).c_str() );
	}

	boost::any view_handle()
	{
		return boost::any(hwnd_);
	}

	void idle()
	{
		on_idle();
	}

	void refresh()
	{
		InvalidateRect(hwnd_, nullptr, TRUE);
	}
	
	bool create();
	
private:
	ATOM register_window_class(HINSTANCE hinst)
	{
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= &win_proc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= GetModuleHandle(NULL);
		wcex.hIcon			= NULL; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32SAMPLEPROJECT));
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= wnd_class_name_;
		wcex.hIconSm		= NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		return RegisterClassEx(&wcex);
	}
	
	LRESULT process_message(UINT message, WPARAM wparam, LPARAM lparam)
	{
		PAINTSTRUCT ps;
		HDC hdc;

		switch (message)
		{
		case WM_CREATE:
			on_create();
			break;
		case WM_PAINT:
			hdc = BeginPaint(hwnd_, &ps);
			on_paint();
			EndPaint(hwnd_, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd_, message, wparam, lparam);
		}
		return 0;
	}

	static LRESULT CALLBACK	win_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	static ATOM					wnd_class_;
	static std::_tchar const*	wnd_class_name_;
	HWND				hwnd_;
	win_application*	app_;
};

ATOM win_window::wnd_class_ = 0;
std::_tchar const* win_window::wnd_class_name_ = _EFLIB_T("SalviaApp");

class win_application: public application
{
public:
	win_application()
	{
		::DefWindowProc(NULL, 0, 0, 0L);
		hinst_ = GetModuleHandle(nullptr);
		main_wnd_ = new win_window(this);
	}

	~win_application()
	{
		delete main_wnd_;
	}

	window* main_window()
	{
		return main_wnd_;
	}

	HINSTANCE instance() const
	{
		return hinst_;
	}

	int run()
	{
		if( !main_wnd_->create() )
		{
			OutputDebugString( _EFLIB_T("Main window creation failed!\n") );
			return 0;
		}

		// Message loop.
		main_wnd_->show();
		
		MSG msg;
		for (;;)
		{
			if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (WM_QUIT == msg.message)
				{
					break;        // WM_QUIT, exit message loop
				}

				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			else
			{
				main_wnd_->idle();
			}
		}

		return (int)msg.wParam;
	}

private:
	win_window*	main_wnd_;
	HINSTANCE	hinst_;
};

bool win_window::create()
{
	if (wnd_class_ == 0)
	{
		wnd_class_ = register_window_class( app_->instance() );
	}
	hwnd_ = CreateWindow(wnd_class_name_, _EFLIB_T(""), WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 512, 512, NULL, NULL, app_->instance(), NULL);
	if (!hwnd_)
	{
		auto err = GetLastError();
		
		LPTSTR lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		// Display the error message and exit the process
		std::wstringstream ss;
		ss << L"Window created failed. Error: " << std::hex << eflib::to_wide_string( std::_tstring(lpMsgBuf) ) << ".";
		OutputDebugStringW( ss.str().c_str() );
		LocalFree(lpMsgBuf);
		return false;
	}

	ShowWindow(hwnd_, SW_SHOW);
	UpdateWindow(hwnd_);

	return true;
}

LRESULT CALLBACK win_window::win_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	win_window* wnd = static_cast<win_window*>(g_app->main_window());
	wnd->hwnd_ = hwnd;
	return wnd->process_message(message, wparam, lparam);
}

application* create_win_application()
{
	if (g_app)
	{
		assert(false);
		exit(1);
	}
	g_app = new win_application();
	return g_app;
}

void delete_win_application(application* app)
{
	if (!dynamic_cast<win_application*>(app))
	{
		assert(false);
		exit(1);
	}
	
	delete app;
}

END_NS_SALVIAU();

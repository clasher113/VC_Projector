#include "Window.hpp"

#include "font_binary.hpp"
#include "gui/Menu.hpp"

#include <cstring>
#include <SFML/Window/Event.hpp>
#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#elif __linux__
#include <sys/shm.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#endif // _WIN32

const float BORDER_THICKNESS = 3.f;
const sf::Vector2u statusContainerSize(230, 30);

vcp::Window::Window(const sf::Vector2u& captureSize) :
	m_captureSize(0, 0),
	m_font(font_binary::getData(), font_binary::getSize()),
	m_statusText(m_font, "", 20U),
	m_statusContainerSprite(m_statusContainer.getTexture())
{
#ifdef _WIN32
	sf::RenderWindow::create(sf::VideoMode(sf::Vector2u(320, 240)), "", sf::Style::None);

	MARGINS margins{};
	margins.cxLeftWidth = -1;

	// enable window transparency
	SetWindowLong(getNativeHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	DwmExtendFrameIntoClientArea(getNativeHandle(), &margins);

	HWND desktop = GetDesktopWindow();
	m_desktopHdc = GetDC(desktop);
	m_hCaptureDC = CreateCompatibleDC(m_desktopHdc);
	m_hCaptureBitmap = CreateCompatibleBitmap(m_desktopHdc, captureSize.x, captureSize.y);
	m_hOldBitmap = SelectObject(m_hCaptureDC, m_hCaptureBitmap);

	m_bmi.bmiHeader.biBitCount = 32;
	m_bmi.bmiHeader.biCompression = BI_RGB;
	m_bmi.bmiHeader.biPlanes = 1;
	m_bmi.bmiHeader.biHeight = captureSize.y;
	m_bmi.bmiHeader.biWidth = captureSize.x;
	m_bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
#elif __linux__
	// make x11 window with transparent background
	m_p_display = XOpenDisplay(NULL);

	XVisualInfo vinfo;
	XMatchVisualInfo(m_p_display, DefaultScreen(m_p_display), 32, TrueColor, &vinfo);

	XSetWindowAttributes attr;
	attr.colormap = XCreateColormap(m_p_display, DefaultRootWindow(m_p_display), vinfo.visual, AllocNone);
	attr.border_pixel = 0;
	attr.background_pixel = 0;

	m_window = XCreateWindow(m_p_display, DefaultRootWindow(m_p_display), 0, 0, 300, 200, 0, vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
	XSelectInput(m_p_display, m_window, StructureNotifyMask);
	GC gc = XCreateGC(m_p_display, m_window, 0, 0);

	Atom wmDeleteWindow = XInternAtom(m_p_display, "WM_DELETE_WINDOW", 0);

	// disable window decorations
	Atom mwmHintsProperty = XInternAtom(m_p_display, "_MOTIF_WM_HINTS", 0);

	struct MwmHints {
		unsigned long flags{};
		unsigned long functions{};
		unsigned long decorations{};
		long input_mode{};
		unsigned long status{};
	};

	struct MwmHints hints;
	hints.flags = (1L << 1);
	hints.decorations = 0;
	XChangeProperty(m_p_display, m_window, mwmHintsProperty, mwmHintsProperty, 32, PropModeReplace, (unsigned char*)&hints, 5);
	XSetWMProtocols(m_p_display, m_window, &wmDeleteWindow, 1);

	sf::RenderWindow::create(m_window);

	m_rootWindow = DefaultRootWindow(m_p_display);
#endif // _WIN32
	
	setTitle("Projector server");
	setAlwaysOnTop(true);

	m_statusContainer.resize(sf::Vector2u(statusContainerSize.x, statusContainerSize.y));
	m_statusText.setPosition(sf::Vector2f(3.f, 0.f));

	m_border.setFillColor(sf::Color::Transparent);
	m_border.setOutlineColor(sf::Color::Yellow);
	m_border.setOutlineThickness(BORDER_THICKNESS);
	m_border.setPosition(sf::Vector2f(BORDER_THICKNESS, BORDER_THICKNESS));
	m_border.setSize(sf::Vector2f(captureSize.x, captureSize.y));

	m_p_menu = new gui::Menu(*this, m_font, captureSize, statusContainerSize.x - 10.f);
	m_p_menu->setOnModeChangeCallback([this]() {
		updateWindowSize(m_captureSize);
	});

	setSize(captureSize);
}

vcp::Window::~Window() {
	delete m_p_menu;
#ifdef _WIN32
	SelectObject(m_hCaptureDC, m_hOldBitmap);
	DeleteObject(m_hCaptureBitmap);
	DeleteDC(m_hCaptureDC);
	ReleaseDC(NULL, m_desktopHdc);
#elif __linux__
	if (m_p_ShmInfo) {
		shmdt(m_p_ShmInfo->shmaddr);
		//shmctl(m_p_ShmInfo->shmid, IPC_RMID, 0);
		XShmDetach(m_p_display, m_p_ShmInfo);
		delete m_p_ShmInfo;
	}
	if (m_p_xImage) XDestroyImage(m_p_xImage);
	if (m_p_display) XCloseDisplay(m_p_display);
#endif // _WIN32
	if (m_p_pixels != nullptr) delete[] m_p_pixels;
}

bool vcp::Window::setSize(const sf::Vector2u& size) {
	if (size == m_captureSize) return true;
	if (!updateWindowSize(size))
		return false;

	m_border.setSize(sf::Vector2f(size.x, size.y));
	m_statusContainerSprite.setPosition(sf::Vector2f(0.f, size.y + BORDER_THICKNESS * 2));
	m_p_menu->onSizeChange(size);

#ifdef _WIN32
	m_bmi.bmiHeader.biWidth = size.x;
	m_bmi.bmiHeader.biHeight = size.y;

	m_hCaptureBitmap = CreateCompatibleBitmap(m_desktopHdc, size.x, size.y);
	SelectObject(m_hCaptureDC, m_hCaptureBitmap);
#elif __linux__
	if (m_p_xImage) XDestroyImage(m_p_xImage);
	if (m_p_ShmInfo == nullptr) m_p_ShmInfo = new XShmSegmentInfo;

	int scr = XDefaultScreen(m_p_display);

	m_p_xImage = XShmCreateImage(m_p_display, DefaultVisual(m_p_display, scr), DefaultDepth(m_p_display, scr), ZPixmap,
		NULL, m_p_ShmInfo, m_captureSize.x, m_captureSize.y);

	m_p_ShmInfo->shmid = shmget(IPC_PRIVATE, m_p_xImage->bytes_per_line * m_p_xImage->height, IPC_CREAT | 0777);
	m_p_ShmInfo->readOnly = False;
	m_p_ShmInfo->shmaddr = m_p_xImage->data = (char*)shmat(m_p_ShmInfo->shmid, 0, 0);

	XShmAttach (m_p_display, m_p_ShmInfo);	
#endif // _WIN32
	if (m_p_pixels != nullptr) delete[] m_p_pixels;
	m_p_pixels = new sf::Color[size.x * size.y];
	m_updateRequire = true;
	return true;
}

void vcp::Window::setPosition(const sf::Vector2i& position) {
	sf::WindowBase::setPosition(position);
	m_updateRequire = true;
}

void vcp::Window::setStatus(Status status) {
	std::string statusStr;
	switch (status) {
		case Status::WAITING: statusStr = "Waiting for VC"; break;
		case Status::CONNECTED: statusStr = "Connected"; break;
		case Status::SYNCING: statusStr = "Syncing"; break;
		case Status::READY: statusStr = "Ready"; break;
		case Status::CAPTURING: statusStr = "Capturing"; break;
		case Status::INITIALIZING: statusStr = "Initializing"; break;
	}
	m_statusText.setString("Status: " + statusStr);
	m_updateRequire = true;
}

void vcp::Window::setAlwaysOnTop(bool flag) {
#ifdef _WIN32
	SetWindowPos(getNativeHandle(), flag ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#elif __linux__
	XClientMessageEvent xClient;
	xClient.type = ClientMessage;
	xClient.window = m_window;
	xClient.message_type = XInternAtom(m_p_display, "_NET_WM_STATE", False);
	xClient.format = 32;
	xClient.data.l[0] = static_cast<long>(flag);
	xClient.data.l[1] = XInternAtom(m_p_display, "_NET_WM_STATE_ABOVE", False);

	XSendEvent(m_p_display, m_rootWindow, False, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent*)&xClient);
	XFlush(m_p_display);
#endif // _WIN32
}

void vcp::Window::setIconified(bool flag) {
#ifdef _WIN32
	ShowWindow(getNativeHandle(), flag ? SW_SHOWMINIMIZED : SW_RESTORE);
#elif __linux__
	if (flag) {
		XIconifyWindow(m_p_display, m_window, DefaultScreen(m_p_display));
	}
	else {
		XClientMessageEvent xClient;
		xClient.type = ClientMessage;
		xClient.window = m_window;
		xClient.message_type = XInternAtom(m_p_display, "_NET_ACTIVE_WINDOW", True);
		xClient.format = 32;
		xClient.data.l[0] = 1;

		XSendEvent(m_p_display, m_rootWindow, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&xClient);
	}
	XFlush(m_p_display);
#endif // _WIN32
}

void vcp::Window::onUpdate(const float deltaTime) {
	m_p_menu->onUpdate(deltaTime, m_updateRequire);
}

void vcp::Window::pollEvents() {
	if (m_grabbed) m_grabbed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if (m_grabbed) {
		sf::WindowBase::setPosition(sf::Mouse::getPosition() + m_grabbedOffset);
		m_updateRequire = true;
	}

	while (const auto e = sf::WindowBase::pollEvent()) {
		m_p_menu->onEvent(e.value(), m_updateRequire, m_statusContainerSprite.getPosition());
		sf::Vector2f cursorPos(sf::Mouse::getPosition(*this));
		if (m_p_menu->isCursorOverElement(cursorPos - m_statusContainerSprite.getPosition())) continue;

		if (const auto pressed = e->getIf<sf::Event::MouseButtonPressed>()) {
			if (pressed->button == sf::Mouse::Button::Left) {
				m_grabbedOffset = sf::WindowBase::getPosition() - sf::Mouse::getPosition();
				m_grabbed = true;
			}
		}
		else if (const auto released = e->getIf<sf::Event::MouseButtonReleased>()) {
			if (released->button == sf::Mouse::Button::Left)
				m_grabbed = false;
			else if (released->button == sf::Mouse::Button::Right)
				sf::WindowBase::close();
		}
	}
}

const sf::Color* const vcp::Window::capture() {
	if (m_p_menu->getMode() == Mode::SCREEN) {
#ifdef _WIN32
		BitBlt(m_hCaptureDC, 0, 0, m_captureSize.x, m_captureSize.y, m_desktopHdc,
			static_cast<int>(getPosition().x + BORDER_THICKNESS), static_cast<int>(getPosition().y + BORDER_THICKNESS), SRCCOPY);
		GetDIBits(m_hCaptureDC, m_hCaptureBitmap, 0, m_captureSize.y, &m_p_pixels[0], &m_bmi, DIB_RGB_COLORS);
#elif __linux__
		// todo fix on wayland 
		// https://github.com/KDE/xwaylandvideobridge
		XShmGetImage(m_p_display, m_rootWindow, m_p_xImage, getPosition().x + BORDER_THICKNESS, getPosition().y + BORDER_THICKNESS, AllPlanes);
		unsigned int location = (m_captureSize.y - 1) * (m_captureSize.x * 4);
		for (int i = 0; i < m_captureSize.y; ++i) {
			memcpy(&m_p_pixels[i * (m_captureSize.x)], &m_p_xImage->data[location], m_captureSize.x * 4);
			location -= m_captureSize.x * 4;
		}
#endif // _WIN32
	} else if (m_p_menu->getMode() == Mode::IMAGE) {
		memcpy(m_p_pixels, m_p_menu->getContentPixels(), m_captureSize.x * m_captureSize.y * 4);
	}
	return m_p_pixels;
}

void vcp::Window::draw() {
	if (!m_updateRequire) return;
	m_statusContainer.clear(sf::Color::Black);
	m_statusContainer.draw(m_statusText);
	m_statusContainer.draw(*m_p_menu);
	m_statusContainer.display();

	clear(sf::Color::Transparent);
	sf::RenderWindow::draw(m_border);
	if (m_p_menu->getMode() != Mode::SCREEN) {
		sf::RenderWindow::draw(m_p_menu->getContent(), sf::RenderStates().transform.translate(sf::Vector2f(BORDER_THICKNESS, BORDER_THICKNESS)));
	}
	sf::RenderWindow::draw(m_statusContainerSprite);
	display();
	m_updateRequire = false;
}

bool vcp::Window::updateWindowSize(const sf::Vector2u& size) {
	const sf::Vector2u menuSize(m_p_menu->getSize());
	const sf::Vector2u newSize(std::max(size.x + static_cast<unsigned int>(BORDER_THICKNESS * 2), menuSize.x),
						   size.y + static_cast<unsigned int>(BORDER_THICKNESS * 2) + menuSize.y);

	if (m_statusContainer.getSize() != newSize) {
		if (!m_statusContainer.resize(sf::Vector2u(menuSize.x, menuSize.y)))
			return false;
		m_statusContainerSprite.setTexture(m_statusContainer.getTexture(), true);
		sf::WindowBase::setSize(newSize);
		setView(sf::View(sf::Vector2f(newSize.x / 2.f, newSize.y / 2.f), sf::Vector2f(newSize)));
		m_captureSize = size;
	}
	return true;
}
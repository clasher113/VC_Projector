#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <SFML/Network/Socket.hpp>
#ifdef _WIN32
#define NOMINMAX
#define LEAN_AND_MEAN
#include <Windows.h>
#elif __linux__
#include <cstdlib>
#endif // _WIN32

#define REFNSIZE(VALUE) &VALUE, sizeof(VALUE)

template<typename T>
extern inline T unpackData(const void*& src) {
    T ret;
    memcpy(&ret, src, sizeof(ret));
    src = static_cast<const uint8_t*>(src) + sizeof(ret);
    return ret;
}

extern inline void packData(std::vector<uint8_t>& dst, const void* src, uint32_t size) {
    size_t currentSize = dst.size();
    if (currentSize <= dst.capacity() + size) {
        dst.resize(currentSize + size);
    }
    memcpy(dst.data() + currentSize, src, size);
}

extern inline std::string toString(sf::Socket::Status status) {
    switch (status) {
        case sf::Socket::Status::Done: return "Done";
        case sf::Socket::Status::NotReady: return "NotReady";
        case sf::Socket::Status::Partial: return "Partial";
        case sf::Socket::Status::Disconnected: return "Disconnected";
        case sf::Socket::Status::Error: return "Error";
    }
    return "";
}

extern inline void showError(std::string_view message) {
    const std::string title = "VC Projector error";
#ifdef _WIN32
    MessageBoxA(NULL, message.data(), title.c_str(), MB_ICONERROR);
#elif __linux__
    std::string command = "zenity --info --title='" + title + "' --text='" + message.data() + "'";
    system(command.c_str());
#endif // _WIN32
}
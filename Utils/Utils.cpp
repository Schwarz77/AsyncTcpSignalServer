// Utils.cpp


#include "utils.h"
#include <iostream>
//#include <format>
#include <boost/format.hpp>

using error_code = boost::system::error_code;


std::string win32_message_english(DWORD code)
{
    LPVOID msgBuf = nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR)&msgBuf,
        0,
        nullptr
    );

    if (!msgBuf)
    {
        return "Unknown error";
    }

    std::string msg = (LPSTR)msgBuf;
    LocalFree(msgBuf);

    // remove extra line breaks
    while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n'))
    {
        msg.pop_back();
    }

    return msg;
}

void write_error(const std::string& text, const error_code& ec)
{
    std::string error;

#if defined(_WIN32)
    //error = std::format("{}: code={} {} \n", text, ec.value(), win32_message_english(ec.value())); // need C++20
    error = boost::str(boost::format("%1%: code=%2% %3%\n" ) % text % ec.value() % win32_message_english(ec.value()));
#else
    //error = std::format("{}: code={} {} \n", text, ec.value(), ec.message());
    error = boost::str(boost::format("%1%: code=%2% %3%\n") % text % ec.value() % (ec.what()));
#endif

    std::cerr << error;
}







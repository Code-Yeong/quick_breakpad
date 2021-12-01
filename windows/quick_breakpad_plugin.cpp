#include "include/quick_breakpad/quick_breakpad_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include <iostream>
#include <atlstr.h>
#include <chrono>
#include <thread>

#include "client/windows/handler/exception_handler.h"

namespace {

    class QuickBreakpadPlugin : public flutter::Plugin {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

        QuickBreakpadPlugin();

        virtual ~QuickBreakpadPlugin();

    private:
        // Called when a method is called on this plugin's channel from Dart.
        void HandleMethodCall(
                const flutter::MethodCall <flutter::EncodableValue> &method_call,
                std::unique_ptr <flutter::MethodResult<flutter::EncodableValue>> result);
    };

// static
    void QuickBreakpadPlugin::RegisterWithRegistrar(
            flutter::PluginRegistrarWindows *registrar) {
        auto channel =
                std::make_unique < flutter::MethodChannel < flutter::EncodableValue >> (
                        registrar->messenger(), "quick_breakpad",
                                &flutter::StandardMethodCodec::GetInstance());

        auto plugin = std::make_unique<QuickBreakpadPlugin>();

        channel->SetMethodCallHandler(
                [plugin_pointer = plugin.get()](const auto &call, auto result) {
                    plugin_pointer->HandleMethodCall(call, std::move(result));
                });

        registrar->AddPlugin(std::move(plugin));
    }

    static bool dumpCallback(
            const wchar_t *dump_path,
            const wchar_t *minidump_id,
            void *context,
            EXCEPTION_POINTERS *exinfo,
            MDRawAssertionInfo *assertion,
            bool succeeded
    ) {
        std::wcout << L"dump_path: " << dump_path << std::endl;
        std::wcout << L"minidump_id: " << minidump_id << std::endl;
        return succeeded;
    }

    QuickBreakpadPlugin::QuickBreakpadPlugin() {
//  static google_breakpad::ExceptionHandler handler(L".", nullptr, dumpCallback, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
    }

    QuickBreakpadPlugin::~QuickBreakpadPlugin() {}

    std::wstring ToStringW(const std::string &strText) {
        std::wstring wstrResult;
        wstrResult.resize(strText.length());
        typedef std::codecvt<wchar_t, char, mbstate_t> widecvt;
        std::locale locGlob;
        std::locale::global(locGlob);
        const widecvt &cvt(std::use_facet<widecvt>(locGlob));
        mbstate_t State;
        const char *cTemp;
        wchar_t *wTemp;
        cvt.in(State,
               &strText[0], &strText[0] + strText.length(), cTemp,
               (wchar_t *) &wstrResult[0], &wstrResult[0] + wstrResult.length(), wTemp);

        return wstrResult;
    }

    // mock crash
    void crash() {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + std::chrono::microseconds(1 * 1000 * 1000);
        do {
            std::this_thread::yield();
        } while (std::chrono::high_resolution_clock::now() < end);

        volatile int *a = (int *) NULL;
        *a = 1;
    }

    void QuickBreakpadPlugin::HandleMethodCall(
            const flutter::MethodCall <flutter::EncodableValue> &method_call,
            std::unique_ptr <flutter::MethodResult<flutter::EncodableValue>> result) {
        if (method_call.method_name().compare("getPlatformVersion") == 0) {
            std::ostringstream version_stream;
            version_stream << "Windows ";
            if (IsWindows10OrGreater()) {
                version_stream << "10+";
            } else if (IsWindows8OrGreater()) {
                version_stream << "8";
            } else if (IsWindows7OrGreater()) {
                version_stream << "7";
            }
            result->Success(flutter::EncodableValue(version_stream.str()));
        } else if (method_call.method_name().compare("init") == 0) {
            const auto *arguments = std::get_if<flutter::EncodableMap>(method_call.arguments());
            auto path = arguments->find(flutter::EncodableValue("dumpFileSavePath"));
            std::string folderPathStr = std::get<std::string>(path->second);
            CString sttt = folderPathStr.c_str();
            std::wstring s = ToStringW(folderPathStr);
            static google_breakpad::ExceptionHandler handler(s.c_str(), nullptr, dumpCallback, nullptr,
                                                             google_breakpad::ExceptionHandler::HANDLER_ALL);
            result->Success();
        } else if (method_call.method_name().compare("mockCrash") == 0) {
            std::thread(crash).detach();
            result->Success();
        } else {
            result->NotImplemented();
        }
    }

}  // namespace

void QuickBreakpadPluginRegisterWithRegistrar(
        FlutterDesktopPluginRegistrarRef registrar) {
    QuickBreakpadPlugin::RegisterWithRegistrar(
            flutter::PluginRegistrarManager::GetInstance()
                    ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

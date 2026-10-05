/*
 * main.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Main entry point for the editor
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStyle>
#include <QStyleHints>
#include <QTimer>

#ifdef Q_OS_WIN
#include <QDir>
#include <QStandardPaths>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

namespace {
HANDLE editorLog = INVALID_HANDLE_VALUE;
wchar_t editorLogPath[MAX_PATH]{};
wchar_t editorDumpPath[MAX_PATH]{};
struct RuntimeLogOutput {
    struct Buffer : std::streambuf {
        std::filebuf file;
        std::mutex mutex;

        std::streamsize xsputn(const char *data,
                               std::streamsize size) override {
            std::lock_guard lock(mutex);
            return file.sputn(data, size);
        }

        int_type overflow(int_type value) override {
            std::lock_guard lock(mutex);
            return traits_type::eq_int_type(value, traits_type::eof())
                       ? traits_type::not_eof(value)
                       : file.sputc(traits_type::to_char_type(value));
        }

        int sync() override {
            std::lock_guard lock(mutex);
            return file.pubsync();
        }
    } buffer;
    std::streambuf *originalOutput = nullptr;
    std::streambuf *originalError = nullptr;

    ~RuntimeLogOutput() {
        if (originalOutput != nullptr)
            std::cout.rdbuf(originalOutput);
        if (originalError != nullptr)
            std::cerr.rdbuf(originalError);
    }

    void open(const QString &path) {
        if (buffer.file.open(std::filesystem::path(path.toStdWString()),
                             std::ios::out | std::ios::trunc) == nullptr)
            return;
        originalOutput = std::cout.rdbuf(&buffer);
        originalError = std::cerr.rdbuf(&buffer);
        std::cout << std::unitbuf;
        std::cerr << std::unitbuf;
    }
} runtimeLogOutput;

void windowsMessageHandler(QtMsgType type, const QMessageLogContext &context,
                           const QString &message) {
    const QByteArray text =
        (qFormatLogMessage(type, context, message) + '\n').toUtf8();
    DWORD written = 0;
    if (editorLog != INVALID_HANDLE_VALUE) {
        WriteFile(editorLog, text.constData(), static_cast<DWORD>(text.size()),
                  &written, nullptr);
        FlushFileBuffers(editorLog);
    }
    std::fwrite(text.constData(), 1, text.size(), stderr);
}

LONG WINAPI windowsCrashHandler(EXCEPTION_POINTERS *exception) {
    HMODULE module = nullptr;
    wchar_t modulePath[MAX_PATH]{};
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(exception->ExceptionRecord->ExceptionAddress),
        &module);
    GetModuleFileNameW(module, modulePath, MAX_PATH);
    wchar_t message[2048]{};
    const auto offset = reinterpret_cast<ULONG_PTR>(
                            exception->ExceptionRecord->ExceptionAddress) -
                        reinterpret_cast<ULONG_PTR>(module);
    std::swprintf(message, 2048,
                  L"Atlas encountered a native crash (0x%08lX).\n"
                  L"Module: %ls\nOffset: 0x%llX\n\nLog: %ls",
                  exception->ExceptionRecord->ExceptionCode, modulePath,
                  static_cast<unsigned long long>(offset), editorLogPath);
    if (editorLog != INVALID_HANDLE_VALUE) {
        char logMessage[8192]{};
        const int length =
            WideCharToMultiByte(CP_UTF8, 0, message, -1, logMessage,
                                sizeof(logMessage), nullptr, nullptr);
        DWORD written = 0;
        if (length > 0)
            WriteFile(editorLog, logMessage, static_cast<DWORD>(length - 1),
                      &written, nullptr);
        FlushFileBuffers(editorLog);
    }
    HMODULE debugHelp =
        LoadLibraryExW(L"dbghelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (debugHelp != nullptr) {
        auto entry = GetProcAddress(debugHelp, "MiniDumpWriteDump");
        decltype(&MiniDumpWriteDump) writeDump = nullptr;
        static_assert(sizeof(writeDump) == sizeof(entry));
        std::memcpy(&writeDump, &entry, sizeof(writeDump));
        HANDLE dump =
            CreateFileW(editorDumpPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (writeDump != nullptr && dump != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception,
                                                FALSE};
            writeDump(GetCurrentProcess(), GetCurrentProcessId(), dump,
                      MiniDumpNormal, &info, nullptr, nullptr);
        }
        if (dump != INVALID_HANDLE_VALUE)
            CloseHandle(dump);
    }
    MessageBoxW(nullptr, message, L"Atlas Engine Crash", MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

void installWindowsDiagnostics() {
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!QDir().mkpath(directory))
        return;
    const QString logPath = QDir::toNativeSeparators(directory + "/editor.log");
    const QString dumpPath =
        QDir::toNativeSeparators(directory + "/editor-crash.dmp");
    if (logPath.size() >= MAX_PATH || dumpPath.size() >= MAX_PATH)
        return;
    const QString runtimeLogPath =
        QDir::toNativeSeparators(directory + "/editor-runtime.log");
    runtimeLogOutput.open(runtimeLogPath);
    logPath.toWCharArray(editorLogPath);
    dumpPath.toWCharArray(editorDumpPath);
    editorLog =
        CreateFileW(editorLogPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    qInstallMessageHandler(windowsMessageHandler);
    SetUnhandledExceptionFilter(windowsCrashHandler);
}
}
#endif

#include "DockManager.h"
#include "DockWidget.h"
#include "../include/editor/application/styling.h"
#include "editor/debug.h"
#include "editor/application/toolchainInstaller.h"
#include "editor/styling/icons.h"
#include "editor/styling/workbench.h"
#include "editor/views/editorWindow.h"
#include "editor/views/projectBrowser.h"
#include "editor/views/splashScreen.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Atlas Engine");
    app.setApplicationDisplayName("Atlas Engine");
    app.setOrganizationName("Neutral Software");
    app.setQuitOnLastWindowClosed(true);
#ifdef Q_OS_WIN
    installWindowsDiagnostics();
#endif

    const int manropeFont = QFontDatabase::addApplicationFont(
        ":/editor/assets/Manrope-VariableFont_wght.ttf");
    if (manropeFont < 0) {
        qWarning() << "Failed to load Manrope";
    }

    QFont applicationFont =
        QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    applicationFont.setPointSizeF(10.5);
    app.setFont(applicationFont);
    styling::loadIconFont();

#ifndef Q_OS_MACOS
#ifdef ATLAS_DEBUG_BUILD
    app.setWindowIcon(
        QIcon(":/editor/assets/atlas-app-dev.png"));
#else
    app.setWindowIcon(
        QIcon(":/editor/assets/atlas-app.png"));
#endif
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

    app.setStyle("Fusion");
    styling::applyTheme(app);
    styling::installWorkbench(app);
    ToolchainInstaller::ensureInstalled();

    auto *startupSplash = new SplashScreen();
    startupSplash->start("Preparing the project browser...");
    QTimer::singleShot(0, &app, [&app, startupSplash] {
        auto *projectBrowser = new ProjectBrowser();
        QObject::connect(
            projectBrowser, &ProjectBrowser::openProjectRequested, &app,
            [projectBrowser](const QString &projectFile) {
                projectBrowser->setEnabled(false);
                projectBrowser->hide();
                auto *splash = new SplashScreen();
                splash->start("Restoring editor workspace...");
                QTimer::singleShot(
                    0, splash, [projectBrowser, projectFile, splash] {
                        auto *editor = new EditorWindow(projectFile);
                        editor->setAttribute(Qt::WA_DeleteOnClose);
#ifdef Q_OS_WIN
                        qInfo().noquote() << "Opening project:" << projectFile;
                        QObject::connect(editor,
                                         &EditorWindow::startupStatusChanged,
                                         editor, [](const QString &status) {
                                             qInfo().noquote() << status;
                                         });
#endif
                        QObject::connect(editor,
                                         &EditorWindow::startupStatusChanged,
                                         splash, &SplashScreen::setStatus);
                        QObject::connect(editor, &EditorWindow::startupReady,
                                         splash,
                                         [projectBrowser, editor,
                                          splash](bool, const QString &) {
                                             splash->finish();
                                             splash->deleteLater();
                                             projectBrowser->deleteLater();
                                             editor->raise();
                                             editor->activateWindow();
                                         });
                        editor->show();
                    });
            });
        projectBrowser->show();
        projectBrowser->raise();
        projectBrowser->activateWindow();
        startupSplash->finish();
        startupSplash->deleteLater();
    });
    return app.exec();
}

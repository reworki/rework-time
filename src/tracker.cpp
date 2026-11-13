#include "tracker.h"

#include <QFileInfo>

#include <windows.h>

namespace {

// Как процессы зовутся в базе. Одно место правды: сэмплер, миграция имён и
// иконки опираются на эту таблицу.
const QHash<QString, QString>& aliasMap() {
    static const QHash<QString, QString> map = {
        {QStringLiteral("WindowsTerminal"), QStringLiteral("Windows Terminal")},
        {QStringLiteral("Code"), QStringLiteral("VS Code")},
        {QStringLiteral("chrome"), QStringLiteral("Chrome")},
        {QStringLiteral("msedge"), QStringLiteral("Edge")},
        {QStringLiteral("brave"), QStringLiteral("Brave")},
        {QStringLiteral("firefox"), QStringLiteral("Firefox")},
        {QStringLiteral("explorer"), QStringLiteral("Проводник")},
        {QStringLiteral("Discord"), QStringLiteral("Discord")},
        {QStringLiteral("Telegram"), QStringLiteral("Telegram")},
        {QStringLiteral("steam"), QStringLiteral("Steam")},
        {QStringLiteral("steamwebhelper"), QStringLiteral("Steam")},
        {QStringLiteral("RobloxPlayerBeta"), QStringLiteral("Roblox")},
        {QStringLiteral("ReworkMusicPRO"), QStringLiteral("Rework Music PRO")},
        {QStringLiteral("ReworkMusicLITE"), QStringLiteral("Rework Music LITE")},
        {QStringLiteral("ReworkPCControl"), QStringLiteral("Rework PC Control")},
        {QStringLiteral("DeepSeek Harness"), QStringLiteral("DeepSeek Harness")},
        {QStringLiteral("ChatGPT"), QStringLiteral("ChatGPT")},
        {QStringLiteral("notepad"), QStringLiteral("Блокнот")},
    };
    return map;
}

QString fullExePath(DWORD pid) {
    if (!pid) return QString();
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return QString();
    wchar_t buffer[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    QString path;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        path = QString::fromWCharArray(buffer, int(size));
    }
    CloseHandle(process);
    return path;
}

} // namespace

const QHash<QString, QString>& Tracker::aliases() { return aliasMap(); }

QString Tracker::prettyName(const QString& baseName) {
    if (baseName.isEmpty()) return baseName;
    const auto it = aliasMap().constFind(baseName);
    if (it != aliasMap().constEnd()) return it.value();
    QString pretty = baseName;
    if (pretty.size() > 1) pretty[0] = pretty[0].toUpper();
    return pretty;
}

Tracker::Tracker(int idleSeconds) : idleLimitSeconds_(idleSeconds) {}

Tracker::Sample Tracker::take() const {
    Sample sample;

    LASTINPUTINFO input = {};
    input.cbSize = sizeof(input);
    if (GetLastInputInfo(&input)) {
        const DWORD idle = (GetTickCount() - input.dwTime) / 1000;
        sample.idle = idleLimitSeconds_ > 0 && idle >= DWORD(idleLimitSeconds_);
    }

    HWND window = GetForegroundWindow();
    if (!window) return sample;

    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    sample.pid = quint32(pid);
    sample.path = fullExePath(pid);
    sample.app = prettyName(QFileInfo(sample.path).completeBaseName());
    if (sample.app.isEmpty()) return sample;

    wchar_t title[512] = {};
    const int length = GetWindowTextW(window, title, 511);
    if (length > 0) sample.title = QString::fromWCharArray(title, length);

    return sample;
}

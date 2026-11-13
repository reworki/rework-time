"""Патч backend.cpp: иконки кэшируются PNG-файлами вместо data-URL."""
import io
import re

path = r'C:\KNOT\REWORK-TIME\src\backend.cpp'
src = io.open(path, encoding='utf-8').read()

new_func = '''QString Backend::iconDataUrl(const QString& path) {
    // Иконку кладём PNG-файлом в кэш: QML надёжно грузит file://, а data-URL
    // в Image иногда просто не отрисовывается.
    if (path.isEmpty() || !QFileInfo::exists(path)) return QString();

    const QString cacheDir = appDataDir() + QStringLiteral("/icons");
    QDir().mkpath(cacheDir);
    const QString key = QString::number(qHash(path), 16);
    const QString pngPath = cacheDir + QLatin1Char('/') + key + QStringLiteral(".png");

    if (!QFileInfo::exists(pngPath)) {
        QFileIconProvider provider;
        const QIcon icon = provider.icon(QFileInfo(path));
        if (icon.isNull()) return QString();
        QImage image = icon.pixmap(48, 48).toImage();
        if (image.isNull()) return QString();
        image = image.convertToFormat(QImage::Format_ARGB32);
        if (!image.save(pngPath, "PNG")) return QString();
    }

    return QUrl::fromLocalFile(pngPath).toString();
}'''

pattern = re.compile(r'QString Backend::iconDataUrl\(const QString& path\) \{.*?\n\}', re.S)
src, n = pattern.subn(new_func, src, count=1)
print('icon func replaced:', n)

if '#include <QUrl>' not in src:
    src = src.replace('#include <QStandardPaths>', '#include <QStandardPaths>\n#include <QUrl>')

io.open(path, 'w', encoding='utf-8', newline='').write(src)
print('ok', len(src))

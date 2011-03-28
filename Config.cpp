#include "Config.h"

Config::Config()
    : mSettings(0)
{
}

Config::~Config()
{
    delete mSettings;
}

QVariant Config::valueFromCommandLine(const QString &key)
{
    const QStringList args = QCoreApplication::arguments();
    QRegExp rx(QString("--?%1=(.*)").arg(key));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    QVariant value;
    int arg = args.indexOf(rx);
    if (arg != -1) {
        value = rx.cap(1);
    } else {
        rx.setPattern(QString("--?%1$").arg(key));
        arg = args.indexOf(rx);
        if (arg != -1 && arg + 1 < args.size()) {
            value = args.value(arg + 1);
        }
    }
    return value;
}

QSettings * Config::settings()
{
    if (!mSettings) {
        QString fileName = valueFromCommandLine("conf").toString();
        if (!fileName.isEmpty()) {
            if (fileName == "none"
                || fileName == "null"
                || fileName == "/dev/null") {
                fileName.clear();
//         } else if (!QFile::exists(fileName)) {
//             qWarning("%s doesn't seem to exist", qPrintable(fileName));
            }
            mSettings = new QSettings(fileName, QSettings::IniFormat);
        } else {
            mSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                      QCoreApplication::organizationName(), QCoreApplication::applicationName());
        }
    }
    return mSettings;
}

bool Config::store()
{
    static enum { DontStore = 0x0, Store = 0x1, Unset = 0x2 } state = Unset;
    if (state == Unset) {
        const QStringList args = QCoreApplication::arguments();
        state = (args.contains("--store", Qt::CaseInsensitive)
                 || args.contains("--save", Qt::CaseInsensitive)
                 ? Store
                 : DontStore);
    }

    return (state == Store);
}

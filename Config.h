#ifndef CONFIG_H
#define CONFIG_H
#include <QtGui>

class Config
{
public:
    Config();
    ~Config();
    void setEnabled(const QString &key, bool on)
    {
        setValue(key, on);
    }

    inline bool isDisabled(const QString &key, bool defaultValue = false)
    {
        return !isEnabled(key, !defaultValue);
    }

    bool isEnabled(const QString &key, bool defaultValue = false)
    {
        const QStringList args = QCoreApplication::arguments();
        enum { Unset = -1, False = 0, True = 1 } v = Unset;
        struct {
            const char *prefix;
            const char *suffix;
            const bool enable;
        } const options[] = {
            { "", "", true, },
            { "enable-?", "", true },
            { "", "=yes", true },
            { "", "=1", true },
            { "", "=true", true },
            { "no-?", "", false },
            { "disable-?", "", false },
            { "", "=no", false },
            { "", "=0", false },
            { "", "=false", false },
            { 0, 0, false }
        };
        QRegExp rx;
        rx.setCaseSensitivity(Qt::CaseInsensitive);

        for (int i=0; options[i].prefix; ++i) {
            rx.setPattern(QString("--?%1%2%3").
                          arg(options[i].prefix).
                          arg(key).
                          arg(options[i].suffix));
            const int arg = args.indexOf(rx);
            if (arg != -1) {
                v = options[i].enable ? True : False;
                break;
            }
        }

        if (v == Unset) {
            const QVariant var = readValueFromSettings(key);
            v = (var.isNull() ? defaultValue : var.toBool()) ? True : False;
        } else if (store()) {
            setValue(key, (v == True));
        }
        return (v == True);
    }

    template <typename T> bool contains(const QString &key)
    {
        bool ok;
        (void)value<T>(key, T(), &ok);
        return ok;
    }

    template <typename T> T value(const QString &key, const T &defaultValue = T(), bool *ok_in = 0)
    {
        QVariant value = Config::valueFromCommandLine(key);
        if (value.isNull()) {
            value = readValueFromSettings(key);
        } else if (store()) {
            writeValueToSettings(key, value);
        }

        if (ok_in)
            *ok_in = !value.isNull();
        return value.isNull() ? defaultValue: qVariantValue<T>(value);
    }

    template <typename T> void setValue(const QString &key, const T &t)
    {
        writeValueToSettings(key, qVariantFromValue<T>(t));
    }

private:
    QVariant readValueFromSettings(const QString &key) const;
    void writeValueToSettings(const QString &key, const QVariant &value);
    bool store();
    static QVariant valueFromCommandLine(const QString &key);
    QSettings *mSettings;
    int mStore;
};

#endif

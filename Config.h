#ifndef CONFIG_H
#define CONFIG_H
#include <QtGui>

/* not thread safe */

#define CONFIG_TYPE(T)                                          \
    Q_DECLARE_METATYPE(T);                                      \
    static inline bool read(const QVariant &v, T &t) {          \
        if (qVariantCanConvert<T>(v)) {                         \
            t = qVariantValue<T>(v);                            \
            return true;                                        \
        }                                                       \
        return false;                                           \
    }                                                           \
    static inline bool read(QSettings *settings,                \
                            const QString &key,                 \
                            T &t) {                             \
        const QVariant var = settings->value(key);              \
        if (!var.isNull() && qVariantCanConvert<T>(var)) {      \
            t = qVariantValue<T>(var);                          \
            return true;                                        \
        }                                                       \
        return false;                                           \
    }                                                           \
    static inline void write(QSettings *settings,               \
                             const QString &key,                \
                             const T &t) {                      \
        settings->setValue(key, qVariantFromValue<T>(t));       \
    }

CONFIG_TYPE(bool);
CONFIG_TYPE(qint8);
CONFIG_TYPE(qint16);
CONFIG_TYPE(qint32);
CONFIG_TYPE(qint64);
CONFIG_TYPE(quint8);
CONFIG_TYPE(quint16);
CONFIG_TYPE(quint32);
CONFIG_TYPE(quint64);
CONFIG_TYPE(float);
CONFIG_TYPE(double);
CONFIG_TYPE(QChar);
CONFIG_TYPE(QString);
CONFIG_TYPE(QStringList);
CONFIG_TYPE(QByteArray);
CONFIG_TYPE(QPoint);
CONFIG_TYPE(QSize);
CONFIG_TYPE(QRect);
CONFIG_TYPE(QLine);
CONFIG_TYPE(QPointF);
CONFIG_TYPE(QSizeF);
CONFIG_TYPE(QRectF);
CONFIG_TYPE(QLineF);
CONFIG_TYPE(QVariantList);
CONFIG_TYPE(QVariantMap);
CONFIG_TYPE(QColor);
typedef QHash<QString, QString> StringHash;
CONFIG_TYPE(StringHash);

class Config
{
public:
    Config();
    ~Config();
    void setEnabled(const QString &key, bool on)
    {
        setValue(key, on);
    }

    inline bool isDisabled(const QString &k, bool defaultValue = false)
    {
        return !isEnabled(k, !defaultValue);
    }

    bool isEnabled(const QString &k, bool defaultValue = false)
    {
        const QString key = k.toLower();
        const QStringList args = QCoreApplication::arguments();
        enum { Unset = -1, False = 0, True = 1 } value = Unset;
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
                value = options[i].enable ? True : False;
                break;
            }
        }

        QSettings *s = settings();
        if (value == Unset) {
            if (s->contains(key)) {
                value = s->value(key).toBool() ? True : False;
            }
        } else if (store()) {
            s->setValue(key, (value == True));
        }
        return value == Unset ? defaultValue : (value == True);
    }

    template <typename T> bool contains(const QString &key)
    {
        bool ok;
        (void)value<T>(key, T(), &ok);
        return ok;
    }

    template <typename T> T value(const QString &k, const T &defaultValue = T(), bool *ok_in = 0)
    {
        const QString key = k.toLower();
        QVariant value = valueFromCommandLine(key);
        T t;
        bool ok = false;
        QSettings *s = settings();
        if (!value.isNull()) {
            ok = ::read(value, t);
            if (ok && store()) {
                setValue<T>(key, t);
            }
        } else {
            ok = ::read(s, k, t);
        }

        if (ok_in)
            *ok_in = ok;
        return ok ? t : defaultValue;
    }

    template <typename T> void setValue(const QString &key, const T &t)
    {
        QSettings *s = settings();
        ::write(s, key, t);
        s->sync();
    }

    void setValue(const QString &key, const QVariant &value)
    {
        QSettings *s = settings();
        s->setValue(key.toLower(), value);
    }

    QVariant value(const QString &k, const QVariant &defaultValue)
    {
        const QString key = k.toLower();
        QVariant value = valueFromCommandLine(key);
        QSettings *s = settings();
        if (value.isNull()) {
            value = s->value(key, defaultValue);
        } else if (store()) {
            setValue(key, value);
        }

        return value;
    }

private:
    QSettings *mSettings;
    QSettings *settings();
    bool store();
    QVariant valueFromCommandLine(const QString &key);
};

#endif

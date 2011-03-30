#include "Config.h"
#include <cJSON.h>

static QVariant fromJson(const cJSON *json)
{
    Q_ASSERT(json);
    switch (json->type) {
    case cJSON_False: return QVariant(false);
    case cJSON_True: return QVariant(true);
    case cJSON_NULL: return QVariant();
    case cJSON_Number: return (json->valuedouble == json->valueint
                               ? QVariant(json->valueint)
                               : QVariant(json->valuedouble));
    case cJSON_String: return QVariant(QString::fromUtf8(json->valuestring));
    case cJSON_Array: {
        QVariantList list;
        for (json=json->child; json; json = json->next)
            list.append(fromJson(json));
        return list; }
    case cJSON_Object: {
        QVariantMap map;
        for (json=json->child; json; json = json->next)
            map[json->string] = fromJson(json);
        return map; }
    default:
        Q_ASSERT(0);
    }
    return QVariant();
}

static QVariant fromJson(const QString &string)
{
    if (string.isEmpty())
        return QVariant();
    cJSON *json = cJSON_Parse(string.toUtf8().constData());
    if (!json) {
        qWarning("Error parsing string:(%s): error: %s", qPrintable(string), cJSON_GetErrorPtr());
        return QVariant();
    }
    const QVariant variant = fromJson(json);
    cJSON_Delete(json);
    return variant;
}

static cJSON *tocJSON(const QVariant &variant)
{
    cJSON *ret = 0;
    switch (variant.type()) {
    case QVariant::Invalid: ret = cJSON_CreateNull(); break;
    case QVariant::Bool: ret = (variant.toBool() ? cJSON_CreateTrue() : cJSON_CreateFalse()); break;
    case QVariant::Int: ret = cJSON_CreateNumber(variant.toInt()); break;
    case QVariant::UInt: ret = cJSON_CreateNumber(variant.toUInt()); break;
    case QVariant::LongLong: ret = cJSON_CreateNumber(variant.toLongLong()); break;
    case QVariant::ULongLong: ret = cJSON_CreateNumber(variant.toULongLong()); break;
    case QVariant::Double: ret = cJSON_CreateNumber(variant.toDouble()); break;
    case QVariant::String: ret = cJSON_CreateString(variant.toString().toUtf8().constData()); break;
    case QVariant::ByteArray: ret = cJSON_CreateString(variant.toByteArray().constData()); break;
    case QVariant::List:
        ret = cJSON_CreateArray();
        Q_ASSERT(ret);
        foreach(const QVariant &item, variant.toList())
            cJSON_AddItemToArray(ret, tocJSON(item));
        break;
    case QVariant::Map: {
        ret = cJSON_CreateObject();
        Q_ASSERT(ret);
        const QVariantMap map = variant.toMap();
        for (QMap<QString, QVariant>::const_iterator it = map.begin(); it != map.end(); ++it)
            cJSON_AddItemToObject(ret, it.key().toUtf8().constData(), tocJSON(it.value()));
        break; }
    case QVariant::StringList:
        ret = cJSON_CreateArray();
        Q_ASSERT(ret);
        foreach(const QString &string, variant.toStringList())
            cJSON_AddItemToArray(ret, tocJSON(string));
        break;
    default:
        qWarning("Can't convert variant to json %d", variant.type());
    }
    return ret;
}

static QString toJSON(const QVariant &variant)
{
    cJSON *json = tocJSON(variant);
    if (!json) {
        qWarning("Can't convert variant (type %d) to json", variant.type());
        return QString();
    }
    const QString string = QString::fromUtf8(cJSON_PrintUnformatted(json));
    cJSON_Delete(json);
    return string;
}

Config::Config()
    : mStore(-1)
{
    QString fileName = valueFromCommandLine("conf").toString();
    if (!fileName.isEmpty()) {
        mSettings = new QSettings(fileName, QSettings::IniFormat);
    } else {
        mSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                  QCoreApplication::organizationName(), QCoreApplication::applicationName());
    }
}

Config::~Config()
{
    delete mSettings;
}

QVariant Config::valueFromCommandLine(const QString &key)
{
    const QStringList args = QCoreApplication::arguments();
    QRegExp rx(QString("--?%1=(.*)").arg(key));
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

bool Config::store()
{
    if (mStore == -1) {
        const QStringList args = QCoreApplication::arguments();
        mStore = (args.contains("--store") || args.contains("--save") ? 1 : 0);
    }

    return (mStore == 1);
}

QVariant Config::readValueFromSettings(const QString &key) const
{
    return fromJson(mSettings->value(key).toString());
}
void Config::writeValueToSettings(const QString &key, const QVariant &value)
{
    mSettings->setValue(key, toJSON(value));
    mSettings->sync();
}

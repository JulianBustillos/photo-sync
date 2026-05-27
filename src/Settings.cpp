#include "Settings.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QString Settings::ConfigFilename = "Configuration.json";
const QString Settings::ImportKey = "ImportPath";
const QString Settings::ExportKey = "ExportPath";
const QString Settings::DeleteKey = "Delete";

Settings::Settings(const QString &executablePath)
    : m_configPath(executablePath + "/" + ConfigFilename), m_importPath(""), m_exportPath(""),
      m_deleteFiles(false)
{
}

Settings::~Settings() {}

bool Settings::parseConfigFile()
{
    QFile configFile(m_configPath);

    if (!configFile.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QByteArray data = configFile.readAll();
    configFile.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << m_configPath << " JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject())
    {
        qWarning() << m_configPath << " root is not a JSON object";
        return false;
    }

    QJsonObject obj = doc.object();

    m_importPath = obj[ImportKey].toString();
    m_exportPath = obj[ExportKey].toString();
    m_deleteFiles = obj[DeleteKey].toBool();

    return true;
}

bool Settings::exportConfigFile() const
{
    QJsonObject obj;
    obj[ImportKey] = m_importPath;
    obj[ExportKey] = m_exportPath;
    obj[DeleteKey] = m_deleteFiles;

    QJsonDocument doc(obj);

    QFile configFile(m_configPath);

    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << m_configPath << " failed to open file for writing";
        return false;
    }

    configFile.write(doc.toJson(QJsonDocument::Indented));

    configFile.close();
    return true;
}

void Settings::setConfig(const QString &importPath, const QString &exportPath, bool deleteFiles)
{
    m_importPath = importPath;
    m_exportPath = exportPath;
    m_deleteFiles = deleteFiles;
}

void Settings::getConfig(QString &importPath, QString &exportPath, bool &deleteFiles) const
{
    importPath = m_importPath;
    exportPath = m_exportPath;
    deleteFiles = m_deleteFiles;
}

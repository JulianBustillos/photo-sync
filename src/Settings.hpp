#pragma once
#include <QString>

class Settings
{
  public:
    static const QString ConfigFilename;
    static const QString ImportKey;
    static const QString ExportKey;
    static const QString DeleteKey;

  public:
    Settings(const QString &executablePath);
    ~Settings();

  public:
    bool parseConfigFile();
    bool exportConfigFile() const;
    void setConfig(const QString &importPath, const QString &exportPath, bool deleteFiles);
    void getConfig(QString &importPath, QString &exportPath, bool &deleteFiles) const;

  public:
    const QString m_configPath;
    QString m_importPath;
    QString m_exportPath;
    bool m_deleteFiles;
};
#pragma once
#include <QString>


class Settings
{
public:
    static const QString ConfigFilename;
    static const QString ImportVar;
    static const QString ExportVar;
    static const QString RemoveVar;

public:
    Settings(const QString &executablePath);
    ~Settings();

public:
    bool parseConfigFile();
    bool exportConfigFile() const;
    void setConfig(const QString &importPath, const QString &exportPath, bool remove);
    void getConfig(QString &importPath, QString &exportPath, bool &remove) const;

public:
    const QString m_configPath;
    QString m_importPath;
    QString m_exportPath;
    bool m_remove;
};
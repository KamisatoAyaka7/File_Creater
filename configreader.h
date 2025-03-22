#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <QString>
#include <QMap>
#include <QColor>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class ConfigReader
{
public:
    ConfigReader(const QString &filePath);
    QMap<QString, QColor> getKeywords() const;

private:
    QMap<QString, QColor> keywords;
};

#endif // CONFIGREADER_H

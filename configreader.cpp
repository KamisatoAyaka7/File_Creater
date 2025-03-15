#include "configreader.h"

ConfigReader::ConfigReader(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Failed to open config file.");
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull()) {
        qWarning("Failed to parse config file.");
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray keywordArray = root["keywords"].toArray();
    for (const QJsonValue &value : keywordArray) {
        QJsonObject obj = value.toObject();
        QString word = obj["word"].toString();
        QString colorStr = obj["color"].toString();
        QColor color(colorStr);
        keywords.insert(word, color);
    }
}

QMap<QString, QColor> ConfigReader::getKeywords() const
{
    return keywords;
}

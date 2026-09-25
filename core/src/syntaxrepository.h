// Loads syntax definitions from the search paths (user dir > exe dir >
// embedded resources). First definition with a given name wins.

#pragma once

#include "syntaxdefinition.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

#include <memory>

namespace AppleCat::Core {

class SyntaxRepository
{
public:
    static SyntaxRepository &instance();
    void init(); // (re)load all definitions

    std::shared_ptr<const SyntaxDefinition> definitionForFile(const QString &filePath) const;
    std::shared_ptr<const SyntaxDefinition> definitionForExtension(const QString &extension) const;
    QList<std::shared_ptr<const SyntaxDefinition>> all() const { return m_all; }

private:
    void loadFromDir(const QString &dir);
    void loadFromFile(const QString &path);
    void index(std::shared_ptr<const SyntaxDefinition> def);

    QList<std::shared_ptr<const SyntaxDefinition>> m_all;
    QHash<QString, std::shared_ptr<const SyntaxDefinition>> m_byExt; // lower ext -> def
    QSet<QString> m_loadedNames;
};

} // namespace AppleCat::Core

// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FontIconPack.h"
#include <QIconEngine>
#include <QPalette>
#include <QtCore/QFile>
#include <QtGui/QPainter>
#include <QtXml/QDomDocument>

std::unique_ptr<FontIconPack> FontIconPack::createDefault() {
  return std::make_unique<FontIconPack>(":/font-icons.xml");
}

FontIconPack::FontIconPack(const QString& iconSheetPath) {
  QFile iconSheetFile(iconSheetPath);
  if (!iconSheetFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }
  QDomDocument doc;
  if (!doc.setContent(&iconSheetFile)) {
    return;
  }
  iconSheetFile.close();

  for (QDomNode node = doc.documentElement().firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement() || (node.nodeName() != "icon")) {
      continue;
    }
    addIconFromElement(node.toElement());
  }
}

namespace {
class FontIconEngine : public QIconEngine {
 public:
  void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

  QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

  FontIconEngine* clone() const override;

  void addChar(const QString& font, int codepoint, float scale, QIcon::State state);

 private:
  struct SymbolData {
    const QString fontName;
    int codepoint;
    float scale;

    SymbolData(const QString& font, int codepoint, float scale) : fontName(font), codepoint(codepoint), scale(scale) {}
  };

  const SymbolData& getSymbolData(QIcon::State state);

  std::map<QIcon::State, SymbolData> m_iconSymbolDataMap;
};

QColor getIconColor(QIcon::Mode iconMode) {
  switch (iconMode) {
    case QIcon::Disabled:
      return QPalette().color(QPalette::Disabled, QPalette::WindowText);
    case QIcon::Selected:
      return QPalette().color(QPalette::Normal, QPalette::HighlightedText);
    default:
      return QPalette().color(QPalette::Normal, QPalette::WindowText);
  }
}

QFont getFont(const QString& fontName, const QSize& size, float scale) {
  QFont font(fontName);
  font.setPixelSize(qRound((double) size.height() * scale));
  return font;
}

void FontIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
  const SymbolData& data = getSymbolData(state);

  painter->save();
  painter->setFont(getFont(data.fontName, rect.size(), data.scale));
  painter->setPen(getIconColor(mode));
  painter->drawText(rect, QChar(data.codepoint), QTextOption(Qt::AlignCenter | Qt::AlignVCenter));
  painter->restore();
}

QPixmap FontIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
  QImage img(size, QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  QPixmap pixmap = QPixmap::fromImage(img, Qt::NoFormatConversion);
  {
    QPainter painter(&pixmap);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
  }
  return pixmap;
}

FontIconEngine* FontIconEngine::clone() const {
  return new FontIconEngine(*this);
}

void FontIconEngine::addChar(const QString& font, int codepoint, float scale, QIcon::State state) {
  m_iconSymbolDataMap.insert(std::make_pair(state, SymbolData(font, codepoint, scale)));
}

const FontIconEngine::SymbolData& FontIconEngine::getSymbolData(QIcon::State state) {
  auto it = m_iconSymbolDataMap.find(state);
  if (it != m_iconSymbolDataMap.end()) {
    return it->second;
  }
  return m_iconSymbolDataMap.begin()->second;
}
}  // namespace

void FontIconPack::addIconFromElement(const QDomElement& iconElement) {
  const QString name = iconElement.attribute("name");
  if (name.isNull()) {
    return;
  }
  auto* iconEngine = new FontIconEngine();
  for (QDomNode node = iconElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement() || (node.nodeName() != "symbol")) {
      continue;
    }
    const QDomElement symbolElement = node.toElement();
    const QString font = symbolElement.attribute("font");
    const int codepoint = symbolElement.attribute("codepoint").toInt(nullptr, 16);
    const float scale = symbolElement.attribute("scale", "1.0").toFloat();
    const QIcon::State state = iconStateFromString(symbolElement.attribute("state", "off"));
    iconEngine->addChar(font, codepoint, scale, state);
  }
  m_iconMap.insert(std::make_pair(name, QIcon(iconEngine)));
}

QIcon FontIconPack::getIcon(const QString& iconKey) const {
  auto it = m_iconMap.find(iconKey);
  if (it != m_iconMap.end()) {
    return it->second;
  }
  return AbstractIconPack::getIcon(iconKey);
}

// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "StyledIconPack.h"

#include <QIconEngine>
#include <QPalette>
#include <QtCore/QFile>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>
#include <QtXml/QDomDocument>
#include <cmath>
#include <functional>
#include <map>
#include <unordered_set>

std::unique_ptr<StyledIconPack> StyledIconPack::createDefault() {
  return std::make_unique<StyledIconPack>(":/icons.xml");
}

StyledIconPack::StyledIconPack(const QString& iconSheetPath) {
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

QIcon StyledIconPack::getIcon(const QString& iconKey) const {
  auto it = m_iconMap.find(iconKey);
  if (it != m_iconMap.end()) {
    return it->second;
  }
  return AbstractIconPack::getIcon(iconKey);
}

namespace {
class SvgStyledIconEngine : public QIconEngine {
 public:
  void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

  QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

  SvgStyledIconEngine* clone() const override;

  void addFile(const QString& fileName, const QSize& size, QIcon::Mode mode, QIcon::State state) override;

 private:
  static QByteArray generateIcon(const QString& fileName, QIcon::Mode mode);

  const QString& getIconFilePath(QIcon::State state);

  std::map<QIcon::State, QString> m_iconFilePathMap;
};

QRectF fitKeepingAspectRatio(const QSizeF& srcSize, const QRectF& boundingRect) {
  const double factor = std::min(boundingRect.width() / srcSize.width(), boundingRect.height() / srcSize.height());
  const QSizeF size = {std::floor(srcSize.width() * factor), std::floor(srcSize.height() * factor)};
  const QPointF origin = {std::floor(boundingRect.x() + ((boundingRect.width() - size.width()) / 2.0)),
                          std::floor(boundingRect.y() + ((boundingRect.height() - size.height()) / 2.0))};
  return {origin, size};
}

void SvgStyledIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
  QSvgRenderer renderer(generateIcon(getIconFilePath(state), mode));
  renderer.render(painter, fitKeepingAspectRatio(renderer.viewBoxF().size(), rect));
}

QPixmap SvgStyledIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
  QImage img(size, QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  QPixmap pixmap = QPixmap::fromImage(img, Qt::NoFormatConversion);
  {
    QPainter painter(&pixmap);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
  }
  return pixmap;
}

SvgStyledIconEngine* SvgStyledIconEngine::clone() const {
  return new SvgStyledIconEngine(*this);
}

void SvgStyledIconEngine::addFile(const QString& fileName, const QSize&, QIcon::Mode, QIcon::State state) {
  m_iconFilePathMap.insert(std::make_pair(state, fileName));
}

const QString& SvgStyledIconEngine::getIconFilePath(QIcon::State state) {
  auto it = m_iconFilePathMap.find(state);
  if (it != m_iconFilePathMap.end()) {
    return it->second;
  }
  return m_iconFilePathMap.begin()->second;
}

void forEachElement(QDomElement el,
                    const std::unordered_set<QString, hashes::hash<QString>>& names,
                    const std::function<void(QDomElement&)>& consumer) {
  if (names.find(el.nodeName()) != names.end()) {
    consumer(el);
    return;
  }

  for (QDomNode node = el.firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    forEachElement(node.toElement(), names, consumer);
  }
}

void fillSvgElement(QDomElement& el, const QColor& color) {
  const QString fillAttr = "fill";
  if (!(el.hasAttribute(fillAttr) || el.hasAttribute("class") || el.hasAttribute("style"))) {
    el.setAttribute(fillAttr, color.name());
  }
}

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

QByteArray SvgStyledIconEngine::generateIcon(const QString& fileName, QIcon::Mode mode) {
  QFile svgFile(fileName);
  if (!svgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return QByteArray();
  }
  QDomDocument doc;
  if (!doc.setContent(&svgFile)) {
    return QByteArray();
  }
  svgFile.close();

  forEachElement(
      doc.documentElement(),
      {"altGlyph", "circle", "ellipse", "path", "polygon", "polyline", "rect", "text", "textPath", "tref", "tspan"},
      std::bind(&fillSvgElement, std::placeholders::_1, getIconColor(mode)));
  return doc.toByteArray();
}
}  // namespace

void StyledIconPack::addIconFromElement(const QDomElement& iconElement) {
  const QString name = iconElement.attribute("name");
  if (name.isNull()) {
    return;
  }
  const bool styled = (iconElement.attribute("styled", "off") == "on");
  QIcon icon = styled ? QIcon(new SvgStyledIconEngine) : QIcon();
  for (QDomNode node = iconElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement() || (node.nodeName() != "file")) {
      continue;
    }
    const QDomElement fileElement = node.toElement();
    const QIcon::Mode mode = iconModeFromString(fileElement.attribute("mode", "normal"));
    const QIcon::State state = iconStateFromString(fileElement.attribute("state", "off"));
    QSize size;
    if (fileElement.hasAttribute("width") && fileElement.hasAttribute("height")) {
      size = {fileElement.attribute("width").toInt(), fileElement.attribute("height").toInt()};
    }
    icon.addFile(fileElement.text(), size, mode, state);
  }
  m_iconMap.insert(std::make_pair(name, icon));
}

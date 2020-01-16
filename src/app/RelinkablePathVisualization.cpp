// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkablePathVisualization.h"

#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>
#include <QStylePainter>
#include <boost/bind.hpp>

#include "ColorSchemeManager.h"
#include "RelinkablePath.h"

struct RelinkablePathVisualization::PathComponent {
  QString label;
  QString prefixPath;         // Including the component itself.
  QString suffixPath;         // Rest of the path.
  RelinkablePath::Type type;  // File or Dir.
  bool exists;

  PathComponent(const QString& lbl, const QString& prefixPath, const QString& suffixPath, RelinkablePath::Type t)
      : label(lbl), prefixPath(prefixPath), suffixPath(suffixPath), type(t), exists(false) {}
};


class RelinkablePathVisualization::ComponentButton : public QPushButton {
 public:
  explicit ComponentButton(QWidget* parent = nullptr) : QPushButton(parent) {}

 protected:
  void paintEvent(QPaintEvent* evt) override;
};


RelinkablePathVisualization::RelinkablePathVisualization(QWidget* parent)
    : QWidget(parent), m_layout(new QHBoxLayout(this)) {
  m_layout->setSpacing(0);
  m_layout->setMargin(0);
}

void RelinkablePathVisualization::clear() {
  for (int count; (count = m_layout->count());) {
    QLayoutItem* item = m_layout->takeAt(count - 1);
    if (QWidget* wgt = item->widget()) {
      wgt->deleteLater();  // We may be called from the widget's signal.
    }
    delete item;
  }
}

void RelinkablePathVisualization::setPath(const RelinkablePath& path, bool clickable) {
  clear();

  QStringList components(path.normalizedPath().split(QChar('/'), QString::SkipEmptyParts));
  if (components.empty()) {
    return;
  }

  // prefixPath is the path up to and including the current component.
  QString prefixPath;

  if (path.normalizedPath().startsWith(QLatin1String("//"))) {
    // That's the Windows \\host\path thing.
    components.front().prepend(QLatin1String("//"));
  } else if (path.normalizedPath().startsWith(QChar('/'))) {
    // Unix paths start with a slash.
    prefixPath += QChar('/');
  }

  std::vector<PathComponent> pathComponents;
  pathComponents.reserve(components.size());

  for (QStringList::const_iterator it(components.begin()); it != components.end(); ++it) {
    const QString& component = *it;

    if (!prefixPath.isEmpty() && !prefixPath.endsWith('/')) {
      prefixPath += QChar('/');
    }
    prefixPath += component;
    // Rest of the path.
    QString suffixPath;
    for (QStringList::const_iterator it2(it); ++it2 != components.end();) {
      if (!suffixPath.isEmpty()) {
        suffixPath += QChar('/');
      }
      suffixPath += *it2;
    }

    pathComponents.emplace_back(component, prefixPath, suffixPath, RelinkablePath::Dir);
  }

  // The last path component is either a file or a dir, while all the previous ones are dirs.
  pathComponents.back().type = path.type();

  checkForExistence(pathComponents);

  int componentIdx = -1;
  for (PathComponent& pathComponent : pathComponents) {
    ++componentIdx;
    auto* btn = new ComponentButton(this);
    m_layout->addWidget(btn);
    btn->setText(pathComponent.label.replace(QChar('/'), QChar('\\')));
    btn->setEnabled(clickable);
    if (clickable) {
      btn->setCursor(QCursor(Qt::PointingHandCursor));
    }
    stylePathComponentButton(btn, pathComponent.exists);

    connect(btn, &ComponentButton::clicked,
            boost::bind(&RelinkablePathVisualization::onClicked, this, componentIdx, pathComponent.prefixPath,
                        pathComponent.suffixPath, pathComponent.type));
  }

  m_layout->addStretch();
}  // RelinkablePathVisualization::setPath

void RelinkablePathVisualization::stylePathComponentButton(QAbstractButton* btn, bool exists) {
  const QColor borderColor = ColorSchemeManager::instance().getColorParam(
      ColorScheme::RelinkablePathVisualizationBorder, palette().color(QPalette::Window).darker(150));

  QString style = "QAbstractButton {\n"
                    "	border: 2px solid "
                    + borderColor.name()
                    + ";\n"
                      "	border-radius: 0.5em;\n"
                      "	padding: 0.2em;\n"
                      "	margin-left: 1px;\n"
                      "	margin-right: 1px;\n"
                      "	min-width: 2em;\n"
                      "	font-weight: bold;\n";
  if (exists) {
    style += "	color: #3a5827;\n"
                 "	background: qradialgradient(cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4, radius: 1.35, stop: 0 #fff, "
                 "stop: 1 #89e74a);\n";
  } else {
    style += "	color: #6f2719;\n"
                 "	background: qradialgradient(cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4, radius: 1.35, stop: 0 #fff, "
                 "stop: 1 #ff674b);\n";
  }
  style += "}\n"
             "QAbstractButton:hover {\n"
             "	color: #333;\n"
             "	background: qradialgradient(cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4, radius: 1.35, stop: 0 #fff, stop: 1 "
             "#bbb);\n"
             "}\n"
             "QAbstractButton:pressed {\n"
             "	color: #333;\n"
             "	background: qradialgradient(cx: 0.4, cy: -0.1, fx: 0.4, fy: -0.1, radius: 1.35, stop: 0 #fff, stop: 1 "
             "#ddd);\n"
             "}\n";

  btn->setStyleSheet(style);
}  // RelinkablePathVisualization::stylePathComponentButton

void RelinkablePathVisualization::paintEvent(QPaintEvent* evt) {
  const int totalItems = m_layout->count();  // Note that there is an extra stretch item.
  for (int i = 0; i < totalItems; ++i) {
    QWidget* widget = m_layout->itemAt(i)->widget();
    if (!widget) {
      continue;
    }

    QStyleOption option;
    option.initFrom(widget);

    if (option.state & QStyle::State_MouseOver) {
      widget->setProperty("highlightEnforcer", true);
      // Update the forceHighlight attribute for all buttons.
      for (int j = 0; j < totalItems; ++j) {
        widget = m_layout->itemAt(j)->widget();
        if (widget) {
          const bool highlight = j <= i;
          if (widget->property("forceHighlight").toBool() != highlight) {
            widget->setProperty("forceHighlight", highlight);
            widget->update();
          }
        }
      }
      break;
    } else if (widget->property("highlightEnforcer").toBool()) {
      widget->setProperty("highlightEnforcer", false);

      // Update the forceHighlight attribute for all buttons.
      for (int j = 0; j < totalItems; ++j) {
        widget = m_layout->itemAt(j)->widget();
        if (widget) {
          const bool highlight = false;
          if (widget->property("forceHighlight").toBool() != highlight) {
            widget->setProperty("forceHighlight", highlight);
            widget->update();
          }
        }
      }
      break;
    }
  }
}  // RelinkablePathVisualization::paintEvent

void RelinkablePathVisualization::onClicked(int componentIdx,
                                            const QString& prefixPath,
                                            const QString& suffixPath,
                                            int type) {
  // We'd like highlighting to stick until this method returns.

  for (int i = 0; i <= componentIdx; ++i) {
    QWidget* widget = m_layout->itemAt(i)->widget();
    if (widget) {
      widget->setProperty("stickHighlight", true);
    }
  }

  emit clicked(prefixPath, suffixPath, type);
  // Note that clear() or setPath() might have been called by a signal handler.
  const int totalItems = m_layout->count();  // Note that there is an extra stretch item.
  for (int i = 0; i <= componentIdx && i < totalItems; ++i) {
    QWidget* widget = m_layout->itemAt(i)->widget();
    if (widget) {
      widget->setProperty("stickHighlight", false);
    }
  }
}

void RelinkablePathVisualization::checkForExistence(std::vector<PathComponent>& components) {
  // Instead of calling QFile::exists() [which also works on directories]
  // for every component, we use binary search. That's especially important
  // when dealing with network paths.
  if (components.empty()) {
    return;
  }

  if (QFile::exists(components.back().prefixPath)) {
    for (PathComponent& comp : components) {
      comp.exists = true;
    }
    return;
  }

  int left = -1;                                         // Existing component (unless -1).
  auto right = static_cast<int>(components.size() - 1);  // Non-existing component (we checked it above).
  while (right - left > 1) {
    const int mid = (left + right + 1) >> 1;
    if (QFile::exists(components[mid].prefixPath)) {
      left = mid;
    } else {
      right = mid;
    }
  }

  for (auto i = static_cast<int>(components.size() - 1); i >= 0; --i) {
    components[i].exists = (i < right);
  }
}  // RelinkablePathVisualization::checkForExistence
/*============================ ComponentButton ============================*/

void RelinkablePathVisualization::ComponentButton::paintEvent(QPaintEvent* evt) {
  QStyleOptionButton option;
  option.initFrom(this);
  option.text = text();
  if (property("forceHighlight").toBool() || property("stickHighlight").toBool()) {
    option.state |= QStyle::State_MouseOver;
  }

  // Prevent weird looking font effects for disabled buttons with Windows XP style.
  option.state |= QStyle::State_Enabled;

  QStylePainter painter(this);
  painter.drawControl(QStyle::CE_PushButton, option);
}

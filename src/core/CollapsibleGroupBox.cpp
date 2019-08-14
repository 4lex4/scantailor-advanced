
#include "CollapsibleGroupBox.h"
#include <foundation/ScopedIncDec.h>
#include <QSettings>
#include <QtCore/QEvent>
#include <QtGui/QShowEvent>
#include "IconProvider.h"

CollapsibleGroupBox::CollapsibleGroupBox(QWidget* parent) : QGroupBox(parent) {
  initialize();
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent) : QGroupBox(title, parent) {
  initialize();
}

void CollapsibleGroupBox::initialize() {
  m_collapseIcon = IconProvider::getInstance().getIcon("collapse");
  m_expandIcon = IconProvider::getInstance().getIcon("expand");
  m_collapseButton = new QToolButton(this);
  m_collapseButton->setObjectName("collapseButton");
  m_collapseButton->setAutoRaise(true);
  m_collapseButton->setFixedSize(14, 14);
  m_collapseButton->setIconSize({10, 10});
  m_collapseButton->setIcon(m_collapseIcon);
  setFocusProxy(m_collapseButton);
  setFocusPolicy(Qt::StrongFocus);

  this->setAlignment(Qt::AlignCenter);

  connect(m_collapseButton, &QAbstractButton::clicked, this, &CollapsibleGroupBox::toggleCollapsed);
  connect(this, &QGroupBox::toggled, this, &CollapsibleGroupBox::checkToggled);
  connect(this, &QGroupBox::clicked, this, &CollapsibleGroupBox::checkClicked);
}

void CollapsibleGroupBox::setCollapsed(const bool collapse) {
  const bool changed = (collapse != m_collapsed);

  if (changed) {
    m_collapsed = collapse;
    m_collapseButton->setIcon(collapse ? m_expandIcon : m_collapseIcon);

    updateWidgets();

    emit collapsedStateChanged(isCollapsed());
  }
}

bool CollapsibleGroupBox::isCollapsed() const {
  return m_collapsed;
}

void CollapsibleGroupBox::checkToggled(bool) {
  m_collapseButton->setEnabled(true);
}

void CollapsibleGroupBox::checkClicked(bool checked) {
  if (checked && isCollapsed()) {
    setCollapsed(false);
  } else if (!checked && !isCollapsed()) {
    setCollapsed(true);
  }
}

void CollapsibleGroupBox::toggleCollapsed() {
  // verify if sender is this group box's collapse button
  auto* sender = dynamic_cast<QToolButton*>(QObject::sender());
  const bool isSenderCollapseButton = (sender && (sender == m_collapseButton));

  if (isSenderCollapseButton) {
    setCollapsed(!isCollapsed());
  }
}

void CollapsibleGroupBox::updateWidgets() {
  const ScopedIncDec<int> guard(m_ignoreVisibilityEvents);

  if (m_collapsed) {
    for (QObject* child : children()) {
      auto* widget = dynamic_cast<QWidget*>(child);
      if (widget && (widget != m_collapseButton) && widget->isVisible()) {
        m_collapsedWidgets.insert(widget);
        widget->hide();
      }
    }
  } else {
    for (QObject* child : children()) {
      auto* widget = dynamic_cast<QWidget*>(child);
      if (widget && (widget != m_collapseButton) && (m_collapsedWidgets.find(widget) != m_collapsedWidgets.end())) {
        m_collapsedWidgets.erase(widget);
        widget->show();
      }
    }
  }
}

void CollapsibleGroupBox::showEvent(QShowEvent* event) {
  // initialize widget on first show event only
  if (m_shown) {
    event->accept();
    return;
  }
  m_shown = true;

  loadState();

  QWidget::showEvent(event);
}

void CollapsibleGroupBox::changeEvent(QEvent* event) {
  QGroupBox::changeEvent(event);

  if ((event->type() == QEvent::EnabledChange) && isEnabled()) {
    m_collapseButton->setEnabled(true);
  }
}

void CollapsibleGroupBox::childEvent(QChildEvent* event) {
  auto* childWidget = dynamic_cast<QWidget*>(event->child());
  if (childWidget && (event->type() == QEvent::ChildAdded)) {
    if (m_collapsed) {
      if (childWidget->isVisible()) {
        m_collapsedWidgets.insert(childWidget);
        childWidget->hide();
      }
    }

    childWidget->installEventFilter(this);
  }

  QGroupBox::childEvent(event);
}

bool CollapsibleGroupBox::eventFilter(QObject* watched, QEvent* event) {
  if (m_collapsed && !m_ignoreVisibilityEvents) {
    auto* childWidget = dynamic_cast<QWidget*>(watched);
    if (childWidget) {
      if (event->type() == QEvent::ShowToParent) {
        const ScopedIncDec<int> guard(m_ignoreVisibilityEvents);

        m_collapsedWidgets.insert(childWidget);
        childWidget->hide();
      } else if (event->type() == QEvent::HideToParent) {
        m_collapsedWidgets.erase(childWidget);
      }
    }
  }

  return QObject::eventFilter(watched, event);
}

CollapsibleGroupBox::~CollapsibleGroupBox() {
  saveState();
}

void CollapsibleGroupBox::loadState() {
  if (!isEnabled()) {
    return;
  }

  const QString key = getSettingsKey();
  if (key.isEmpty()) {
    return;
  }

  setUpdatesEnabled(false);

  QSettings settings;

  if (isCheckable()) {
    QVariant val = settings.value(key + "/checked");
    if (!val.isNull()) {
      setChecked(val.toBool());
    }
  }

  {
    QVariant val = settings.value(key + "/collapsed");
    if (!val.isNull()) {
      setCollapsed(val.toBool());
    }
  }

  setUpdatesEnabled(true);
}

void CollapsibleGroupBox::saveState() {
  if (!m_shown || !isEnabled()) {
    return;
  }

  const QString key = getSettingsKey();
  if (key.isEmpty()) {
    return;
  }

  QSettings settings;

  if (isCheckable()) {
    settings.setValue(key + "/checked", isChecked());
  }
  settings.setValue(key + "/collapsed", isCollapsed());
}

QString CollapsibleGroupBox::getSettingsKey() const {
  if (objectName().isEmpty()) {
    return QString();
  }

  QString saveKey = '/' + objectName();
  saveKey = "CollapsibleGroupBox" + saveKey;
  return saveKey;
}

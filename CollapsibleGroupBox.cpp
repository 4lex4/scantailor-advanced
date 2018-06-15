
#include "CollapsibleGroupBox.h"
#include <foundation/ScopedIncDec.h>
#include <QSettings>
#include <QtCore/QEvent>
#include <QtGui/QShowEvent>

CollapsibleGroupBox::CollapsibleGroupBox(QWidget* parent) : QGroupBox(parent) {
  initialize();
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent) : QGroupBox(title, parent) {
  initialize();
}

void CollapsibleGroupBox::initialize() {
  collapseIcon.addPixmap(QPixmap(QString::fromLatin1(":/icons/minus-16.png")));
  expandIcon.addPixmap(QPixmap(QString::fromLatin1(":/icons/plus-16.png")));
  collapseButton = new QToolButton(this);
  collapseButton->setObjectName("collapseButton");
  collapseButton->setAutoRaise(true);
  collapseButton->setFixedSize(14, 14);
  collapseButton->setIconSize(QSize(12, 12));
  collapseButton->setIcon(collapseIcon);
  setFocusProxy(collapseButton);
  setFocusPolicy(Qt::StrongFocus);

  connect(collapseButton, &QAbstractButton::clicked, this, &CollapsibleGroupBox::toggleCollapsed);
  connect(this, &QGroupBox::toggled, this, &CollapsibleGroupBox::checkToggled);
  connect(this, &QGroupBox::clicked, this, &CollapsibleGroupBox::checkClicked);
}

void CollapsibleGroupBox::setCollapsed(const bool collapse) {
  const bool changed = (collapse != collapsed);

  if (changed) {
    collapsed = collapse;
    collapseButton->setIcon(collapse ? expandIcon : collapseIcon);

    updateWidgets();

    emit collapsedStateChanged(isCollapsed());
  }
}

bool CollapsibleGroupBox::isCollapsed() const {
  return collapsed;
}

void CollapsibleGroupBox::checkToggled(bool) {
  collapseButton->setEnabled(true);
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
  const bool isSenderCollapseButton = (sender && (sender == collapseButton));

  if (isSenderCollapseButton) {
    setCollapsed(!isCollapsed());
  }
}

void CollapsibleGroupBox::updateWidgets() {
  const ScopedIncDec<int> guard(ignoreVisibilityEvents);

  if (collapsed) {
    for (QObject* child : children()) {
      auto* widget = dynamic_cast<QWidget*>(child);
      if (widget && (widget != collapseButton) && widget->isVisible()) {
        collapsedWidgets.insert(widget);
        widget->hide();
      }
    }
  } else {
    for (QObject* child : children()) {
      auto* widget = dynamic_cast<QWidget*>(child);
      if (widget && (widget != collapseButton) && (collapsedWidgets.find(widget) != collapsedWidgets.end())) {
        collapsedWidgets.erase(widget);
        widget->show();
      }
    }
  }
}

void CollapsibleGroupBox::showEvent(QShowEvent* event) {
  // initialize widget on first show event only
  if (shown) {
    event->accept();
    return;
  }
  shown = true;

  loadState();

  QWidget::showEvent(event);
}

void CollapsibleGroupBox::changeEvent(QEvent* event) {
  QGroupBox::changeEvent(event);

  if ((event->type() == QEvent::EnabledChange) && isEnabled()) {
    collapseButton->setEnabled(true);
  }
}

void CollapsibleGroupBox::childEvent(QChildEvent* event) {
  auto* childWidget = dynamic_cast<QWidget*>(event->child());
  if (childWidget && (event->type() == QEvent::ChildAdded)) {
    if (collapsed) {
      if (childWidget->isVisible()) {
        collapsedWidgets.insert(childWidget);
        childWidget->hide();
      }
    }

    childWidget->installEventFilter(this);
  }

  QGroupBox::childEvent(event);
}

bool CollapsibleGroupBox::eventFilter(QObject* watched, QEvent* event) {
  if (collapsed && !ignoreVisibilityEvents) {
    auto* childWidget = dynamic_cast<QWidget*>(watched);
    if (childWidget) {
      if (event->type() == QEvent::ShowToParent) {
        const ScopedIncDec<int> guard(ignoreVisibilityEvents);

        collapsedWidgets.insert(childWidget);
        childWidget->hide();
      } else if (event->type() == QEvent::HideToParent) {
        collapsedWidgets.erase(childWidget);
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
  if (!shown || !isEnabled()) {
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

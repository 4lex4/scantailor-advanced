// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_COLLAPSIBLEGROUPBOX_H_
#define SCANTAILOR_CORE_COLLAPSIBLEGROUPBOX_H_


#include <QtWidgets/QGroupBox>
#include <QtWidgets/QToolButton>
#include <unordered_set>

class CollapsibleGroupBox : public QGroupBox {
  Q_OBJECT

  /**
   * The collapsed state of this group box. If it is set to true, all content is hidden
   * if it is set to false all content is shown.
   */
  Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed USER true)

 public:
  explicit CollapsibleGroupBox(QWidget* parent = nullptr);

  explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);

  ~CollapsibleGroupBox() override;

  /**
   * Returns the current collapsed state of this group box.
   */
  bool isCollapsed() const;

  /**
   * Collapse or expand this group box.
   *
   * \param collapse Will collapse on true and expand on false
   */
  void setCollapsed(bool collapse);

 signals:

  /** Signal emitted when the group box collapsed/expanded state is changed, and when first shown */
  void collapsedStateChanged(bool collapsed);

 public slots:

  void checkToggled(bool);

  void checkClicked(bool checked);

  void toggleCollapsed();

 protected:
  void updateWidgets();

  void showEvent(QShowEvent* event) override;

  void changeEvent(QEvent* event) override;

  void childEvent(QChildEvent* event) override;

  bool eventFilter(QObject* watched, QEvent* event) override;

  void initialize();

  void loadState();

  void saveState();

  QString getSettingsKey() const;

 private:
  bool m_collapsed = false;
  bool m_shown = false;
  QToolButton* m_collapseButton = nullptr;

  QIcon m_collapseIcon;
  QIcon m_expandIcon;

  int m_ignoreVisibilityEvents = 0;
  std::unordered_set<QWidget*> m_collapsedWidgets;
};


#endif  // SCANTAILOR_CORE_COLLAPSIBLEGROUPBOX_H_


#ifndef SCANTAILOR_COLLAPSIBLEGROUPBOX_H
#define SCANTAILOR_COLLAPSIBLEGROUPBOX_H


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
  bool collapsed = false;
  bool shown = false;
  QToolButton* collapseButton = nullptr;

  QIcon collapseIcon;
  QIcon expandIcon;

  int ignoreVisibilityEvents = 0;
  std::unordered_set<QWidget*> collapsedWidgets;
};


#endif  // SCANTAILOR_COLLAPSIBLEGROUPBOX_H


#ifndef SCANTAILOR_STATUSBARPANEL_H
#define SCANTAILOR_STATUSBARPANEL_H

#include <QtCore/QMutex>
#include <QtWidgets/QWidget>
#include "Dpi.h"
#include "ImageViewInfoObserver.h"
#include "ImageViewInfoProvider.h"
#include "UnitsObserver.h"
#include "ui_StatusBarPanel.h"

class PageId;

class StatusBarPanel : public QWidget, public UnitsObserver, public ImageViewInfoObserver {
  Q_OBJECT
 public:
  StatusBarPanel();

  ~StatusBarPanel() override = default;

 public:
  void updateMousePos(const QPointF& mousePos) override;

  void updatePhysSize(const QSizeF& physSize) override;

  void updateDpi(const Dpi& dpi) override;

  void clearImageViewInfo() override;

  void updatePage(int pageNumber, size_t pageCount, const PageId& pageId);

  void clear();

  void updateUnits(Units) override;

  void setInfoProvider(ImageViewInfoProvider* infoProvider) override;

 private:
  void mousePosChanged();

  void physSizeChanged();

  Ui::StatusBarPanel ui;
  mutable QMutex m_mutex;
  QPointF m_mousePos;
  QSizeF m_physSize;
  Dpi m_dpi;
  ImageViewInfoProvider* m_infoProvider;
};


#endif  // SCANTAILOR_STATUSBARPANEL_H

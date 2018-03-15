
#ifndef SCANTAILOR_BINARIZATIONOPTIONSWIDGET_H
#define SCANTAILOR_BINARIZATIONOPTIONSWIDGET_H

#include <QWidget>
#include <PageId.h>

namespace output {
class BinarizationOptionsWidget : public QWidget {
    Q_OBJECT
public:
    virtual void preUpdateUI(const PageId& m_pageId) = 0;

signals:

    /**
     * \brief To be emitted by subclasses when their state has changed.
     */
    void stateChanged();
};
}  // namespace output


#endif  // SCANTAILOR_BINARIZATIONOPTIONSWIDGET_H


#ifndef SCANTAILOR_WOLFBINARIZATIONOPTIONSWIDGET_H
#define SCANTAILOR_WOLFBINARIZATIONOPTIONSWIDGET_H

#include "ui_WolfBinarizationOptionsWidget.h"
#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "IntrusivePtr.h"
#include "Settings.h"
#include <QtCore>

namespace output {
    class WolfBinarizationOptionsWidget : public BinarizationOptionsWidget, private Ui::WolfBinarizationOptionsWidget {
    Q_OBJECT

    private:
        IntrusivePtr<Settings> m_ptrSettings;
        PageId m_pageId;
        ColorParams m_colorParams;
        QTimer delayedStateChanger;

    public:
        explicit WolfBinarizationOptionsWidget(IntrusivePtr<Settings> settings);

        ~WolfBinarizationOptionsWidget() override = default;

        void preUpdateUI(const PageId& m_pageId) override;

    private slots:

        void windowSizeChanged(int value);

        void wolfCoefChanged(double value);

        void lowerBoundChanged(int value);

        void upperBoundChanged(int value);

        void whiteOnBlackModeToggled(bool checked);

        void sendStateChanged();

    private:
        void updateView();
    };
}


#endif //SCANTAILOR_WOLFBINARIZATIONOPTIONSWIDGET_H



#ifndef SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H
#define SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H

#include "ui_SauvolaBinarizationOptionsWidget.h"
#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "IntrusivePtr.h"
#include "Settings.h"
#include <QtCore>

namespace output {
    class SauvolaBinarizationOptionsWidget : public BinarizationOptionsWidget, private Ui::SauvolaBinarizationOptionsWidget {
    Q_OBJECT

    private:
        IntrusivePtr<Settings> m_ptrSettings;
        PageId m_pageId;
        ColorParams m_colorParams;
        QTimer delayedStateChanger;

    public:
        explicit SauvolaBinarizationOptionsWidget(IntrusivePtr<Settings> settings);

        ~SauvolaBinarizationOptionsWidget() override = default;

        void preUpdateUI(const PageId& m_pageId) override;

    private slots:

        void windowSizeChanged(int value);

        void sauvolaCoefChanged(double value);

        void sendStateChanged();

    private:
        void updateView();
    };
}


#endif //SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H



#ifndef SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H
#define SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H

#include "ui_SauvolaBinarizationOptionsWidget.h"
#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "intrusive_ptr.h"
#include "Settings.h"
#include <QtCore>

namespace output {
    class SauvolaBinarizationOptionsWidget
            : public BinarizationOptionsWidget, private Ui::SauvolaBinarizationOptionsWidget {
    Q_OBJECT

    private:
        intrusive_ptr<Settings> m_ptrSettings;
        PageId m_pageId;
        ColorParams m_colorParams;
        QTimer delayedStateChanger;

    public:
        explicit SauvolaBinarizationOptionsWidget(intrusive_ptr<Settings> settings);

        ~SauvolaBinarizationOptionsWidget() override = default;

        void preUpdateUI(const PageId& m_pageId) override;

    private slots:

        void windowSizeChanged(int value);

        void sauvolaCoefChanged(double value);

        void whiteOnBlackModeToggled(bool checked);

        void sendStateChanged();

    private:
        void updateView();
    };
}


#endif //SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H

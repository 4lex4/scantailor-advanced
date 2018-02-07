

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
        OutputProcessingParams m_outputProcessingParams;

    public:
        explicit SauvolaBinarizationOptionsWidget(intrusive_ptr<Settings> settings);

        ~SauvolaBinarizationOptionsWidget() override = default;

        void preUpdateUI(const PageId& m_pageId) override;

    private slots:

        void windowSizeChanged(int value);

        void sauvolaCoefChanged(double value);

        void sendStateChanged();

    private:
        void updateView();

        void setupUiConnections();

        void removeUiConnections();
    };
}


#endif //SCANTAILOR_SAUVOLABINARIZATIONOPTIONS_H

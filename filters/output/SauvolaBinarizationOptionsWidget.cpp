
#include "SauvolaBinarizationOptionsWidget.h"


namespace output {

    SauvolaBinarizationOptionsWidget::SauvolaBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
            : m_ptrSettings(settings) {
        setupUi(this);

        updateView();

        delayedStateChanger.setSingleShot(true);

        connect(
                windowSize, SIGNAL(valueChanged(int)),
                this, SLOT(windowSizeChanged(int))
        );
        connect(
                sauvolaCoef, SIGNAL(valueChanged(double)),
                this, SLOT(sauvolaCoefChanged(double))
        );
        connect(
                &delayedStateChanger, SIGNAL(timeout()),
                this, SLOT(sendStateChanged())
        );
        connect(
                whiteOnBlackModeCB, SIGNAL(clicked(bool)),
                this, SLOT(whiteOnBlackModeToggled(bool))
        );
    }

    void SauvolaBinarizationOptionsWidget::preUpdateUI(PageId const& page_id) {
        const Params params(m_ptrSettings->getParams(page_id));
        m_pageId = page_id;
        m_colorParams = params.colorParams();

        updateView();
    }

    void SauvolaBinarizationOptionsWidget::windowSizeChanged(int value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setWindowSize(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        delayedStateChanger.start(750);
    }

    void SauvolaBinarizationOptionsWidget::sauvolaCoefChanged(double value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setSauvolaCoef(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        delayedStateChanger.start(750);
    }

    void SauvolaBinarizationOptionsWidget::whiteOnBlackModeToggled(bool checked) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setWhiteOnBlackMode(checked);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit stateChanged();
    }

    void SauvolaBinarizationOptionsWidget::updateView() {
        BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
        windowSize->setValue(blackWhiteOptions.getWindowSize());
        sauvolaCoef->setValue(blackWhiteOptions.getSauvolaCoef());
        whiteOnBlackModeCB->setChecked(blackWhiteOptions.isWhiteOnBlackMode());
    }

    void SauvolaBinarizationOptionsWidget::sendStateChanged() {
        emit stateChanged();
    }

}
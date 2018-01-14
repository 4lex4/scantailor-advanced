

#include "WolfBinarizationOptionsWidget.h"

namespace output {

    WolfBinarizationOptionsWidget::WolfBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
            : m_ptrSettings(settings) {
        setupUi(this);

        delayedStateChanger.setSingleShot(true);

        setupUiConnections();
    }

    void WolfBinarizationOptionsWidget::preUpdateUI(PageId const& page_id) {
        removeUiConnections();

        const Params params(m_ptrSettings->getParams(page_id));
        m_pageId = page_id;
        m_colorParams = params.colorParams();
        m_outputProcessingParams = m_ptrSettings->getOutputProcessingParams(page_id);

        updateView();

        setupUiConnections();
    }

    void WolfBinarizationOptionsWidget::windowSizeChanged(int value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setWindowSize(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        delayedStateChanger.start(750);
    }

    void WolfBinarizationOptionsWidget::lowerBoundChanged(int value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setWolfLowerBound(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        delayedStateChanger.start(750);
    }

    void WolfBinarizationOptionsWidget::upperBoundChanged(int value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setWolfUpperBound(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        delayedStateChanger.start(750);
    }

    void WolfBinarizationOptionsWidget::wolfCoefChanged(double value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setWolfCoef(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        delayedStateChanger.start(750);
    }

    void WolfBinarizationOptionsWidget::whiteOnBlackModeToggled(bool checked) {
        m_outputProcessingParams.setWhiteOnBlackMode(checked);
        m_ptrSettings->setOutputProcessingParams(m_pageId, m_outputProcessingParams);

        emit stateChanged();
    }

    void WolfBinarizationOptionsWidget::updateView() {
        BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
        windowSize->setValue(blackWhiteOptions.getWindowSize());
        lowerBound->setValue(blackWhiteOptions.getWolfLowerBound());
        upperBound->setValue(blackWhiteOptions.getWolfUpperBound());
        wolfCoef->setValue(blackWhiteOptions.getWolfCoef());
        whiteOnBlackModeCB->setChecked(m_outputProcessingParams.isWhiteOnBlackMode());
    }

    void WolfBinarizationOptionsWidget::sendStateChanged() {
        emit stateChanged();
    }

    void WolfBinarizationOptionsWidget::setupUiConnections() {
        connect(
                windowSize, SIGNAL(valueChanged(int)),
                this, SLOT(windowSizeChanged(int))
        );
        connect(
                lowerBound, SIGNAL(valueChanged(int)),
                this, SLOT(lowerBoundChanged(int))
        );
        connect(
                upperBound, SIGNAL(valueChanged(int)),
                this, SLOT(upperBoundChanged(int))
        );
        connect(
                wolfCoef, SIGNAL(valueChanged(double)),
                this, SLOT(wolfCoefChanged(double))
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

    void WolfBinarizationOptionsWidget::removeUiConnections() {
        disconnect(
                windowSize, SIGNAL(valueChanged(int)),
                this, SLOT(windowSizeChanged(int))
        );
        disconnect(
                lowerBound, SIGNAL(valueChanged(int)),
                this, SLOT(lowerBoundChanged(int))
        );
        disconnect(
                upperBound, SIGNAL(valueChanged(int)),
                this, SLOT(upperBoundChanged(int))
        );
        disconnect(
                wolfCoef, SIGNAL(valueChanged(double)),
                this, SLOT(wolfCoefChanged(double))
        );
        disconnect(
                &delayedStateChanger, SIGNAL(timeout()),
                this, SLOT(sendStateChanged())
        );
        disconnect(
                whiteOnBlackModeCB, SIGNAL(clicked(bool)),
                this, SLOT(whiteOnBlackModeToggled(bool))
        );
    }


}
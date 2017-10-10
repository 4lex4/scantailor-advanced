
#include <QtWidgets/QToolTip>
#include <foundation/ScopedIncDec.h>
#include "OtsuBinarizationOptionsWidget.h"
#include "../../Utils.h"

namespace output {

    OtsuBinarizationOptionsWidget::OtsuBinarizationOptionsWidget(IntrusivePtr<Settings> settings)
            : m_ptrSettings(settings) {
        setupUi(this);

        darkerThresholdLink->setText(
                Utils::richTextForLink(darkerThresholdLink->text())
        );
        lighterThresholdLink->setText(
                Utils::richTextForLink(lighterThresholdLink->text())
        );
        thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));

        thresholdSlider->setMinimum(-50);
        thresholdSlider->setMaximum(50);
        thresholLabel->setText(QString::number(thresholdSlider->value()));

        updateView();

        connect(
                lighterThresholdLink, SIGNAL(linkActivated(QString const &)),
                this, SLOT(setLighterThreshold())
        );
        connect(
                darkerThresholdLink, SIGNAL(linkActivated(QString const &)),
                this, SLOT(setDarkerThreshold())
        );
        connect(
                thresholdSlider, SIGNAL(sliderReleased()),
                this, SLOT(bwThresholdChanged())
        );
        connect(
                thresholdSlider, SIGNAL(valueChanged(int)),
                this, SLOT(thresholdValueChanged(int))
        );
        connect(
                neutralThresholdBtn, SIGNAL(clicked()),
                this, SLOT(setNeutralThreshold())
        );
    }

    void OtsuBinarizationOptionsWidget::preUpdateUI(PageId const& page_id) {
        const Params params(m_ptrSettings->getParams(page_id));
        m_pageId = page_id;
        m_colorParams = params.colorParams();

        updateView();
    }

    void OtsuBinarizationOptionsWidget::bwThresholdChanged() {
        int const value = thresholdSlider->value();
        QString const tooltip_text(QString::number(value));
        thresholdSlider->setToolTip(tooltip_text);

        thresholLabel->setText(QString::number(value));

        if (m_ignoreThresholdChanges) {
            return;
        }

        QPoint const center(thresholdSlider->rect().center());
        QPoint tooltip_pos(thresholdSlider->mapFromGlobal(QCursor::pos()));
        tooltip_pos.setY(center.y());
        tooltip_pos.setX(qBound(0, tooltip_pos.x(), thresholdSlider->width()));
        tooltip_pos = thresholdSlider->mapToGlobal(tooltip_pos);
        QToolTip::showText(tooltip_pos, tooltip_text, thresholdSlider);

        if (thresholdSlider->isSliderDown()) {
            return;
        }

        thresholdValueChanged(value);
    }      // OtsuBinarizationOptionsWidget::bwThresholdChanged

    void OtsuBinarizationOptionsWidget::thresholdValueChanged(int value) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        if (opt.thresholdAdjustment() == value) {
            return;
        }

        thresholLabel->setText(QString::number(value));

        opt.setThresholdAdjustment(value);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        emit stateChanged();
    }

    void OtsuBinarizationOptionsWidget::setLighterThreshold() {
        thresholdSlider->setValue(thresholdSlider->value() - 1);
        thresholdValueChanged(thresholdSlider->value());
    }

    void OtsuBinarizationOptionsWidget::setDarkerThreshold() {
        thresholdSlider->setValue(thresholdSlider->value() + 1);
        thresholdValueChanged(thresholdSlider->value());
    }

    void OtsuBinarizationOptionsWidget::setNeutralThreshold() {
        thresholdSlider->setValue(0);
        thresholdValueChanged(thresholdSlider->value());
    }

    void OtsuBinarizationOptionsWidget::updateView() {
        BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
        ScopedIncDec<int> const guard(m_ignoreThresholdChanges);
        thresholdSlider->setValue(blackWhiteOptions.thresholdAdjustment());
        thresholLabel->setText(QString::number(blackWhiteOptions.thresholdAdjustment()));
    }

}    // namespace output
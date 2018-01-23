
#ifndef SCANTAILOR_SPLITTINGOPTIONS_H
#define SCANTAILOR_SPLITTINGOPTIONS_H

#include <QtXml/QDomElement>

namespace output {
    class SplittingOptions {
    public:
        enum ForegroundType {
            BLACK_AND_WHITE_FOREGROUND,
            COLOR_FOREGROUND
        };

        SplittingOptions();

        explicit SplittingOptions(QDomElement const& el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        bool isSplitOutput() const;

        void setSplitOutput(bool splitOutput);

        ForegroundType getForegroundType() const;

        void setForegroundType(ForegroundType foregroundType);

        bool operator==(const SplittingOptions& other) const;

        bool operator!=(const SplittingOptions& other) const;

        bool isOriginalBackground() const;

        void setOriginalBackground(bool originalBackground);

    private:
        static ForegroundType parseForegroundType(const QString& str);

        static QString formatForegroundType(ForegroundType type);

        bool splitOutput;
        ForegroundType foregroundType;
        bool originalBackground;
    };
}


#endif //SCANTAILOR_SPLITTINGOPTIONS_H

#ifndef SPECTRAL_HOM_LOG_H
#define SPECTRAL_HOM_LOG_H

#include <string>

#include <QPlainTextEdit>

namespace spec_hom {

    class BgThread;

    class LogPanel : public QPlainTextEdit {
        Q_OBJECT

    public:
        explicit LogPanel(QWidget *parent);

        void connectToThread(BgThread *thread);

    public slots:
        void log(std::string str); // copy necessary to avoid concurrent access
        void warn(std::string str);
        void err(std::string str);

    private:

    };

}

#endif //SPECTRAL_HOM_LOG_H

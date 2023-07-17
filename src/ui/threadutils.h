#ifndef SPECTRAL_HOM_THREADUTILS_H
#define SPECTRAL_HOM_THREADUTILS_H

#include <QObject>
#include <QRunnable>
#include <QProgressDialog>

namespace spec_hom {

    class LogPanel;
    class BgProgressBar;

    // Handles common functionality of background long-running threads
    class BgThread : public QObject, public QRunnable {
        Q_OBJECT

    public:
        BgThread();

        void run() final;
        virtual void execute() = 0; // override this in the child classes
        [[nodiscard]] bool shouldCancel() const;

    signals:
        void log(std::string str);
        void warn(std::string str);
        void err(std::string str);

        // controls for the progress bar
        void setProgress(int value); // between 0 and 100
        void setProgressIndefinite(bool value);
        void setProgressText(std::string str);
        void setProgressTextColor(QColor color);

        void threadDone(); // emitted when the thread finishes

    public slots:
        void cancel();

    private:
        bool mShouldCancel;
    };

}

#endif //SPECTRAL_HOM_THREADUTILS_H

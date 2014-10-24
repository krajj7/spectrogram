#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

/** \file mainwindow.hpp
 * \brief Definitions for everything that has to do with GUI.*/

#include <QFutureWatcher>
#include "spectrogram.hpp"
#include "ui_mainwindow.h"

/// Represents the main application window.
class MainWindow : public QMainWindow
{
    Q_OBJECT
    public:
        MainWindow();
    private:
        Ui::MainWindow ui;
        Soundfile soundfile;
        bool soundfileOk();
        QImage image;
        bool imageOk();
        Spectrogram* spectrogram;
        void setValues();
        void loadValues();
        void updatePalette();

        void workingState();
        void idleState();

        QFutureWatcher<QImage>* image_watcher;
        QFutureWatcher<real_vec>* sound_watcher;
    private slots:
        void setFilterUnits(int scale);

        void makeSpectrogram();
        bool checkAnalysisValues();
        void makeSound();
        bool checkSynthesisValues();

        void chooseImage();
        void loadImage();
        void updateImage();
        void resetImage();
        void saveImage();

        void chooseSoundfile();
        void loadSoundfile();
        void updateSoundfile();
        void resetSoundfile();
        void saveSoundfile(const real_vec& signal);

        void choosePalette();

        //void showHelp(QWidget* parent, const QString& text) const;
        bool confirmWarnings(const QStringList& errors);

        void newSpectrogram();
        void newSound();
    signals:
        //void makeSound(Spectrogram* spectrogram, QImage image, int samplerate);
        void makeSpectrogram(Spectrogram* spectrogram, real_vec signal,
                int samplerate);
};

#endif

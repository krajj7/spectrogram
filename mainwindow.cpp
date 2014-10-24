#include <QCompleter>
#include <QtConcurrentRun>
#include <QDirModel>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QTime>
#include <QPixmap>
#include <QWhatsThis>
#include "mainwindow.hpp"
#include <iostream>
#include <cassert>

namespace
{
    void setCombo(QComboBox* combo, int value)
    {
        for (int index = 0; index < combo->count(); ++index)
            if (combo->itemData(index) == value)
                combo->setCurrentIndex(index);
    }
}

MainWindow::MainWindow()
{
    ui.setupUi(this);

    QCompleter* completer = new QCompleter(ui.locationEdit);
    completer->setModel(new QDirModel(completer));
    ui.locationEdit->setCompleter(completer);
    ui.speclocEdit->setCompleter(completer);

    resetSoundfile();
    connect(ui.locationEdit, SIGNAL(editingFinished()),
            this, SLOT(loadSoundfile()));
    connect(ui.locationButton, SIGNAL(clicked()),
            this, SLOT(chooseSoundfile()));

    connect(ui.paletteButton, SIGNAL(clicked()), this, SLOT(choosePalette()));

    connect(ui.specSaveAsButton, SIGNAL(clicked()), this, SLOT(saveImage()));
    connect(ui.speclocButton, SIGNAL(clicked()), this, SLOT(chooseImage()));
    connect(ui.makeButton, SIGNAL(clicked()), this, SLOT(makeSpectrogram()));

    //connect(ui.soundSaveAsButton,SIGNAL(clicked()),this, SLOT(saveSoundfile()));
    connect(ui.makeSoundButton, SIGNAL(clicked()), this, SLOT(makeSound()));

    ui.intensityCombo->addItem("logarithmic", (int)SCALE_LOGARITHMIC);
    ui.intensityCombo->addItem("linear", (int)SCALE_LINEAR);

    ui.frequencyCombo->addItem("logarithmic", (int)SCALE_LOGARITHMIC);
    ui.frequencyCombo->addItem("linear", (int)SCALE_LINEAR);
    connect(ui.frequencyCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setFilterUnits(int)));

    ui.windowCombo->addItem("Hann", (int)WINDOW_HANN);
    ui.windowCombo->addItem("Blackman", (int)WINDOW_BLACKMAN);
    ui.windowCombo->addItem("Triangular", (int)WINDOW_TRIANGULAR);
    ui.windowCombo->addItem("Rectangular (none)", (int)WINDOW_RECTANGULAR);

    ui.syntCombo->addItem("sine", (int)SYNTHESIS_SINE);
    ui.syntCombo->addItem("noise", (int)SYNTHESIS_NOISE);

    ui.brightCombo->addItem("none", (int)BRIGHT_NONE);
    ui.brightCombo->addItem("square root", (int)BRIGHT_SQRT);

    spectrogram = new Spectrogram(this);
    connect(ui.cancelButton, SIGNAL(clicked()), spectrogram, SLOT(cancel()));
    connect(spectrogram, SIGNAL(progress(int)),
            ui.specProgress, SLOT(setValue(int)));
    connect(spectrogram, SIGNAL(status(const QString&)),
            ui.specStatus, SLOT(setText(const QString&)));
    setValues();

    image_watcher = new QFutureWatcher<QImage>(this);
    connect(image_watcher, SIGNAL(finished()), this, SLOT(newSpectrogram()));
    sound_watcher = new QFutureWatcher<real_vec>(this);
    connect(sound_watcher, SIGNAL(finished()), this, SLOT(newSound()));

    ui.lengthEdit->setDisplayFormat("hh:mm:ss");

    idleState();
}

void MainWindow::resetSoundfile()
{
    soundfile.reset();
    ui.lengthEdit->setTime(QTime(0,0,0));
    ui.channelsEdit->setText("0");
    ui.channelSpin->setMaximum(0);
    ui.samplerateSpin->setValue(0);
}

void MainWindow::loadSoundfile()
{
    const QString& filename = ui.locationEdit->text();
    if (filename.isEmpty())
        return;
    soundfile.load(filename);
    if (!soundfileOk())
    {
        QString error;
        QTextStream serror(&error);
        serror << "The specified file is not readable or not supported.";
        if (!soundfile.error().isEmpty())
            serror << "\n\n" << soundfile.error();
        QMessageBox::warning(this, "Invalid file", error);
        return;
    }
    updateSoundfile();
}

void MainWindow::updateSoundfile()
{
    if (soundfileOk())
    {
        ui.lengthEdit->setTime(QTime().addSecs((int)soundfile.data().length()));
        ui.channelSpin->setMinimum(1);
        ui.channelSpin->setMaximum(soundfile.data().channels());
        ui.channelsEdit->setText(QString::number(soundfile.data().channels()));
        ui.samplerateSpin->setValue(soundfile.data().samplerate());
    }
    else
        resetSoundfile();
}

bool MainWindow::soundfileOk()
{
    return soundfile.valid();
}

void MainWindow::choosePalette()
{
    QString filename = QFileDialog::getOpenFileName(this, 
            "Choose the palette image", ".",
            "Images (*.png *.jpg *.bmp *.gif);;All files (*.*)");
    if (filename == NULL)
        return;
    QImage img(filename);
    if (img.isNull())
    {
        QMessageBox::warning(this, "Invalid image", 
                "The picture format was not recognised.");
        return;
    }
    spectrogram->palette = Palette(img);
    updatePalette();
}

void MainWindow::chooseSoundfile()
{
    QString filename = QFileDialog::getOpenFileName(this, 
            "Choose the sound file", ".",
            "Sound files (*.wav *.mp3 *.ogg *.flac);;All files (*.*)");
    if (filename == NULL)
        return;
    ui.locationEdit->setText(filename);
    loadSoundfile();
}

void MainWindow::updatePalette()
{
    ui.paletteLabel->setPixmap(spectrogram->palette.preview(
                spectrogram->palette.numColors(), ui.paletteLabel->height()));
}

void MainWindow::saveImage()
{
    if (image.isNull())
    {
        QMessageBox::warning(this, "Couldn't save file",
                "There is nothing to save yet.");
        return;
    }
    QString filename = 
        QFileDialog::getSaveFileName(this, "Save spectrogram",
            "spectrogram.png", "Images (*.png *.xpm)");
    if (filename == NULL)
        return;
    if (filename.endsWith(".jpg") || filename.endsWith(".JPG"))
    {
        QMessageBox::warning(this, "Couldn't save file",
                "JPG is not supported for writing.  As a lossy compression format, it is a poor choice for spectrograms anyway.");
        return;
    }
    if (!filename.contains("."))
        filename.append(".png");
    const bool worked = image.save(filename);
    if (!worked)
    {
        QMessageBox::warning(this, "Couldn't save file",
                "The file could not be saved at the specified location, or you specified a not supported format extension.");
        return;
    }
    ui.speclocEdit->setText(filename);
}

void MainWindow::makeSpectrogram()
{
    if (!soundfileOk())
    {
        QMessageBox::warning(this, "No sound file",
                "Choose a valid sound file first.");
        return;
    }

    loadValues();

    if (!checkAnalysisValues())
        return;

    workingState();

    const int channelidx = ui.channelSpin->value()-1;
    ui.specStatus->setText("Loading sound file");
    qApp->processEvents();
    real_vec signal = soundfile.read_channel(channelidx);
    if (!signal.size())
    {
        QMessageBox::warning(this, "Error", "Error reading sound file.");
        idleState();
        return;
    }

    QFuture<QImage> future = QtConcurrent::run(spectrogram,
          &Spectrogram::to_image, signal, soundfile.data().samplerate());
    image_watcher->setFuture(future);
}

void MainWindow::loadValues()
{
    spectrogram->bandwidth = ui.bandwidthSpin->value();
    spectrogram->basefreq = ui.basefreqSpin->value();
    spectrogram->maxfreq = ui.maxfreqSpin->value();
    spectrogram->overlap = ui.overlapSpin->value()/100;
    spectrogram->pixpersec = ui.ppsSpin->value();
    spectrogram->window = (Window)ui.windowCombo->
        itemData(ui.windowCombo->currentIndex()).toInt();
    spectrogram->frequency_axis = (AxisScale)ui.frequencyCombo->
        itemData(ui.frequencyCombo->currentIndex()).toInt();
    spectrogram->intensity_axis = (AxisScale)ui.intensityCombo->
        itemData(ui.intensityCombo->currentIndex()).toInt();
    spectrogram->correction = (BrightCorrection)ui.brightCombo->
        itemData(ui.brightCombo->currentIndex()).toInt();
}

void MainWindow::newSpectrogram()
{
    if (!image_watcher->future().result().isNull()) // cancelled?
    {
        image = image_watcher->future().result();
        ui.speclocEdit->setText("unsaved");
        updateImage();
    }
    idleState();
}

void MainWindow::workingState()
{
    ui.specProgress->setValue(0);
    ui.specStatus->setText("Idle");
    ui.cancelButton->setEnabled(true);
    ui.makeButton->setEnabled(false);
    ui.makeSoundButton->setEnabled(false);
    ui.speclocButton->setEnabled(false);
    ui.paletteButton->setEnabled(false);
    ui.locationButton->setEnabled(false);
}

void MainWindow::idleState()
{
    ui.specProgress->setValue(0);
    ui.specStatus->setText("Idle");
    ui.cancelButton->setEnabled(false);
    ui.makeButton->setEnabled(true);
    ui.makeSoundButton->setEnabled(true);
    ui.speclocButton->setEnabled(true);
    ui.paletteButton->setEnabled(true);
    ui.locationButton->setEnabled(true);
}

void MainWindow::setFilterUnits(int index)
{
    AxisScale scale = (AxisScale)ui.frequencyCombo->itemData(index).toInt();
    switch (scale)
    {
        case SCALE_LINEAR:
            ui.bandwidthSpin->setSuffix(" Hz");
            break;
        case SCALE_LOGARITHMIC:
            ui.bandwidthSpin->setSuffix(" cents");
            break;
    }
}

void MainWindow::chooseImage()
{
    QString filename = QFileDialog::getOpenFileName(this, 
            "Choose the spectrogram", ".",
            "Images (*.png *.jpg *.bmp *.gif);;All files (*.*)");
    if (filename == NULL)
        return;
    ui.speclocEdit->setText(filename);
    loadImage();
}

bool MainWindow::imageOk()
{
    return !image.isNull();
}

void MainWindow::loadImage()
{
    QString filename = ui.speclocEdit->text();
    if (filename.isEmpty())
        return;
    image.load(filename);
    if (!imageOk())
    {
        QMessageBox::warning(this, "Invalid file", 
                "The specified file is not readable or not supported.");
        return;
    }
    QString params = image.text("Spectrogram");
    if (!params.isNull())
    {
        spectrogram->deserialize(params);
        setValues();
    }

    updateImage();
}

void MainWindow::resetImage()
{
    ui.speclocEdit->setText("");
    ui.sizeEdit->setText("");
    ui.spectrogramLabel->setText("");
}

void MainWindow::updateImage()
{
    if (imageOk())
    {
        if (image.width() > 30000)
            ui.spectrogramLabel->setText("Image too large to preview");
        else
            ui.spectrogramLabel->setPixmap(QPixmap::fromImage(image));
        QString sizetext_;
        QTextStream sizetext(&sizetext_);
        sizetext << image.width() << "x" << image.height() << " px";
        ui.sizeEdit->setText(sizetext_);
    }
    else
        resetImage();
}

void MainWindow::saveSoundfile(const real_vec& signal)
{
    QString filename;
    while (filename.isNull())
    {
        filename = QFileDialog::getSaveFileName(this, "Save sound",
                "synt.wav", "Sound (*.wav *.ogg *.flac)");
        QMessageBox msg;
        msg.setText("If you don't save the sound, it will be discarded.");
        msg.setIcon(QMessageBox::Warning);
        msg.setStandardButtons(QMessageBox::Discard|QMessageBox::Save);
        msg.setDefaultButton(QMessageBox::Discard);
        msg.setEscapeButton(QMessageBox::Discard);
        if (filename.isNull() && msg.exec() == QMessageBox::Discard)
            return;
    }
    const int samplerate = 44100;
    //const int samplerate = ui.samplerateSpin->value();
    Soundfile::writeSound(filename, signal, samplerate);
    ui.locationEdit->setText(filename);
}

void MainWindow::makeSound()
{
    if (!imageOk())
    {
        QMessageBox::warning(this, "No spectrogram",
                "Choose or generate a spectrogram first.");
        return;
    }

    loadValues();
    if (!checkSynthesisValues())
        return;

    workingState();
    SynthesisType type = (SynthesisType)ui.syntCombo->
        itemData(ui.syntCombo->currentIndex()).toInt();
    //const int samplerate = ui.samplerateSpin->value();
    const int samplerate = 44100;
    QFuture<real_vec> future = QtConcurrent::run(spectrogram,
           &Spectrogram::synthetize, image, samplerate, type);
    sound_watcher->setFuture(future);
}

void MainWindow::newSound()
{
    if (sound_watcher->future().result().size()) // cancelled?
    {
        saveSoundfile(sound_watcher->future().result());
        loadSoundfile();
    }

    idleState();
}

void MainWindow::setValues()
{
    ui.bandwidthSpin->setValue((int)spectrogram->bandwidth);
    ui.basefreqSpin->setValue(spectrogram->basefreq);
    ui.maxfreqSpin->setValue(spectrogram->maxfreq);
    ui.overlapSpin->setValue(spectrogram->overlap*100);
    ui.ppsSpin->setValue((int)spectrogram->pixpersec);
    setCombo(ui.windowCombo, spectrogram->window);
    setCombo(ui.intensityCombo, spectrogram->intensity_axis);
    setCombo(ui.frequencyCombo, spectrogram->frequency_axis);
    setCombo(ui.brightCombo, spectrogram->correction);
    updatePalette();
}

bool MainWindow::checkAnalysisValues()
{
    QStringList errors;
    if (spectrogram->maxfreq > soundfile.data().samplerate()/2)
    {
        errors.append("Maximum frequency of the spectrogram has to be at most half the sampling frequency (aka. Nyquist frequency) of the sound file.  It will be changed automatically if you continue.");
        spectrogram->maxfreq = soundfile.data().samplerate()/2;
    }
    if (spectrogram->frequency_axis == SCALE_LOGARITHMIC &&
            spectrogram->basefreq == 0 )
    {
        errors.append("Base frequency of a logarithmic spectrogram has to be larger than zero.  It will be set to 27.5 hz.");
        spectrogram->basefreq = 27.5;
    }
    if (spectrogram->window != WINDOW_RECTANGULAR && spectrogram->overlap < 0.4)
    {
        errors.append("The specified overlap is likely insufficient for use with the selected window function.");
    }
    const size_t size = soundfile.data().length()*spectrogram->pixpersec;
    if (size > 30000)
    {
        errors.append(QString());
        QTextStream(&errors[errors.size()-1]) << "The resulting spectrogram will be very large (" << size << " px), you may have problems viewing it.  Try lowering the Pixels per second value or using a shorter sound.";
    }

    return confirmWarnings(errors);
}

bool MainWindow::confirmWarnings(const QStringList& errors)
{
    if (errors.size())
    {
        QMessageBox msg;
        msg.setWindowTitle("Please note...");
        msg.setIcon(QMessageBox::Warning);
        msg.setText(errors.join("\n\n"));
        msg.setStandardButtons(QMessageBox::Ok|QMessageBox::Abort);
        msg.setDefaultButton(QMessageBox::Ok);
        msg.setEscapeButton(QMessageBox::Abort);
        int res = msg.exec();
        if (res == QMessageBox::Ok)
            setValues();
        else if (res == QMessageBox::Abort)
            return false;
    }
    return true;
}

bool MainWindow::checkSynthesisValues()
{
    QStringList errors;
    size_t badcolors = 0;
    for (int x = 0; x < image.width(); ++x)
        for (int y = 0; y < image.height(); ++y)
            if (!spectrogram->palette.has_color(image.pixel(x,y)))
                ++badcolors;
    if (badcolors)
    {
        errors.append(QString());
        QTextStream(&errors[errors.size()-1]) << "The spectrogram contains "<< badcolors << (badcolors > 1 ? " pixels":" pixel") <<" whose color is not in the selected palette.  Unknown colors are assumed to be zero intensity.  Synthesis quality will likely be affected.";
    }

    return confirmWarnings(errors);
}


/** \mainpage %Spectrogram -- documentation
 * This program can generate spectrograms from sound files and synthesize
 * spectrograms back into sound.
 *
 * The manual is divided into two parts:
 * \li \subpage user
 * \li \subpage prog
 */
/** \page prog Compilation and code overview
 * \section compiling Compiling the program
 * The program can be compiled with g++ on Linux or with mingw on Windows.
 *
 * \subsection deps Dependencies
 * The project uses the CMake build system: http://www.cmake.org
 *
 * The development versions of the following libraries need to be usable
 * before compiling:
 * \li Qt4 (used for the GUI): http://www.qtsoftware.com/products
 * \li FFTW (the single-precision version, used for fast fourier transform):
 * http://www.fftw.org
 * \li SRC (aka libsamplerate, used for audio resampling):
 * http://www.mega-nerd.com/SRC/
 * \li libsndfile (for many audio formats support):
 * http://mega-nerd.com/libsndfile
 * \li MAD (for mp3 support): http://www.underbit.com/products/mad/
 *
 * In Debian for example, you can run the following command to install all the
 * dependencies: <tt>apt-get install cmake libqt4-dev libfftw3-dev
 * libsndfile-dev libsamplerate-dev libmad0-dev</tt>
 *
 * For flac and ogg format support libsndfile version 1.0.18 or higher with
 * libflac and libogg plugins built-in has to be used.
 *
 * \subsection Linux Linux
 * To prepare the makefile, go to the \c build directory in the source tree and
 * type <tt> cmake ..</tt>
 *
 * If there are no errors (like missing libraries), you can build the program
 * by typing \c make
 *
 * The executable \c spectrogram will appear in the \c build directory.
 *
 * \subsection Windows Windows
 * Besides the dependencies you should have MinGW and MSYS installed.
 *
 * Once you have all the dependencies configured and compiled with mingw, start
 * the cmake-gui program and enter the source code directory and use the \c
 * build directory to build the binaries in.
 *
 * Then hit "Configure" and use the "MSYS Makefiles" generator.  For each
 * library that wasn't found automatically, enter the path manually and
 * continue with the configuration process.
 *
 * Once configuring is done, press "Generate"
 *
 * A Makefile now appears in the build directory.  Navigate to that directory in the MSYS shell and type \c make to compile the program.
 *
 * \section code Code overview
 * The most interesting class of the program is Spectrogram, it holds the
 * parameters for a spectrogram and performs analysis (turning sounds to
 * images) and synthesis (turning images to sounds).
 *
 * The MainWindow class handles the GUI.  The MainWindow::ui member is used to
 * access the widgets as designed in mainwindow.ui created with Qt Designer.
 * In the main window the user can specify parameters for the spectrogram and
 * supply audio data or a spectrogram to work with.  The analysis and synthesis
 * itself is then performed in a separate thread to keep the GUI responsive.
 *
 * The Soundfile class provides abstraction for working with audio files.  It
 * implements high-level operations like reading a channel of audio data from a
 * given file.  It aggregates all implementations of SndfileData.
 *
 * SndfileData is an abstract class whose implementations perform low-level
 * format-dependent operations.  Different libraries can be used to implement
 * the class and thus provide support for multiple audio formats.
 *
 * For more details, see documentation of these classes.
 */

/** \page user User documentation
 * \section Introduction
 * All the functionality of the program is available from the main window.  In
 * this window you can configure parameters for spectrogram generation or
 * synthesis and supply the data.
 *
 * Progress of long operations is indicated on the right side of the window.
 * The \c X button can be used to interrupt an operation that is taking too
 * long.
 *
 * \section analysis Spectrogram analysis
 * To turn a sound to a spectrogram, select the sound file in the upper right
 * part of the window.  Many sound files are stereo, which will appear as two
 * channels you can choose from.  The "Length" and "Samplerate" indicators are
 * purely informative.
 *
 * Depending on the purpose of the spectrogram and the nature of the supplied
 * audio data, different parameters are optimal.  The meaning of the main
 * parameters is explained below.  If you hover your mouse over a parameter
 * dial, an explanaition appears as a tooltip.
 *
 * \li <b>Frequency scale</b>:  This setting determines the type of the
 * frequency (vertical) axis.  Human hearing is logarithmic in nature.  For
 * music and other audio that contains a high range of frequencies, logarithmic
 * frequency scale is a good choice.  For speech or artificial sounds, linear
 * scale can also be used with good results.
 * \li <b>Intensity scale</b>  This affects the mapping of sound intensity to
 * pixel brightness.  The logarithmic setting is better for sounds with high
 * range of loundess where a linear setting would make the spectrogram too
 * dark.
 * \li <b>Base frequency</b>  For a logarithmic frequency spectrogram, the
 * first band (the row of the spectrogram) will be centered at this frequency.
 * For a linear frequency spectrogram, the first band starts at the this
 * frequency.
 * \li <b>Maximum frequency</b>  Sets the top frequency displayed in the
 * spectrogram.  For speech, value of about 8000 Hz can be sufficient.
 * \li <b>Pixels per second</b>  Determines the time resolution of the
 * spectrogram.  The larger the value, the wider the spectrogram.  For
 * synthesis, 100 or above is recommended.  For viewing, 50 can be sufficient
 * and make the spectrogram more "compact" with long sound samples.
 * \li <b>Brightness correction</b>  Some spectrograms can be very dark even
 * with logarithmic intensity scale.  Using the square root brightness
 * correction will make the spectrogram easier to read, but may affect
 * synthesis quality.
 * \li <b>Bandwidth</b>  Each horizontal band of the spectrogram will be as
 * wide as set here.  Lower value means more detail in the frequency domain,
 * but less detail in the time domain.
 * \li <b>Window function</b>:  Window function is applied to the frequency
 * intervals of the given bandwidth to lessen artifacts on their edges.  The
 * Hann window is a good general choice.
 * \li <b>Overlap</b>:  Larger overlap gives more detail in the frequency
 * domain and makes the spectrogram taller.  If no window function is used, it
 * can be set to zero, otherwise setting at least 60% overlap is recommended.
 * \li <b>%Palette</b>:  Shows the colors in which the spectrogram will be
 * drawn.  You can supply your own palette from an image, in that case the
 * first row of pixels of the image is used.  For synthesis, the colors in the
 * palette shouldn't repeat to make the intensity -> color mapping unambiguous.
 *
 * Once you are happy with the parameters, click the "Make spectrogram"
 * button.  A preview will appear and you can save the resulting image.
 *
 * \section synthesis Spectrogram synthesis
 * To turn a spectrogram back into sound, first select the spectrogram image in
 * the lower right.
 *
 * The parameters and palette of the spectrogram should be set to the same
 * values as were used for its generation.  If the spectrogram was generated by
 * this program, the parameters will be loaded automatically from metadata
 * saved in the image.  You can work with spectrograms from different sources
 * too, in which case you need to know or guess the parameters with which they
 * have been created.
 *
 * Two modes of synthesis are provided:
 * \li <b>Sine synthesis</b> is fast and produces decent results.
 * \li <b>Noise synthesis</b> is slower but may give better results for "busy"
 * spectrograms.
 *
 * When the parameters are set, you can press "Make sound" to synthesize the
 * chosen spectrogram.  After it's finished, you can save the resulting sound
 * file.
 *
 * \section formats Supported file formats
 * The program supports most commonly used sound file formats like mp3, wav,
 * flac and ogg.  For the last two the build has to be linked with the
 * libsndfile library version 1.0.18 or higher with libflac and libogg
 * plugins built-in.
 * See http://www.mega-nerd.com/libsndfile/#Features for a full list of
 * sound file formats supported via libsndfile.  MP3 is supported via libmad.
 *
 * Certain MP3 files with variable bitrate can display the wrong length in the
 * GUI.  They can still be used to generate spectrograms without problems.
 *
 * Most commonly used image formats are supported, for example png, bmp, tiff,
 * xpm, jpg (read only), gif (read only) and others.
 * See http://doc.trolltech.com/4.6/qimage.html#reading-and-writing-image-files
 * for a full list of supported image formats.
 */

/** \file main.cpp
 * \brief Code to start up the application.  Also contains the main
 * documentation.
 */

#include <iostream>
#include <cmath>
#include <cassert>

#include "soundfile.hpp"
#include "mainwindow.hpp"
#include "spectrogram.hpp"

// testing functions
namespace 
{
    void image_test()
    {
        //Soundfile file("/home/jan/ads/violin.ogg");
        Soundfile file("/home/jan/music/Windir/1999-Arntor/01-Byrjing.mp3");
        Spectrogram spec;
        //spec.palette = Palette("/home/jan/spectrogram/palettes/fiery.png");
        real_vec signal = file.read_channel(0);
        QImage out = spec.to_image(signal, file.data().samplerate());
        out.save("out.png");
    }

    void synt_test()
    {
        QImage img = QImage("/home/jan/spectrogram/out.png");
        //QImage img = QImage("/home/jan/or/vion-xx6.png");
        assert(!img.isNull());
        Spectrogram spec;
        real_vec data = spec.synthetize(img, 44100, SYNTHESIS_SINE);
        std::cout << "hotovo: "<<data.size()<<"\n";
        //Soundfile::writeSound("/home/jan/synt.wav", data);
    }
}

int main(int argc, char* argv[])
{
    //image_test();
    //synt_test();
    //return 0;
    QApplication app(argc, argv);
    MainWindow main_window;
    main_window.show();
    return app.exec();
}

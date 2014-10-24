#ifndef SPECTROGRAM_HPP
#define SPECTROGRAM_HPP

/** \file spectrogram.hpp
 *  \brief Definitions for classes used for spectrogram analysis and generation.
 */

#include <vector>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QThread>
#include <complex>
#include <memory>
#include "soundfile.hpp"
#include "fft.hpp"

#include <QVector>
#include <QRgb>

/// Represents a palette used to draw a spectrogram.
/** It is basically a mapping of intensity values from the interval <0,1> to a
 * set of colors (the palette), where 0 represents zero intensity of the pixel
 * and 1 represents maximum intensity.  Ideally, the mapping is 1:1
 * (bijection), otherwise there will be ambiguity in the synthesis process and
 * quality will be affected.  Both the Intensity -> Color and Color ->
 * Intensity mappings are implemented by the Palette::get_color() and
 * Palette::get_intensity() functions respectively.
 *
 * For optimal image sizes, the palette will be either indexed or RGB,
 * depending on how many colors it contains.  If there are 256 or less colors
 * (eg. in the default grayscale case), the palette will be 8-bit indexed,
 * otherwise it's RGB (24-bit, no index).
 *
 * This is transparent in usage because in both cases the make_canvas() function
 * will create a QImage with the appropriate color mode and QImage::setPixel()
 * then takes the same parameter type as the Palette::get_color() function
 * returns.
 */
class Palette
{
    public:
        /// Default constructor -- 8-bit grayscale palette.
        Palette();
        /// Creates a palette from an image.
        /** 
         * This constructor takes an image and fills the palette with colors
         * from the first row of an image.  \param img The image used to
         * generate the palette.
         */
        Palette(const QImage& img);
        //const QVector<QRgb>& colors() const;
        /// Mapping of intensity values from <0,1> to an index or RGB value.
        /** 
         * \param val A float where 0 <= val <= 1
         * \return Index of the color for an indexed palette or RGB value.
         */
        int get_color(float val) const;
        /// Inverse mapping of color values to intensity, used for spectrogram synthesis.
        /** \return Corresponding intensity, a value from <0,1>.  */
        float get_intensity(QRgb color) const;
        /// Returns true if the palette contains the given color, false otherwise.
        bool has_color(QRgb color) const;
        /// Creates a QImage with an appropriate color mode and dimensions.
        /** The resulting QImage will have the specified dimensions and color
         * mode depending on the number of colors in the palette.  For 256 or
         * less colors, it will be indexed, otherwise RGB.
         */
        QImage make_canvas(int width, int height) const;
        /// Used to determine if the palette is indexed or RGB.
        /** \return \c true if the palette is indexed, otherwise \c false.*/
        bool indexable() const;
        /// Generate a preview of the palette suitable for display in a widget.
        QPixmap preview(int width, int height) const;
        /// Returns the number of colors in the palette.
        int numColors() const;
    private:
        QVector<QRgb> colors_;
};

// ---

/// Represents the window function used for spectrogram generation.
enum Window 
{
    WINDOW_HANN, /**< See http://en.wikipedia.org/wiki/Hann_window */
    WINDOW_BLACKMAN, /**< See http://en.wikipedia.org/wiki/Window_function#Blackman_windows */
    WINDOW_RECTANGULAR, /**< Doesn't do anything. */
    WINDOW_TRIANGULAR /**< http://en.wikipedia.org/wiki/Triangular_window#Triangular_window_.28non-zero_end-points.29 */
};
/// Represents the linear or logarithmic mode for frequency and intensity axes.
enum AxisScale {SCALE_LINEAR, SCALE_LOGARITHMIC};
/// Represents spectrogram synthesis mode.
enum SynthesisType {SYNTHESIS_SINE, SYNTHESIS_NOISE};
/// Represents the brightness correction used in spectrogram generation.
enum BrightCorrection {BRIGHT_NONE, BRIGHT_SQRT};

/// This class holds the parameters for a spectrogram and implements its synthesis and generation.
class Spectrogram : public QObject
{
    Q_OBJECT
    public:
        Spectrogram(QObject* parent = 0);
        /// Generates a spectrogram for the given signal.
        QImage to_image(real_vec& signal, int samplerate) const;
        /// Synthesizes the given spectrogram to sound.
        real_vec synthetize(const QImage& image, int samplerate,
                SynthesisType type) const;
        /// Serializes the Spectrogram object.
        /** The serialized string is saved in image metadata to indicate parameters with which the spectrogram has been generated.  */
        QString serialized() const;
        /// Loads the serialized parameters into this object.
        void deserialize(const QString& serialized);

        /// Bandwidth of the frequency-domain filters.
        /** In Hz for linear spectrograms, in cents (cent = octave/1200) for logarithmic spectrograms.*/
        double bandwidth;
        /// Base frequency of the spectrogram.
        double basefreq;
        /// Maximum frequency of the spectrogram.
        double maxfreq;
        /// Overlap of the frequency-domain filters, 1 = full overlap (useless), 0 = no overlap.
        double overlap;
        /// Time resolution of the spectrogram, pixels per second.
        double pixpersec;
        /// Window function used on the frequency-domain intervals.
        Window window;
        /// Scale type of the intensity axis (linear or logarithmic)
        AxisScale intensity_axis;
        /// Scale type of the frequency axis (linear or logarithmic)
        AxisScale frequency_axis;
        /// Brightness correction used in generation of the spectrogram.
        BrightCorrection correction;
        /// Palette used for drawing the spectrogram.
        Palette palette;
    private:
        /// Performs sine synthesis on the given spectrogram.
        real_vec sine_synthesis(const QImage& image, int samplerate) const;
        /// Performs noise synthesis on the given spectrogram.
        real_vec noise_synthesis(const QImage& image, int samplerate) const;
        /// Draws an image from the given image data.
        QImage make_image(const std::vector<real_vec>& data) const;
        /// Returns intensity values (from <0,1>) from a row of pixels.
        real_vec envelope_from_spectrogram(const QImage& image, int row) const;
        /// Applies the window function to a frequency-domain interval.
        void apply_window(complex_vec& chunk, int lowidx,
                double filterscale) const;
        /// Delimiter of the serialized data
        static const char delimiter = ';';
        void band_progress(int x, int of, int from=0, int to=100) const;
        /// Indicates if the computation should be interrupted.
        bool cancelled() const;
        mutable bool cancelled_;
    signals:
        /// Signals percentual progress to the main application.
        void progress(int value) const;
        /// Signals the state of the computation to the main application.
        void status(const QString& text) const;
    public slots:
        /// Informs the working thread of a request to interrupt the computation.
        void cancel();
};

// --

typedef std::pair<int,int> intpair;

/// Used to divide the frequency domain into suitable intervals.
/** Each interval represents a horizontal band in a spectrogram. */
class Filterbank
{
    public:
        static std::auto_ptr<Filterbank> get_filterbank(AxisScale type,
                double scale, double base, double hzbandwidth, double overlap);

        Filterbank(double scale);
        /// Returns start-finish indexes for a given filterband.
        virtual intpair get_band(int i) const = 0;
        /// Returns the index of the filterband's center.
        virtual int get_center(int i) const = 0;
        /// Estimated total number of intervals.
        virtual int num_bands_est(double maxfreq) const = 0;
        virtual ~Filterbank();
    protected:
        /// The proportion of frequency versus vector indices.
        const double scale_;
};

/// Divides the frequency domain to intervals of constant bandwidth.
class LinearFilterbank : public Filterbank
{
    public:
        LinearFilterbank(double scale, double base, double hzbandwidth,
                double overlap);
        intpair get_band(int i) const;
        int get_center(int i) const;
        int num_bands_est(double maxfreq) const;
    private:
        const double bandwidth_;
        const int startidx_;
        const double step_;
};

/// Divides the frequency domain to intervals with variable (logarithmic, constant-Q) bandwidth.
class LogFilterbank : public Filterbank
{
    public:
        LogFilterbank(double scale, double base, double centsperband,
                double overlap);
        intpair get_band(int i) const;
        int get_center(int i) const;
        int num_bands_est(double maxfreq) const;
    private:
        const double centsperband_;
        const double logstart_;
        const double logstep_;
};

#endif

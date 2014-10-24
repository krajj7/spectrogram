#include "spectrogram.hpp"

#include <cmath>
#include <cstring>
#include <cassert>
#include <QVector>
#include <QTextStream>
#include <QRgb>

#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>
#include "samplerate.h"

namespace 
{
    float log10scale(float val)
    {
        assert(val >= 0 && val <= 1);
        return std::log10(1+9*val);
    }

    float log10scale_inv(float val)
    {
        assert(val >= 0 && val <= 1);
        return (std::pow(10, val)-1)/9;
    }

    // cent = octave/1200
    double cent2freq(double cents)
    {
        return std::pow(2, cents/1200);
    }
    double freq2cent(double freq)
    {
        return std::log(freq)/std::log(2)*1200;
    }
    double cent2oct(double cents)
    {
        return cents/1200;
    }
    double oct2cent(double oct)
    {
        return oct*1200;
    }

    void shift90deg(Complex& x)
    {
        x = std::conj(Complex(x.imag(), x.real()));
    }

    /// Uses libsrc to resample the input vector to a given length.
    real_vec resample(const real_vec& in, size_t len)
    {
        assert(len > 0);
        //std::cout << "resample(data size: "<<in.size()<<", len: "<<len<<")\n";
        if (in.size() == len)
            return in;

        const double ratio = (double)len/in.size();
        if (ratio >= 256)
            return resample(resample(in, in.size()*50), len);
        else if (ratio <= 1.0/256)
            return resample(resample(in, in.size()/50), len);

        real_vec out(len);

        SRC_DATA parms = {const_cast<float*>(&in[0]),
            &out[0], in.size(), out.size(), 0,0,0, ratio};
        src_simple(&parms, SRC_SINC_FASTEST, 1);

        return out;
    }

    /// Envelope detection: http://www.numerix-dsp.com/envelope.html
    real_vec get_envelope(complex_vec& band)
    {
        assert(band.size() > 1);

        // copy + phase shift
        complex_vec shifted(band);
        std::for_each(shifted.begin(), shifted.end(), shift90deg);

        real_vec envelope = padded_IFFT(band);
        real_vec shifted_signal = padded_IFFT(shifted);

        for (size_t i = 0; i < envelope.size(); ++i)
            envelope[i] = std::sqrt(envelope[i]*envelope[i] + 
                shifted_signal[i]*shifted_signal[i]);

        return envelope;
    }

    double blackman_window(double x)
    {
        assert(x >= 0 && x <= 1);
        return std::max(0.42 - 0.5*cos(2*PI*x) + 0.08*cos(4*PI*x), 0.0);
    }

    double hann_window(double x)
    {
        assert(x >= 0 && x <= 1);
        return 0.5*(1-std::cos(x*2*PI));
    }

    double triangular_window(double x)
    {
        assert(x >= 0 && x <= 1);
        return 1-std::abs(2*(x-0.5));
    }

    double window_coef(double x, Window window)
    {
        assert(x >= 0 && x <= 1);
        if (window == WINDOW_RECTANGULAR)
            return 1.0;
        switch (window)
        {
            case WINDOW_HANN:
                return hann_window(x);
            case WINDOW_BLACKMAN:
                return blackman_window(x);
            case WINDOW_TRIANGULAR:
                return triangular_window(x);
            default:
                assert(false);
        }
    }

    float calc_intensity(float val, AxisScale intensity_axis)
    {
        assert(val >= 0 && val <= 1);
        switch (intensity_axis)
        {
            case SCALE_LOGARITHMIC:
                return log10scale(val);
            case SCALE_LINEAR:
                return val;
            default:
                assert(false);
        }
    }

    float calc_intensity_inv(float val, AxisScale intensity_axis)
    {
        assert(val >= 0 && val <= 1);
        switch (intensity_axis)
        {
            case SCALE_LOGARITHMIC:
                return log10scale_inv(val);
            case SCALE_LINEAR:
                return val;
            default:
                assert(false);
        }
    }

    // to <0,1> (cutoff negative)
    void normalize_image(std::vector<real_vec>& data)
    {
        float max = 0.0f;
        for (std::vector<real_vec>::iterator it=data.begin();
                it!=data.end(); ++it)
            max = std::max(*std::max_element(it->begin(), it->end()), max);
        if (max == 0.0f)
            return;
        for (std::vector<real_vec>::iterator it=data.begin();
                it!=data.end(); ++it)
            for (real_vec::iterator i = it->begin(); i != it->end(); ++i)
                *i = std::abs(*i)/max;
    }

    // to <-1,1>
    void normalize_signal(real_vec& vector)
    {
        float max = 0;
        for (real_vec::iterator it = vector.begin(); it != vector.end(); ++it)
            max = std::max(max, std::abs(*it));
        //std::cout <<"max: "<<max<<"\n";
        assert(max > 0);
        for (real_vec::iterator it = vector.begin(); it != vector.end(); ++it)
            *it /= max;
    }

    // random number from <0,1>
    double random_double()
    {
        return ((double)rand()/(double)RAND_MAX);
    }

    float brightness_correction(float intensity, BrightCorrection correction)
    {
        switch (correction)
        {
            case BRIGHT_NONE:
                return intensity;
            case BRIGHT_SQRT:
                return std::sqrt(intensity);
        }
        assert(false);
    }

    /// Creates a random pink noise signal in the frequency domain
    /** \param size Desired number of samples in time domain (after IFFT). */
    complex_vec get_pink_noise(size_t size)
    {
        complex_vec res;
        for (size_t i = 0; i < (size+1)/2; ++i)
        {
            const float mag = std::pow((float) i, -0.5f);
            const double phase = (2*random_double()-1) * PI;//+-pi random phase 
            res.push_back(Complex(mag*std::cos(phase), mag*std::sin(phase)));
        }
        return res;
    }
}

Spectrogram::Spectrogram(QObject* parent) // defaults
    : QObject(parent)
    , bandwidth(100)
    , basefreq(55)
    , maxfreq(22050)
    , overlap(0.8)
    , pixpersec(100)
    , window(WINDOW_HANN)
    , intensity_axis(SCALE_LOGARITHMIC)
    , frequency_axis(SCALE_LOGARITHMIC)
    , cancelled_(false)
{
}

QImage Spectrogram::to_image(real_vec& signal, int samplerate) const
{
    emit status("Transforming input");
    emit progress(0);
    const complex_vec spectrum = padded_FFT(signal);

    const size_t width = (spectrum.size()-1)*2*pixpersec/samplerate;

    // transformation of frequency in hz to index in spectrum
    const double filterscale = ((double)spectrum.size()*2)/samplerate;
    //std::cout << "filterscale: " << filterscale<<"\n";

    std::auto_ptr<Filterbank> filterbank = Filterbank::get_filterbank(
            frequency_axis, filterscale, basefreq, bandwidth, overlap);
    const int bands = filterbank->num_bands_est(maxfreq);
    const int top_index = maxfreq*filterscale;
    // maxfreq has to be at most nyquist
    assert(top_index <= (int)spectrum.size());

    std::vector<real_vec> image_data;
    for (size_t bandidx = 0;; ++bandidx)
    {
        if (cancelled())
            return QImage();
        band_progress(bandidx, bands, 5, 93);

        // filtering
        intpair range = filterbank->get_band(bandidx);
        //std::cout << "-----\n";
        //std::cout << "spectrum size: " << spectrum.size() << "\n";
        //std::cout << "lowidx: "<<range.first<<" highidx: "<<range.second<<"\n";
        //std::cout << "(real)lowfreq: " << range.first/filterscale << " (real)highfreq: "<<range.second/filterscale<< "\n";
        //std::cout << "skutecna sirka: " << (range.second-range.first)/filterscale<< " hz\n";
        //std::cout << "svislych hodnot: "<<(range.second-range.first)<<"\n";
        //std::cout << "dava vzorku: "<<(range.second-range.first-1)*2<<"\n";
        //std::cout << "teoreticky staci: " << 2*(range.second-range.first)/filterscale<< " hz samplerate\n";
        //std::cout << "ja beru: " <<width << "\n";

        complex_vec filterband(range.second - range.first);
        std::copy(spectrum.begin()+range.first, 
                spectrum.begin()+std::min(range.second, top_index),
                filterband.begin());
                
        if (range.first > top_index)
            break;
        if (range.second > top_index)
            std::fill(filterband.begin()+top_index-range.first,
                    filterband.end(), Complex(0,0));

        // windowing
        apply_window(filterband, range.first, filterscale);

        // envelope detection + resampling
        const real_vec envelope = resample(get_envelope(filterband), width);
        image_data.push_back(envelope);
    }

    normalize_image(image_data);

    emit progress(99);
    return make_image(image_data);
}

/** \param data innermost values from 0 to 1, same sized vectors */
QImage Spectrogram::make_image(const std::vector<real_vec>& data) const
{
    emit status("Generating image");
    const size_t height = data.size();
    const size_t width = data[0].size();
    std::cout << "image size: " << width <<" x "<<height<<"\n";
    QImage out = palette.make_canvas(width, height);
    for (size_t y = 0; y < height; ++y)
    {
        assert(data[y].size() == width);
        for (size_t x = 0; x < width; ++x)
        {
            float intensity = calc_intensity(data[y][x], intensity_axis);
            intensity = brightness_correction(intensity, correction);
            out.setPixel(x, (height-1-y), palette.get_color(intensity));
        }
    }
    out.setText("Spectrogram", serialized()); // save parameters
    emit progress(100);
    emit status("Displaying image");
    return out;
}

void Spectrogram::apply_window(complex_vec& chunk, int lowidx, double filterscale) const
{
    const int highidx = lowidx+chunk.size();
    if (frequency_axis == SCALE_LINEAR)
        for (size_t i = 0; i < chunk.size(); ++i)
            chunk[i] *= window_coef((double)i/(chunk.size()-1), window);
    else
    {
        const double rloglow = freq2cent(lowidx/filterscale); // po zaokrouhleni
        const double rloghigh = freq2cent((highidx-1)/filterscale);
        for (size_t i = 0; i < chunk.size(); ++i)
        {
            const double logidx = freq2cent((lowidx+i)/filterscale);
            const double winidx = (logidx - rloglow)/(rloghigh - rloglow);
            chunk[i] *= window_coef(winidx, window);
        }
    }
}

real_vec Spectrogram::synthetize(const QImage& image, int samplerate,
                SynthesisType type) const
{
    switch (type)
    {
        case SYNTHESIS_SINE:
            return sine_synthesis(image, samplerate);
        case SYNTHESIS_NOISE:
            return noise_synthesis(image, samplerate);
    }
    assert(false);
}

real_vec Spectrogram::sine_synthesis(const QImage& image, int samplerate) const
{
    const size_t samples = image.width()*samplerate/pixpersec;
    complex_vec spectrum(samples/2+1);

    const double filterscale = ((double)spectrum.size()*2)/samplerate;

    std::auto_ptr<Filterbank> filterbank = Filterbank::get_filterbank(
            frequency_axis, filterscale, basefreq, bandwidth, overlap);

    for (int bandidx = 0; bandidx < image.height(); ++bandidx)
    {
        if (cancelled())
            return real_vec();
        band_progress(bandidx, image.height()-1);

        real_vec envelope = envelope_from_spectrogram(image, bandidx);

        // random phase between +-pi
        const double phase = (2*random_double()-1) * PI; 

        real_vec bandsignal(envelope.size()*2); 
        for (int j = 0; j < 4; ++j)
        {
            const double sine = std::cos(j*PI/2 + phase);
            for (size_t i = j; i < bandsignal.size(); i += 4)
                bandsignal[i] = envelope[i/2] * sine;
        }
        complex_vec filterband = padded_FFT(bandsignal);

        for (size_t i = 0; i < filterband.size(); ++i)
        {
            const double x = (double)i/(filterband.size()-1);
            // normalized blackman window antiderivative
            filterband[i] *= x - ((0.5/(2.0*PI))*sin(2.0*PI*x) +
                   (0.08/(4.0*PI))*sin(4.0*PI*x)/0.42);
        }

        //std::cout << "spectrum size: " << spectrum.size() << "\n";
        //std::cout << bandidx << ". filterband size: " << filterband.size() << "; start: " << filterbank->get_band(bandidx).first <<"; end: " << filterbank->get_band(bandidx).second << "\n";

        const size_t center = filterbank->get_center(bandidx);
        const size_t offset = std::max((size_t)0, center - filterband.size()/2);
        //std::cout << "offset: " <<offset<<" = "<<offset/filterscale<<" hz\n";
        for (size_t i = 0; i < filterband.size(); ++i)
            if (offset+i > 0 && offset+i < spectrum.size())
                spectrum[offset+i] += filterband[i];
    }

    real_vec out = padded_IFFT(spectrum);
    //std::cout << "samples: " << out.size() << " -> " << samples << "\n";
    normalize_signal(out);
    return out;
}

real_vec Spectrogram::noise_synthesis(const QImage& image, int samplerate) const
{
    size_t samples = image.width()*samplerate/pixpersec;

    complex_vec noise = get_pink_noise(samplerate*10); // 10 sec loop

    const double filterscale = ((double)noise.size()*2)/samplerate;
    std::auto_ptr<Filterbank> filterbank = Filterbank::get_filterbank(
            frequency_axis, filterscale, basefreq, bandwidth, overlap);

    const int top_index = maxfreq*filterscale;

    real_vec out(samples);

    for (int bandidx = 0; bandidx < image.height(); ++bandidx)
    {
        if (cancelled())
            return real_vec();
        band_progress(bandidx, image.height()-1);

        // filter noise
        intpair range = filterbank->get_band(bandidx);
        //std::cout << bandidx << "/"<<image.height()<<"\n";
        //std::cout << "(noise) vzorku: "<<range.second-range.first<<"\n";

        complex_vec filtered_noise(noise.size());
        std::copy(noise.begin()+range.first, 
                noise.begin()+std::min(range.second, top_index),
                filtered_noise.begin()+range.first);

        //apply_window(filtered_noise, range.first, filterscale);

        // ifft noise
        real_vec noise_mod = padded_IFFT(filtered_noise);
        // resample spectrogram band
        real_vec envelope =
            resample(envelope_from_spectrogram(image, bandidx), samples);
        // modulate with looped noise
        for (size_t i = 0; i < samples; ++i)
            out[i] += envelope[i] * noise_mod[i % noise_mod.size()];
    }
    normalize_signal(out);
    return out;
}

void Spectrogram::band_progress(int x, int of, int from, int to) const
{
    QString bandstatus;
    bandstatus.sprintf("Processing band %i of %i", x, of);
    //std::cout << bandstatus.toStdString()<<"\n";
    emit status(bandstatus);
    emit progress(to*x/of+from);
}

void Spectrogram::cancel()
{
    cancelled_ = true;
    emit status("Cancelling...");
    //std::cout << "cancelled!\n";
}

bool Spectrogram::cancelled() const
{
    bool was = cancelled_;
    cancelled_ = false;
    return was;
}

// returns real_vec of numbers from <0,1> from a row of pixels
real_vec Spectrogram::envelope_from_spectrogram(const QImage& image, int row) const
{
    real_vec envelope(image.width());
    for (int x = 0; x < image.width(); ++x)
        envelope[x] = calc_intensity_inv(palette.get_intensity(
                image.pixel(x, image.height()-row-1)), intensity_axis);
    return envelope;
}

void Spectrogram::deserialize(const QString& text)
{
    QStringList tokens = text.split(delimiter);
    bandwidth = tokens[1].toDouble();
    basefreq = tokens[2].toDouble();
    maxfreq = tokens[3].toDouble();
    overlap = tokens[4].toDouble()/100.0;
    pixpersec = tokens[5].toDouble();
    window = (Window)tokens[6].toInt();
    intensity_axis = (AxisScale)tokens[7].toInt();
    frequency_axis = (AxisScale)tokens[8].toInt();
}

QString Spectrogram::serialized() const
{
    QString out;
    QTextStream desc(&out);
    desc.setRealNumberPrecision(4);
    desc.setRealNumberNotation(QTextStream::FixedNotation);
    desc << "Spectrogram:" << delimiter
        << bandwidth << delimiter
        << basefreq << delimiter
        << maxfreq << delimiter
        << overlap*100 << delimiter
        << pixpersec << delimiter
        << (int)window << delimiter
        << (int)intensity_axis << delimiter
        << (int)frequency_axis << delimiter
        ;
    //std::cout << "serialized: " << out.toStdString() << "\n";
    return out;
}

Palette::Palette(const QImage& img)
{
    assert(!img.isNull());
    for (int x = 0; x < img.width(); ++x)
        colors_.append(img.pixel(x, 0));
}

Palette::Palette()
{
    QVector<QRgb> colors;
    for (int i = 0; i < 256; ++i)
        colors.append(qRgb(i, i, i));
    colors_ = colors;
}

int Palette::get_color(float val) const
{
    assert(val >= 0 && val <= 1);
    if (indexable())
        // returns the color index
        return (colors_.size()-1)*val;
    else
        // returns the RGB value
        return colors_[(colors_.size()-1)*val];
}

bool Palette::has_color(QRgb color) const
{
    return colors_.indexOf(color) != -1;
}

// ne moc efektivni
float Palette::get_intensity(QRgb color) const
{
    int index = colors_.indexOf(color);
    if (index == -1) // shouldn't happen
        return 0;
    return (float)index/(colors_.size()-1);
}

QImage Palette::make_canvas(int width, int height) const
{
    if (indexable())
    {
        QImage out(width, height, QImage::Format_Indexed8);
        out.setColorTable(colors_);
        out.fill(0);
        return out;
    }
    else
    {
        QImage out(width, height, QImage::Format_RGB32);
        out.fill(colors_[0]);
        return out;
    }
}

bool Palette::indexable() const
{
    return colors_.size() <= 256;
}

QPixmap Palette::preview(int width, int height) const
{
    QImage out = make_canvas(width, height);
    for (int x = 0; x < width; ++x)
        out.setPixel(x, 0, get_color((double)x/(width-1)));
    int bytes = out.bytesPerLine();
    for (int y = 1; y < height; ++y)
        std::memcpy(out.scanLine(y), out.scanLine(0), bytes);
    return QPixmap::fromImage(out);
}

int Palette::numColors() const
{
    return colors_.size();
}

Filterbank::Filterbank(double scale)
    : scale_(scale)
{
}

Filterbank::~Filterbank()
{
}

LinearFilterbank::LinearFilterbank(double scale, double base, 
        double hzbandwidth, double overlap)
    : Filterbank(scale)
    , bandwidth_(hzbandwidth*scale)
    , startidx_(std::max(scale_*base-bandwidth_/2, 0.0))
    , step_((1-overlap)*bandwidth_)
{
    //std::cout << "bandwidth: " << bandwidth_ << "\n";
    //std::cout << "step_: " << step_ << " hz\n";
    assert(step_ > 0);
}

int LinearFilterbank::num_bands_est(double maxfreq) const
{
    return (maxfreq*scale_-startidx_)/step_;
}

intpair LinearFilterbank::get_band(int i) const
{
    intpair out;
    out.first = startidx_ + i*step_;
    out.second = out.first + bandwidth_;
    return out;
}

int LinearFilterbank::get_center(int i) const
{
    return startidx_ + i*step_ + bandwidth_/2.0;
}

LogFilterbank::LogFilterbank(double scale, double base, 
        double centsperband, double overlap)
    : Filterbank(scale)
    , centsperband_(centsperband)
    , logstart_(freq2cent(base))
    , logstep_((1-overlap)*centsperband_)
{
    assert(logstep_ > 0);
    //std::cout << "bandwidth: " << centsperband_ << " cpb\n";
    //std::cout << "logstep_: " << logstep_ << " cents\n";
}

int LogFilterbank::num_bands_est(double maxfreq) const
{
    return (freq2cent(maxfreq)-logstart_)/logstep_+4;
}

int LogFilterbank::get_center(int i) const
{
    const double logcenter = logstart_ + i*logstep_;
    return cent2freq(logcenter)*scale_;
}

intpair LogFilterbank::get_band(int i) const
{
    const double logcenter = logstart_ + i*logstep_;
    const double loglow = logcenter - centsperband_/2.0;
    const double loghigh = loglow + centsperband_;
    intpair out;
    out.first = cent2freq(loglow)*scale_;
    out.second = cent2freq(loghigh)*scale_;
    //std::cout << "centerfreq: " << cent2freq(logcenter)<< "\n";
    //std::cout << "lowfreq: " << cent2freq(loglow) << " highfreq: "<<cent2freq(loghigh)<< "\n";
    return out;
}

std::auto_ptr<Filterbank> Filterbank::get_filterbank(AxisScale type,
        double scale, double base, double bandwidth, double overlap)
{
    Filterbank* filterbank;
    if (type == SCALE_LINEAR)
        filterbank=new LinearFilterbank(scale, base, bandwidth, overlap);
    else
        filterbank=new LogFilterbank(scale, base, bandwidth, overlap);
    return std::auto_ptr<Filterbank>(filterbank);
}

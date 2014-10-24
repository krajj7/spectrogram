#include <cassert>
#include <iostream>
#include "soundfile.hpp"

namespace 
{
    const size_t MAX_FIXED_T_VAL=std::pow(2.0,(int)(8*sizeof(mad_fixed_t)-1)-1);
}

QString Soundfile::writeSound(const QString& fname, const real_vec& data,
        int samplerate, int format)
{
    if (format == -1)
    {
        format = guessFormat(fname);
        if (!format)
            return "Unsupported filetype for writing.";
    }
    SF_INFO check = {0, samplerate, 1, format, 0, 0};
    if (!sf_format_check(&check))
        return "Format didn't pass sf_format_check()"; // shouldn't happen

    //XXX zmeni unicode nazvy
    SndfileHandle file(fname.toLocal8Bit(), SFM_WRITE, format, 1, samplerate);
    if (!file)
        assert(false);
    if (file.error())
        return file.strError();

    file.writef(&data[0], data.size());
    return QString();
}

/// Returns the format specification guessed from the specified file extension.
int Soundfile::guessFormat(const QString& filename)
{
    assert(!filename.isNull());
    if (filename.endsWith(".wav"))
        return SF_FORMAT_WAV|SF_FORMAT_PCM_16;
    else if (filename.endsWith(".ogg"))
        return SF_FORMAT_OGG|SF_FORMAT_VORBIS;
    else if (filename.endsWith(".flac"))
        return SF_FORMAT_FLAC|SF_FORMAT_PCM_16;
    return 0;
}

Soundfile::Soundfile()
    : data_(NULL)
{
}

Soundfile::Soundfile(const QString& filename)
{
    load(filename);
}

real_vec Soundfile::read_channel(int channel)
{
    return data_->read_channel(channel);
}

bool Soundfile::valid() const
{
    return data_ != NULL;
}

const QString& Soundfile::error() const
{
    return error_;
}

void Soundfile::load(const QString& filename)
{
    if (filename.endsWith(".mp3"))
        data_ = new MP3Data(filename);
    else
        data_ = new SndfileData(filename);

    if (!data_->valid())
    {
        error_ = data_->error();
        delete data_;
        data_ = NULL;
    }
}

void Soundfile::reset()
{
    delete data_;
    data_ = NULL;
}

const SoundfileData& Soundfile::data() const
{
    assert(data_ != NULL);
    return *data_;
}

// ----

SndfileData::SndfileData(const QString& filename)
{
    file_ = SndfileHandle(filename.toLocal8Bit());
}

bool SndfileData::valid() const
{
    return (file_ && !file_.error());
}

QString SndfileData::error() const
{
    return file_.strError();
}

size_t SndfileData::frames() const
{
    return file_.frames();
}

double SndfileData::length() const
{
    return (double)frames()/samplerate();
}

int SndfileData::channels() const
{
    return file_.channels();
}

int SndfileData::samplerate() const
{
    return file_.samplerate();
}

real_vec SndfileData::read_channel(int channel)
{
    assert(channel < channels());
    real_vec buffer(frames()*channels());
    file_.readf(&buffer[0], frames());
    //size_t frames_read = file_.readf(&buffer[0], frames());
    //assert(frames_read == frames());
    file_.seek(0, SEEK_SET);

    if (channels() > 1)
    {
        for (size_t i = channel, j = 0; j < frames(); i += channels(), ++j)
            buffer[j] = buffer[i];
        buffer.resize(frames());
    }
    return buffer;
}

SndfileData::~SndfileData()
{
}

// ---

MP3Data::MP3Data(const QString& fname)
    : frames_(0)
    , length_(0)
    , samplerate_(0)
    , channels_(0)
    , filename_(fname)
{
    get_mp3_stats();
}

void MP3Data::get_mp3_stats()
{
    mad_stream stream;
    mad_header header;
    mad_stream_init(&stream);
    mad_header_init(&header);

    QFile file(filename_);
    if (!file.open(QIODevice::ReadOnly))
    {
        error_ = "Error opening file.";
        goto cleanup;
    }
    {
    uchar* buf = file.map(0, file.size());
    mad_stream_buffer(&stream, buf, file.size());

    while (true)
    {
        if (mad_header_decode(&header, &stream) == -1)
        {
            if (stream.error == MAD_ERROR_LOSTSYNC)
                continue;
            else if (stream.error == MAD_ERROR_BUFLEN)
                break;
            else
            {
                error_ = "Error decoding mp3 headers!";
                break;
            }
        }
        frames_++;
        length_ += header.duration.fraction;
        if (!samplerate_)
        {
            samplerate_ = header.samplerate;
            if (header.mode == MAD_MODE_SINGLE_CHANNEL)
                channels_ = 1;
            else
                channels_ = 2;
        }
    }
    length_ /= MAD_TIMER_RESOLUTION;
    if (!samplerate_)
        error_ = "Invalid mp3 file.";
    }
cleanup:
    mad_stream_finish(&stream);
    mad_header_finish(&header);
}

QString MP3Data::error() const
{
    return error_;
}

real_vec MP3Data::read_channel(int channel)
{
    mad_stream stream;
    mad_frame frame;
    mad_synth synth;
    mad_stream_init(&stream);
    mad_frame_init(&frame);
    mad_synth_init(&synth);

    real_vec result;

    QFile file(filename_);
    if (!file.open(QIODevice::ReadOnly))
        goto cleanup;
    {
    uchar* buf = file.map(0, file.size());
    mad_stream_buffer(&stream, buf, file.size());

    while (true)
    {
        if (mad_frame_decode(&frame, &stream) == -1)
        {
            if (stream.error == MAD_ERROR_LOSTSYNC)
                continue;
            else if (stream.error == MAD_ERROR_BUFLEN)
                break;
            else
            {
                error_ = "Error decoding mp3 file.";
                break;
            }
        }
        mad_synth_frame(&synth, &frame);
        mad_fixed_t* samples = synth.pcm.samples[channel];
        for (short i = 0; i < synth.pcm.length; ++i)
            result.push_back((double)samples[i]/MAX_FIXED_T_VAL);
    }
    }
cleanup:
    mad_synth_finish(&synth);
    mad_frame_finish(&frame);
    mad_stream_finish(&stream);

    return result;
}

size_t MP3Data::frames() const
{
    return frames_;
}

double MP3Data::length() const//in seconds
{
    return length_;
}

int MP3Data::samplerate() const
{
    return samplerate_;
}

int MP3Data::channels() const
{
    return channels_;
}

bool MP3Data::valid() const
{
    return error_.isEmpty();
}

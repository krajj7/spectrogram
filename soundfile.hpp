#ifndef SOUNDFILE_HPP
#define SOUNDFILE_HPP

/** \file soundfile.hpp
 *  \brief Definitions for classes that work with audio file formats. */

#include <QString>
#include <QFile>
#include <vector>
#include <sndfile.hh>
#include "types.hpp"
#include "mad.h"

/// An abstract interface for decoding sound files.
/** It provides abstraction for all low-level functions used on sound files, implementation can be different for each format. */
class SoundfileData
{
    public:
        virtual ~SoundfileData() {};
        /// Used to get details in case of an error.
        virtual QString error() const = 0;
        /// Loads a specified channel into a real-valued vector.
        virtual real_vec read_channel(int channel) = 0;
        /// Returns the number of audio frames in each channel.
        virtual size_t frames() const = 0;
        /// Returns the length of the audio track in seconds.
        virtual double length() const = 0;
        /// Returns the samplerate of the audio file in Hz.
        virtual int samplerate() const = 0;
        /// Returns the number of channels.
        virtual int channels() const = 0;
        /// Checks if the audio file is loaded correctly and ready for use.
        virtual bool valid() const = 0;
};

/// Implements the SoundfileData interface using libsndfile.
/** This provides support for multiple file formats: wav, ogg, flac and many others. */
class SndfileData : public SoundfileData
{
    public:
        SndfileData(const QString& fname);
        ~SndfileData();
        QString error() const;
        real_vec read_channel(int channel);
        size_t frames() const;
        double length() const; //in seconds
        int samplerate() const;
        int channels() const;
        bool valid() const;
    private:
        SndfileHandle file_;
};

/// Implements the SoundfileData interface using libmad.
/** This provides support for MP3 files. */
class MP3Data : public SoundfileData
{
    public:
        MP3Data(const QString& fname);
        QString error() const;
        real_vec read_channel(int channel);
        size_t frames() const;
        double length() const;//in seconds
        int samplerate() const;
        int channels() const;
        bool valid() const;
    private:
        void get_mp3_stats();
        size_t frames_;
        size_t length_;
        int samplerate_;
        int channels_;
        QString filename_;
        QString error_;
};

/// A format-independent class that provides functionality for sound manipulation, reading and writing.
/** It aggregates all implementations of SndfileData. */
class Soundfile
{
    public:
        /// Writes pcm data to an audio file encoded according to the extension.
        /** Encoding is performed by libsndfile (no mp3 writing support).
         * \return a string error, or null string on success. */
        static QString writeSound(const QString& fname, const real_vec& data,
                int samplerate, int format=-1);
        static int guessFormat(const QString& filename);

        Soundfile();
        Soundfile(const QString& fname);
        /// Forget the loaded file.
        void reset();
        /// Loads the specified file.
        void load(const QString& fname);
        /// If the loaded file isn't valid, this function gives the reason.
        const QString& error() const;
        /// Used to determine if a file was loaded successfully.
        bool valid() const;
        /// Read the audio data of the given channel from the loaded file.
        /** \return PCM data of the specified audio channel */
        real_vec read_channel(int channel);
        /// Allows access to low-level information about the file (eg. samplerate).
        const SoundfileData& data() const;
    private:
        SoundfileData* data_;
        QString error_;
};

#endif

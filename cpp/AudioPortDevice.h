/*
 * Author: Rodrigo Rolim Mendes de Alencar <alencar.fmce@imbel.gov.br>
 *
 * Copyright 2018 IMBEL/FMCE.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef AUDIOPORTDEVICE_I_IMPL_H
#define AUDIOPORTDEVICE_I_IMPL_H

#include "AudioPortDevice_base.h"

#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <algorithm>
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <sys/time.h>

#define EV_PRESSED 		1
#define EV_RELEASED 	0
#define EV_REPEAT 		2

#define MAX_PAYLOAD_SIZE_H 		16383
#define MIN_PAYLOAD_SIZE_H 		512
#define MIN_OVERRIDE_TIMEOUT_H 	50

#define TONE_SAMPLE_RATE 8000

struct StreamControl{
	snd_pcm_t *pcm_handle;
	Packet::SeqNum seq_number;
};

class ToneControl{

	public:
		ToneControl(Audio::AudibleAlertsAndAlarms::ToneProfileType profile, AudioPortDevice_i* dev);
		~ToneControl();

		void start();
		void stop();

		CORBA::ULong getNumSamples();

	private:

		AudioPortDevice_i *audio_device;
		bool status;
		pthread_mutex_t lock;
		pthread_t thread;
		Audio::AudibleAlertsAndAlarms::ToneProfileType profile;

		void simple_tone_thread();
		static void *simple_tone_thread_helper(void *context)
		{
			((ToneControl *)context)->simple_tone_thread();
			return NULL;
		}

		void complex_tone_thread();
		static void *complex_tone_thread_helper(void *context)
		{
			((ToneControl *)context)->complex_tone_thread();
			return NULL;
		}

};

class AudioPortDevice_i : public AudioPortDevice_base
{
    ENABLE_LOGGING

	friend class ToneControl;
	friend class Audio_AudibleAlertsAndAlarms_In_i;
	friend class Audio_SampleStreamControl_In_i;
	friend class Audio_SampleStream_In_i;
	friend class Audio_SampleMessageControl_In_i;
	friend class Audio_AudioPTT_Signal_Out_i;
	friend class Audio_SampleStreamControl_Out_i;
	friend class Audio_SampleStream_Out_i;

    public:
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~AudioPortDevice_i();

        void construct();
        void constructor();

        int serviceFunction();

        void releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError);
        void start() throw (CF::Resource::StartError, CORBA::SystemException);
		void stop() throw (CF::Resource::StopError, CORBA::SystemException);

    protected:

        void updateUsageState();

    private:

        bool tx_active;
        bool tx_abort;
        bool rx_active;

        pthread_mutex_t tx_lock;
        pthread_mutex_t tx_stream_lock;
        pthread_mutex_t rx_lock;

        snd_pcm_t *tx_handle;

        char *tx_buffer;

        pthread_t tx_thread;
        Packet::Stream tx_stream;

        int ptt_fd;
        pthread_t ptt_thread;

        std::string input_device_name;
        std::string output_device_name;

        CORBA::ULong tx_override_timeout;
        CORBA::ULong tx_payload_size;

        CORBA::ULong rx_min_override_timeout;
		CORBA::ULong rx_max_payload_size;
		CORBA::ULong rx_min_payload_size;
		CORBA::ULong rx_desired_payload_size;

        void txThread();
        static void *tx_thread_helper(void *context)
		{
			((AudioPortDevice_i *)context)->txThread();
			return NULL;
		}

        static int writeBuffer(snd_pcm_t *pcm_handle, const void *vbuffer, int nframes, unsigned sizeof_frame);
        int readBuffer(void *vbuffer, int nframes, unsigned sizeof_frame);

        void pttThread();
        static void *ptt_thread_helper(void *context)
		{
        	((AudioPortDevice_i *)context)->pttThread();
        	return NULL;
		}

        static CORBA::ULong init_pcm(snd_pcm_t **pcm_handle, const char *card_name, snd_pcm_stream_t stream, unsigned int *sample_rate, snd_pcm_format_t format, int mode);
};

#endif // AUDIOPORTDEVICE_I_IMPL_H

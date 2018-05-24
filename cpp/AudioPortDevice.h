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

class AudioPortDevice_i : public AudioPortDevice_base
{
    ENABLE_LOGGING

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

        snd_pcm_t *tx_handle;
        char *tx_buffer;
        pthread_t tx_thread;
        Packet::Stream tx_stream;
        CORBA::ULong tx_desired_payload;
        CORBA::ULong tx_override_timeout;

        int ptt_fd;
        pthread_t ptt_thread;

        void txThread();
        static void *tx_thread_helper(void *context)
		{
			((AudioPortDevice_i *)context)->txThread();
			return NULL;
		}

        int readBuffer(void *vbuffer, int nframes, unsigned sizeof_frame);

        void pttThread();
        static void *ptt_thread_helper(void *context)
		{
        	((AudioPortDevice_i *)context)->pttThread();
        	return NULL;
		}
};

#endif // AUDIOPORTDEVICE_I_IMPL_H

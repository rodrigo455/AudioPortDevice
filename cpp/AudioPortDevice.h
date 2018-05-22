#ifndef AUDIOPORTDEVICE_I_IMPL_H
#define AUDIOPORTDEVICE_I_IMPL_H

#include "AudioPortDevice_base.h"

#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <algorithm>
#include <alsa/asoundlib.h>

#define EV_PRESSED 1
#define EV_RELEASED 0
#define EV_REPEAT 2

class AudioPortDevice_i : public AudioPortDevice_base
{
    ENABLE_LOGGING
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

        snd_pcm_t *input_handle;
        snd_pcm_uframes_t input_period_size;
        char *input_buffer;
        pthread_t tx_thread;
        Packet::Stream tx_stream;

        /*ptt input event device descriptor*/
        int ptt_fd;
        pthread_t ptt_thread;

        void txThread();
        static void *tx_thread_helper(void *context)
		{
			((AudioPortDevice_i *)context)->txThread();
			return NULL;
		}

        bool readBuffer(void *vbuffer, unsigned nframes, unsigned sizeof_frame);

        void pttThread();
        static void *ptt_thread_helper(void *context)
		{
        	((AudioPortDevice_i *)context)->pttThread();
        	return NULL;
		}
};

#endif // AUDIOPORTDEVICE_I_IMPL_H
